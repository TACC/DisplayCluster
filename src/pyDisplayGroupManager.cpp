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

#include "pyDisplayGroupManager.h"
#include "pyContentWindowManager.h"
#include "DisplayGroupManager.h"

pyDisplayGroupManager::pyDisplayGroupManager()
{
    extern boost::shared_ptr<DisplayGroupManager> g_displayGroupManager;
    // ptr_ = boost::shared_ptr<pyDisplayGroupManager>(new pyDisplayGroupManager(g_displayGroupManager));
    ptr_ = g_displayGroupManager;
}

void
pyDisplayGroupManager::addContentWindowManager(pyContentWindowManager pcwm)
{
    ptr_->addContentWindowManager(pcwm.get());
}

void
pyDisplayGroupManager::removeContentWindowManager(pyContentWindowManager pcwm)
{
    ptr_->removeContentWindowManager(pcwm.get());
}

void
pyDisplayGroupManager::moveContentWindowManagerToFront(pyContentWindowManager pcwm)
{
    ptr_->moveContentWindowManagerToFront(pcwm.get());
}

void
pyDisplayGroupManager::saveState(const char * filename)
{
    std::string filenameString(filename);

    ptr_->saveState(filename);
}

void
pyDisplayGroupManager::loadState(const char * filename)
{
    std::string filenameString(filename);

    ptr_->loadState(filename);
}

void
pyDisplayGroupManager::pushState()
{
    ptr_->pushState();
}

void
pyDisplayGroupManager::popState()
{
    ptr_->popState();
}

void
pyDisplayGroupManager::suspendSynchronization()
{
    ptr_->suspendSynchronization();
}

void
pyDisplayGroupManager::resumeSynchronization()
{
    ptr_->resumeSynchronization();
}

int 
pyDisplayGroupManager::getNumContentWindowManagers()
{
    return ptr_->getContentWindowManagers().size();
}

pyContentWindowManager
pyDisplayGroupManager::getPyContentWindowManager(int index)
{
    return pyContentWindowManager(ptr_->getContentWindowManagers()[index]);
}

