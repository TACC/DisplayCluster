/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>     */
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

#define BOOST_TEST_MODULE WebBrowser
#include <boost/test/unit_test.hpp>

#include <QApplication>
#include <QWebElementCollection>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>

#include "WebkitPixelStreamer.h"

namespace ut = boost::unit_test;


// We need a global fixture because a bug in QApplication prevents
// deleting then recreating a QApplication in the same process.
// https://bugreports.qt-project.org/browse/QTBUG-7104
struct GlobalQtApp
{
    GlobalQtApp()
    {
        // need QApplication to instantiate WebkitPixelStreamer
        ut::master_test_suite_t& testSuite = ut::framework::master_test_suite();
        app = new QApplication( testSuite.argc, testSuite.argv );
    }
    ~GlobalQtApp()
    {
        delete app;
    }

    QApplication* app;
};

BOOST_GLOBAL_FIXTURE( GlobalQtApp );

BOOST_AUTO_TEST_CASE( test_webgl_support )
{
    // load the webgl website, exec() returns when loading is finished
    WebkitPixelStreamer* streamer = new WebkitPixelStreamer( "testBrowser" );
    QObject::connect( streamer->getView(), SIGNAL(loadFinished(bool)),
                      QApplication::instance(), SLOT(quit()));
    streamer->setUrl( "http://get.webgl.org" );
    QApplication::instance()->exec();

    QWebPage* page = streamer->getView()->page();
    BOOST_REQUIRE( page );
    QWebFrame* frame = page->mainFrame();
    BOOST_REQUIRE( frame );
    QWebElementCollection webglCanvases = frame->findAllElements( "canvas[id=webgl-logo]" );
    BOOST_REQUIRE_EQUAL( webglCanvases.count(), 1 );

    // http://stackoverflow.com/questions/11871077/proper-way-to-detect-webgl-support
    QVariant hasContext = frame->evaluateJavaScript("var hasContext = false;"
                                                    "if( window.WebGLRenderingContext ) {"
                                                    "  hasContext = true;"
                                                    "}");

    QVariant hasGL = frame->evaluateJavaScript("var hasGL = false;"
                                               "gl = canvas.getContext(\"webgl\");"
                                               "if( gl ) {"
                                               "  hasGL = true;"
                                               "}");

    QVariant hasExperimentalGL = frame->evaluateJavaScript("var hasGL = false;"
                                                           "gl = canvas.getContext(\"experimental-webgl\");"
                                                           "if( gl ) {"
                                                           "  hasGL = true;"
                                                           "}");

    BOOST_CHECK( hasContext.toBool( ));
    BOOST_CHECK( hasGL.toBool() || hasExperimentalGL.toBool( ));

    delete streamer;
}

BOOST_AUTO_TEST_CASE( test_webgl_interaction )
{
    // load the webgl website, exec() returns when loading is finished
    WebkitPixelStreamer* streamer = new WebkitPixelStreamer( "testBrowser" );
    QObject::connect( streamer->getView(), SIGNAL(loadFinished(bool)),
                      QApplication::instance(), SLOT(quit()));
    streamer->setUrl( "./webgl_interaction.html" );
    QApplication::instance()->exec();

    QWebPage* page = streamer->getView()->page();
    BOOST_REQUIRE( page );
    QWebFrame* frame = page->mainFrame();
    BOOST_REQUIRE( frame );
    QWebElementCollection webglCanvases = frame->findAllElements( "canvas[id=webgl-canvas]" );
    BOOST_REQUIRE_EQUAL( webglCanvases.count(), 1 );

    // Normalized mouse coordinates
    InteractionState pressState;
    pressState.mouseX = 0.1;
    pressState.mouseY = 0.1;
    pressState.mouseLeft = true;
    pressState.type = InteractionState::EVT_PRESS;

    InteractionState moveState;
    moveState.mouseX = 0.2;
    moveState.mouseY = 0.2;
    moveState.mouseLeft = true;
    moveState.type = InteractionState::EVT_MOVE;

    InteractionState releaseState;
    releaseState.mouseX = 0.2;
    releaseState.mouseY = 0.2;
    releaseState.mouseLeft = true;
    releaseState.type = InteractionState::EVT_RELEASE;

    streamer->updateInteractionState(pressState);
    streamer->updateInteractionState(moveState);
    streamer->updateInteractionState(releaseState);

    const int expectedDisplacementX = (releaseState.mouseX-pressState.mouseX) *
                                streamer->size().width() / streamer->getView()->zoomFactor();
    const int expectedDisplacementY = (releaseState.mouseY-pressState.mouseY) *
                                streamer->size().height() / streamer->getView()->zoomFactor();

    QString jsX = QString("deltaX == %1;").arg(expectedDisplacementX);
    QString jsY = QString("deltaY == %1;").arg(expectedDisplacementY);

    QVariant validDeltaX = frame->evaluateJavaScript(jsX);
    QVariant validDeltaY = frame->evaluateJavaScript(jsY);

    BOOST_CHECK( validDeltaX.toBool());
    BOOST_CHECK( validDeltaY.toBool());

    delete streamer;
}
