#pragma once
#include <netdb.h>
#include <queue>
#include <string>
#include <sys/socket.h>

#define DEFAULT_BACKLOG 10

using Clients = std::queue<int>;

class IrcServer
{
    int port;
    int socketFd;
    std::string password; // tmp
    bool closeConnection = false;
    Clients clientsFd;
public:
    IrcServer() = delete;
    IrcServer(const char *port, const char *password);
    void start();
};
