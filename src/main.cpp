#include "irc.hpp"

#include <atomic>
#include <csignal>
#include <cstdlib>

static std::atomic<bool> shutdownRequested{false};

static void handleSignal(int signo)
{
    (void)signo;
    shutdownRequested.store(true, std::memory_order_relaxed);
}

static void installSignalHandlers()
{
    struct sigaction sa{};
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGQUIT, &sa, nullptr);

    signal(SIGPIPE, SIG_IGN);
}

int main(int argc, char **args)
{
    if (argc != 3)
    {
        Logger::error("Missing required arguments: usage ./ircserv port password");
        exit(EXIT_FAILURE);
    }
    installSignalHandlers();
    try
    {
        irc::server server(DEFAULT_SERVER_NAME, args[1], args[2]);
        server.setShutdownFlag(&shutdownRequested);
        server.start();
        server.shutdown();
    } catch(std::exception &err)
    {
        Logger::error(err.what());
        exit(EXIT_FAILURE);
    }
}
