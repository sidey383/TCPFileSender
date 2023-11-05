#pragma once
#include "../tcp/tcp.h"
#include "tcpFileProtocol.hpp"

#define BUF_SIZE 1048576
#define TIME_TO_UPDATE 3000000

class FileTCPServer : public TCPServer {
private:
    struct timeval timeout;
    void acceptor(int in, address_t address) override;

public:
    FileTCPServer(char *ip, int port);

    FileTCPServer(char *ip, int port, struct timeval timeout);
};
