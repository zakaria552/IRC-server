#pragma once
#include <string>

class Client
{
    unsigned int socket;
    std::string password;
    std::string nickname;
    std::string username;
    bool operatorPriv = false;
public:
    Client() =  default;
    Client(const unsigned int &socket);
    ~Client() = default;
    const std::string &getNick() const;
    const std::string &getPass() const;
    const std::string &getUsername() const;
    const unsigned int &getSocket() const;
    void setNick(const std::string &nick);
    void setPass(const std::string &pass);
    void setUsername(const std::string &username);

    bool isOperator();
    void promote();
    void demote();
};
