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

#ifndef DISPLAY_GROUP_INTERFACE_H
#define DISPLAY_GROUP_INTERFACE_H

#include "main.h"

#include "Content.h"
#include <QtWidgets>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class DisplayGroupManager;
class ContentWindowManager;

class DisplayGroupInterface : public QObject {
    Q_OBJECT

    public:

        DisplayGroupInterface() { }
        DisplayGroupInterface(boost::shared_ptr<DisplayGroupManager> displayGroupManager);

        boost::shared_ptr<DisplayGroupManager> getDisplayGroupManager();

        std::vector<boost::shared_ptr<ContentWindowManager> > getContentWindowManagers();
        boost::shared_ptr<ContentWindowManager> getContentWindowManager(std::string uri, CONTENT_TYPE contentType=CONTENT_TYPE_ANY);

        // remove all current ContentWindowManagers and add the vector of provided ContentWindowManagers
        void setContentWindowManagers(std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers);

				void suspendSynchronization();
				void resumeSynchronization();
				void pushState();
				void popState();


    public slots:

        // these methods set the local copies of the state variables if source != this
        // they will emit signals if source == NULL or if this is a DisplayGroup object
        // the source argument should not be provided by users -- only by these functions
        virtual void addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        virtual void removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        virtual void moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        virtual void saveState(std::string filename, DisplayGroupInterface * source=NULL);
        virtual void loadState(std::string filename, DisplayGroupInterface * source=NULL);

    signals:

        // emitting these signals will trigger updates on the corresponding DisplayGroup
        // as well as all other DisplayGroupInterfaces to that DisplayGroup
        void contentWindowManagerAdded(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void contentWindowManagerRemoved(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void contentWindowManagerMovedToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void stateSaved(std::string filename, DisplayGroupInterface * source=NULL);
        void stateLoaded(std::string filename, DisplayGroupInterface * source=NULL);

    protected:

        // optional: reference to DisplayGroupManager for non-DisplayGroupManager objects
        boost::weak_ptr<DisplayGroupManager> displayGroupManager_;

        // vector of all of its content window managers
        std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers_;
};

#endif
