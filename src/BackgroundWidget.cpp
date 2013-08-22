#include "BackgroundWidget.h"

#include <QtGui>

#include "main.h"
#include "ContentFactory.h"
#include "ContentWindowManager.h"

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
    setBackgroundContentFromUri(previousBackgroundURI_);

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

    if(setBackgroundContentFromUri(filename))
    {
        backgroundLabel_->setText(filename);
        g_configuration->setBackgroundUri(filename);
    }
}


bool BackgroundWidget::setBackgroundContentFromUri(const QString& filename)
{
    if(!filename.isEmpty())
    {
        boost::shared_ptr<Content> c = ContentFactory::getContent(filename.toStdString());

        if(c != NULL)
        {
            boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

            g_displayGroupManager->setBackgroundContentWindowManager(cwm);

            return true;
        }
        else
        {
            QMessageBox messageBox;
            messageBox.setText(tr("Error: Unsupported file format"));
            messageBox.exec();
        }
    }
    return false;
}

void BackgroundWidget::removeBackground()
{
    g_displayGroupManager->setBackgroundContentWindowManager(boost::shared_ptr<ContentWindowManager>());

    backgroundLabel_->setText("");
    g_configuration->setBackgroundUri("");
}
