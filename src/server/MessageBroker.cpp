#include "MessageBroker.hpp"
#include "utils/Logger.hpp"
#include <cerrno>
#include <cstring>
#include <sys/socket.h>

void MessageBroker::enqueue(const Message &msg)
{
    queueMessages.push_back(msg);
}

void MessageBroker::enqueue(const BroadcastMessage &msg)
{
    queueBroadcastMessages.push_back(msg);
}

void MessageBroker::flush()
{
    retryFailedMessages();
    Logger::debug("Total msgs: " + std::to_string(queueMessages.size()));
    std::erase_if(queueMessages, [&](Message &msg) {
        if (send(msg.clientFd, msg.msg.c_str(), msg.msg.length(), MSG_DONTWAIT | MSG_NOSIGNAL) == -1)
        {
            Logger::error("Failed to send message: " + std::string(std::strerror(errno)));
            retries.push_back({msg});
        }
        return true;
    });
    Logger::debug("After total msgs: " + std::to_string(queueMessages.size()));

    Logger::debug("Total broadcast msgs: " + std::to_string(queueBroadcastMessages.size()));
    std::erase_if( queueBroadcastMessages, [&](BroadcastMessage &msg) {
        std::erase_if(msg.clientFds, [&](int clientFd) {
            if (send(clientFd, msg.msg.c_str(), msg.msg.length(), MSG_DONTWAIT | MSG_NOSIGNAL) == -1)
            {
                Logger::error("Failed to send message: " + std::string(std::strerror(errno)));
                return false;
            }
            return true;
        });
        return msg.clientFds.empty();
    });
    Logger::debug("After total broadcast msgs: " + std::to_string(queueBroadcastMessages.size()));
}

void MessageBroker::removeStaleMessages(int clientFd)
{
    std::erase_if(queueMessages, [&](Message &msg) {
        return (msg.clientFd == clientFd);
    });

    std::erase_if(retries, [&](RetryMessage &retry) {
        return (retry.message.clientFd == clientFd);
    });

    for(BroadcastMessage &msg: queueBroadcastMessages)
    {
        std::erase_if(msg.clientFds, [&](int fd) {
            return (fd == clientFd);
        });
    }

    for(RetryBroadcastMessage &retry: broadcastRetries)
    {
        std::erase_if(retry.message.clientFds, [&](int fd) {
            return (fd == clientFd);
        });
    }
}

void MessageBroker::retryFailedMessages()
{
    Logger::debug("Total retry msgs: " + std::to_string(retries.size()));
    std::erase_if(retries, [&](RetryMessage &retry) {
        if (send(retry.message.clientFd, retry.message.msg.c_str(), retry.message.msg.length(), MSG_DONTWAIT | MSG_NOSIGNAL) == -1)
        {
            Logger::error("Failed to send retry message: " + std::string(std::strerror(errno)));
            retry.attempts++;
        }
        return (retry.attempts == MAX_RETRY_ATTEMPT);
    });
    Logger::debug("After total retry msgs: " + std::to_string(retries.size()));

    Logger::debug("Total retry broadcast msgs: " + std::to_string(retries.size()));
    std::erase_if( broadcastRetries, [&](RetryBroadcastMessage &retry) {
        std::erase_if(retry.message.clientFds, [&](int clientFd) {
            if (send(clientFd, retry.message.msg.c_str(), retry.message.msg.length(), MSG_DONTWAIT | MSG_NOSIGNAL) == -1)
            {
                Logger::error("Failed to send retry message: " + std::string(std::strerror(errno)));
                return false;
            }
            return true;
        });
        retry.attempts++;
        return retry.message.clientFds.empty() || (retry.attempts == MAX_RETRY_ATTEMPT);
    });
    Logger::debug("After total retry broadcast msgs: " + std::to_string(retries.size()));
}
