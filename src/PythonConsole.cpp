#include <Python.h>
#include <iostream>
#include <QApplication>
#include <QTextCursor>
#include "PythonConsole.h"

static MyPythonQt    *thePythonQt;
static PythonConsole *thePythonConsole;

PythonConsole *PythonConsole::self() {return thePythonConsole;}

void
PythonConsole::init()
{
  thePythonQt = new MyPythonQt();
  thePythonConsole = new PythonConsole();

  PythonQt::init(PythonQt::IgnoreSiteModule | PythonQt::RedirectStdOut);

  connect(PythonQt::self(), SIGNAL(pythonStdOut(const QString&)), thePythonQt, SLOT(pythonStdOut(const QString&)));
  connect(PythonQt::self(), SIGNAL(pythonStdErr(const QString&)), thePythonQt, SLOT(pythonStdErr(const QString&)));
}

void
MyPythonQt::pythonStdOut(const QString &str)
{
  emit(myPythonStdOut(str));
}

void
MyPythonQt::pythonStdErr(const QString &str)
{
  emit(myPythonStdErr(str));
}

void
MyPythonQt::loadFile(QString *str)
{
  QString cmd = QString("execfile('") + *str + QString("')");
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  PyObject* dict = PyModule_GetDict(context);
  PyRun_String(cmd.toLatin1().data(), Py_single_input, dict, dict);
  delete str;
  emit(load_done());
}

void
MyPythonQt::evalString(QString *code)
{
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  PyObject* dict = PyModule_GetDict(context);
  PyRun_String(code->toLatin1().data(), Py_single_input, dict, dict);
  delete code;
  emit(eval_done());
}

PythonConsole::PythonConsole()
{
  pythonThread = new QThread;
  pythonThread->start();
  thePythonQt->moveToThread(pythonThread);

  QGridLayout *layout = new QGridLayout;

  typeIn = new PythonTypeIn;
  layout->addWidget(typeIn, 0, 0, 0);

  output = new QTextEdit;
  output->setReadOnly(true);
  layout->addWidget(output, 1, 0, 0);

  QWidget *widget = new QWidget;
  setCentralWidget(widget);

  widget->setLayout(layout);

  connect(thePythonQt, SIGNAL(myPythonStdOut(const QString &)), this, SLOT(put_result(const QString &)));
  connect(thePythonQt, SIGNAL(myPythonStdErr(const QString &)), this, SLOT(put_result(const QString &)));

  connect(typeIn, SIGNAL(python_command(QString *)), thePythonQt, SLOT(evalString(QString *)));
  connect(thePythonQt, SIGNAL(eval_done()), typeIn, SLOT(python_eval_finished()));

  connect(this, SIGNAL(python_file(QString *)), thePythonQt, SLOT(loadFile(QString *)));
  connect(thePythonQt, SIGNAL(load_done()), this, SLOT(python_load_finished()));

  fileMenu = menuBar()->addMenu(tr("File"));
  QAction *runPythonFileAction = new QAction(tr("Run Python file..."), this);
  connect(runPythonFileAction, SIGNAL(triggered()), this, SLOT(selectPythonScript()));
  fileMenu->addAction(runPythonFileAction);

  editMenu = menuBar()->addMenu(tr("Edit"));

  QAction *tt = new QAction(tr("Clear"), this);
  connect(tt, SIGNAL(triggered()), this, SLOT(clear()));
  editMenu->addAction(tt);

  tt = new QAction(tr("Clear input"), this);
  connect(tt, SIGNAL(triggered()), this, SLOT(clear_input()));
  editMenu->addAction(tt);

  tt = new QAction(tr("Clear output"), this);
  connect(tt, SIGNAL(triggered()), this, SLOT(clear_output()));
  editMenu->addAction(tt);
}

PythonConsole::~PythonConsole()
{
  pythonThread->quit();
  while (pythonThread->isRunning())
    sleep(1);

  delete pythonThread;
}

void
PythonConsole::clear_input()
{
  typeIn->clear();
  typeIn->prompt(false);
}
  
void
PythonConsole::clear_output()
{
  output->clear();
}
  
void
PythonConsole::clear()
{
  clear_input();
  clear_output();
}
  
void
PythonConsole::put_result(const QString& str)
{
  output->textCursor().movePosition(QTextCursor::Start);
  output->insertPlainText(str);
}

void
PythonConsole::selectPythonScript()
{
  char cwd[256];
  getcwd(cwd, sizeof(cwd));
  QString filename = QFileDialog::getOpenFileName(this, tr("Load File"), cwd, tr("*.py"));
  if (filename != "")
  {
    setEnabled(false);
    QString *_filename = new QString(filename);
    emit python_file(_filename);
  }
  else
    std::cerr << "cancelled\n";
}

void
PythonConsole::evalString(QString *code)
{
  thePythonQt->evalString(code);
}

void
PythonConsole::python_load_finished()
{
  setEnabled(true);
}

PythonTypeIn::PythonTypeIn()
{
  prompt(false);
  QTextCursor tc = textCursor();
  tc.movePosition(QTextCursor::End);
  tail = tc.position();
  setTabStopWidth(4);

  _completer = new QCompleter(this);
  _completer->setWidget(this);
  QObject::connect(_completer, SIGNAL(activated(const QString&)), this, SLOT(insertCompletion(const QString&)));
}

void
PythonTypeIn::prompt(bool storeOnly)
{
  const char *c = storeOnly ? "...> " : "py> ";
  _commandPrompt = c;
  append(c);
  QTextCursor tc = textCursor();
  tc.movePosition(QTextCursor::End);
  tail = tc.position();
}

void
PythonTypeIn::executeLine(bool storeOnly)
{
  QTextCursor textCursor = this->textCursor();
  textCursor.movePosition(QTextCursor::End);

  // Select the text from the command prompt until the end of the block
  // and get the selected text.
  textCursor.setPosition(commandPromptPosition());
  textCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
  QString code = textCursor.selectedText();

  // i don't know where this trailing space is coming from, blast it!
  if (code.endsWith(" ")) {
    code.truncate(code.length()-1);
  }

  if (!code.isEmpty()) {
    // Update the history
    _history << code;
    _historyPosition = _history.count();
    _currentMultiLineCode += code + "\n";

    if (!storeOnly) {
      QString *emittedString = new QString(_currentMultiLineCode);
      emit python_command(emittedString);
      _currentMultiLineCode = "";
    }
  }
}

void
PythonTypeIn::changeHistory()
{
    // Select the text after the last command prompt ...
    QTextCursor textCursor = this->textCursor();
    textCursor.movePosition(QTextCursor::End);
    textCursor.setPosition(commandPromptPosition(), QTextCursor::KeepAnchor);

    // ... and replace it with the history text.
    textCursor.insertText(_history.value(_historyPosition));

    textCursor.movePosition(QTextCursor::End);
    setTextCursor(textCursor);
}

void 
PythonTypeIn::insertCompletion(const QString& completion)
{
  QTextCursor tc = textCursor();
  tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
  if (tc.selectedText()==".") {
    tc.insertText(QString(".") + completion);
  } else {
    tc = textCursor();
    tc.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    tc.insertText(completion);
    setTextCursor(tc);
  }
}

void
PythonTypeIn::keyPressEvent(QKeyEvent* event)
{

  if (_completer && _completer->popup()->isVisible()) {
    // The following keys are forwarded by the completer to the widget
    switch (event->key()) {
    case Qt::Key_Return:
      if (!_completer->popup()->currentIndex().isValid()) {
        insertCompletion(_completer->currentCompletion());
        _completer->popup()->hide();
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
  bool        eventHandled = false;
  QTextCursor textCursor   = this->textCursor();

  int key = event->key();
  switch (key) {

  case Qt::Key_Left:

    // Moving the cursor left is limited to the position
    // of the command prompt.

    if (textCursor.position() <= commandPromptPosition()) {

      QApplication::beep();
      eventHandled = true;
    }
    break;

  case Qt::Key_Up:

    // Display the previous command in the history
    if (_historyPosition>0) {
      _historyPosition--;
      changeHistory();
    }

    eventHandled = true;
    break;

  case Qt::Key_Down:

    // Display the next command in the history
    if (_historyPosition+1<_history.count()) {
      _historyPosition++;
      changeHistory();
    }

    eventHandled = true;
    break;

  case Qt::Key_Return:

    executeLine(event->modifiers() & Qt::ShiftModifier);
    eventHandled = true;
    break;

  case Qt::Key_Backspace:

    if (textCursor.hasSelection()) {

      cut();
      eventHandled = true;

    } else {

      // Intercept backspace key event to check if
      // deleting a character is allowed. It is not
      // allowed, if the user wants to delete the
      // command prompt.

      if (textCursor.position() <= commandPromptPosition()) {

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

    if (key >= Qt::Key_Space && key <= Qt::Key_division) {

      if (textCursor.hasSelection() && !verifySelectionBeforeDeletion()) {

        // The selection must not be deleted.
        eventHandled = true;

      } else {

        // The key is an input character, check if the cursor is
        // behind the last command prompt, else inserting the
        // character is not allowed.

        int commandPromptPosition = this->commandPromptPosition();
        if (textCursor.position() < commandPromptPosition) {

          textCursor.setPosition(commandPromptPosition);
          setTextCursor(textCursor);
        }
      }
    }
  }

  if (eventHandled) {

    _completer->popup()->hide();
    event->accept();

  } else {

    QTextEdit::keyPressEvent(event);
    QString text = event->text();
    if (!text.isEmpty()) {
      handleTabCompletion();
    } else {
      _completer->popup()->hide();
    }
    eventHandled = true;
  }
}

void
PythonTypeIn::python_eval_finished()
{
  void *x = PyErr_Occurred();
  if (x)
    PyErr_Print();

  content = "";
  setEnabled(true);
  prompt(false);
  setFocus(Qt::OtherFocusReason);
}

void PythonTypeIn::handleTabCompletion()
{
  QTextCursor textCursor   = this->textCursor();
  int pos = textCursor.position();
  textCursor.setPosition(commandPromptPosition());
  textCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
  int startPos = textCursor.selectionStart();

  int offset = pos-startPos;
  QString text = textCursor.selectedText();

  QString textToComplete;
  int cur = offset;
  while (cur--) {
    QChar c = text.at(cur);
    if (c.isLetterOrNumber() || c == '.' || c == '_') {
      textToComplete.prepend(c);
    } else {
      break;
    }
  }


  QString lookup;
  QString compareText = textToComplete;
  int dot = compareText.lastIndexOf('.');
  if (dot!=-1) {
    lookup = compareText.mid(0, dot);
    compareText = compareText.mid(dot+1, offset);
  }
  if (!lookup.isEmpty() || !compareText.isEmpty()) {
    compareText = compareText.toLower();
    QStringList found;

// GDA
    // QStringList l = PythonQt::self()->introspection(_context, lookup, PythonQt::Anything);
    QStringList l = PythonQt::self()->introspection(PythonQt::self()->getMainModule(), lookup, PythonQt::Anything);

    foreach (QString n, l) {
      if (n.toLower().startsWith(compareText)) {
        found << n;
      }
    }

    if (!found.isEmpty()) {
      _completer->setCompletionPrefix(compareText);
      _completer->setCompletionMode(QCompleter::PopupCompletion);
      _completer->setModel(new QStringListModel(found, _completer));
      _completer->setCaseSensitivity(Qt::CaseInsensitive);
      QTextCursor c = this->textCursor();
      c.movePosition(QTextCursor::StartOfWord);
      QRect cr = cursorRect(c);
      cr.setWidth(_completer->popup()->sizeHintForColumn(0)
        + _completer->popup()->verticalScrollBar()->sizeHint().width());
      cr.translate(0,8);
      _completer->complete(cr);
    } else {
      _completer->popup()->hide();
    }
  } else {
    _completer->popup()->hide();
  }
}

bool PythonTypeIn::verifySelectionBeforeDeletion() {

  bool deletionAllowed = true;

  QTextCursor textCursor = this->textCursor();

  int commandPromptPosition = this->commandPromptPosition();
  int selectionStart        = textCursor.selectionStart();
  int selectionEnd          = textCursor.selectionEnd();

  if (textCursor.hasSelection()) {

    // Selected text may only be deleted after the last command prompt.
    // If the selection is partly after the command prompt set the selection
    // to the part and deletion is allowed. If the selection occurs before the
    // last command prompt, then deletion is not allowed.
  
    if (selectionStart < commandPromptPosition ||
      selectionEnd < commandPromptPosition) {

      // Assure selectionEnd is bigger than selection start
      if (selectionStart > selectionEnd) {
        int tmp         = selectionEnd;
        selectionEnd    = selectionStart;
        selectionStart  = tmp;
      }       
    
      if (selectionEnd < commandPromptPosition) {
  
        // Selection is completely before command prompt,
        // so deletion is not allowed.
        QApplication::beep();
        deletionAllowed = false;
  
      } else {
        
        // The selectionEnd is after the command prompt, so set
        // the selection start to the commandPromptPosition.
        selectionStart = commandPromptPosition;
        textCursor.setPosition(selectionStart);
        textCursor.setPosition(selectionStart, QTextCursor::KeepAnchor);
        setTextCursor(textCursor);
      } 
    }   

  } else { // if (hasSelectedText())
    // When there is no selected text, deletion is not allowed before the
    // command prompt.
    if (textCursor.position() < commandPromptPosition) {

      QApplication::beep();
      deletionAllowed = false;
    }
  }

  return deletionAllowed;
}

int PythonTypeIn::commandPromptPosition() {

  QTextCursor textCursor(this->textCursor());
  textCursor.movePosition(QTextCursor::End);

  return textCursor.block().position() + _commandPrompt.length();
}


