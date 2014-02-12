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

#define BOOST_TEST_MODULE TextInputHandlerTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "MinimalGlobalQtApp.h"

#include "ws/TextInputHandler.h"
#include "ws/DisplayGroupManagerAdapter.h"

#include "dcWebservice/Response.h"
#include "dcWebservice/Request.h"
#include "dcWebservice/types.h"

#include "MockTextInputDispatcher.h"

BOOST_GLOBAL_FIXTURE( MinimalGlobalQtApp );

class MockDisplayGroupManagerAdapter : public DisplayGroupManagerAdapter
{
public:
    MockDisplayGroupManagerAdapter(bool hasWindows)
        : DisplayGroupManagerAdapter(DisplayGroupManagerPtr())
        , hasWindows_(hasWindows)
    {}

    virtual bool hasWindows() const
    {
        return hasWindows_;
    }
private:
    bool hasWindows_;
};

BOOST_AUTO_TEST_CASE( testWhenRequestHasCharThenTextInputDispatcherReceivesIt )
{
    DisplayGroupManagerAdapterPtr adapter(new MockDisplayGroupManagerAdapter(true));
    TextInputHandler handler(adapter);

    MockTextInputDispatcher mockDispatcher;
    mockDispatcher.connect(&handler, SIGNAL(receivedKeyInput(char)),
                           SLOT(sendKeyEventToActiveWindow(char)));
    // Checking default value
    BOOST_REQUIRE_EQUAL(mockDispatcher.getKey(), '0');

    dcWebservice::RequestPtr request(new dcWebservice::Request());

    request->data = "a";
    handler.handle(*request);
    BOOST_CHECK_EQUAL(mockDispatcher.getKey(), 'a');

    request->data = "7";
    handler.handle(*request);
    BOOST_CHECK_EQUAL(mockDispatcher.getKey(), '7');
}

BOOST_AUTO_TEST_CASE( testWhenRequestIsTooLongThenReturnCodeIs400 )
{
    DisplayGroupManagerAdapterPtr adapter(new MockDisplayGroupManagerAdapter(true));
    TextInputHandler handler(adapter);

    dcWebservice::RequestPtr request(new dcWebservice::Request());
    dcWebservice::ConstResponsePtr response;

    request->data = "iamtoolong";
    response = handler.handle(*request);
    BOOST_CHECK_EQUAL(response->statusCode, 400);
    BOOST_CHECK_EQUAL(response->statusMsg, "Bad Request");
}

BOOST_AUTO_TEST_CASE( testWhenRequestEmptyInvalidThenReturnCodeIs400 )
{
    DisplayGroupManagerAdapterPtr adapter(new MockDisplayGroupManagerAdapter(true));
    TextInputHandler handler(adapter);

    dcWebservice::RequestPtr request(new dcWebservice::Request());
    dcWebservice::ConstResponsePtr response;

    request->data = "";
    response = handler.handle(*request);
    BOOST_CHECK_EQUAL(response->statusCode, 400);
    BOOST_CHECK_EQUAL(response->statusMsg, "Bad Request");
}

BOOST_AUTO_TEST_CASE( testWhenDisplayGroupHasNoWindowsThenReturnCodeIs404 )
{
    DisplayGroupManagerAdapterPtr adapter(new MockDisplayGroupManagerAdapter(false));
    TextInputHandler handler(adapter);

    dcWebservice::RequestPtr request(new dcWebservice::Request());

    request->data = "a";
    dcWebservice::ConstResponsePtr response = handler.handle(*request);
    BOOST_CHECK_EQUAL(response->statusCode, 404);
    BOOST_CHECK_EQUAL(response->statusMsg, "Not Found");
}
