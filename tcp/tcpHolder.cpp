#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include "tcp.h"

std::string toString(address_t address) {
    char buf[128];
    std::stringstream ss;
    if (address.ip.sa_family == AF_INET6) {
        if (inet_ntop(AF_INET6, static_cast<void*>(&(address.ipv6.sin6_addr)), buf, 128) == nullptr)
            return "";
        ss << buf << ":" << ntohs(address.ipv6.sin6_port);
    }
    if (address.ip.sa_family == AF_INET) {
        if (inet_ntop(AF_INET, static_cast<void*>(&(address.ipv4.sin_addr)), buf, 128) == nullptr)
            return "";
        ss << buf << ":" << ntohs(address.ipv4.sin_port);
    }
    return ss.str();
}

std::string TCPHolder::getAddressString() {
    return addressString;
}


TCPHolder::TCPHolder(char *ip, int port) {
    if (port <= 0 || port > 65535) {
        throw TCPError("Wrong port number");
    }
    if (!((initAddrIPV4(ip, port) || initAddrIPV6(ip, port)))) {
        throw TCPError("Address create error");
    }
    addressString = toString(address);
}

int TCPHolder::initAddrIPV4(char *ip, int port) {
    int err;
    err = inet_pton(AF_INET, ip, &(address.ipv4.sin_addr));
    if (err != 1)
        return 0;

    address.ipv4.sin_family = AF_INET;
    address.ipv4.sin_port = htons(port);
    return 1;
}

int TCPHolder::initAddrIPV6(char *ip, int port) {
    int err;
    err = inet_pton(AF_INET6, ip, &(address.ipv6.sin6_addr));
    if (err != 1)
        return 0;

    address.ipv6.sin6_family = AF_INET6;
    address.ipv6.sin6_port = htons(port);
    return 1;
}
