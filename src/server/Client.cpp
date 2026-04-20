#include "Client.hpp"

Client::Client(int socket): socket(socket){};

const std::string &Client::getNick() const {return nickname;}
const std::string &Client::getPass() const {return password;};
const std::string &Client::getUsername() const {return username;};
const std::string &Client::getFullname() const {return fullName;};
int Client::getSocket() const {return socket;};

void Client::setNick(const std::string &nick) {nickname = nick;}
void Client::setPass(const std::string &pass) {password = pass;};
void Client::setUsername(const std::string &user) {username = user;};
void Client::setFullname(const std::string &name) {fullName = name;};

bool Client::isOperator() {return operatorPriv;};
void Client::promote() {operatorPriv = true;};
void Client::demote() {operatorPriv = false;};
