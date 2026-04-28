#include "irc.hpp"

#include <unistd.h> // DELETEME!!!

int main(int argc, char **args)
{
    if (argc != 3)
    {
        Logger::error("Missing required arguments: usage ./ircserv port password");
        exit(EXIT_FAILURE);
    }
    try
    {
        irc::server server(DEFAULT_SERVER_NAME, args[1], args[2]);
        server.start();
    } catch(std::exception &err)
    {
        Logger::error(err.what());
        exit(EXIT_FAILURE);
    }
}
