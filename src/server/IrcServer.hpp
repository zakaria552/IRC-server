#pragma once
#include <netdb.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "IOEventPoller.hpp"
#include "parser/RawCommandParser.hpp"
#include <unistd.h>
#include <unordered_map>
#include "commands/IrcCommand.hpp"
#include "server/ChannelsManager.hpp"
#include "server/Client.hpp"
#include "server/MessageBroker.hpp"

using Clients = std::unordered_map<int, Client>;
#define DEFAULT_BACKLOG 10

class IrcServer
{
    int socketFd = -1;
    std::string password;
    bool closeConnection = false;
    IOEventPoller ioEvents;
    RawCommandParser parser;
    Clients clients;
    MessageBroker msgBroker;
    ChannelsManager channels;
public:
    IrcServer() = delete;
    IrcServer(const std::string &serverName, const char *port, const char *password);
    void start();
private:
    // Translates raw commands containing strings into type-safe commands.
    void newClient();
    void clientDisconnected(int clientFd);
    void processRequest(int clientFd, const char *body, const size_t length);
    std::queue<IrcCommand> translateRawCommands(RawIrcCommands& raws, int clientFd);
    void authenticate(Client &client);
    void HandlePrivMsgCmd(const IrcCommand::PrivMsgCmd &cmd);
    void HandleUserCmd(const IrcCommand::UserCmd &cmd);
    void HandleInviteCmd(const IrcCommand::InviteCmd &cmd);
    void HandleModeCmd(const IrcCommand::ModeCmd &cmd);
    void HandleJoinCmd(const IrcCommand::JoinCmd &cmd);
    void HandleNickCmd(const IrcCommand::NickCmd &cmd);
    void HandlePassCmd(const IrcCommand::PassCmd &cmd);
    void HandleCapCmd(const IrcCommand::CapCmd &cmd);
    void HandlePingCmd(const IrcCommand::PingCmd &cmd);
    void HandleTopicCmd(const IrcCommand::TopicCmd &cmd);
    void sendListOfUsers(const Client &client, Channel *channel);
    Client *getClientByNick(const std::string &nick);
};
