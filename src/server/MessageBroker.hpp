#pragma once
#include <string>
#include <set>
#include <deque>

class MessageBroker
{
public:
    struct Message
    {
        int clientFd;
        std::string msg;
    };

    struct BroadcastMessage
    {
        std::set<int> clientFds;
        std::string msg;
    };

    struct RetryMessage
    {
        Message message;
        int attempts = 0;
    };

    struct RetryBroadcastMessage
    {
        BroadcastMessage message;
        int attempts = 0;
    };
private:
    std::deque<Message> queueMessages;
    std::deque<BroadcastMessage> queueBroadcastMessages;
    std::deque<RetryMessage> retries;
    std::deque<RetryBroadcastMessage> broadcastRetries;
public:
    MessageBroker() = default;
    ~MessageBroker() = default;

    void enqueue(const Message &msg);
    void enqueue(const BroadcastMessage &msg);
    void flush();
    void removeStaleMessages(int clientFd);
private:
    void retryFailedMessages();
    static constexpr int MAX_RETRY_ATTEMPT = 3;
};
