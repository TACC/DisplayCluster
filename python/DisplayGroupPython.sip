class pDisplayGroupPython
{
%TypeHeaderCode
#include <DisplayGroupPython.h>
%End
};

class pyDisplayGroupPython
{
%TypeHeaderCode
#include <DisplayGroupPython.h>
%End

  public:
    pyDisplayGroupPython();

    void moveContentWindowToFront(const pyContentWindow&);
    void addContentWindow(const pyContentWindow&);
    void removeContentWindow(const pyContentWindow&);

};


