#ifndef DCSOCKETMANAGER_H
#define DCSOCKETMANAGER_H

#include "SocketInterface.h"
#include <QObject>
#include <QThread>

class DCSocketManager : public QObject
{
	Q_OBJECT

public:
	DCSocketManager(int port);

protected:
	void update_client(Connection *);

public slots:
	void handleConnection(int);
};

class DCSocketInterface : public QThread
{
	Q_OBJECT

public:
	DCSocketInterface(int port);
	void run() override;

signals:
	void connectionReady(int);

private:
	SocketInterface *m_srvr;
};

#endif
