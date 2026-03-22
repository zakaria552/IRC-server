#include "irc.hpp"

int main(int argc, char **args)
{
    if (argc != 3)
    {
        Logger::error("Missing required arguments: usage ./ircserv port password");
        exit(1);
    }
    irc::server server(args[1], args[2]);
    server.start();
    return 0;
}
