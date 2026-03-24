#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
// io libs

#include <iostream>

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
