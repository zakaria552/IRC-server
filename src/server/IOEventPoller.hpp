#pragma once
#include <sys/poll.h>
#include <vector>
#include <poll.h>

class IOEventPoller
{
public:
    std::vector<struct pollfd> polls;
    IOEventPoller() = default;
    void pollEvents();
    void add(const pollfd &);
    void remove(const int index);
    std::vector<struct pollfd>::iterator begin() {return this->polls.begin();};
    std::vector<struct pollfd>::iterator end() {return this->polls.end();};
};
