#include <iostream>
#include <unistd.h>
#include "PythonConsole.h"

PythonConsole::PythonConsole(int argc, char **argv) 
{
    pythonObject = new PythonObject(argc, argv);

    pythonThread = new QThread();
    pythonObject->moveToThread(pythonThread);

    cw = new QWidget();
    inpt = new QTextEdit();
    outpt = new QTextEdit();
    grid = new QGridLayout();
    button = new QPushButton("Do It");

    setCentralWidget(cw);
    cw->setLayout(grid);

    grid->addWidget(outpt, 0, 0, 1, 1);
    grid->addWidget(inpt, 1, 0, 1, 1);
    grid->addWidget(button, 2, 0, 1, 1);

    QAction *loadAction = new QAction("Load File", this);
    loadAction->setStatusTip("open file");

    QMenu * fileMenu = this->menuBar()->addMenu("&Load");
    fileMenu->addAction(loadAction);

    connect(loadAction, SIGNAL(triggered()), this, SLOT(loadFile()));
    connect(button, &QPushButton::released, this, &PythonConsole::doit);

    show();

}

void
PythonConsole::loadFile()
{
    char cwd[1024];

    getcwd(cwd, sizeof(cwd));
    QString filename = QFileDialog::getOpenFileName(this, tr("Load File"), cwd, tr("*.py"));

    if(filename == "")
        return;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        std::cerr << "unable to open file\n";
        return;
    }

    QString content = file.readAll();
    file.close();

    inpt->clear();
    inpt->setText(content);
}


void
PythonConsole::doit()
{

    QString s = inpt->toPlainText();

    std::string out, err;

    if (pythonObject->RunString(s.toStdString(), out, err))
    {
        outpt->moveCursor(QTextCursor::End);
        QTextCursor cursor = outpt->textCursor();

        if (! s.endsWith('\n'))
          cursor.insertText("\n");

        QTextCharFormat format;
        cursor.setCharFormat(format);

        format.setBackground(QBrush(QColor("red")));
        cursor.setCharFormat(format);

        QString qerr = QString::fromStdString(err);
        cursor.insertText(qerr);

        cursor.insertText("\n");
    }
    else
    {
        outpt->moveCursor(QTextCursor::End);
        QTextCursor cursor = outpt->textCursor();

        if (! s.endsWith('\n'))
          cursor.insertText("\n");

        QTextCharFormat format;
        cursor.setCharFormat(format);

        format.setBackground(QBrush(QColor("yellow")));
        cursor.setCharFormat(format);
        cursor.insertText(s);

        cursor.insertText("\n");

        QString qout = QString::fromStdString(out);

        format.setBackground(QBrush(QColor("lightGray")));
        cursor.setCharFormat(format);
        cursor.insertText(qout);

        inpt->clear();
    }
}

PythonObject::PythonObject(int argc, char **argv)
{
    python = initEmbeddedPython(argc, argv);
}

void
PythonObject::foo()
{
    std::cerr << "crap\n";
}

bool
PythonObject::RunString(std::string str, std::string& out, std::string& err) 
{
    return python->RunString(str, out, err);
}
