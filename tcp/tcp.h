#ifndef LAB2_TCP_H
#define LAB2_TCP_H
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <exception>
#include <utility>
#include <sstream>


class TCPError : public std::exception {
private:
    std::string message;
public:
    explicit TCPError(std::string message);

    [[nodiscard]] const char *what() const noexcept override;
};

typedef union {
    struct sockaddr ip;
    struct sockaddr_in6 ipv6;
    struct sockaddr_in ipv4;
} address_t;

std::string toString(address_t address);

class TCPHolder {
private:
    int initAddrIPV4(char* ip, int port);
    int initAddrIPV6(char* ip, int port);
protected:
    std::string addressString;
    address_t address{};
public:
    TCPHolder(char *ip, int port);
    std::string getAddressString();
};

class TCPServer: public TCPHolder {
    virtual void acceptor(int fd, address_t address) = 0;
    int fdS = -1;

public:
    TCPServer(char *ip, int port) : TCPHolder(ip, port) {}
    void bindAddress();
    void startLister();

};

class TCPClient: public TCPHolder {
public:
    TCPClient(char *ip, int port) : TCPHolder(ip, port) {}
    int openConnection();
};

#endif
