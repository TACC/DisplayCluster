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

#include "PythonConsole.h"

void MyPythonQt::evalString(QString * code)
{
    PythonQtObjectPtr context = PythonQt::self()->getMainModule();
    PyObject * dict = PyModule_GetDict(context);
    PyRun_String(code->toLatin1().data(), Py_single_input, dict, dict);

    delete code;

    emit(evalDone());
}

void MyPythonQt::loadFile(QString * str)
{
    QString cmd = QString("execfile('") + *str + QString("')");
    PythonQtObjectPtr context = PythonQt::self()->getMainModule();
    PyObject * dict = PyModule_GetDict(context);
    PyRun_String(cmd.toLatin1().data(), Py_single_input, dict, dict);

    delete str;

    emit(loadDone());
}

void MyPythonQt::pythonStdOut(const QString & str)
{
    emit(myPythonStdOut(str));
}

void MyPythonQt::pythonStdErr(const QString & str)
{
    emit(myPythonStdErr(str));
}

PythonTypeIn::PythonTypeIn()
{
    prompt(false);

    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::End);
    tail_ = tc.position();

    setTabStopWidth(4);

    completer_ = new QCompleter(this);
    completer_->setWidget(this);

    QObject::connect(completer_, SIGNAL(activated(const QString &)), this, SLOT(insertCompletion(const QString &)));
}

QString PythonTypeIn::getContent()
{
    return content_;
}
void PythonTypeIn::clearContent()
{
    content_ = "";
}

void PythonTypeIn::prompt(bool storeOnly)
{
    const char * c = storeOnly ? "...> " : "py> ";
    commandPrompt_ = c;
    append(c);

    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::End);
    tail_ = tc.position();
}

void PythonTypeIn::pythonEvalFinished()
{
    void * x = PyErr_Occurred();

    if(x)
    {
        PyErr_Print();
    }

    content_ = "";
    setEnabled(true);
    prompt(false);
    setFocus(Qt::OtherFocusReason);
}

void PythonTypeIn::insertCompletion(const QString & completion)
{
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);

    if(tc.selectedText() == ".")
    {
        tc.insertText(QString(".") + completion);
    }
    else
    {
        tc = textCursor();
        tc.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
        tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        tc.insertText(completion);
        setTextCursor(tc);
    }
}

void PythonTypeIn::handleTabCompletion()
{
    QTextCursor textCursor   = this->textCursor();
    int pos = textCursor.position();
    textCursor.setPosition(commandPromptPosition());
    textCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    int startPos = textCursor.selectionStart();
    int offset = pos - startPos;
    QString text = textCursor.selectedText();
    QString textToComplete;

    int cur = offset;

    while(cur--)
    {
        QChar c = text.at(cur);

        if(c.isLetterOrNumber() || c == '.' || c == '_')
        {
            textToComplete.prepend(c);
        }
        else
        {
            break;
        }
    }

    QString lookup;
    QString compareText = textToComplete;
    int dot = compareText.lastIndexOf('.');

    if(dot != -1)
    {
        lookup = compareText.mid(0, dot);
        compareText = compareText.mid(dot + 1, offset);
    }

    if(!lookup.isEmpty() || !compareText.isEmpty())
    {
        compareText = compareText.toLower();
        QStringList found;

        QStringList l = PythonQt::self()->introspection(PythonQt::self()->getMainModule(), lookup, PythonQt::Anything);

        foreach(QString n, l)
        {
            if(n.toLower().startsWith(compareText))
            {
                found << n;
            }
        }

        if(!found.isEmpty())
        {
            completer_->setCompletionPrefix(compareText);
            completer_->setCompletionMode(QCompleter::PopupCompletion);
            completer_->setModel(new QStringListModel(found, completer_));
            completer_->setCaseSensitivity(Qt::CaseInsensitive);

            QTextCursor c = this->textCursor();
            c.movePosition(QTextCursor::StartOfWord);

            QRect cr = cursorRect(c);
            cr.setWidth(completer_->popup()->sizeHintForColumn(0)
                        + completer_->popup()->verticalScrollBar()->sizeHint().width());
            cr.translate(0, 8);

            completer_->complete(cr);
        }
        else
        {
            completer_->popup()->hide();
        }
    }
    else
    {
        completer_->popup()->hide();
    }
}

void PythonTypeIn::keyPressEvent(QKeyEvent * event)
{
    if(completer_ && completer_->popup()->isVisible())
    {
        // The following keys are forwarded by the completer to the widget
        switch(event->key())
        {
        case Qt::Key_Return:
            if(!completer_->popup()->currentIndex().isValid())
            {
                insertCompletion(completer_->currentCompletion());
                completer_->popup()->hide();
                event->accept();
            }

            event->ignore();
            return;
            break;

        case Qt::Key_Enter:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            event->ignore();
            return; // let the completer do default behavior

        default:
            break;
        }
    }

    bool eventHandled = false;
    QTextCursor textCursor   = this->textCursor();
    int key = event->key();

    switch(key)
    {
    case Qt::Key_Left:

        // Moving the cursor left is limited to the position
        // of the command prompt.
        if(textCursor.position() <= commandPromptPosition())
        {
            QApplication::beep();
            eventHandled = true;
        }

        break;

    case Qt::Key_Up:

        // Display the previous command in the history
        if(historyPosition_ > 0)
        {
            historyPosition_--;
            changeHistory();
        }

        eventHandled = true;
        break;

    case Qt::Key_Down:

        // Display the next command in the history
        if(historyPosition_ + 1 < history_.count())
        {
            historyPosition_++;
            changeHistory();
        }

        eventHandled = true;
        break;

    case Qt::Key_Return:
        executeLine(event->modifiers() & Qt::ShiftModifier);
        eventHandled = true;
        break;

    case Qt::Key_Backspace:
        if(textCursor.hasSelection())
        {
            cut();
            eventHandled = true;
        }
        else
        {
            // Intercept backspace key event to check if
            // deleting a character is allowed. It is not
            // allowed, if the user wants to delete the
            // command prompt.
            if(textCursor.position() <= commandPromptPosition())
            {
                QApplication::beep();
                eventHandled = true;
            }
        }

        break;

    case Qt::Key_Delete:
        cut();
        eventHandled = true;
        break;

    default:
        if(key >= Qt::Key_Space && key <= Qt::Key_division)
        {
            if(textCursor.hasSelection() && !verifySelectionBeforeDeletion())
            {
                // The selection must not be deleted.
                eventHandled = true;
            }
            else
            {
                // The key is an input character, check if the cursor is
                // behind the last command prompt, else inserting the
                // character is not allowed.
                int commandPromptPosition = this->commandPromptPosition();

                if(textCursor.position() < commandPromptPosition)
                {
                    textCursor.setPosition(commandPromptPosition);
                    setTextCursor(textCursor);
                }
            }
        }
    }

    if(eventHandled)
    {
        completer_->popup()->hide();
        event->accept();
    }
    else
    {
        QTextEdit::keyPressEvent(event);
        QString text = event->text();

        if(!text.isEmpty())
        {
            handleTabCompletion();
        }
        else
        {
            completer_->popup()->hide();
        }

        eventHandled = true;
    }
}

int PythonTypeIn::commandPromptPosition()
{
    QTextCursor textCursor(this->textCursor());
    textCursor.movePosition(QTextCursor::End);

    return textCursor.block().position() + commandPrompt_.length();
}

bool PythonTypeIn::verifySelectionBeforeDeletion()
{
    bool deletionAllowed = true;
    QTextCursor textCursor = this->textCursor();
    int commandPromptPosition = this->commandPromptPosition();
    int selectionStart = textCursor.selectionStart();
    int selectionEnd = textCursor.selectionEnd();

    if(textCursor.hasSelection())
    {
        // Selected text may only be deleted after the last command prompt.
        // If the selection is partly after the command prompt set the selection
        // to the part and deletion is allowed. If the selection occurs before the
        // last command prompt, then deletion is not allowed.
        if(selectionStart < commandPromptPosition ||
                selectionEnd < commandPromptPosition)
        {
            // Assure selectionEnd is bigger than selection start
            if(selectionStart > selectionEnd)
            {
                int tmp         = selectionEnd;
                selectionEnd    = selectionStart;
                selectionStart  = tmp;
            }

            if(selectionEnd < commandPromptPosition)
            {
                // Selection is completely before command prompt,
                // so deletion is not allowed.
                QApplication::beep();
                deletionAllowed = false;
            }
            else
            {
                // The selectionEnd is after the command prompt, so set
                // the selection start to the commandPromptPosition.
                selectionStart = commandPromptPosition;
                textCursor.setPosition(selectionStart);
                textCursor.setPosition(selectionStart, QTextCursor::KeepAnchor);
                setTextCursor(textCursor);
            }
        }
    }
    else     // if (hasSelectedText())
    {
        // When there is no selected text, deletion is not allowed before the
        // command prompt.
        if(textCursor.position() < commandPromptPosition)
        {
            QApplication::beep();
            deletionAllowed = false;
        }
    }

    return deletionAllowed;
}

void PythonTypeIn::executeLine(bool storeOnly)
{
    QTextCursor textCursor = this->textCursor();
    textCursor.movePosition(QTextCursor::End);

    // Select the text from the command prompt until the end of the block
    // and get the selected text.
    textCursor.setPosition(commandPromptPosition());
    textCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    QString code = textCursor.selectedText();

    // i don't know where this trailing space is coming from, blast it!
    if(code.endsWith(" "))
    {
        code.truncate(code.length() - 1);
    }

    if(!code.isEmpty())
    {
        // Update the history
        history_ << code;
        historyPosition_ = history_.count();
        currentMultiLineCode_ += code + "\n";

        if(!storeOnly)
        {
            QString * emittedString = new QString(currentMultiLineCode_);
            emit pythonCommand(emittedString);
            currentMultiLineCode_ = "";
        }
        else
        {
            prompt(true);
        }
    }
}

void PythonTypeIn::changeHistory()
{
    // Select the text after the last command prompt ...
    QTextCursor textCursor = this->textCursor();
    textCursor.movePosition(QTextCursor::End);
    textCursor.setPosition(commandPromptPosition(), QTextCursor::KeepAnchor);

    // ... and replace it with the history text.
    textCursor.insertText(history_.value(historyPosition_));
    textCursor.movePosition(QTextCursor::End);
    setTextCursor(textCursor);
}

MyPythonQt * PythonConsole::thePythonQt_;
PythonConsole * PythonConsole::thePythonConsole_;

PythonConsole * PythonConsole::self()
{
    return thePythonConsole_;
}

void PythonConsole::init()
{
    PythonQt::init(PythonQt::IgnoreSiteModule | PythonQt::RedirectStdOut);

    thePythonQt_ = new MyPythonQt();
    thePythonConsole_ = new PythonConsole();

    connect(PythonQt::self(), SIGNAL(pythonStdOut(const QString &)), thePythonQt_, SLOT(pythonStdOut(const QString &)));
    connect(PythonQt::self(), SIGNAL(pythonStdErr(const QString &)), thePythonQt_, SLOT(pythonStdErr(const QString &)));
}

PythonConsole::PythonConsole()
{
    thePythonQt_->evalString(new QString("import pydc"));

    pythonThread_ = new QThread;
    pythonThread_->start();

    thePythonQt_->moveToThread(pythonThread_);

    QGridLayout * layout = new QGridLayout;

    typeIn_ = new PythonTypeIn;
    layout->addWidget(typeIn_, 0, 0, 0);

    output_ = new QTextEdit;
    output_->setReadOnly(true);
    layout->addWidget(output_, 1, 0, 0);

    QWidget * widget = new QWidget;
    setCentralWidget(widget);
    widget->setLayout(layout);

    connect(thePythonQt_, SIGNAL(myPythonStdOut(const QString &)), this, SLOT(putResult(const QString &)));
    connect(thePythonQt_, SIGNAL(myPythonStdErr(const QString &)), this, SLOT(putResult(const QString &)));
    connect(typeIn_, SIGNAL(pythonCommand(QString *)), thePythonQt_, SLOT(evalString(QString *)));
    connect(thePythonQt_, SIGNAL(evalDone()), typeIn_, SLOT(pythonEvalFinished()));
    connect(this, SIGNAL(pythonFile(QString *)), thePythonQt_, SLOT(loadFile(QString *)));
    connect(thePythonQt_, SIGNAL(loadDone()), this, SLOT(pythonLoadFinished()));

    fileMenu_ = menuBar()->addMenu(tr("File"));

    QAction * runPythonFileAction = new QAction(tr("Run Python file..."), this);
    connect(runPythonFileAction, SIGNAL(triggered()), this, SLOT(selectPythonScript()));
    fileMenu_->addAction(runPythonFileAction);

    editMenu_ = menuBar()->addMenu(tr("Edit"));

    QAction * tt = new QAction(tr("Clear"), this);
    connect(tt, SIGNAL(triggered()), this, SLOT(clear()));
    editMenu_->addAction(tt);

    tt = new QAction(tr("Clear input"), this);
    connect(tt, SIGNAL(triggered()), this, SLOT(clearInput()));
    editMenu_->addAction(tt);

    tt = new QAction(tr("Clear output"), this);
    connect(tt, SIGNAL(triggered()), this, SLOT(clearOutput()));
    editMenu_->addAction(tt);
}

PythonConsole::~PythonConsole()
{
    pythonThread_->quit();

    while(pythonThread_->isRunning())
    {
        sleep(1);
    }

    delete pythonThread_;
}

void PythonConsole::selectPythonScript()
{
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    QString filename = QFileDialog::getOpenFileName(this, tr("Load File"), cwd, tr("*.py"));

    if(filename != "")
    {
        setEnabled(false);
        QString * _filename = new QString(filename);
        emit pythonFile(_filename);
    }
}

void PythonConsole::pythonLoadFinished()
{
    setEnabled(true);
}

void PythonConsole::evalString(QString * code)
{
    thePythonQt_->evalString(code);
}

void PythonConsole::putResult(const QString & str)
{
    output_->textCursor().movePosition(QTextCursor::Start);
    output_->insertPlainText(str);
}

void PythonConsole::clearInput()
{
    typeIn_->clear();
    typeIn_->prompt(false);
}

void PythonConsole::clearOutput()
{
    output_->clear();
}

void PythonConsole::clear()
{
    clearInput();
    clearOutput();
}
