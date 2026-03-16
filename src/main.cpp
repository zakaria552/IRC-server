#include <iostream>
#include <sys/socket.h>
#include "irc.hpp"

void client(void);
void server(void);

int main(int argc, char **args)
{
    if (argc < 2)
       server();
    else
       client();
    (void) args;
}

void client(void)
{

}

void server(void)
{
    SA saddr;
    saddr.sin_port = htons(4545); // port
    saddr.sin_family = AF_INET; // ipv4
    saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int socketFd = socket(PF_INET, SOCK_STREAM, 0);
    if (socketFd == -1)
        std::cout << "Failed to create socket: " << errno << std::endl;
    std::cout << "Successfully created socket: " << socketFd << std::endl;
    int b = bind(socketFd, (sockaddr *)&saddr, sizeof(saddr));
    if (b == -1)
        std::cout << "Failed to bind socket addr: " << errno << std::endl, exit(1);
    std::cout << "Successfully binded to socket" << std::endl;
    if (listen(socketFd, 10) == -1)
        std::cout << "Failed to listen to socket addr: " << errno << std::endl, exit(1);
    std::cout << "Successfully listening on port: " << 4545 << std::endl;
    SA clientAddr;
    socklen_t len;
    if (accept(socketFd, (sockaddr *)&clientAddr, &len) == -1)
        std::cout << "Failed to accept requests " << errno << std::endl, exit(1);
    std::cout << "Waiting for connection..." << std::endl;
}
