#pragma once

#include "server/Client.hpp"
#include "server/MessageBroker.hpp"
#include <string>

class NumericReplies
{
public:
    NumericReplies() = delete;
    static std::string makeBody(int errCode, const std::string &nick, const std::string &channel, const std::string &msg);
    static std::string makeBody(int errCode, const std::string &nick, const std::string &msg);
    static std::string welcome();
    static std::string passMisMatch();
    static std::string topicReply(const std::string &channel, const std::string &nick, const std::string &topic);
    static std::string noTopicReply(const std::string &channel, const std::string &nick);
    static std::string topicSetBy(const std::string &channel, const std::string &nick, const std::string &setter, const std::string &time);
    static MessageBroker::Message channelNotFound(const std::string &channel, const Client &client);
    static MessageBroker::Message notChannelMember(const std::string &channel, const Client &client);
    static MessageBroker::Message isChannelMember(const std::string &channel, const Client &sender, const Client &receiver);
    static MessageBroker::Message isInviteOnly(const std::string &channel, const Client &client);
    static MessageBroker::Message invalidChannelKey(const std::string &channel, const Client &client);
    static MessageBroker::Message nickInUse(const std::string &nick, const Client &client);
//    static Message invalidModeParam(const std::string &channel, const Client &client);
    static MessageBroker::Message setServerName(const std::string &name);
    static MessageBroker::Message noSuchUser(const Client &client, const std::string &targetNick);
    static MessageBroker::Message userNotInChannel(const std::string &channel, const Client &client, const Client &target);
    static MessageBroker::Message channelIsFull(const std::string &channel, const Client &client);
    // modes
    static MessageBroker::Message isNotOperator(const std::string &channel, const Client &client);
    static MessageBroker::Message listModes(const std::string &channel, const std::string &modes, const Client &client);
    static MessageBroker::Message invalidModeParams(const std::string &channel, const Client &client, const std::string &mode, const std::string &description);
};
