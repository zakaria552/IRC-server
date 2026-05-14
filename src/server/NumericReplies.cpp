#include "NumericReplies.hpp"
#include "server/Client.hpp"
#include <string>
#include <sys/socket.h>
#include "server/globals.hpp"

std::string NumericReplies::makeBody(int errCode, const std::string &nick, const std::string &channel, const std::string &msg)
{
    std::string body = nick + " #" + channel + " :" + msg;
    std::string fullBody = ":" + serverName + " " + std::to_string(errCode) + " " + body + "\r\n";
    return fullBody;
}

std::string NumericReplies::makeBody(int errCode, const std::string &nick, const std::string &msg)
{
    std::string body = nick + " :" + msg;
    std::string fullBody = ":" + serverName + " " + std::to_string(errCode) + " " + body + "\r\n";
    return fullBody;
}

MessageBroker::Message NumericReplies::channelNotFound(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(403, client.getNick(), channel, "No such channel")};
}

MessageBroker::Message NumericReplies::notChannelMember(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(442, client.getNick(), channel, "You're not on that channel")};
}

MessageBroker::Message NumericReplies::bannedFromChannel(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(474, client.getNick(), channel, "You're banned from channel")};
}

MessageBroker::Message NumericReplies::isChannelMember(const std::string &channel, const Client &sender, const Client &receiver)
{
    return {sender.getSocket(), NumericReplies::makeBody(443, sender.getNick() + " " + receiver.getNick(), channel, "is already on channel")};
}

MessageBroker::Message NumericReplies::isInviteOnly(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(473, client.getNick(), channel, "Cannot join channel (+i)")};
}

MessageBroker::Message NumericReplies::invalidChannelKey(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(475, client.getNick(), channel, "Cannot join channel (+k)")};
}
//401 <requesting_nick> <target_nick> :No such nick/channel
MessageBroker::Message NumericReplies::noSuchUser(const Client &client, const std::string &targetNick)
{
    return {client.getSocket(), NumericReplies::makeBody(401, client.getNick() + " " + targetNick, "", "No such nick")};
}
//441 <requesting_nick> <target_nick> <channel> :They aren't on that channel
MessageBroker::Message NumericReplies::userNotInChannel(const std::string &channel, const Client &client, const Client &target)
{
    return {client.getSocket(), NumericReplies::makeBody(441, client.getNick() + " " + target.getNick(), channel, "They aren't on that channel")};
}
//<client> <channel> :You're not channel operator
MessageBroker::Message NumericReplies::isNotOperator(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(482, client.getNick(), channel, "You're not channel operator")};
}

MessageBroker::Message NumericReplies::listModes(const std::string &channel, const std::string &modes, const Client &client)
{
    std::string body = client.getNick() + " #" + channel + " " + modes;
    std::string fullBody = ":" + serverName + " " + std::to_string(324) + " " + body + "\r\n";
    return {client.getSocket(), fullBody};
}
std::string NumericReplies::welcome()
{
    return "001 ";
};

std::string NumericReplies::passMisMatch() {
    return "464 : Invalid password\r\n";
};

std::string NumericReplies::topicReply(const std::string &channel, const std::string &nick, const std::string &topic)
{
    return makeBody(332, nick, channel, topic);
}

std::string NumericReplies::noTopicReply(const std::string &channel, const std::string &nick)
{
    return makeBody(331, nick, channel, "No topic is set");
}

std::string NumericReplies::topicSetBy(const std::string &channel, const std::string &nick, const std::string &setter, const std::string &time)
{
    return ":" + serverName + " 333 " + nick + " #" + channel + " " + setter + " " + time + "\r\n";
}
//"<client> <channel> :Cannot join channel (+l)"
MessageBroker::Message NumericReplies::channelIsFull(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(471, client.getNick(), channel, "Cannot join channel (+l)")};
}
//"<client> <target chan/user> <mode char> <parameter> :<description>"
MessageBroker::Message NumericReplies::invalidModeParams(const std::string &channel, const Client &client, const std::string &mode, const std::string &description)
{
    return {client.getSocket(), NumericReplies::makeBody(696, client.getNick(), channel + " " + mode, description)};
}

MessageBroker::Message NumericReplies::nickInUse(const std::string &newNick, const Client &client)
{
    std::string nickName = client.getNick();
    if (nickName.empty())
        nickName = "*";
    return {client.getSocket(), NumericReplies::makeBody(433, nickName + " " + newNick, "Nickname is already in use")};
}
