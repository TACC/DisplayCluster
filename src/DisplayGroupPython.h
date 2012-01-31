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
