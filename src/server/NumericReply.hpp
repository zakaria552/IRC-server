#pragma once

#include "server/Client.hpp"
#include <string>

class NumericRepies
{
    std::string serverName;
public:
    NumericRepies() = default;
    std::string body(const std::string &);
    std::string makeBody(const std::string &nick, const std::string &channel, const std::string &msg);
    void sendReply(int clientFd, int code, const std::string &body);

    static std::string welcome() {return "001 ";};
    static std::string passMisMatch() {return "464 : Invalid password\r\n";};
    static void channelNotFound(const std::string &serverName, const std::string &channel, const Client &client);
    static void notChannelMember(const std::string &channel, const Client &client);
    static void isChannelMember(const std::string &channel, const Client &sender, const Client &receiver);
    static void isInviteOnly(const std::string &channel, const Client &client);
    static NumericRepies &getInstance();
    static void setServerName(const std::string &name);
};
