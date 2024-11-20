class pContentWindowManager
{
%TypeHeaderCode
#include <pyContentWindowManager.h>
%End
};

class pyContentWindowManager
{
%TypeHeaderCode
#include <pyContentWindowManager.h>
%End

  public:
    pyContentWindowManager(pyContent);

    pContentWindowManager get();

    pyContent getPyContent();

    PyObject *getCoordinates();
    PyObject *getPosition();
    PyObject *getSize();
    PyObject *getCenter();
    PyObject *getZoom();
    PyObject *getSelected();
    void setCoordinates(double i, double j, double k, double l);
    void setPosition(double i, double j);
    void setSize(double i, double j);
    void scaleSize(double f);
    void setCenter(double i, double j);
    void setZoom(double z);
    void setSelected(int s);
    void moveToFront();

};
