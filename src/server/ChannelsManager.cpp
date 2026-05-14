#include "ChannelsManager.hpp"
#include "server/Channel.hpp"
#include "server/Client.hpp"
#include "server/MessageBroker.hpp"
#include "utils/Logger.hpp"


ChannelsManager::ChannelsManager(MessageBroker &broker)
    : msgBroker(broker)
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
}


void ChannelsManager::sendMessage(const Client &sender, const std::string &targets, const std::string &msg)
{
    std::string room = targets.substr(1);
    if (channels.find(room) == channels.end())
    {
        Logger::debug("Channel not found: [" + room + "]");
        return;
    }
    msgBroker.enqueue(channels[room].constructMessage(sender, msg));
}


void ChannelsManager::broadcastModeChange(const Client &client, const std::string &channel, const std::string &rawCmd)
{
    MessageBroker::BroadcastMessage broadcast;
    broadcast.clientFds = channels[channel].getClients();
    broadcast.msg = ":" + client.getNick() + " " + rawCmd + "\r\n";
    msgBroker.enqueue(broadcast);
}

void ChannelsManager::broadcastJoinedUser(const Client &client, const std::string &channel)
{
    MessageBroker::BroadcastMessage broadcast; //:WiZ JOIN #Twilight_zone
    broadcast.clientFds = channels[channel].getClients();
    broadcast.msg = ":" + client.getNick() + " JOIN " + "#" + channel + "\r\n";
    msgBroker.enqueue(broadcast);
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

void ChannelsManager::updateChannelMode(const std::string &channel, Channel::Mode mode, char intent)
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

void ChannelsManager::leaveAll(int clientFd)
{
    for(auto it = channels.begin(); it != channels.end();)
    {
        if (it->second.isMember(clientFd))
        {
            it->second.removeClient(clientFd);
            if (it->second.isEmpty())
            {
                it = channels.erase(it);
                continue;
            }
        }
        it++;
    }
}

Channel *ChannelsManager::leaveChannel(const std::string &channel, int clientFd)
{
    if (channels.find(channel) != channels.end() && channels[channel].isMember(clientFd))
    {
        channels[channel].removeClient(clientFd);
        if (channels[channel].isEmpty())
        {
            channels.erase(channel);
            return nullptr;
        }
    }
    return &channels[channel];
}
