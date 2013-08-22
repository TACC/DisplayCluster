#ifndef BACKGROUNDWIDGET_H
#define BACKGROUNDWIDGET_H

#include <QDialog>

class QLabel;

class BackgroundWidget : public QDialog
{
    Q_OBJECT
public:
    explicit BackgroundWidget(QWidget *parent = 0);

public slots:
    void accept();
    void reject();

private slots:
    void chooseColor();
    void openBackgroundContent();
    void removeBackground();

private:
    QLabel *colorLabel_;
    QLabel *backgroundLabel_;

    QColor previousColor_;
    QString previousBackgroundURI_;

    bool setBackgroundContentFromUri(const QString &filename);
};

#endif // BACKGROUNDWIDGET_H
