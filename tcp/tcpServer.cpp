#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "tcp.h"

#define MAX_CONNECTION 10

TCPServer::~TCPServer() {
    if (fdS > 0)
        close(fdS);
}

void TCPServer::bindAddress() {
    fdS = socket(AF_INET, SOCK_STREAM, 0);
    if (fdS == -1) {
        throw TCPError("socket creation failed");
    }
    if (address.ip.sa_family == AF_INET) {
        if (bind(fdS, static_cast<struct sockaddr*>(&address.ip), sizeof(struct sockaddr_in)) != 0) {
            throw TCPError(strerror(errno));
        }
        return;
    }
    if (address.ip.sa_family == AF_INET6) {
        if (bind(fdS, static_cast<struct sockaddr*>(&address.ip), sizeof(struct sockaddr_in6)) != 0) {
            throw TCPError(strerror(errno));
        }
        return;
    }
}

void TCPServer::startLister() {
    int err;
    socklen_t len = address.ip.sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    address_t cliAddr;
    cliAddr.ip.sa_family = address.ip.sa_family;
    while (true) {
        printf("[%d] Start Listen\n", getpid());
        if ((err = listen(fdS, MAX_CONNECTION)) < 0) {
            fprintf(stderr, "[%d] Listen error: %s", getpid(), strerror(err));
            return;
        }
        int fd;
        if ((fd = accept(fdS, &cliAddr.ip, &len)) < 0) {
            fprintf(stderr, "[%d] Connect error: %s", getpid(), strerror(fd));
        } else {
            int pid = fork();
            if (pid < 0)
                fprintf(stderr, "[%d] Fork fail: %s", getpid(), strerror(pid));
            if (pid == 0) {
                printf("[%d] Connect from %s\n", getpid(), toString(cliAddr).c_str());
                acceptor(fd, cliAddr);
                close(fd);
                printf("[%d] Close connect from %s \n", getpid(), toString(cliAddr).c_str());
                return;
            }
        }
    }
}

