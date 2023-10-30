#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define MAX_CONNECTION 10

#define BUF_SIZE 8192

int readToBuff(int fd, void* val, size_t size) {
    size_t readBytes = 0;
    size_t totalBytes = 0;
    while ((readBytes = read(fd, val + totalBytes, size - totalBytes)) >= 0 && totalBytes < size) {
        totalBytes += readBytes;
    }
    if (readBytes < 0) {
        return readBytes;
    }
    return 0;
}

int readToFile(int in, int out, size_t size) {
    size_t totalReadBytes = 0;
    char buf[BUF_SIZE];
    while (totalReadBytes < size) {
        size_t curReadBytes = read(in, buf, BUF_SIZE < size - totalReadBytes ? BUF_SIZE :  size - totalReadBytes);
        if (curReadBytes == 0)
            continue;
        if (curReadBytes < 0) {
            return  curReadBytes;
        }
        size_t totalWriteBytes = 0;
        while (totalWriteBytes < curReadBytes) {
            size_t curWriteBytes = write(out, buf + totalWriteBytes, curReadBytes - totalWriteBytes);
            if (curWriteBytes < 0) {
                return curWriteBytes;
            }
            totalWriteBytes += curWriteBytes;
        }
        totalReadBytes += curReadBytes;
    }
    return 0;
}

int readSize(int fd, size_t* val) {
    return readToBuff(fd, val, sizeof(size_t));
}

void connectionHandler(int in) {
    size_t fSize = 0;

    readSize(in, &fSize);
    printf("[%d] File Size %llu\n", getpid(), fSize);

    size_t headerSize = 0;
    readSize(in, &headerSize);

    char name[headerSize + 1];
    readToBuff(in, name, headerSize);
    name[headerSize] = 0;
    printf("[%d] File name %s\n", getpid(), name);

    int out = open(name, O_WRONLY | O_CREAT | O_TRUNC);

    if (out < 0) {
        fprintf(stderr, "[%d] File open error: %s", getpid(), strerror(out));
        return;
    }

    printf("[%d] Start read file\n", getpid());

    readToFile(in, out, fSize);

    printf("[%d] Complete read file\n", getpid());

    close(out);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("[%d] Usage %s: <port>\n", getpid(), argv[0]);
        return 0;
    }
    unsigned short port = (unsigned short) atoi(argv[1]);
    if (port == 0) {
        port = 8080;
    }
    struct sockaddr_in servaddr, cliaddr;
    int fdS = socket(AF_INET, SOCK_STREAM, 0);
    if (fdS < 0) {
        fprintf(stderr, "[%d] Socket open error: %s", getpid(), strerror(fdS));
        return 0;
    }

    printf("[%d] Create socket", getpid());

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    socklen_t casize = sizeof(cliaddr);

    int err;
    if ((err = bind(fdS, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
        fprintf(stderr, "[%d] Address bind error: %s", getpid(), strerror(err));
        return 0;
    }
    printf("[%d] Bind port %u\n", getpid(), ntohs(servaddr.sin_port));
    while (1) {
        printf("[%d] Start Listen\n", getpid());
        if ((err = listen(fdS, MAX_CONNECTION)) < 0) {
            fprintf(stderr, "[%d] Listen error: %s", getpid(), strerror(err));
            return 0;
        }
        int fd;
        if ((fd = accept(fdS, (struct sockaddr *)&cliaddr, &casize)) < 0) {
            fprintf(stderr, "[%d] Connect error: %s", getpid(), strerror(fd));
        } else {
            int pid = fork();
            if (pid < 0)
                fprintf(stderr, "[%d] Fork fail: %s", getpid(), strerror(pid));
            if (pid == 0) {
                printf("[%d] Connect from %s port %d\n", getpid(), inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
                connectionHandler(fd);
                close(fd);
                printf("[%d] Close connect from %s port %d\n", getpid(), inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
                return 0;
            }
        }
    }
    return 0;
}
