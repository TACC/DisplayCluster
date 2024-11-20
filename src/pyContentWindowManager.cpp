/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include <iostream>
#include <string>

#include "pyContentWindowManager.h"
#include "ContentWindowManager.h"

pyContentWindowManager::pyContentWindowManager(pyContent content)
{
    boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(content.get()));
    ptr_ = cwm;
}

pyContentWindowManager::pyContentWindowManager(boost::shared_ptr<ContentWindowManager> cwm)
{
    ptr_ = cwm;
}

boost::shared_ptr<ContentWindowManager> pyContentWindowManager::get()
{
    return ptr_;
}

pyContent pyContentWindowManager::getPyContent()
{
    return pyContent(get()->getContent());
}

PyObject *pyContentWindowManager::getCoordinates()
{
    double x, y, w, h;
    ptr_->getCoordinates(x, y, w, h);
    return Py_BuildValue("(dddd)", x, y, w, h);
}

PyObject *
pyContentWindowManager::getPosition()
{
    double x, y;
    ptr_->getPosition(x, y);
    return Py_BuildValue("(dd)", x, y);
}

PyObject *
pyContentWindowManager::getSize()
{
    double x, y, w, h;
    ptr_->getCoordinates(x, y, w, h);
    return Py_BuildValue("(dd)", w, h);
}

PyObject *
pyContentWindowManager::getCenter()
{
    double x, y;
    ptr_->getCenter(x, y);
    return Py_BuildValue("(dd)", x, y);
}

PyObject *
pyContentWindowManager::getZoom()
{
    return Py_BuildValue("d", ptr_->getZoom());
}

PyObject *
pyContentWindowManager::getSelected()
{
  return Py_BuildValue("d", ptr_->getSelected() ? 1 : 0);
}

void
pyContentWindowManager::setCoordinates(double i, double j, double k, double l)
{
    ptr_->setCoordinates(i, j, k, l);
}

void
pyContentWindowManager::setPosition(double i, double j)
{
    ptr_->setPosition(i, j);
}

void
pyContentWindowManager::setSize(double i, double j)
{
    ptr_->setSize(i, j);
}

void
pyContentWindowManager::scaleSize(double f)
{
    ptr_->scaleSize(f);
}

void
pyContentWindowManager::setCenter(double i, double j)
{
    ptr_->setCenter(i, j);
}

void
pyContentWindowManager::setZoom(double z)
{
    ptr_->setZoom(z);
}

void
pyContentWindowManager::setSelected(int s)
{
    ptr_->setSelected(s == 1 ? true : false);
}

void
pyContentWindowManager::moveToFront()
{
    ptr_->moveToFront();
}
