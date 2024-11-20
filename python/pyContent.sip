
class pyContent
{
%TypeHeaderCode
#include <pyContent.h>
%End

  public:
    pyContent(const char * str);

    char *getURI();
    int   getWidth();
    int   getHeight();

    PyObject *getDimensions();
};
