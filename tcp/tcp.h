#ifndef LAB2_TCP_H
#define LAB2_TCP_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <exception>
#include <string>
#include <utility>

int setSocketTimeout(int socketFd, struct timeval timeout);

void readSocket(int socket, void *val, size_t size);

void sendSocket(int socket, void *data, size_t size);

class ConnectionException : public std::exception {
private:
    std::string reason;
public:

    explicit ConnectionException(std::string reason) : reason(std::move(reason)) {}

    explicit ConnectionException(const char *reason) : reason(reason) {}

    const char *what() const noexcept override {
        return reason.c_str();
    }

};

class TCPError : public std::exception {
private:
    std::string message;
public:
    explicit TCPError(std::string message): message{std::move(message)} {}

    const char *what() const noexcept override {
        return message.c_str();
    }
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
    ~TCPServer();

};

class TCPClient: public TCPHolder {
public:
    TCPClient(char *ip, int port) : TCPHolder(ip, port) {}
    int openConnection();
};

#endif
