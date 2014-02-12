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

#ifndef DISPLAY_GROUP_LIST_WIDGET_PROXY_H
#define DISPLAY_GROUP_LIST_WIDGET_PROXY_H

#include "DisplayGroupInterface.h"
#include <QtGui>

class DisplayGroupListWidgetProxy : public DisplayGroupInterface {
    Q_OBJECT

    public:

        DisplayGroupListWidgetProxy(DisplayGroupManagerPtr displayGroupManager);
        ~DisplayGroupListWidgetProxy();

        QListWidget * getListWidget();

        // re-implemented DisplayGroupInterface slots
        void addContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source=NULL);
        void removeContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source=NULL);
        void moveContentWindowManagerToFront(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source=NULL);

    private slots:

        void moveListWidgetItemToFront(QListWidgetItem * item);

    private:

        // we make this a member since we can't have multiple inheritance of QObject and still use signals/slots
        // see the "Diamond problem"
        QListWidget * listWidget_;

        void refreshListWidget();
};

#endif
