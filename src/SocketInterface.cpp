#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
#endif

#include "SocketInterface.h"

namespace
{

void ensureWinsock()
{
#ifdef _WIN32
    static bool started = false;
    if (!started)
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        started = true;
    }
#endif
}

void closeSocket(socket_t skt)
{
#ifdef _WIN32
    closesocket(skt);
#else
    close(skt);
#endif
}

}

Connection::Connection(const char *host, int port)
{
    ensureWinsock();

    m_skt = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &serverAddress.sin_addr) <= 0)
        throw std::invalid_argument("Invalid address or address not supported");

    if (connect(m_skt, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        throw std::invalid_argument("Connection failed");
}

Connection::~Connection()
{
    closeSocket(m_skt);
}

json
Connection::Receive()
{
    int sz;
    recv(m_skt, (char *)&sz, sizeof(sz), 0);
    char *buf = new char[sz + 1];
    recv(m_skt, (char *)buf, sz, 0);
    buf[sz] = '\0';
    json j = json::parse(buf);
    delete[] buf;
    return j;
}

void
Connection::Send(json j)
{
    std::string msg = j.dump();

    int sz = msg.length();
    send(m_skt, (const char *)&sz, sizeof(sz), 0);
    send(m_skt, (const char *)msg.c_str(), (int)(msg.length()), 0);
}


SocketInterface::SocketInterface(int port)
{
    ensureWinsock();

    m_srvr = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    std::cerr << "Opening Python access on port " << port << "\n";

    int b = bind(m_srvr, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    while (b)
    {
        std::cerr << "waiting for interface socket to become free...\n";
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
        b = bind(m_srvr, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    }

    listen(m_srvr, 5);
}

SocketInterface::~SocketInterface()
{
    closeSocket(m_srvr);
}

Connection *
SocketInterface::Accept()
{
    socket_t skt = accept(m_srvr, nullptr, nullptr);
    return new Connection(skt);
}

socket_t
SocketInterface::WaitForConnection()
{
	  return accept(m_srvr, nullptr, nullptr);
}
