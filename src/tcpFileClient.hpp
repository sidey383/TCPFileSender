#pragma once
#include "tcpFileProtocol.hpp"
#include "../tcp/tcp.h"

#define BUF_SIZE 1048576

#define TIME_TO_UPDATE 1000000
void receiveServerStatus(int fd, int *status);

class FileTCPClient : public TCPClient {
private:
    struct timeval timeout;

    int acceptServerStatus(int status);

    int sendHeader(int fd, size_t fSize, char* sendName);

    void writeFile(int socket, int file, size_t size);

public:

    FileTCPClient(char *ip, int port, struct timeval timeout);

    FileTCPClient(char *ip, int port);

    void sendFile(char *fileName, char *sendName);

};
