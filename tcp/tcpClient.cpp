#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "tcp.h"

int TCPClient::openConnection() {
    socklen_t len = address.ip.sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(fd, &address.ip, len) != 0) {
        return -1;
    }
    return fd;
}
