class pDisplayGroup
{
%TypeHeaderCode
#include <DisplayGroup.h>
%End
};

class pyDisplayGroup
{
  public:
    pyDisplayGroup(pDisplayGroup  pdg);
    pDisplayGroup getp();

    bool hasContent(const char *);
    void addContentWindow(const pyContentWindow&);
    void removeContentWindow(const pyContentWindow&);
    void dump();

};

pyDisplayGroup  getThePyDisplayGroup();


