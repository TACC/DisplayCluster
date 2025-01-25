#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include "SocketInterface.h"

Connection::Connection(const char *host, int port)
{    
    m_skt = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &serverAddress.sin_addr) <= 0)
        throw std::invalid_argument("Invalid address or address not supported");

    if (connect(m_skt, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) 
        throw std::invalid_argument("Connection failed");
}

json 
Connection::Receive()
{
    int sz;
    recv(m_skt, (void *)&sz, sizeof(sz), 0);
    char *buf = new char[sz + 1];
    recv(m_skt, (void *)buf, sz, 0);
    buf[sz] = '\0';
    std::cerr << "Received " << sz << " bytes: " << buf << "X\n";
    json j = json::parse(buf);
    delete[] buf;
    return j;
} 

void 
Connection::Send(json j)
{
    std::string msg = j.dump(); 

    int sz = msg.length();       
    std::cerr << "Sending " << sz << " bytes: " << msg.c_str() << "X\n";
    send(m_skt, (const void*)&sz, sizeof(sz), 0);
    send(m_skt, (const void*)msg.c_str(), (size_t)(msg.length()), 0);
}  


SocketInterface::SocketInterface(int port)
{
    m_srvr = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int b = bind(m_srvr, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    while (b)
    {
        std::cerr << "waiting for interface socket to become free...\n";
        sleep(1);
        b = bind(m_srvr, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    }

    listen(m_srvr, 5);
}

Connection *
SocketInterface::Accept()
{
    int skt = accept(m_srvr, nullptr, nullptr);
    return new Connection(skt);
}

int
SocketInterface::WaitForConnection()
{
	  return accept(m_srvr, nullptr, nullptr);
}
