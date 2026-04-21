#include "Channel.hpp"
#include "server/Client.hpp"
#include "utils/Logger.hpp"
#include <algorithm>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>

Channel::Channel(const std::string &name) : name(name){};

bool Channel::isBlackListed(int clientId)
{
    return find(blackList.begin(), blackList.end(), clientId) != blackList.end();
}

bool Channel::isMember(int clientId)
{
    return find(clients.begin(), clients.end(), clientId) != clients.end();
}

void Channel::addClient(int clientId)
{
    clients.push_back(clientId);
}

void Channel::sendMessage(const Client &sender, const std::string &msg)
{
    for(auto client: clients)
    {
        if (client == sender.getSocket())
            continue;
        std::string src = ":" + sender.getNick();
        std::string body = src + " PRIVMSG #" + name + " :" + msg + "\r\n";
        send(client, body.c_str(), body.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
        Logger::info("Sent message to client: " + std::to_string(client) + " , " + body);
    }
}

bool Channel::modeIsSet(Mode mode)
{
    return (modes >> mode) & 1;
}
void Channel::setMode(Mode mode)
{
    modes |= mode;
}
void Channel::unsetMode(Mode mode)
{
    //       111     010
    modes = modes & ~mode;
}
