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
