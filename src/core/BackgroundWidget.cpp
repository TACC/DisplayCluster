/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
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

#include "BackgroundWidget.h"

#include <QtGui>

#include "globals.h"
#include "configuration/Configuration.h"
#include "ContentFactory.h"
#include "ContentWindowManager.h"
#include "DisplayGroupManager.h"

BackgroundWidget::BackgroundWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("Background settings"));

    int frameStyle = QFrame::Sunken | QFrame::Panel;

    // Get current variables

    previousColor_ = g_configuration->getBackgroundColor();
    previousBackgroundURI_ = g_configuration->getBackgroundUri();

    // Color chooser

    colorLabel_ = new QLabel(previousColor_.name());
    colorLabel_->setFrameStyle(frameStyle);
    colorLabel_->setPalette(QPalette(previousColor_));
    colorLabel_->setAutoFillBackground(true);

    QPushButton *colorButton = new QPushButton(tr("Choose background color..."));
    connect(colorButton, SIGNAL(clicked()), this, SLOT(chooseColor()));


    // Background chooser

    backgroundLabel_ = new QLabel(previousBackgroundURI_);
    backgroundLabel_->setFrameStyle(frameStyle);
    QPushButton *backgroundButton = new QPushButton(tr("Choose background content..."));
    connect(backgroundButton, SIGNAL(clicked()), this, SLOT(openBackgroundContent()));

    QPushButton *backgroundClearButton = new QPushButton(tr("Remove background"));
    connect(backgroundClearButton, SIGNAL(clicked()), this, SLOT(removeBackground()));


    // Standard buttons

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel, Qt::Horizontal, this);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));


    // Layout

    QGridLayout *layout = new QGridLayout;
    layout->setColumnStretch(1, 1);
    layout->setColumnMinimumWidth(1, 250);
    setLayout(layout);

    layout->addWidget(colorButton, 0, 0);
    layout->addWidget(colorLabel_, 0, 1);
    layout->addWidget(backgroundButton, 1, 0);
    layout->addWidget(backgroundLabel_, 1, 1);
    layout->addWidget(backgroundClearButton, 2, 0);
    layout->addWidget(buttonBox, 2, 1);
}

void BackgroundWidget::accept()
{
    if (g_configuration->save())
    {
        QDialog::accept();
    }
    else
    {
        QMessageBox messageBox;
        messageBox.setText("An error occured while saving the configuration xml file. Changes cannot be saved.");
        messageBox.exec();
    }
}

void BackgroundWidget::reject()
{
    // Revert to saved settings
    colorLabel_->setText(previousColor_.name());
    backgroundLabel_->setText(previousBackgroundURI_);

    g_configuration->setBackgroundColor(previousColor_);
    g_configuration->setBackgroundUri(previousBackgroundURI_);

    g_displayGroupManager->setBackgroundColor(previousColor_);
    g_displayGroupManager->setBackgroundContentFromUri(previousBackgroundURI_);

    QDialog::reject();
}

void BackgroundWidget::chooseColor()
{
    QColor color;
    color = QColorDialog::getColor(Qt::green, this);

    if (color.isValid()) {
        colorLabel_->setText(color.name());
        colorLabel_->setPalette(QPalette(color));

        g_displayGroupManager->setBackgroundColor(color);

        // Store settings
        g_configuration->setBackgroundColor(color);
    }
}

void BackgroundWidget::openBackgroundContent()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Choose content"), QString(), ContentFactory::getSupportedFilesFilterAsString());

    if(filename.isEmpty())
        return;

    if(g_displayGroupManager->setBackgroundContentFromUri(filename))
    {
        backgroundLabel_->setText(filename);
        g_configuration->setBackgroundUri(filename);
    }
    else
    {
        QMessageBox messageBox;
        messageBox.setText(tr("Error: Unsupported file format"));
        messageBox.exec();
    }
}

void BackgroundWidget::removeBackground()
{
    g_displayGroupManager->setBackgroundContentWindowManager(ContentWindowManagerPtr());

    backgroundLabel_->setText("");
    g_configuration->setBackgroundUri("");
}
