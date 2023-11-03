/*********************************************************************/
/* Copyright (c) 2011 - 2023, The University of Texas at Austin.     */
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


#include "main.h"
#include "string.h"
#include "SSaver.h"
#include "Remote.h"
#include "log.h"
#include <iostream>
using namespace std;

Remote::Remote(QObject* parent): QObject(parent)
{
  connect(&server, SIGNAL(newConnection()), this, SLOT(acceptConnection()));

  // get port to listen on
  if(getenv("DISPLAYCLUSTER_REMOTEPORT") == NULL)
  {
      cerr << "DISPLAYCLUSTER_REMOTEPORT environment variable must be set for listening";
      exit(-1);
  }

  int displaycluster_port = atoi(std::string(getenv("DISPLAYCLUSTER_REMOTEPORT")).c_str());

  server.listen(QHostAddress::Any, displaycluster_port);
}

Remote::~Remote()
{
  server.close();
}

void Remote::acceptConnection()
{
  client = server.nextPendingConnection();
  connect(client, SIGNAL(readyRead()), this, SLOT(startRead()));
}

void Remote::startRead()
{
	char buffer[1024] = {0};
	client->read(buffer, client->bytesAvailable());
	cout << buffer << endl;
	client->close();

	cerr << "RECEIVED " << buffer << "\n";

	((QSSApplication *)g_app)->wakeup();

	if (strcmp(buffer, "None"))
	{
		QString *qstr = new QString("");
		if (buffer[0] != '/') 
		{
			qstr->append(getenv("DISPLAYCLUSTER_DIR"));
			qstr->append("/");
		}
		qstr->append(buffer);

		g_mainWindow->loadState(qstr);
		delete qstr;
	}
}
