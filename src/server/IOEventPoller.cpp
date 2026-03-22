#include "IOEventPoller.hpp"
#include <stdexcept>
#include <sys/poll.h>
#include "Logger.hpp"

void IOEventPoller::pollEvents()
{
    Logger::info("Polling events");
    if (poll(polls.data(), polls.size(), -1) < 0)
        std::runtime_error("Poll failed");
}

void IOEventPoller::add(const pollfd &newPollfd)
{
    polls.push_back((newPollfd));
}

void IOEventPoller::remove(const int index)
{
    polls.erase(begin() + index);
}
