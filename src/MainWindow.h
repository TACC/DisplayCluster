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

// increment this whenever when serialized state information changes
#define CONTENTS_FILE_VERSION_NUMBER 1

#include "config.h"
#include "GLWindow.h"
#include <QtGui>
#include <QtWidgets>
#include <boost/shared_ptr.hpp>
#include <functional>
#include <vector>

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:

        MainWindow();

        bool getConstrainAspectRatio();

        boost::shared_ptr<GLWindow> getGLWindow(int index=0);
        boost::shared_ptr<GLWindow> getActiveGLWindow();
        std::vector<boost::shared_ptr<GLWindow> > getGLWindows();

        void loadState(QString *);

        // opens up to cols x rows of the openable files in dir, in name order,
        // tiled across the whole wall; returns how many it opened
        int openContentsGrid(QString dir, int cols, int rows);

    public slots:

        void openContent();
        void openContentsDirectory();
        void clearContents();
        void saveState();
        void loadState();
        void computeImagePyramid();
        void constrainAspectRatio(bool set);

        // brings the View menu's checkboxes back in line with the options,
        // which the remote API can change behind the menu's back
        void refreshOptionActions();

        void updateGLWindows();

        void finalize();

    signals:

        void updateGLWindowsFinished();

    private:

        std::vector<boost::shared_ptr<GLWindow> > glWindows_;
        boost::shared_ptr<GLWindow> activeGLWindow_;

        bool constrainAspectRatio_;

        // each View menu checkbox and the option it shows
        std::vector<std::pair<QAction *, std::function<bool()> > > optionActions_;

        // polling timer for updating parallel pixel streams
        QTimer parallelPixelStreamTimer_;
};

#endif
