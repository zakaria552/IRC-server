#pragma once
#include <string>

class Client
{
    unsigned int socket;
    std::string password;
    std::string nickname;
    std::string username;
    std::string fullName;
    bool operatorPriv = false;
public:
    Client() =  default;
    Client(const unsigned int &socket);
    ~Client() = default;
    const std::string &getNick() const;
    const std::string &getPass() const;
    const std::string &getUsername() const;
    const std::string &getFullname() const;
    const unsigned int &getSocket() const;
    void setNick(const std::string &nick);
    void setPass(const std::string &pass);
    void setUsername(const std::string &username);
    void setFullname(const std::string &fullname);

    bool isOperator();
    void promote();
    void demote();
};
