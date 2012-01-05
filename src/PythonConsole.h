#ifndef PYTHON_CONSOLE_H
#define PYTHON_CONSOLE_H

#include <Python.h>
#include <PythonQt.h>
#include <QtGui>

class MyPythonQt : public QObject
{
    Q_OBJECT

    signals:

        void evalDone();
        void loadDone();
        void myPythonStdOut(const QString &);
        void myPythonStdErr(const QString &);

    public slots:

        void evalString(QString * str);
        void loadFile(QString * str);

    private slots:

        void pythonStdOut(const QString&);
        void pythonStdErr(const QString&);
};

class PythonTypeIn : public QTextEdit
{
    Q_OBJECT

    public:

        PythonTypeIn();
        QString getContent();
        void clearContent();
        void prompt(bool);

    signals:

        void pythonCommand(QString *);

    public slots:

        void pythonEvalFinished();
        void insertCompletion(const QString&);

    protected:

        void handleTabCompletion();
        void keyPressEvent(QKeyEvent * event);
        int commandPromptPosition();
        bool verifySelectionBeforeDeletion();
        void executeLine(bool storeOnly);
        void changeHistory();

    private:

        QCompleter * completer_;
        QString commandPrompt_;
        QString content_;
        QString currentMultiLineCode_;
        int tail_;
        QStringList history_;
        int historyPosition_;
};

class PythonConsole : public QMainWindow
{
    Q_OBJECT

    public:

        static PythonConsole * self();
        static void init();

    protected:

        PythonConsole();
        ~PythonConsole();

    signals:

        void pythonFile(QString *);

    public slots:

        void selectPythonScript();
        void pythonLoadFinished();
        void evalString(QString *);
        void putResult(const QString & str);
        void clearInput();
        void clearOutput();
        void clear();

    private:

        static MyPythonQt * thePythonQt_;
        static PythonConsole * thePythonConsole_;

        QMenu * fileMenu_;
        QMenu * editMenu_;
        QThread * pythonThread_;
        PythonTypeIn * typeIn_;
        QTextEdit * output_;
};

#endif
