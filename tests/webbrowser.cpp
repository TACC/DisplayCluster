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
namespace ut = boost::unit_test;

#include "globals.h"
#include "Options.h"
#include "configuration/Configuration.h"
#include "WebkitPixelStreamer.h"

#include <QApplication>
#include <QWebElementCollection>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>

#include <X11/Xlib.h>

#define TEST_PAGE_URL         "webgl_interaction.html"
#define CONFIG_TEST_FILENAME  "configuration.xml"

namespace ut = boost::unit_test;

bool hasGLXDisplay()
{
    Display* display = XOpenDisplay( 0 );
    if( !display )
        return false;
    int major, event, error;
    const bool hasGLX = XQueryExtension( display, "GLX", &major, &event,
                                         &error );
    XCloseDisplay( display );
    return hasGLX;
}

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

        // To test wheel events the WebkitPixelStreamer needs access to the g_configuration element
        OptionsPtr options(new Options());
        g_configuration = new Configuration(CONFIG_TEST_FILENAME, options);
    }
    ~GlobalQtApp()
    {
        delete g_configuration;
        delete app;
    }

    QApplication* app;
};

BOOST_GLOBAL_FIXTURE( GlobalQtApp );

BOOST_AUTO_TEST_CASE( test_webgl_support )
{
    if( !hasGLXDisplay( ))
        return;

    // load the webgl website, exec() returns when loading is finished
    WebkitPixelStreamer* streamer = new WebkitPixelStreamer( "testBrowser" );
    QObject::connect( streamer->getView(), SIGNAL(loadFinished(bool)),
                      QApplication::instance(), SLOT(quit()));
    streamer->setUrl( TEST_PAGE_URL );
    QApplication::instance()->exec();

    QWebPage* page = streamer->getView()->page();
    BOOST_REQUIRE( page );
    QWebFrame* frame = page->mainFrame();
    BOOST_REQUIRE( frame );
    QWebElementCollection webglCanvases = frame->findAllElements( "canvas[id=webgl-canvas]" );
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
    if( !hasGLXDisplay( ))
        return;

    // load the webgl website, exec() returns when loading is finished
    WebkitPixelStreamer* streamer = new WebkitPixelStreamer( "testBrowser" );
    QObject::connect( streamer->getView(), SIGNAL(loadFinished(bool)),
                      QApplication::instance(), SLOT(quit()));
    streamer->setUrl( TEST_PAGE_URL );
    QApplication::instance()->exec();

    QWebPage* page = streamer->getView()->page();
    BOOST_REQUIRE( page );
    QWebFrame* frame = page->mainFrame();
    BOOST_REQUIRE( frame );

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


BOOST_AUTO_TEST_CASE( test_webgl_click )
{
    // load the webgl website, exec() returns when loading is finished
    WebkitPixelStreamer* streamer = new WebkitPixelStreamer( "testBrowser" );
    QObject::connect( streamer->getView(), SIGNAL(loadFinished(bool)),
                      QApplication::instance(), SLOT(quit()));
    streamer->setUrl( TEST_PAGE_URL );
    QApplication::instance()->exec();

    QWebPage* page = streamer->getView()->page();
    BOOST_REQUIRE( page );
    QWebFrame* frame = page->mainFrame();
    BOOST_REQUIRE( frame );

    // Normalized mouse coordinates
    InteractionState clickState;
    clickState.mouseX = 0.1;
    clickState.mouseY = 0.1;
    clickState.mouseLeft = true;
    clickState.type = InteractionState::EVT_CLICK;

    streamer->updateInteractionState(clickState);

    const int expectedPosX = clickState.mouseX * streamer->size().width() /
                             streamer->getView()->zoomFactor();
    const int expectedPosY = clickState.mouseY * streamer->size().height() /
                             streamer->getView()->zoomFactor();

    QString jsX = QString("lastMouseX == %1;").arg(expectedPosX);
    QString jsY = QString("lastMouseY == %1;").arg(expectedPosY);

    BOOST_CHECK( frame->evaluateJavaScript(jsX).toBool());
    BOOST_CHECK( frame->evaluateJavaScript(jsY).toBool());

    delete streamer;
}



BOOST_AUTO_TEST_CASE( test_webgl_wheel )
{
    // load the webgl website, exec() returns when loading is finished
    WebkitPixelStreamer* streamer = new WebkitPixelStreamer( "testBrowser" );
    QObject::connect( streamer->getView(), SIGNAL(loadFinished(bool)),
                      QApplication::instance(), SLOT(quit()));
    streamer->setUrl( TEST_PAGE_URL );
    QApplication::instance()->exec();

    QWebPage* page = streamer->getView()->page();
    BOOST_REQUIRE( page );
    QWebFrame* frame = page->mainFrame();
    BOOST_REQUIRE( frame );

    // Normalized mouse coordinates
    InteractionState wheelState;
    wheelState.mouseX = 0.1;
    wheelState.mouseY = 0.1;
    wheelState.dy = 0.05;
    wheelState.type = InteractionState::EVT_WHEEL;

    streamer->updateInteractionState(wheelState);

    const int expectedPosX = wheelState.mouseX * streamer->size().width() /
                             streamer->getView()->zoomFactor();
    const int expectedPosY = wheelState.mouseY * streamer->size().height() /
                             streamer->getView()->zoomFactor();
    const int expectedWheelDelta = wheelState.dy * g_configuration->getTotalHeight();

    QString jsX = QString("lastMouseX == %1;").arg(expectedPosX);
    QString jsY = QString("lastMouseY == %1;").arg(expectedPosY);
    QString jsD = QString("wheelDelta == %1;").arg(expectedWheelDelta);

    BOOST_CHECK( frame->evaluateJavaScript(jsX).toBool());
    BOOST_CHECK( frame->evaluateJavaScript(jsY).toBool());
    BOOST_CHECK( frame->evaluateJavaScript(jsD).toBool());

    delete streamer;
}
