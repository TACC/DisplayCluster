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

#ifndef SSAVER_H
#define SSAVER_H

#include <iostream>
#include <math.h>
#include <QtGui>

class QSSApplication : public QApplication
{
	enum {sleeping, going_to_sleep, waking_up, awake};

	Q_OBJECT

public:
	QTimer m_timer;

	QSSApplication(int& argc, char **argv);

signals:
	void idling(bool);

public slots:

	void go_to_sleep()
	{
		if (sleepState == awake)
		{
			sleepState = going_to_sleep;
			sleep_start();
			sleepState = sleeping;
			m_timer.stop();
			emit(idling(true));
		}
	}

public:

	bool notify(QObject *r, QEvent *e)
	{
		if (e->type() == QEvent::MouseMove || e->type() == QEvent::MouseButtonPress || e->type() == QEvent::KeyPress)
		{
			if (sleepState == sleeping)
				wakeup();
			else 
			{
				m_timer.stop();
				m_timer.start();
			}
		}
		return QApplication::notify(r, e);
	}

	void wakeup()
	{
		sleepState = waking_up;
		sleep_end();
		emit(idling(false));
		sleepState = awake;
		m_timer.stop();
		m_timer.start();
	}
		

public:

	virtual void sleep_start();
	virtual void sleep_end();

private:
	int sleepState;
	int interval;
	
};

#endif
