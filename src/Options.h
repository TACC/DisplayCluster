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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "config.h"
#include <QtGui>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

class Options : public QObject {
    Q_OBJECT

    public:
        Options();

        bool getShowWindowBorders();
        bool getShowMouseCursor();
        bool getShowMovieControls();
        bool getShowTestPattern();
        bool getEnableMullionCompensation();
        bool getShowZoomContext();
        bool getEnableStreamingSynchronization();
        bool getShowStreamingSegments();
        bool getShowStreamingStatistics();

#if ENABLE_SKELETON_SUPPORT
        bool getShowSkeletons();
#endif

    public slots:
        void setShowWindowBorders(bool set);
        void setShowMouseCursor(bool set);
        void setShowMovieControls(bool set);
        void setShowTestPattern(bool set);
        void setEnableMullionCompensation(bool set);
        void setShowZoomContext(bool set);
        void setEnableStreamingSynchronization(bool set);
        void setShowStreamingSegments(bool set);
        void setShowStreamingStatistics(bool set);

#if ENABLE_SKELETON_SUPPORT
        void setShowSkeletons(bool set);
#endif

    signals:
        void updated();

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & showWindowBorders_;
            ar & showMouseCursor_;
            ar & showMovieControls_;
            ar & showTestPattern_;
            ar & enableMullionCompensation_;
            ar & showZoomContext_;
            ar & enableStreamingSynchronization_;
            ar & showStreamingSegments_;
            ar & showStreamingStatistics_;

#if ENABLE_SKELETON_SUPPORT
            ar & showSkeletons_;
#endif
        }

        bool showWindowBorders_;
        bool showMouseCursor_;
        bool showMovieControls_;
        bool showTestPattern_;
        bool enableMullionCompensation_;
        bool showZoomContext_;
        bool enableStreamingSynchronization_;
        bool showStreamingSegments_;
        bool showStreamingStatistics_;

#if ENABLE_SKELETON_SUPPORT
        bool showSkeletons_;
#endif
};

#endif
