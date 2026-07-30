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

#ifndef MOVIE_H
#define MOVIE_H

#include "FactoryObject.h"
#include "Decoder.h"

#include <QOpenGLFunctions>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <chrono>
#include <iostream>

#include <cuda.h>
#include <cudaGL.h>

using namespace std::chrono;

class Movie : public FactoryObject {

    public:
        Movie(std::string uri);
        ~Movie();

        void getDimensions(int &width, int &height);
        void render(float tX, float tY, float tW, float tH);
        void nextFrame(bool);

        int getLastRenderedFrame() { return last_rendered_frame_; }
        void Pause() { decoder->Pause(); paused_ = true; }
        void Resume() { decoder->Resume(); paused_ = false; }
        bool isPaused() { return paused_; }
        void Resync(time_point<high_resolution_clock> now) { decoder->Resync(now); }
        void Freeze(time_point<high_resolution_clock> now) { decoder->Freeze(now); }
        bool isSynced() { return decoder->isSynced(); }
        void RequestQuit() { decoder->RequestQuit(); }

        // when true, render() keeps showing whatever's already in
        // textureY_/textureUV_ instead of consuming a fresh decoded frame -
        // set by MainWindow once per render frame from the cluster-wide
        // "is every rank showing this movie caught up" reduction, so a
        // straggler holds the whole display on its last agreed-good frame
        // instead of the rest advancing without it
        void setHold(bool hold) { hold_ = hold; }

    private:
        Decoder *decoder = NULL;
        bool hold_ = false;

        // NVDEC hands back NV12 (one full-res luma plane, one half-res
        // interleaved chroma plane) resident on the GPU; each plane is
        // registered with CUDA as its own GL texture so the decoded surface
        // can be copied device-to-device straight into it (no host
        // round-trip), and a fragment shader does the NV12->RGB conversion
        // at draw time
        GLuint textureY_;
        GLuint textureUV_;
        CUgraphicsResource cudaResourceY_;
        CUgraphicsResource cudaResourceUV_;
        GLuint shaderProgram_;
        bool initialized_;
        bool paused_;

        int last_rendered_frame_ = -1;
};

#endif
