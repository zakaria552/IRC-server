#include "NumericReplies.hpp"
#include "server/Client.hpp"
#include <string>
#include <sys/socket.h>
#include "server/globals.hpp"
#include "utils/Logger.hpp"

std::string NumericReplies::makeBody(int errCode, const std::string &nick, const std::string &channel, const std::string &msg)
{
    std::string body = nick + " #" + channel + " :" + msg;
    std::string fullBody = ":" + serverName + " " + std::to_string(errCode) + " " + body + "\r\n";
    return fullBody;
}

Message NumericReplies::channelNotFound(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(402, client.getNick(), channel, "No such channel")};
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

Message NumericReplies::invalidChannelKey(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(475, client.getNick(), channel, "Cannot join channel (+k)")};
}
//401 <requesting_nick> <target_nick> :No such nick/channel
Message NumericReplies::noSuchUser(const Client &client, const std::string &targetNick)
{
    return {client.getSocket(), NumericReplies::makeBody(401, client.getNick() + " " + targetNick, "", "No such nick")};
}
//441 <requesting_nick> <target_nick> <channel> :They aren't on that channel
Message NumericReplies::userNotInChannel(const std::string &channel, const Client &client, const Client &target)
{
    return {client.getSocket(), NumericReplies::makeBody(441, client.getNick() + " " + target.getNick(), channel, "They aren't on that channel")};
}
//<client> <channel> :You're not channel operator
Message NumericReplies::isNotOperator(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(482, client.getNick(), channel, "You're not channel operator")};
}

Message NumericReplies::listModes(const std::string &channel, const std::string &modes, const Client &client)
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

//"<client> <channel> :Cannot join channel (+l)"
Message NumericReplies::channelIsFull(const std::string &channel, const Client &client)
{
    return {client.getSocket(), NumericReplies::makeBody(471, client.getNick(), channel, "Cannot join channel (+l)")};
}

//"<client> <target chan/user> <mode char> <parameter> :<description>"
Message NumericReplies::invalidModeParams(const std::string &channel, const Client &client, const std::string &mode, const std::string &description)
{
    return {client.getSocket(), NumericReplies::makeBody(696, client.getNick(), channel + " " + mode, description)};
}
