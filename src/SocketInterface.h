#ifndef SOCKETINTERFACE_H
#define SOCKETINTERFACE_H

#include <unistd.h>
#include <iostream>

#include "json.hpp" 
using json = nlohmann::json;

class Connection 
{
public:
    Connection(const char *host, int port);
    Connection(int skt) : m_skt(skt) {std::cerr << "accepted " << m_skt << "\n"; }
    ~Connection() { std::cerr << "closing socket: " << m_skt << "\n"; close(m_skt); }

    json Receive();
    void Send(json j);
    
protected:
    int m_skt;
};

class SocketInterface
{
public:
    SocketInterface(int port);
    ~SocketInterface() { close(m_srvr); }

    Connection *Accept();
    int WaitForConnection();

protected:
    int m_srvr;
};


#endif
