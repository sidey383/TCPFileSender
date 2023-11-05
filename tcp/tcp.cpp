#include "tcp.h"
#include <cstring>

int setSocketTimeout(int socketFd, struct timeval timeout) {
    return setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

void readSocket(int socket, void *val, size_t size) {
    size_t totalRead = 0;
    while (totalRead < size) {
        size_t curRead = recv(socket, ((char *) val) + totalRead, size - totalRead, 0);
        if (curRead == 0)
            throw ConnectionException("Connection closed");
        if (curRead == -1)
            throw ConnectionException(strerror(errno));
        totalRead += curRead;
    }
}

void sendSocket(int socket, void *data, size_t size) {
    size_t totalSend = 0;
    while (totalSend < size) {
        size_t curSend = send(socket, (char *) data + totalSend, size, totalSend);
        if (curSend == 0)
            throw ConnectionException("Connection closed");
        if (curSend == -1)
            throw ConnectionException(strerror(errno));
        totalSend += curSend;
    }
}