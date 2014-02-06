/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
/*                     Stefan.Eilemann@epfl.ch                       */
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

#define BOOST_TEST_MODULE Socket
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "DisplayGroupManager.h"
#include "MinimalGlobalQtApp.h"
#include "NetworkListener.h"
#include "configuration/MasterConfiguration.h"
#include "dcstream/Stream.h"

#define WIDTH  (1920u)
#define HEIGHT (1080u)
#define NPIXELS (WIDTH * HEIGHT)
#define NBYTES  (NPIXELS * 4u)
#define NIMAGES (100u)

BOOST_GLOBAL_FIXTURE( MinimalGlobalQtApp );

class DCThread : public QThread
{
    void run()
    {
        uint8_t* pixels = new uint8_t[ NBYTES ];
        ::memset( pixels, 0, NBYTES );
        dc::ImageWrapper image( pixels, WIDTH, HEIGHT, dc::RGBA );

        dc::Stream stream( "test", "localhost" );
        BOOST_CHECK( stream.isConnected( ));

        QElapsedTimer timer;
        timer.start();

        for( size_t i = 0; i < NIMAGES; ++i )
        {
            BOOST_CHECK( stream.send( image ));
            BOOST_CHECK( stream.finishFrame( ));
        }

        const float time = timer.elapsed() / 1000.f;
        std::cout << NPIXELS / float(1024*1024) / time << " megapixel/s ("
                  << NIMAGES / time << " FPS)" << std::endl;

        delete [] pixels;
        QApplication::instance()->exit();
    }
};

BOOST_AUTO_TEST_CASE( testSocketConnection )
{
    ut::master_test_suite_t& testSuite = ut::framework::master_test_suite();
    MPI_Init( &testSuite.argc, &testSuite.argv );

    g_displayGroupManager.reset( new DisplayGroupManager );
    g_configuration =
        new MasterConfiguration( "configuration.xml",
                                 g_displayGroupManager->getOptions( ));
    NetworkListener listener( *g_displayGroupManager );

    DCThread thread;
    thread.start();
    QApplication::instance()->exec();
    BOOST_CHECK( thread.wait( ));

    MPI_Finalize();
}
