class pContentWindowManager
{
%TypeHeaderCode
#include <ContentWindowManager.h>
%End
};

class pyContentWindowManager
{
%TypeHeaderCode
#include <ContentWindowManager.h>
%End

  public:
    pyContentWindowManager(pyContent);

    pContentWindowManager get();

    pyContent getPyContent();

    void getCoordinates();
%MethodCode
    pContentWindowManager win = sipCpp->get();
    double x, y, w, h;
    win->getCoordinates(x, y, w, h);
    return Py_BuildValue("(dddd)", x, y, w, h);
%End

    void getPosition();
%MethodCode
    pContentWindowManager win = sipCpp->get();
    double x, y;
    win->getPosition(x, y);
    return Py_BuildValue("(dd)", x, y);
%End

    void getSize();
%MethodCode
    pContentWindowManager win = sipCpp->get();
    double x, y, w, h;
    win->getCoordinates(x, y, w, h);
    return Py_BuildValue("(dd)", w, h);
%End

    void getCenter();
%MethodCode
    pContentWindowManager win = sipCpp->get();
    double x, y;
    win->getCenter(x, y);
    return Py_BuildValue("(dd)", x, y);
%End

    void getZoom();
%MethodCode
    return Py_BuildValue("d", sipCpp->get()->getZoom());
%End

    void getSelected();
%MethodCode
    return Py_BuildValue("d", sipCpp->get()->getSelected() ? 1 : 0);
%End

    void setCoordinates(double i, double j, double k, double l);
%MethodCode
    pContentWindowManager win = sipCpp->get();
    win->setCoordinates(a0, a1, a2, a3);
%End

    void setPosition(double i, double j);
%MethodCode
    pContentWindowManager win = sipCpp->get();
    win->setPosition(a0, a1);
%End

    void setSize(double i, double j);
%MethodCode
    pContentWindowManager win = sipCpp->get();
    win->setSize(a0, a1);
%End

    void setCenter(double i, double j);
%MethodCode
    pContentWindowManager win = sipCpp->get();
    sipCpp->get()->setCenter(a0, a1);
%End

    void setZoom(double z);
%MethodCode
    pContentWindowManager win = sipCpp->get();
    sipCpp->get()->setZoom(a0);
%End

    void setSelected(int s);
%MethodCode
    pContentWindowManager win = sipCpp->get();
    sipCpp->get()->setSelected(a0 == 1 ? true : false);
%End

    void moveToFront();
%MethodCode
    pContentWindowManager win = sipCpp->get();
    sipCpp->get()->moveToFront();
%End
};
