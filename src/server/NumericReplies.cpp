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

Message NumericReplies::channelNotFound(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(403, client.getNick(), channel, "No such channel")};
}

Message NumericReplies::notChannelMember(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(442, client.getNick(), channel, "You're not on that channel")};
}

Message NumericReplies::isChannelMember(const std::string &channel, const Client &sender, const Client &receiver)
{
    return {sender.getSocket(), NumericReplies::makeBody(443, sender.getNick() + " " + receiver.getNick(), channel, "is already on channel")};
}

Message NumericReplies::isInviteOnly(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(473, client.getNick(), channel, "Cannot join channel (+i)")};
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
