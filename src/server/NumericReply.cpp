#include "NumericReply.hpp"
#include "server/Client.hpp"
#include <string>
#include <sys/socket.h>


NumericRepies &NumericRepies::getInstance()
{
    static NumericRepies replies;
    return replies;
}

std::string NumericRepies::makeBody(const std::string &nick, const std::string &channel, const std::string &msg)
{
    return nick + " #" + channel + " :" + msg;
}

void NumericRepies::sendReply(int clientFd, int code, const std::string &body)
{
    std::string fullBody = ":" + serverName + " " + std::to_string(code) + " " + body + "\r\n";
    send(clientFd, fullBody.c_str(), fullBody.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
}
void NumericRepies::channelNotFound(const std::string &serverName, const std::string &channel, const Client &client)
{
    (void) serverName;
    NumericRepies instance = getInstance();
    std::string body = instance.makeBody(client.getNick(), channel, "No such channel");
    instance.sendReply(client.getSocket(), 402, body);
}

void NumericRepies::notChannelMember(const std::string &channel, const Client &client)
{
    NumericRepies instance = getInstance();
    std::string body = instance.makeBody(client.getNick(), channel, "You're not on that channel");
    instance.sendReply(client.getSocket(), 442, body);
}

void NumericRepies::isChannelMember(const std::string &channel, const Client &sender, const Client &receiver)
{
    NumericRepies instance = getInstance();
    std::string body = instance.makeBody(sender.getNick() + " " + receiver.getNick(), channel, "is already on channel");
    instance.sendReply(sender.getSocket(), 443, body);
}

void NumericRepies::isInviteOnly(const std::string &channel, const Client &client)
{
    NumericRepies instance = getInstance();
    std::string body = instance.makeBody(client.getNick(), channel, "Cannot join channel (+i)");
    instance.sendReply(client.getSocket(), 473, body);
}

void NumericRepies::setServerName(const std::string &name)
{
    NumericRepies::getInstance().serverName = name;
}
