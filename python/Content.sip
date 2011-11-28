class pContent
{
%TypeHeaderCode
#include <Content.h>
%End
};

class pyContent
{
%TypeHeaderCode
#include <Content.h>
%End

  public:
    pyContent(const char *str);
    pContent get();
};
