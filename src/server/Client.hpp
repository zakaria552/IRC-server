#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

class Client;
using Clients = std::unordered_map<int, Client>;

class Client
{
public:
    enum Handshake: std::uint8_t
    {
        RECEIVED_NONE = 0,
        RECEIVED_CAP = 1 << 0,
        RECEIVED_PASS = 1 << 1,
        RECEIVED_USER = 1 << 2,
        RECEIVED_NICK = 1 << 3,
        AUTHENTICATED = 1 << 4,
    };
private:
    int socket;
    uint32_t ip; // Big-Endian
    std::string password;
    std::string nickname;
    std::string username;
    std::string fullName;
    std::string host;
    uint8_t handshakeState = RECEIVED_NONE;
public:
    Client() =  default;
    Client(int socket, uint32_t ipAddress);
    ~Client() = default;
    const std::string &getNick() const;
    const std::string &getPass() const;
    const std::string &getUsername() const;
    const std::string &getFullname() const;
    int getSocket() const;
    uint32_t getIpAddress() const;
    void setNick(const std::string &nick);
    void setPass(const std::string &pass);
    void setUsername(const std::string &username);
    void setFullname(const std::string &fullname);
    void setHost(const std::string &host);
    std::string info() const;
    void updateHandshakeState(Handshake state);
    bool isAuthenticated();
    uint8_t getHandshakeState();
    bool readyToAuthenticate();
    std::string prefix() const;
};
