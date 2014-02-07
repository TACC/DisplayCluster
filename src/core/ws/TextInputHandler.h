/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#ifndef TEXTINPUTHANDLER_H
#define TEXTINPUTHANDLER_H

#include <QObject>

#include "dcWebservice/Handler.h"

#include "types.h"

/**
 * Handle "/textinput" requests for the WebService.
 *
 * When a valid request is received, the receivedText() signal is emitted.
 * This class is typically used in the WebServiceServer thread and communicates
 * with the TextInputDispatcher in the main thread via signals/slots.
 */
class TextInputHandler : public QObject, public dcWebservice::Handler
{
    Q_OBJECT

public:
    /**
     * Handle TextInput requests.
     * @param displayGroupManagerAdapter An adapter over the displayGroupManager,
     * used for unit testing. If provided, the class takes ownership of it.
     */
    TextInputHandler(DisplayGroupManagerAdapterPtr displayGroupManagerAdapter);

    /** Destructor */
    virtual ~TextInputHandler();

    /**
     * Handle a request.
     * @param request A valid dcWebservice::Request object.
     * @return A valid Response object.
     */
    virtual dcWebservice::ConstResponsePtr handle(const dcWebservice::Request& request) const;

signals:
    /**
     * Emitted whenever a request is successfully handled.
     * @param key The key code received in the Request.
     */
    void receivedKeyInput(char key) const;

private:
    DisplayGroupManagerAdapterPtr displayGroupManagerAdapter_;
};

#endif // TEXTINPUTHANDLER_H
