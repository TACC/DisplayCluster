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

    void moveContentWindowManagerToFront(const pyContentWindowManager&);
    void addContentWindowManager(const pyContentWindowManager&);
    void removeContentWindowManager(const pyContentWindowManager&);

    int size();
    pyContentWindowManager getPyContentWindowManager(int);

};


