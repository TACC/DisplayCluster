#include <string>
#include <iostream>

#include <Python.h>
#include "EmbeddedPython.h"

class EmbeddedPythonImplementation : public EmbeddedPython
{
public: 
    EmbeddedPythonImplementation(int argc, char **argv);
    ~EmbeddedPythonImplementation();
    bool RunString(std::string std, std::string& out, std::string& err);

private:
    PyObject *pyModule;
    PyObject *errorCatcher;
    PyObject *outputCatcher;
};

EmbeddedPythonImplementation::EmbeddedPythonImplementation(int argc, char **argv)
{
    PyStatus status;

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    status = PyConfig_SetBytesString(&config, &config.program_name, argv[0]);
    if (PyStatus_Exception(status))
    {
        std::cerr << "EmbeddedPython init err1\n";
        exit(1);
    }

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status))
    {
        std::cerr << "EmbeddedPython init err1\n";
        exit(1);
    }

    PyConfig_Clear(&config);

    pyModule = PyImport_AddModule("__main__"); 

    PyRun_SimpleString("\
import sys\n\
class CatchStdOut:\n\
    def __init__(self):\n\
        self.value = ''\n\
    def write(self, stuff):\n\
        self.value += stuff\n\
    def clear(self):\n\
        self.value = ''\n\
class CatchStdErr:\n\
    def __init__(self):\n\
        self.value = ''\n\
    def write(self, stuff):\n\
        self.value += stuff\n\
    def clear(self):\n\
        self.value = ''\n\
import sys\n\
catchStdOut = CatchStdOut()\n\
sys.stdout = catchStdOut\n\
catchStdErr = CatchStdErr()\n\
sys.stderr = catchStdErr\n");

    outputCatcher = PyObject_GetAttrString(pyModule,"catchStdOut");
    errorCatcher = PyObject_GetAttrString(pyModule,"catchStdErr");
}

EmbeddedPythonImplementation::~EmbeddedPythonImplementation()
{
    if (Py_FinalizeEx() < 0)
    {
        exit(120);
    }
}

bool
EmbeddedPythonImplementation::RunString(std::string str, std::string& out, std::string& err)
{
    PyRun_SimpleString(str.c_str());

    PyObject *output = PyObject_GetAttrString(outputCatcher,"value");
    PyObject *encoded_output = PyUnicode_AsEncodedString(output, "utf-8", "strict");
    out = std::string(PyBytes_AsString(encoded_output));

    PyObject *error = PyObject_GetAttrString(errorCatcher,"value");
    PyObject *encoded_error = PyUnicode_AsEncodedString(error, "utf-8", "strict");
    err = std::string(PyBytes_AsString(encoded_error));

    PyRun_SimpleString("catchStdOut.clear()\ncatchStdErr.clear()");

    return err != "";
}


static EmbeddedPython *theEmbeddedPython = NULL;

EmbeddedPython *
getTheEmbeddedPython() { return theEmbeddedPython; }

EmbeddedPython *
initEmbeddedPython(int argc, char **argv)
{
    if (theEmbeddedPython)
    {
        std::cerr << "EmbeddedPython already initted\n";
        exit(1);
    }

    theEmbeddedPython = (EmbeddedPython *) new EmbeddedPythonImplementation(argc, argv);
    return theEmbeddedPython;
}
