#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "DCSocketManager.h"
#include "ContentWindowManager.h"

#include "main.h"

DCSocketManager::DCSocketManager(int port)
{
	DCSocketInterface *iface = new DCSocketInterface(port);
	connect(iface, SIGNAL(connectionReady(int)), this, SLOT(handleConnection(int)));
	iface->start();
}

void 
DCSocketManager::update_client(Connection* conn)
{
	json j_out = json::array();

	std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();
	for (unsigned int i = 0; i < contentWindowManagers.size(); i++)
	{
		boost::shared_ptr<ContentWindowManager> cm = contentWindowManagers[i];
		boost::shared_ptr<Content> c = cm->getContent();
		std::cerr << c->getURI() << "\n";
		double x, y, w, h;
		cm->getCoordinates(x, y, w, h);
		j_out.push_back({x, y, w, h, c->getURI().c_str()});
	}

	conn->Send(j_out);
}
void
DCSocketManager::handleConnection(int skt)
{
	Connection conn(skt);

    json j_in = conn.Receive();

	std::string cmd = j_in["cmd"];
	if (cmd == "update")
	{
		update_client(&conn);
	}
	else if (cmd == "reposition")
	{
		std::string uri = j_in["uri"];
		double x = j_in["x"];
		double y = j_in["y"];
		double w = j_in["w"];
		double h = j_in["h"];
		std::cerr << "reposition " << uri << " to " << x << " " << y << " " << w << " " << h << "\n";
		
		std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();
		for (unsigned int i = 0; i < contentWindowManagers.size(); i++)
		{
			boost::shared_ptr<ContentWindowManager> cm = contentWindowManagers[i];
			boost::shared_ptr<Content> c = cm->getContent();
			if (uri == c->getURI())
			{
				cm->setCoordinates(x, y, w, h);
				break;
			}
		}
	}	
	else if (cmd == "open")
	{
		std::string uri = j_in["uri"];
		double x = j_in["x"];
		double y = j_in["y"];
		double w = j_in["w"];
		double h = j_in["h"];
		std::cerr << "open  " << uri << " to " << x << " " << y << " " << w << " " << h << "\n";

		boost::shared_ptr<Content> c = Content::getContent(uri);
		boost::shared_ptr<ContentWindowManager> cm = boost::shared_ptr<ContentWindowManager>(new ContentWindowManager(c));
		cm->setCoordinates(x, y, w, h);
		g_displayGroupManager->addContentWindowManager(cm);
	}
	else if (cmd == "close")
	{
		std::string uri = j_in["uri"];		
		
		std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();
		
		for (unsigned int i = 0; i < contentWindowManagers.size(); i++)
		{
			boost::shared_ptr<ContentWindowManager> cm = contentWindowManagers[i];
			boost::shared_ptr<Content> c = cm->getContent();
			if (uri == c->getURI())
			{
				g_displayGroupManager->removeContentWindowManager(cm);
				break;				
			}
		}
	}
	else if (cmd == "top")
	{
		std::string uri = j_in["uri"];		
		
		std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();
		
		for (unsigned int i = 0; i < contentWindowManagers.size(); i++)
		{
			boost::shared_ptr<ContentWindowManager> cm = contentWindowManagers[i];
			boost::shared_ptr<Content> c = cm->getContent();
			if (uri == c->getURI())
			{
				g_displayGroupManager->moveContentWindowManagerToFront(cm);
				break;				
			}
		}
	}
}

DCSocketInterface::DCSocketInterface(int port)
{
	m_srvr = new SocketInterface(port);
}
  
void 
DCSocketInterface::run()
{
	while (true)
	{
		int skt = m_srvr->WaitForConnection();
		emit connectionReady(skt);
	}
}
