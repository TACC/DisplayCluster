#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "QSSApp.h"
#include "DCSocketManager.h"
#include "ContentWindowManager.h"

#include "main.h"
#include "log.h"

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
		double x, y, w, h;
		cm->getCoordinates(x, y, w, h);
		j_out.push_back({x * g_configuration->getNumTilesWidth(), y * g_configuration->getNumTilesHeight(), 
				w * g_configuration->getNumTilesWidth(), h * g_configuration->getNumTilesHeight(), c->getURI().c_str(), cm->getHidden()});
	}

	conn->Send(j_out);
}
void
DCSocketManager::handleConnection(int skt)
{
	QSSApplication *q_app = (QSSApplication *)g_app;

	q_app->pause_screensaver();

	Connection conn(skt);
    json j_in = conn.Receive();

	std::string cmd = j_in["cmd"];
	if (cmd == "update")
	{
		put_flog(LOG_WARN, "UPDATE\n");
		update_client(&conn);
	}
	else if (cmd == "reposition")
	{
		std::string uri = j_in["uri"];
		double x = (double)j_in["x"] / g_configuration->getNumTilesWidth();
		double y = (double)j_in["y"] / g_configuration->getNumTilesHeight();
		double w = (double)j_in["w"] / g_configuration->getNumTilesWidth();
		double h = (double)j_in["h"] / g_configuration->getNumTilesHeight();
		
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

		double x = (double)j_in["x"];
		double y = (double)j_in["y"];
		double w = (double)j_in["w"];
		double h = (double)j_in["h"];

		if (g_configuration->getNumTilesWidth() != 0 && g_configuration->getNumTilesHeight() != 0)
		{
			x = x / g_configuration->getNumTilesWidth();
			y = y / g_configuration->getNumTilesHeight();
			w = w / g_configuration->getNumTilesWidth();
			h = h / g_configuration->getNumTilesHeight();
		}

		put_flog(LOG_WARN, "Opening %s at (%g %g %g %g)\n", uri.c_str(), x, y, w, h);

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
	else if (cmd == "hide")
	{
		std::string uri = j_in["uri"];

		std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();

		for (unsigned int i = 0; i < contentWindowManagers.size(); i++)
		{
			boost::shared_ptr<ContentWindowManager> cm = contentWindowManagers[i];
			boost::shared_ptr<Content> c = cm->getContent();
			if (uri == c->getURI())
			{
				cm->setHidden(true);
				break;
			}
		}
	}
	else if (cmd == "reveal")
	{
		std::string uri = j_in["uri"];

		std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();

		for (unsigned int i = 0; i < contentWindowManagers.size(); i++)
		{
			boost::shared_ptr<ContentWindowManager> cm = contentWindowManagers[i];
			boost::shared_ptr<Content> c = cm->getContent();
			if (uri == c->getURI())
			{
				cm->setHidden(false);
				break;
			}
		}
	}
	else if (cmd == "clear")
	{
		g_displayGroupManager->setContentWindowManagers(std::vector<boost::shared_ptr<ContentWindowManager> >());
	}
	else if (cmd == "constrain aspect ratio")
	{
		g_mainWindow->constrainAspectRatio(j_in["state"] == "on");
	}
	else if (cmd == "show window borders")
	{
		g_displayGroupManager->getOptions()->setShowWindowBorders(j_in["state"] == "on");
	}
	else if (cmd == "show content labels")
	{
		g_displayGroupManager->getOptions()->setShowContentLabels(j_in["state"] == "on");
	}
	else if (cmd == "get configuration")
	{
        json j_out = json::array();
        j_out.push_back(g_configuration->getNumTilesWidth());
        j_out.push_back(g_configuration->getNumTilesHeight());
        conn.Send(j_out);
	}
	else if (cmd == "clear state")
	{
		g_mainWindow->clearContents();
	}
	else if (cmd == "load state")
	{
		std::string state = j_in["state"];
		g_displayGroupManager->loadStateXMLFile(state);
	}
	
	q_app->resume_screensaver();
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
