#include "IrcServer.hpp"
#include <netdb.h>
#include <string>
#include <sys/socket.h>

IrcServer::IrcServer(const int &port, const char *password) : port(port), password(password)
{
}
