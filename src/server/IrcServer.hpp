#pragma once
#include <netdb.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "IOEventPoller.hpp"
#include "parser/RawCommandParser.hpp"
#include <atomic>
#include "commands/IrcCommand.hpp"
#include "server/ChannelsManager.hpp"
#include "server/Client.hpp"
#include "server/MessageBroker.hpp"
#include <ctime>

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
    std::atomic<bool> *shutdownFlag = nullptr;
    std::string startTime;
public:
    IrcServer() = delete;
    IrcServer(const std::string &serverName, const char *port, const char *password);
    void setShutdownFlag(std::atomic<bool> *flag);
    void start();
    void shutdown();
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
    void HandlePartCmd(const IrcCommand::PartCmd &cmd);
    void HandleKickCmd(const IrcCommand::KickCmd &cmd);
    void HandleQuitCmd(const IrcCommand::QuitCmd &cmd);
    void sendListOfUsers(const Client &client, Channel *channel);
    Client *getClientByNick(const std::string &nick);
    void setStartupTime();
};
