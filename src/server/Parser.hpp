#pragma once
#include <unordered_map>
#include <queue>
#include <string>

using Messages = std::queue<std::string>;

class Parser
{
    std::unordered_map<int, std::queue<std::string>> clientBuffers;
    std::unordered_map<int, std::queue<std::string>> messages;
public:
    Parser() = default;
    Messages parseBody(const int &clientFd, const char *body, const int &length);
};
