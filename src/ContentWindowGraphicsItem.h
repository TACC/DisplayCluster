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

#ifndef CONTENT_WINDOW_GRAPHICS_ITEM_H
#define CONTENT_WINDOW_GRAPHICS_ITEM_H

#include "ContentWindowInterface.h"
#include <QtGui>
#include <boost/shared_ptr.hpp>

class ContentWindowManager;

class ContentWindowGraphicsItem : public QGraphicsRectItem, public ContentWindowInterface {

    public:

        ContentWindowGraphicsItem(boost::shared_ptr<ContentWindowManager> contentWindowManager);

        // QGraphicsRectItem painting
        virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget=0);

        // re-implemented ContentWindowInterface slots
        virtual void adjustSize( const SizeState state, ContentWindowInterface * source=NULL );
        virtual void setCoordinates(double x, double y, double w, double h, ContentWindowInterface * source=NULL);
        virtual void setPosition(double x, double y, ContentWindowInterface * source=NULL);
        virtual void setSize(double w, double h, ContentWindowInterface * source=NULL);
        virtual void setCenter(double centerX, double centerY, ContentWindowInterface * source=NULL);
        virtual void setZoom(double zoom, ContentWindowInterface * source=NULL);
        virtual void setSelected(bool selected, ContentWindowInterface * source=NULL);

        // increment the Z value of this item
        void setZToFront();

    protected:

        QVariant itemChange(GraphicsItemChange change, const QVariant &value);

        // QGraphicsRectItem events
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
        virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
        virtual void wheelEvent(QGraphicsSceneWheelEvent * event);

    private:

        // resizing state
        bool resizing_;

        bool moving_;

        // counter used to determine stacking order in the UI
        static qreal zCounter_;
};

#endif
