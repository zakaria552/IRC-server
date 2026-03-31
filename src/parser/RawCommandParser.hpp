#pragma once
#include <unordered_map>
#include <queue>
#include <string>
#include "commands/RawIrcCommand.hpp"

using ClientMappedRawCommands = std::unordered_map<int, std::queue<RawIrcCommand>>;
using StreamBuffers = std::unordered_map<int, std::queue<std::string>>;
using RawIrcCommands = std::queue<RawIrcCommand>;

class RawCommandParser
{
    StreamBuffers buffers;
    ClientMappedRawCommands commands;
    void updateCommands(const int &clientFd, const char *body, const int &length);
public:
    RawCommandParser() = default;
    RawIrcCommands parse(const int &clientFd, const char *body, const int &length);
};
