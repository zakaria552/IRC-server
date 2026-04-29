#pragma once

#include "server/Client.hpp"
#include <string>
#include "server/QueueMessages.hpp"

class NumericReplies
{
public:
    NumericReplies() = delete;
    static std::string makeBody(int errCode, const std::string &nick, const std::string &channel, const std::string &msg);
    static std::string welcome();
    static std::string passMisMatch();
    static std::string topicReply(const std::string &channel, const std::string &nick, const std::string &topic);
    static std::string noTopicReply(const std::string &channel, const std::string &nick);
    static std::string topicSetBy(const std::string &channel, const std::string &nick, const std::string &setter, const std::string &time);
    static Message channelNotFound(const std::string &channel, const Client &client);
    static Message notChannelMember(const std::string &channel, const Client &client);
    static Message isChannelMember(const std::string &channel, const Client &sender, const Client &receiver);
    static Message isInviteOnly(const std::string &channel, const Client &client);
    static Message setServerName(const std::string &name);
};
