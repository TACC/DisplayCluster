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

    void getLocation();
%MethodCode
	pContentWindowManager win = sipCpp->get();
	double x, y, w, h;
	win->getCoordinates(x, y, w, h);
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

   void setLocation(double i, double j);
	pContentWindowManager win = sipCpp->get();
	double x, y, w, h;
	win->getCoordinates(x, y, w, h);
	win->setCoordinates(a0, a1, w, h);
%End

   void setSize(double i, double j);
%MethodCode
	pContentWindowManager win = sipCpp->get();
	double x, y, w, h;
	win->getCoordinates(x, y, w, h);
	win->setCoordinates(x, y, a0, a1);
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
};
