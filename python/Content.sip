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
    pContent get();
    void dump();
};
