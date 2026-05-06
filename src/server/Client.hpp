#pragma once
#include <string>

class Client
{
    int socket;
    std::string ip;
    std::string password;
    std::string nickname;
    std::string username;
    std::string fullName;
    bool operatorPriv = false;
public:
    Client() =  default;
    Client(int socket, const std::string &ipAddress);
    ~Client() = default;
    const std::string &getNick() const;
    const std::string &getPass() const;
    const std::string &getUsername() const;
    const std::string &getFullname() const;
    int getSocket() const;
    const std::string &getIpAddress() const;
    void setNick(const std::string &nick);
    void setPass(const std::string &pass);
    void setUsername(const std::string &username);
    void setFullname(const std::string &fullname);
    std::string info() const;
};
