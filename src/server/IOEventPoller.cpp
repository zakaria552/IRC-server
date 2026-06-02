#include "IOEventPoller.hpp"
#include <cerrno>
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include "utils/Logger.hpp"

void IOEventPoller::pollEvents()
{
    Logger::debug("Polling events");
    while (!newPolls.empty())
    {
        polls.push_back(newPolls.top());
        newPolls.pop();
    }
    if (poll(polls.data(), polls.size(), -1) < 0)
    {
        if (errno == EINTR)
            return;
        throw std::runtime_error("Poll failed");
    }
}

void IOEventPoller::add(const pollfd &newPollfd)
{
    newPolls.push(newPollfd);
}

void IOEventPoller::remove(const int &fd)
{
    for(auto it = polls.begin(); it != polls.end();)
    {
        if (it->fd == fd)
            it = polls.erase(it);
        else
            it++;
    }
}
