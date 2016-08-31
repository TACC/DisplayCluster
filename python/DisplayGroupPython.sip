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

    pDisplayGroupPython get();

    void addContentWindowManager(const pyContentWindowManager &);
    void removeContentWindowManager(const pyContentWindowManager &);
    void moveContentWindowManagerToFront(const pyContentWindowManager &);
    void saveState(const char * filename);
    void loadState(const char * filename);
    void pushState();
    void popState();

    int getNumContentWindowManagers();
    pyContentWindowManager getPyContentWindowManager(int);
};
