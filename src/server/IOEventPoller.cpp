#include "IOEventPoller.hpp"
#include <stdexcept>
#include <sys/poll.h>
#include "Logger.hpp"

void IOEventPoller::pollEvents()
{
    Logger::info("Polling events");
    while (!newPolls.empty())
    {
        polls.push_back(newPolls.top());
        newPolls.pop();
    }
    if (poll(polls.data(), polls.size(), -1) < 0)
        throw std::runtime_error("Poll failed");
}

void IOEventPoller::add(const pollfd &newPollfd)
{
    newPolls.push(newPollfd);
}

void IOEventPoller::remove(const int index)
{
    polls.erase(begin() + index);
}
