#pragma once
#include <netdb.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include "IOEventPoller.hpp"
#include <unistd.h>

#define DEFAULT_BACKLOG 10
class IrcServer
{
    int socketFd = -1;
    std::string password;
    bool closeConnection = false;
    IOEventPoller ioEvents;
public:
    IrcServer() = delete;
    IrcServer(const char *port, const char *password);
    void start();
    void newClient();
    void clientDisconnected(const int index);
    void processRequest(const int clientFd, const char *body, const size_t length);
};
