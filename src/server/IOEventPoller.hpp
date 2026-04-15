#pragma once
#include <sys/poll.h>
#include <vector>
#include <stack>
#include <poll.h>

class IOEventPoller
{
    std::vector<struct pollfd> polls;
    std::stack<struct pollfd> newPolls;
public:
    IOEventPoller() = default;
    void pollEvents();
    void add(const pollfd &);
    void remove(const int &fd);
    std::vector<struct pollfd>::iterator begin() {return this->polls.begin();};
    std::vector<struct pollfd>::iterator end() {return this->polls.end();};
};
