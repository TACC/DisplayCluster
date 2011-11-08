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
MyPythonQt::loadFile(const QString &str)
{
  QString cmd = QString("execfile('") + str + QString("')");
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  PyObject* dict = PyModule_GetDict(context);
  PyRun_String(cmd.toLatin1().data(), Py_single_input, dict, dict);
  emit(load_done());
}

void
MyPythonQt::evalStringSynchronous(const QString &code)
{
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  PyObject* dict = PyModule_GetDict(context);
  PyRun_String(code.toLatin1().data(), Py_single_input, dict, dict);
}

void
MyPythonQt::evalString(const QString &code)
{
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  PyObject* dict = PyModule_GetDict(context);
  PyRun_String(code.toLatin1().data(), Py_single_input, dict, dict);
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

  connect(typeIn, SIGNAL(python_command(const QString &)), thePythonQt, SLOT(evalString(const QString &)));
  connect(thePythonQt, SIGNAL(eval_done()), typeIn, SLOT(python_eval_finished()));

  connect(this, SIGNAL(python_file(const QString &)), thePythonQt, SLOT(loadFile(const QString &)));
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
  typeIn->prompt(">");
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
    emit python_file(filename);
  }
  else
    std::cerr << "cancelled\n";
}

void
PythonConsole::evalString(const QString &code)
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
  prompt(">");
  QTextCursor tc = textCursor();
  tc.movePosition(QTextCursor::End);
  tail = tc.position();
  setTabStopWidth(4);
}

void
PythonTypeIn::prompt(const char *c)
{
  append(c);
  QTextCursor tc = textCursor();
  tc.movePosition(QTextCursor::End);
  tail = tc.position();
}

void
PythonTypeIn::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Return)
  {
    QTextCursor tc = textCursor();
    tc.setPosition(tail);
    tc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    QString code = tc.selectedText();

    if (code == "")
    {
      if (content != "")
      {
	setEnabled(false);
	emit python_command(content);
      }
      else
   	prompt(">");
    }
    else
    {
      if (code.endsWith(":"))
      {
	content += code + "\n";
	prompt(".");
      }
      else if (code.startsWith(" ") || code.startsWith("\t"))
      {
	if (content == "")
	{
	  append("indent error\n");
	  content = "";
	  indent = 0;
	  prompt(">");
	}
	else
	{
	  content += code + "\n";
	  prompt(".");
	}
      }
      else
      {
	content += code + "\n";
	if (! code.endsWith(":"))
	{
	  setEnabled(false);
	  emit python_command(content);
	}
	else
	  prompt(".");
      }
    } 
  }
  else
    QTextEdit::keyPressEvent(event);
}

void
PythonTypeIn::python_eval_finished()
{
  void *x = PyErr_Occurred();
  if (x)
    PyErr_Print();

  content = "";
  setEnabled(true);
  prompt(">");
  setFocus(Qt::OtherFocusReason);
}
