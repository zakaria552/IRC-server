#include "ChannelsManager.hpp"
#include "server/Channel.hpp"
#include "server/Client.hpp"
#include "utils/Logger.hpp"

void ChannelsManager::add(const std::string &channel, const unsigned int &clientId)
{
    Channel *room;
    if (channelExist(channel))
    {
        room = &channels[channel];
        if (room->isBlackListed(clientId))
        {
            Logger::info("Client " + std::to_string(clientId) + " blacklisted from: " + channel);
            return;
        }
        if (room->isMember(clientId))
            return;
    }
    else
    {
        channels[channel] = Channel(channel);
        room = &channels[channel];
    }
    room->addClient(clientId);
    Logger::info("Client "+ std::to_string(clientId) + " joined the channel: " + channel);
}


void ChannelsManager::sendMessage(const Client &sender, const std::string &targets, const std::string &msg)
{
    std::string room = targets.substr(1);
    if (channels.find(room) == channels.end())
    {
        Logger::info("Channel not found: [" + room + "]");
        return;
    }
    channels[room].sendMessage(sender, msg);
    Logger::info("Sent message");
}


bool ChannelsManager::channelExist(const std::string &channel)
{
    return channels.find(channel) != channels.end();
}
