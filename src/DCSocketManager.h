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
	// qintptr (not int/socket_t) because this crosses the Qt signal/slot
	// boundary, and a Windows SOCKET is pointer-sized and won't fit in a
	// plain int
	void handleConnection(qintptr);
};

class DCSocketInterface : public QThread
{
	Q_OBJECT

public:
	DCSocketInterface(int port);
	void run() override;

signals:
	void connectionReady(qintptr);

private:
	SocketInterface *m_srvr;
};

#endif
