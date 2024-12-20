#include <iostream>
#include <string>

#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QGridLayout>
#include <QPushButton>
#include <QThread>

#include <QtWidgets>

#include "EmbeddedPython.h"

class PythonObject : public QObject
{
    Q_OBJECT

public:
    PythonObject(int, char **);

    bool RunString(std::string, std::string&, std::string&);

public slots:

    void foo();

private:
    EmbeddedPython* python;
};

class PythonConsole : public QMainWindow
{
    Q_OBJECT

public:

    PythonConsole(int argc, char **argv);

private:
    QWidget *cw;
    QGridLayout *grid;
    QTextEdit *inpt, *outpt;
    QPushButton *button;

    QThread *pythonThread;

    PythonObject *pythonObject;

public slots:
    void doit();
    void loadFile();
};
