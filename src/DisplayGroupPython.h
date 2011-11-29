#ifndef DISPLAY_GROUP_PYTHON_H
#define DISPLAY_GROUP_PYTHON_H

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "ContentWindow.h"
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

        void addContentWindow(pyContentWindow pcw) { get()->addContentWindow(pcw.get()); }
        void removeContentWindow(pyContentWindow pcw) { get()->removeContentWindow(pcw.get()); }
        void moveContentWindowToFront(pyContentWindow pcw) { get()->moveContentWindowToFront(pcw.get()); }

        boost::shared_ptr<DisplayGroupPython> get() const {return ptr_;}

    private:

        boost::shared_ptr<DisplayGroupPython> ptr_;
};

#endif
