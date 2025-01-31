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
#include "main.h"

#include "QSSApp.h"

#include "Content.h"
#include "ContentWindowManager.h"

QSSApplication::QSSApplication(int& argc, char **argv) : QApplication(argc, argv)
{
	interval = (getenv("DISPLAYCLUSTER_TIMEOUT") == NULL) ? 5000 : 1000*atoi(getenv("DISPLAYCLUSTER_TIMEOUT"));
	m_timer.setInterval(interval);
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(go_to_sleep()));
	m_timer.start();
}

void 
QSSApplication::sleep()
{
	if (sleeping)
		sleep_move();
	else	
		sleep_start();

	m_timer.setInterval(33);
	m_timer.start();
}

void
QSSApplication::sleep_start()
{
	sleeping = true;

	auto dgm = g_displayGroupManager;

	dgm->pushState();

	while (dgm->getContentWindowManagers().size() > 0)
	{
		auto cw = dgm->getContentWindowManager(0);
		dgm->removeContentWindowManager(cw);
	}

	if (! ss_cwm)
	{
		std::string ss_image = getenv("DISPLAYCLUSTER_SCREENSAVER_IMAGE");
		if (ss_image.size() > 0)
		{
			if (ss_image.substr(0, 1) != "/")
				ss_image = g_displayClusterDir + "/" + ss_image;

			auto content = Content::getContent(ss_image);
			if (content)
				ss_cwm = boost::shared_ptr<ContentWindowManager>(new ContentWindowManager(content));
		}
	}

	if (ss_cwm)
	{
		dgm->addContentWindowManager(ss_cwm);
		ss_cwm->getCoordinates(x, y, w, h);		
	}

}

void 
QSSApplication::sleep_move()
{
	if (ss_cwm)
	{
		x = x + dx;

		if ((x+w) > 1.0)
		{
				x = 2.0 - (x+w) - w;
				dx = -dx;		
		}
		else if (x < 0.0)
		{
			x = -x;
			dx = -dx;
		}

		y = y + dy;

		if ((y+h) > 1.0)
		{
			y = 2.0 - (y+h) - h;
			dy = -dy;
		}
		else if (y < 0.0)
		{
			y = -y;
			dy = -dy;
		}

		ss_cwm->setPosition(x, y);
	}	

	m_timer.start();
}

void 
QSSApplication::sleep_end()
{
	sleeping = false;
	
	m_timer.stop();		
	
	auto dgm = g_displayGroupManager;

	while (dgm->getContentWindowManagers().size() > 0)
	{
		auto cw = dgm->getContentWindowManager(0);
		dgm->removeContentWindowManager(cw);
	}

	dgm->popState();

	emit(idling(false));

	m_timer.setInterval(interval);
	m_timer.start();
}
