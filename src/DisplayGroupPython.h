#ifndef DISPLAY_GROUP_PYTHON_H
#define DISPLAY_GROUP_PYTHON_H

#include "DisplayGroupInterface.h"
#include "ContentWindowManager.h"
#include <QtGui>

class DisplayGroupPython : public DisplayGroupInterface, public boost::enable_shared_from_this<DisplayGroupPython> {
    Q_OBJECT

    public:

        DisplayGroupPython(boost::shared_ptr<DisplayGroupManager> displayGroupManager);
};

// needed for SIP
typedef boost::shared_ptr<DisplayGroupPython> pDisplayGroupPython;

class pyDisplayGroupPython
{
    public:

        pyDisplayGroupPython()
        {
            // attach to g_displayGroupManager on construction

            // declared in main.cpp, but we don't want to bring in everything from main.h...
            extern boost::shared_ptr<DisplayGroupManager> g_displayGroupManager;

            ptr_ = boost::shared_ptr<DisplayGroupPython>(new DisplayGroupPython(g_displayGroupManager));
        }

        boost::shared_ptr<DisplayGroupPython> get()
        {
            return ptr_;
        }

        void addContentWindowManager(pyContentWindowManager pcwm)
        {
            get()->addContentWindowManager(pcwm.get());
        }

        void removeContentWindowManager(pyContentWindowManager pcwm)
        {
            get()->removeContentWindowManager(pcwm.get());
        }

        void moveContentWindowManagerToFront(pyContentWindowManager pcwm)
        {
            get()->moveContentWindowManagerToFront(pcwm.get());
        }

        void saveState(const char * filename)
        {
            std::string filenameString(filename);

            get()->saveState(filenameString);
        }

        void loadState(const char * filename)
        {
            std::string filenameString(filename);

            get()->loadState(filenameString);
        }

        int getNumContentWindowManagers()
        {
            return get()->getContentWindowManagers().size();
        }

        pyContentWindowManager getPyContentWindowManager(int index)
        {
            return pyContentWindowManager(get()->getContentWindowManagers()[index]);
        }

    private:

        boost::shared_ptr<DisplayGroupPython> ptr_;
};

#endif
