#ifndef _RAW_IRC_COMMAND_
#define _RAW_IRC_COMMAND_

#include <string>

struct RawIrcCommand
{
    std::string cmd;
    int client;
};

#endif
