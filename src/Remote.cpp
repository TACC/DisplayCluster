#include "main.h"
#include "string.h"
#include "SSaver.h"
#include "Remote.h"
#include <iostream>
using namespace std;

Remote::Remote(QObject* parent): QObject(parent)
{
  connect(&server, SIGNAL(newConnection()), this, SLOT(acceptConnection()));

  server.listen(QHostAddress::Any, 1900);
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
