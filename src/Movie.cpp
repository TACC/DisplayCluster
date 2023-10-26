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

Movie::Movie(std::string uri)
{
    initialized_ = false;
    paused_ = false;
    decoder.Setup(uri);
}

Movie::~Movie()
{
    if(textureBound_ == true)
        glDeleteTextures(1, &textureId_);
}

void Movie::getDimensions(int &width, int &height)
{
    decoder.getFrameDimensions(width, height);
}

void Movie::render(float tX, float tY, float tW, float tH)
{
    updateRenderedFrameCount();

    int w, h;
    decoder.getFrameDimensions(w, h);

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
    glEnable(GL_TEXTURE_2D);

    if(initialized_ != true)
    {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glShadeModel(GL_FLAT);
        glEnable(GL_DEPTH_TEST);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glGenTextures(1, &textureId_);
        glBindTexture(GL_TEXTURE_2D, textureId_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        initialized_ = true;
    }
    else
        glBindTexture(GL_TEXTURE_2D, textureId_);

    if (decoder.ready())
    {
      uint8_t *data = decoder.getFrame();
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
      decoder.releaseFrame();
    }

    glBegin(GL_QUADS);

#if 1

    glTexCoord2f(tX,tY);
    glVertex2f(0.,0.);

    glTexCoord2f(tX+tW,tY);
    glVertex2f(1.,0.);

    glTexCoord2f(tX+tW,tY+tH);
    glVertex2f(1.,1.);

    glTexCoord2f(tX,tY+tH);
    glVertex2f(0.,1.);


#if 0
    glTexCoord2f(tX+tW, tY+tH); glVertex2f( 1., -1.);
    glTexCoord2f(tX+tW, tY); glVertex2f( 1.,  1.);
    glTexCoord2f(tX, tY); glVertex2f(-1.,  1.);
    glTexCoord2f(tX, tY+tH); glVertex2f(-1., -1.);
    glTexCoord2f(tX, tY+tH); glVertex2f(-1., -1.);
    glTexCoord2f(tX, tY); glVertex2f(-1.,  1.);
    glTexCoord2f(tX+tW, tY); glVertex2f( 1.,  1.);
    glTexCoord2f(tX+tW, tY+tH); glVertex2f( 1., -1.);
#endif

#else

    static int i = 0;
    float r = (i & 0x1) ? 1. : 0.;
    float g = (i & 0x2) ? 1. : 0.;
    float b = (i & 0x4) ? 1. : 0.;
    i ++;

    glColor4f(r, g, b, 1);

    glVertex2f( 1., -1.);
    glVertex2f( 1.,  1.);
    glVertex2f(-1.,  1.);
    glVertex2f(-1., -1.);
#endif

    glEnd();

    glPopAttrib();
}

void
Movie::nextFrame(bool skip)
{
    if (skip && !paused_)
    {
      // std::cerr << "pause on " << g_mpiRank << "\n";
      decoder.Pause();
      paused_ = true;
    }
    else if (!skip && paused_)
    {
      // std::cerr << "resume on " << g_mpiRank << "\n";
      decoder.Resume();
      paused_ = false;
    }
}
