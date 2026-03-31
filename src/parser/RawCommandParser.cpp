#include "parser/RawCommandParser.hpp"
#include "utils/Logger.hpp"

RawIrcCommands RawCommandParser::parse(const int &clientFd, const char *body, const int &length)
{
    updateCommands(clientFd, body, length);
    if (commands[clientFd].empty())
        return {};
    RawIrcCommands rawCommands;
    while (!commands[clientFd].empty())
    {
        rawCommands.push(commands[clientFd].front());
        commands[clientFd].pop();
    }
    return rawCommands;
}

void RawCommandParser::updateCommands(const int &clientFd, const char *body, const int &length)
{
    std::string sbody = std::string(body, length);
    buffers[clientFd].push(sbody);
    if (!sbody.find("\r\n"))
    {
        Logger::info("Waiting for more data");
        return;
    }
    std::string msg;
    while (!buffers[clientFd].empty())
    {
        msg+= buffers[clientFd].front();
        buffers[clientFd].pop();
    }
    size_t pos, pos_start = 0;
    while((pos = msg.find("\r\n", pos_start)) != std::string::npos)
    {
        std::string token = msg.substr(pos_start, pos - pos_start);
        pos_start = pos + 2;
        commands[clientFd].push({token, clientFd});
    }
    if (pos_start != (msg.length() - 1))
        buffers[clientFd].push(msg.substr(pos_start));
}
