class pyContent
{
%TypeHeaderCode
#include <pyContent.h>
%End

  public:
    pyContent(const char * str);

    Content *get();

    const char * getURI();
};
