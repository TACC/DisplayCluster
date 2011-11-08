#ifndef __PYTHON_CONSOLE_H__
#define __PYTHON_CONSOLE_H__

#include <Python.h>
#include <PythonQt.h>
#include <QtGui>
#include <QMainWindow>
#include <QTextEdit>

class MyPythonQt : public QObject
{
   Q_OBJECT

public:
   void evalStringSynchronous(const QString& str);

signals:
   void eval_done();
   void load_done();
   void myPythonStdOut(const QString&);
   void myPythonStdErr(const QString&);

public slots:
   void evalString(const QString& str);
   void loadFile(const QString &str);

private slots:
   void pythonStdOut(const QString&);
   void pythonStdErr(const QString&);
};

class PythonTypeIn : public QTextEdit
{
    Q_OBJECT

public:
    PythonTypeIn();
    QString getContent() {return content;}
    void clearContent() {content = "";}
    void prompt(const char *c);

public slots:
    void python_eval_finished();

signals:
    void python_command(const QString &);

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    QString content;
    int tail;
    int indent;
};      

class PythonConsole : public QMainWindow
{
    Q_OBJECT

public:
    static PythonConsole *self();
   static void init();

protected:
    PythonConsole();
    ~PythonConsole();

signals:
    void python_file(const QString &);

public slots:
    void selectPythonScript();
    void python_load_finished();
    void evalString(const QString& str);
    void put_result(const QString& str);
    void clear_input();
    void clear_output();
    void clear();

private:
    QMenu   *fileMenu;
    QMenu   *editMenu;
    QThread *pythonThread;
    QString filename;
    PythonTypeIn *typeIn;
    QTextEdit *output;
};      

#endif  

