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
#include "Marker.h"
#include "Configuration.h"
#include "DisplayGroupManager.h"

DisplayGroupGraphicsScene::DisplayGroupGraphicsScene()
{
    setSceneRect(0., 0., 1., 1.);

    // for the tiled display
    addRect(0., 0., 1., 1.);

    // add rectangles for the tiles
    refreshTileRects();

    // get marker for this scene
    marker_ = g_displayGroupManager->getNewMarker();
}

void DisplayGroupGraphicsScene::refreshTileRects()
{
    // add rectangles for tiled display and each monitor

    // tiled display parameters
    int numTilesWidth = g_configuration->getNumTilesWidth();
    int screenWidth = g_configuration->getScreenWidth();
    int mullionWidth = g_configuration->getMullionWidth();

    int numTilesHeight = g_configuration->getNumTilesHeight();
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

    for(int i=0; i<numTilesWidth; i++)
    {
        for(int j=0; j<numTilesHeight; j++)
        {
            // border calculations
            double left = (double)i / (double)numTilesWidth * ( (double)numTilesWidth * (double)screenWidth ) + (double)i * (double)mullionWidth;
            double right = left + (double)screenWidth;
            double bottom = j / (double)numTilesHeight * ( (double)numTilesHeight * (double)screenHeight ) + (double)j * (double)mullionHeight;
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

void DisplayGroupGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    marker_->setPosition(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mouseMoveEvent(event);
}

void DisplayGroupGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    marker_->setPosition(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mousePressEvent(event);
}

void DisplayGroupGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    marker_->setPosition(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mouseReleaseEvent(event);
}
