class pDisplayGroupManager
{
%TypeHeaderCode
#include <pyDisplayGroupManager.h>
%End
};

class pyDisplayGroupManager
{
%TypeHeaderCode
#include <pyDisplayGroupManager.h>
%End

  public:
    pyDisplayGroupManager();

    void addContentWindowManager(const pyContentWindowManager &);
    void removeContentWindowManager(const pyContentWindowManager &);
    void moveContentWindowManagerToFront(const pyContentWindowManager &);
    void saveState(const char * filename);
    void loadState(const char * filename);
    void pushState();
    void popState();
    void suspendSynchronization();
    void resumeSynchronization();

    int getNumContentWindowManagers();
    pyContentWindowManager getPyContentWindowManager(int);
};
