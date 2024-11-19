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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "main.h"

// increment this whenever when serialized state information changes
#define CONTENTS_FILE_VERSION_NUMBER 1

#include "config.h"
#include "GLWindow.h"
#include <QtWidgets>
#include <QGLWidget>
#include <boost/shared_ptr.hpp>

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:

        MainWindow();

        bool getConstrainAspectRatio();

        boost::shared_ptr<GLWindow> getGLWindow(int index=0);
        boost::shared_ptr<GLWindow> getActiveGLWindow();
        std::vector<boost::shared_ptr<GLWindow> > getGLWindows();

        void loadState(QString *);

    public slots:

        void openContent();
        void openContentsDirectory();
        void clearContents();
        void saveState();
        void loadState();
        void computeImagePyramid();
        void constrainAspectRatio(bool set);

#if ENABLE_SKELETON_SUPPORT
        void setEnableSkeletonTracking(bool enable);
#endif

        void updateGLWindows();

        void finalize();

    signals:

        void updateGLWindowsFinished();

#if ENABLE_SKELETON_SUPPORT
        void enableSkeletonTracking();
        void disableSkeletonTracking();
#endif

    private:

        std::vector<boost::shared_ptr<GLWindow> > glWindows_;
        boost::shared_ptr<GLWindow> activeGLWindow_;

        bool constrainAspectRatio_;

        // polling timer for updating parallel pixel streams
        QTimer parallelPixelStreamTimer_;
};

#endif
