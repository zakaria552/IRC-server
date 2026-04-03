#pragma once
#include <netdb.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include "IOEventPoller.hpp"
#include "parser/RawCommandParser.hpp"
#include <unistd.h>
#include <unordered_map>
#include "commands/IrcCommand.hpp"

struct client
{
    std::string nick;
    std::string user;
};

using Clients = std::unordered_map<unsigned int, client>;

#define DEFAULT_BACKLOG 10
class IrcServer
{
    int socketFd = -1;
    std::string password;
    bool closeConnection = false;
    IOEventPoller ioEvents;
    RawCommandParser parser;
    std::queue<IrcCommand> commands;
    Clients clients;
public:
    IrcServer() = delete;
    IrcServer(const char *port, const char *password);
    void start();
    void newClient();
    void clientDisconnected(const int index);
    void processRequest(const int clientFd, const char *body, const size_t length);
private:
    // Translates raw commands containing strings into type-safe commands.
    std::queue<IrcCommand> translateRawCommands(RawIrcCommands& raws);
};
