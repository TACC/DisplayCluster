/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "Movie.h"
#include "main.h"
#include "log.h"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

namespace
{

// one CUDA context for the whole process, shared by every Movie's GL
// interop calls; safe across the FFmpeg-owned NVDEC context because Maxwell+
// GPUs use unified virtual addressing, so device pointers from one context
// on a GPU are valid in any other context on the same GPU
CUcontext g_cudaContext = nullptr;

void ensureCudaContext()
{
    if (g_cudaContext)
    {
        cuCtxSetCurrent(g_cudaContext);
        return;
    }

    cuInit(0);
    CUdevice device;
    cuDeviceGet(&device, 0);
    // cuCtxCreate() is the wrong tool here: cuda.h redirects the bare name
    // to whatever versioned symbol (_v2/_v3/_v4/...) is current for the
    // CUDA version this was BUILT against, but that's a compile-time
    // decision baked into the binary - it doesn't track what the actual
    // RUNTIME driver on whatever machine the binary ends up running on
    // actually implements. A binary built against CUDA 13 headers
    // reliably fails with "undefined symbol: cuCtxCreate_v4" on any
    // machine whose installed driver predates CUDA 13, since older
    // drivers never exported that symbol at all (confirmed happening in
    // practice: built via the Ubuntu 22.04 container's CUDA 13.3
    // toolkit, then run on a TACC GPU node with an older driver).
    // cuDevicePrimaryCtxRetain() has never had versioned variants and
    // gets a perfectly usable context for our purposes (we don't need
    // any of the newer per-context creation flags), so it sidesteps this
    // whole class of build-vs-runtime-driver mismatch entirely.
    cuDevicePrimaryCtxRetain(&g_cudaContext, device);
    cuCtxSetCurrent(g_cudaContext);
}

void checkCu(CUresult result, const char *what)
{
    if (result != CUDA_SUCCESS)
    {
        const char *msg = nullptr;
        cuGetErrorString(result, &msg);
        std::cerr << "CUDA error in " << what << ": " << (msg ? msg : "unknown") << "\n";
    }
}

GLuint compileShader(QOpenGLExtraFunctions *gl, GLenum type, const char *src)
{
    GLuint shader = gl->glCreateShader(type);
    gl->glShaderSource(shader, 1, &src, nullptr);
    gl->glCompileShader(shader);

    GLint ok = GL_FALSE;
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        gl->glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "movie NV12 shader compile failed: " << log << "\n";
    }

    return shader;
}

// fragment-only program: with no vertex shader attached, fixed-function
// vertex processing still runs and populates gl_TexCoord[0] from the
// existing glTexCoord2f()/glVertex2f() immediate-mode calls in render(),
// so the quad-drawing code below doesn't need to change to a VBO/VAO
GLuint buildNV12ShaderProgram(QOpenGLExtraFunctions *gl)
{
    static const char *fragSrc =
        "uniform sampler2D texY;\n"
        "uniform sampler2D texUV;\n"
        "void main()\n"
        "{\n"
        "    float y = texture2D(texY, gl_TexCoord[0].st).r;\n"
        "    vec2 uv = texture2D(texUV, gl_TexCoord[0].st).rg;\n"
        "    float u = uv.x - 0.5;\n"
        "    float v = uv.y - 0.5;\n"
        "    y = 1.164 * (y - 0.0625);\n"
        "    float r = y + 1.596 * v;\n"
        "    float g = y - 0.391 * u - 0.813 * v;\n"
        "    float b = y + 2.018 * u;\n"
        "    gl_FragColor = vec4(r, g, b, 1.0);\n"
        "}\n";

    GLuint fragShader = compileShader(gl, GL_FRAGMENT_SHADER, fragSrc);

    GLuint program = gl->glCreateProgram();
    gl->glAttachShader(program, fragShader);
    gl->glLinkProgram(program);

    GLint ok = GL_FALSE;
    gl->glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        gl->glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "movie NV12 shader link failed: " << log << "\n";
    }

    gl->glDeleteShader(fragShader);

    return program;
}

}

Movie::Movie(std::string uri)
{
    initialized_ = false;
    paused_ = false;
    decoder = new Decoder();
    decoder->Setup(uri);
}

Movie::~Movie()
{
    if (initialized_)
    {
        cuGraphicsUnregisterResource(cudaResourceY_);
        cuGraphicsUnregisterResource(cudaResourceUV_);

        auto *gl = QOpenGLContext::currentContext()->extraFunctions();
        gl->glDeleteProgram(shaderProgram_);

        glDeleteTextures(1, &textureY_);
        glDeleteTextures(1, &textureUV_);
    }

    delete decoder;
}

void Movie::getDimensions(int &width, int &height)
{
    decoder->getFrameDimensions(width, height);
}

void Movie::render(float tX, float tY, float tW, float tH)
{
    updateRenderedFrameCount();

    int w, h;
    decoder->getFrameDimensions(w, h);
    int cw = (w + 1) / 2;
    int ch = (h + 1) / 2;

    auto *gl = QOpenGLContext::currentContext()->extraFunctions();

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
    glEnable(GL_TEXTURE_2D);

    if(initialized_ != true)
    {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glShadeModel(GL_FLAT);
        glEnable(GL_DEPTH_TEST);

        ensureCudaContext();

        glGenTextures(1, &textureY_);
        glBindTexture(GL_TEXTURE_2D, textureY_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

        glGenTextures(1, &textureUV_);
        glBindTexture(GL_TEXTURE_2D, textureUV_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, cw, ch, 0, GL_RG, GL_UNSIGNED_BYTE, 0);

        checkCu(cuGraphicsGLRegisterImage(&cudaResourceY_, textureY_, GL_TEXTURE_2D,
                                           CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD), "register Y texture");
        checkCu(cuGraphicsGLRegisterImage(&cudaResourceUV_, textureUV_, GL_TEXTURE_2D,
                                           CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD), "register UV texture");

        shaderProgram_ = buildNV12ShaderProgram(gl);

        initialized_ = true;
    }

    if (decoder->ready())
    {
        AVFrame *frame = decoder->getFrame();

        ensureCudaContext();

        CUgraphicsResource resources[2] = { cudaResourceY_, cudaResourceUV_ };
        checkCu(cuGraphicsMapResources(2, resources, 0), "map movie textures");

        CUarray arrayY, arrayUV;
        checkCu(cuGraphicsSubResourceGetMappedArray(&arrayY, cudaResourceY_, 0, 0), "get Y array");
        checkCu(cuGraphicsSubResourceGetMappedArray(&arrayUV, cudaResourceUV_, 0, 0), "get UV array");

        CUDA_MEMCPY2D copyY = {};
        copyY.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        copyY.srcDevice = (CUdeviceptr)frame->data[0];
        copyY.srcPitch = frame->linesize[0];
        copyY.dstMemoryType = CU_MEMORYTYPE_ARRAY;
        copyY.dstArray = arrayY;
        copyY.WidthInBytes = w;
        copyY.Height = h;
        checkCu(cuMemcpy2D(&copyY), "copy Y plane");

        CUDA_MEMCPY2D copyUV = {};
        copyUV.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        copyUV.srcDevice = (CUdeviceptr)frame->data[1];
        copyUV.srcPitch = frame->linesize[1];
        copyUV.dstMemoryType = CU_MEMORYTYPE_ARRAY;
        copyUV.dstArray = arrayUV;
        copyUV.WidthInBytes = cw * 2;
        copyUV.Height = ch;
        checkCu(cuMemcpy2D(&copyUV), "copy UV plane");

        checkCu(cuGraphicsUnmapResources(2, resources, 0), "unmap movie textures");

        decoder->releaseFrame();
    }

    gl->glUseProgram(shaderProgram_);
    gl->glUniform1i(gl->glGetUniformLocation(shaderProgram_, "texY"), 0);
    gl->glUniform1i(gl->glGetUniformLocation(shaderProgram_, "texUV"), 1);

    gl->glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureUV_);
    gl->glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureY_);

    glBegin(GL_QUADS);

    glTexCoord2f(tX,tY);
    glVertex2f(0.,0.);

    glTexCoord2f(tX+tW,tY);
    glVertex2f(1.,0.);

    glTexCoord2f(tX+tW,tY+tH);
    glVertex2f(1.,1.);

    glTexCoord2f(tX,tY+tH);
    glVertex2f(0.,1.);

    glEnd();

    gl->glUseProgram(0);

    glPopAttrib();
}

void
Movie::nextFrame(bool skip)
{
    if (! skip)
      last_rendered_frame_ = g_frameCount;
}
