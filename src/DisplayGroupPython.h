#ifndef DISPLAY_GROUP_PYTHON_H
#define DISPLAY_GROUP_PYTHON_H

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "ContentWindowManager.h"
#include "DisplayGroupInterface.h"

class DisplayGroupPython : public DisplayGroupInterface, public boost::enable_shared_from_this<DisplayGroupPython> {

    Q_OBJECT

    public:
        DisplayGroupPython(boost::shared_ptr<DisplayGroupManager> displayGroupManager);
};

typedef boost::shared_ptr<DisplayGroupPython> pDisplayGroupPython;

class pyDisplayGroupPython
{
    public:
        pyDisplayGroupPython() {
          extern boost::shared_ptr<DisplayGroupManager> getTheDisplayGroupManager();
          ptr_ = boost::shared_ptr<DisplayGroupPython>(new DisplayGroupPython(getTheDisplayGroupManager()));
        }

        ~pyDisplayGroupPython() {}

        void addContentWindowManager(pyContentWindowManager pcw) { get()->addContentWindowManager(pcw.get()); }
        void removeContentWindowManager(pyContentWindowManager pcw) { get()->removeContentWindowManager(pcw.get()); }
        void moveContentWindowManagerToFront(pyContentWindowManager pcw) { get()->moveContentWindowManagerToFront(pcw.get()); }

        boost::shared_ptr<DisplayGroupPython> get() const {return ptr_;}

	int size() {return get()->getContentWindowManagers().size();}
 	pyContentWindowManager getPyContentWindowManager(int indx) { return pyContentWindowManager(get()->getContentWindowManagers()[indx]); }

    private:

        boost::shared_ptr<DisplayGroupPython> ptr_;
};

#endif
