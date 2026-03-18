#include "irc.hpp"
#include "Logger.hpp"
#include "IrcServer.hpp"

void client(void);
void server(void);
void iplookup(char *domain);

int main(int argc, char **args)
{
    if (argc != 3)
    {
        Logger::error("Missing required arguments: usage ./ircserv port password");
        exit(1);
    }
    IrcServer server(args[1], args[2]);
    server.start();
    return 0;
}

void client(void)
{

}

void iplookup(char *domain)
{
    struct addrinfo req{}, *res, *p;
    char ipstr[INET6_ADDRSTRLEN];
    int status;
    req.ai_family = AF_UNSPEC;
    req.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(domain, nullptr, &req, &res)) != 0)
        std::cout << "Error: " << gai_strerror(status), exit(1);
    for( p = res; p != nullptr; p = p->ai_next)
    {
        void *addr;
        std::string ipver;

        if (p->ai_family == AF_INET) // ipv4
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        }
        else // ipv6
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        std::cout << ipver << " " << ipstr << std::endl;
    }
    freeaddrinfo(res);
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
