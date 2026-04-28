#include "ChannelsManager.hpp"
#include "server/Channel.hpp"
#include "server/Client.hpp"
#include "utils/Logger.hpp"


ChannelsManager::ChannelsManager(std::queue<BroadcastMessage> &queue)
    : queue(queue)
{

}

Channel *ChannelsManager::newChannel(const std::string &channel)
{
    channels[channel] = Channel(channel);
    return &channels[channel];
}
void ChannelsManager::add(const std::string &channel, int clientId)
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
    queue.push(channels[room].constructMessage(sender, msg));
    Logger::info("Sent message");
}


bool ChannelsManager::channelExist(const std::string &channelName)
{
    return channels.find(channelName) != channels.end();
}

bool ChannelsManager::isMemberOfChannel(const std::string &channel, int client)
{
    auto it = channels.find(channel);
    if (it != channels.end())
        return it->second.isMember(client);
    return false;
}

void ChannelsManager::updateChannelMode(const std::string &channel, Mode mode, char intent)
{
    if (intent == '-')
        channels[channel].unsetMode(mode);
    else if (intent == '+')
        channels[channel].setMode(mode);
}

uint8_t ChannelsManager::getChannelModes(const std::string &channel)
{
   return channels[channel].getModes();
}

Channel *ChannelsManager::getChannel(const std::string &channel)
{
    return channelExist(channel) ? &channels[channel] : nullptr;
}
