#include "Client.hpp"

Client::Client(int socket, const std::string &ipAddress): socket(socket), ip(ipAddress)
{
};

const std::string &Client::getNick() const
{
    return nickname;
}

const std::string &Client::getPass() const
{
    return password;
};

const std::string &Client::getUsername() const
{
    return username;
};

const std::string &Client::getFullname() const
{
    return fullName;
};

int Client::getSocket() const
{
    return socket;
};

const std::string &Client::getIpAddress() const
{
    return ip;
}

void Client::setNick(const std::string &nick)
{
    nickname = nick;
}

void Client::setPass(const std::string &pass)
{
    password = pass;
}

void Client::setUsername(const std::string &user)
{
    username = user;
}

void Client::setFullname(const std::string &name)
{
    fullName = name;
}


std::string Client::info() const
{
    return std::to_string(socket) + "," + nickname + "," + ip;
}
