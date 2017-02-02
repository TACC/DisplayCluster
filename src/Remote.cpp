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
  if(getenv("DISPLAYCLUSTER_PORT") == NULL)
  {
      cerr << "DISPLAYCLUSTER_PORT environment variable must be set for listening";
      exit(-1);
  }

  int displaycluster_port = atoi(std::string(getenv("DISPLAYCLUSTER_PORT")).c_str());

  server.listen(QHostAddress::Any, displaycluster_port);
  cerr << "XXXXXXXXXXXXXXXXXXXXX listening on " << displaycluster_port << " XXXXXXXXXXXXXXXXXXXXXXXX\n";
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
