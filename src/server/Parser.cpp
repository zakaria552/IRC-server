#include "Parser.hpp"
#include "utils/Logger.hpp"

Messages Parser::parseBody(const int &clientFd, const char *body, const int &length)
{
    std::string sbody = std::string(body, length);
    clientBuffers[clientFd].push(sbody);
    if (!sbody.find("\r\n"))
    {
        Logger::info("Waiting for more data");
        return {};
    }
    std::string msg;
    while (!clientBuffers[clientFd].empty())
    {
        msg+= clientBuffers[clientFd].front();
        clientBuffers[clientFd].pop();
    }
    size_t pos, pos_start = 0;
    while((pos = msg.find("\r\n", pos_start)) != std::string::npos)
    {
       std::string token = msg.substr(pos_start, pos - pos_start);
       pos_start = pos + 2;
       messages[clientFd].push(token);
    }
    if (pos_start != (msg.length() - 1))
        clientBuffers[clientFd].push(msg.substr(pos_start));
    Messages msgs;
    while (!messages[clientFd].empty())
    {
        std::string msg = messages[clientFd].front();
        msgs.push(msg);
        messages[clientFd].pop();
    }
    return msgs;
}

//:localhost CAP * LS :none
