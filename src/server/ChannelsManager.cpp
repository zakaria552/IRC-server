#include "ChannelsManager.hpp"
#include "utils/Logger.hpp"

void ChannelsManager::add(const std::string &channel, const unsigned int &clientId)
{
    Channel &room = channels[channel];
    if (room.isBlackListed(clientId))
    {
        Logger::info("Client " + std::to_string(clientId) + " blacklisted from: " + channel);
        return;
    }
    if (room.isMember(clientId))
        return;
    room.addClient(clientId);
    Logger::info("Client "+ std::to_string(clientId) + " joined the channel: " + channel);
}
