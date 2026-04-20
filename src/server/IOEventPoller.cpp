#include "IOEventPoller.hpp"
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include "utils/Logger.hpp"

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

void IOEventPoller::remove(const int &fd)
{
    for(size_t i = 0; i < polls.size(); i++)
    {
        if (polls[i].fd == fd)
        {
            polls.erase(polls.begin() + i);
            return;
        }
    }
}
