/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#include "DockToolbar.h"

#include <QPainter>
#include <QColor>

DockToolbar::DockToolbar(const unsigned int height)
    : height_(height)
{
}

void DockToolbar::render(QImage& buffer)
{
    QPainter painter;
    painter.begin(&buffer);

    area_ = QRect(0,0,buffer.width(), height_);
    QBrush brush;
    brush.setColor(Qt::gray);
    brush.setStyle(Qt::SolidPattern);
    painter.fillRect(area_, brush);

    int i = 0;
    foreach(ToolbarButton button, buttons)
    {
        drawButton(painter, button, i++);
    }

    painter.end();
}

void DockToolbar::addButton(const ToolbarButton& button)
{
    buttons.push_back(button);
}

unsigned int DockToolbar::getHeight() const
{
    return height_;
}

QString DockToolbar::getClickResult(const QPoint pos) const
{
    if (!area_.contains(pos))
        return QString();

    const int index = (float)pos.x() / (float)area_.width() * buttons.size();

    return buttons.at(index).command;
}

void DockToolbar::drawButton(QPainter& painter, const ToolbarButton& button, const int index)
{
    const unsigned int n = buttons.size();

    const unsigned int margin = area_.height() * 0.1;
    const unsigned int buttonWidth = (area_.width() - (n+1)*margin) / n;

    QPoint topLeft(margin + index*(buttonWidth+margin), margin);
    QSize buttonSize(buttonWidth, height_ - 2*margin);
    QRect buttonArea(topLeft, buttonSize);

    QBrush brush;
    brush.setColor(Qt::lightGray);
    brush.setStyle(Qt::SolidPattern);
    painter.fillRect(buttonArea, brush);

    QRect imageArea(buttonArea);
    imageArea.setWidth(imageArea.height());
    painter.drawImage(imageArea, button.icon);

    QRect textArea(imageArea.topRight() + QPoint(margin, margin),
                   buttonArea.bottomRight() - QPoint(margin, margin));
    QFont font("Arial", textArea.height());
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(textArea, button.caption, QTextOption(Qt::AlignVCenter));
}
