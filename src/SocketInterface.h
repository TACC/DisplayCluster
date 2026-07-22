#ifndef SOCKETINTERFACE_H
#define SOCKETINTERFACE_H

#ifdef _WIN32
    #include <winsock2.h>
    typedef SOCKET socket_t;
#else
    #include <unistd.h>
    typedef int socket_t;
#endif

#include <iostream>

#include "json.hpp"
using json = nlohmann::json;

class Connection
{
public:
    Connection(const char *host, int port);
    Connection(socket_t skt) : m_skt(skt) {}
    ~Connection();

    json Receive();
    void Send(json j);

protected:
    socket_t m_skt;
};

class SocketInterface
{
public:
    SocketInterface(int port);
    ~SocketInterface();

    Connection *Accept();
    socket_t WaitForConnection();

protected:
    socket_t m_srvr;
};


#endif
