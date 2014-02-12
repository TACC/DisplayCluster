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

#include "DisplayGroupGraphicsScene.h"
#include "globals.h"
#include "configuration/Configuration.h"
#include "DisplayGroupManager.h"
#include "Marker.h"

DisplayGroupGraphicsScene::DisplayGroupGraphicsScene()
{
    setSceneRect(0., 0., 1., 1.);

    // for the tiled display
    addRect(0., 0., 1., 1.);

    // add rectangles for the tiles
    refreshTileRects();

    // get marker for this scene
    markers_.push_back( g_displayGroupManager->getNewMarker());
}

void DisplayGroupGraphicsScene::refreshTileRects()
{
    // add rectangles for tiled display and each monitor

    // tiled display parameters
    int numTilesX = g_configuration->getTotalScreenCountX();
    int screenWidth = g_configuration->getScreenWidth();
    int mullionWidth = g_configuration->getMullionWidth();

    int numTilesY = g_configuration->getTotalScreenCountY();
    int screenHeight = g_configuration->getScreenHeight();
    int mullionHeight = g_configuration->getMullionHeight();

    int totalWidth = g_configuration->getTotalWidth();
    int totalHeight = g_configuration->getTotalHeight();

    // rendering parameters

    // border
    QPen pen;
    pen.setColor(QColor(0,0,0));

    // fill color / opacity
    QBrush brush = QBrush(QColor(0, 0, 0, 32));

    // clear existing tile rects
    for(unsigned int i=0; i<tileRects_.size(); i++)
    {
        delete tileRects_[i];
    }

    tileRects_.clear();

    for(int i=0; i<numTilesX; i++)
    {
        for(int j=0; j<numTilesY; j++)
        {
            // border calculations
            double left = (double)i / (double)numTilesX * ( (double)numTilesX * (double)screenWidth ) + (double)i * (double)mullionWidth;
            double right = left + (double)screenWidth;
            double bottom = j / (double)numTilesY * ( (double)numTilesY * (double)screenHeight ) + (double)j * (double)mullionHeight;
            double top = bottom + (double)screenHeight;

            // normalize to 0->1
            left /= (double)totalWidth;
            right /= (double)totalWidth;
            bottom /= (double)totalHeight;
            top /= (double)totalHeight;

            tileRects_.push_back(addRect(left, bottom, right-left, top-bottom, pen, brush));
        }
    }
}

bool DisplayGroupGraphicsScene::event(QEvent *event)
{
    switch( event->type())
    {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        if( g_displayGroupManager->getOptions()->getShowTouchPoints( ))
        {
            QTouchEvent* touchEvent = static_cast< QTouchEvent* >( event );

            while( markers_.size() < size_t( touchEvent->touchPoints().size( )))
                markers_.push_back( g_displayGroupManager->getNewMarker());

            for( int i = 0; i < touchEvent->touchPoints().size(); ++i )
            {
                markers_[i]->setPosition(touchEvent->touchPoints()[i].normalizedPos().x(),
                                         touchEvent->touchPoints()[i].normalizedPos().y());
            }
        }
        return QGraphicsScene::event(event);
    }
    case QEvent::KeyPress:
    {
        QKeyEvent *k = static_cast<QKeyEvent*>(event);

        // Override default behaviour to process TAB key events
        QGraphicsScene::keyPressEvent(k);

        if( k->key() == Qt::Key_Backtab ||
            k->key() == Qt::Key_Tab ||
           (k->key() == Qt::Key_Tab && (k->modifiers() & Qt::ShiftModifier)))
        {
            event->accept();
        }
        return true;
    }
    default:
        return QGraphicsScene::event(event);
    }
}

void DisplayGroupGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    markers_[0]->setPosition(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mouseMoveEvent(event);
}

void DisplayGroupGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    markers_[0]->setPosition(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mousePressEvent(event);
}

void DisplayGroupGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    markers_[0]->setPosition(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mouseReleaseEvent(event);
}
