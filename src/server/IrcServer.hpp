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
#include "ChannelsManager.hpp"
#include "server/Client.hpp"

using Clients = std::unordered_map<int, Client>;

#define DEFAULT_BACKLOG 10
class IrcServer
{
    int socketFd = -1;
    std::string serverName;
    std::string password;
    bool closeConnection = false;
    IOEventPoller ioEvents;
    RawCommandParser parser;
    Clients clients;
    ChannelsManager channels;
public:
    IrcServer() = delete;
    IrcServer(const std::string &serverName, const char *port, const char *password);
    void start();
    void newClient();
    void clientDisconnected(int clientFd);
    void processRequest(int clientFd, const char *body, const size_t length);
private:
    // Translates raw commands containing strings into type-safe commands.
    std::queue<IrcCommand> translateRawCommands(RawIrcCommands& raws, int clientFd);
    bool authenticate(const Client &client);
    void HandlePrivMsgCmd(const IrcCommand::PrivMsgCmd &cmd);
    void HandleUserCmd(const IrcCommand::UserCmd &cmd);
    void HandleInviteCmd(const IrcCommand::InviteCmd &cmd);
    void HandleModeCmd(const IrcCommand::ModeCmd &cmd);
    void HandleJoinCmd(const IrcCommand::JoinCmd &cmd);
};
