#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>


#define MAX_CONNECTION 10

#define BUF_SIZE 1048576

#define TIME_TO_UPDATE 1000000

long writ = 0;

void handler(int code) {
    exit(1);
}

int getFileSize(char *filename, size_t* size) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
        return -1;
    if (fseek(fp, 0, SEEK_END) < 0) {
        fclose(fp);
        return -1;
    }
    *size = ftell(fp);
    fclose(fp);
    return 0;
}

unsigned long getMs(struct timeval* start, struct timeval* end) {
    return ((end->tv_sec - start->tv_sec) * 1000000 + end->tv_usec - start->tv_usec);
}

int openIPV4Connection(char* addr, int port) {
    int err;
    struct in_addr in_addr;
    err = inet_pton(AF_INET, addr, &in_addr);
    if (err == 0)
        return 0;
    if (err < 0)
        return err;

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr = in_addr;
    serverAddr.sin_port = htons(port);

    int fdS = socket(AF_INET, SOCK_STREAM, 0);
    if ((err = connect(fdS, (const struct sockaddr*) &serverAddr, sizeof(serverAddr))) < 0) {
        return err;
    }

    return fdS;
}

int openIPV6Connection(char* addr, int port) {
    int err;
    struct in6_addr in_addr;
    err = inet_pton(AF_INET6, addr, &in_addr);
    if (err == 0)
        return 0;
    if (err < 0) {
        perror("Inet_pron");
        return err;
    }

    struct sockaddr_in6 serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(port);
    serverAddr.sin6_addr = in_addr;

    int fdS = socket(AF_INET6, SOCK_STREAM, 0);
    if ((err = connect(fdS, (const struct sockaddr*) &serverAddr, sizeof(serverAddr))) < 0) {
        perror("connect");
        return err;
    }

    return fdS;
}

int openConnection(char* addrStr, char* portStr) {
    unsigned short port = (unsigned short) atoi(portStr);
    int fdS;
    fdS = openIPV4Connection(addrStr, port);
    if (fdS > 0)
        return fdS;
    fdS = openIPV6Connection(addrStr, port);
    return fdS;
}

void writeBig(int fd, void* data, size_t size) {
    size_t writed = 0;
    while (writed < size) {
        ssize_t w = write(fd, data + writed, size - writed);
        if (w < 0) {
            perror("Write error");
            exit(0);
        }
        writed += w;
    }
}

int main(int argc, char** argv) {
    int err;
    signal(SIGSEGV, handler);
    signal(SIGINT, handler);
    if (argc < 3) {
        printf("Usage %s: <addr> <port> <file>\n", argv[0]);
        return 0;
    }

    size_t fSize;

    err = getFileSize(argv[3], &fSize);
    if (err != 0) {
        write(2, "File open error\n", 16);
        return 0;
    }

    int file = open(argv[3], O_RDONLY);
    if (file < 0) {
        fprintf(stderr, "File open error: %s", strerror(file));
        return 0;
    }

    int fdS = openConnection(argv[1], argv[2]);
    if (fdS <= 0) {
        fprintf(stderr, "Connection open error");
        return 0;
    }
    puts("Connected");

    write(fdS, &fSize, sizeof(fSize));
    printf("Write file size %llu\n", fSize);

    size_t headerSize = strlen(argv[3]);
    write(fdS, &headerSize, sizeof(headerSize));
    write(fdS, argv[3], headerSize);

    printf("Write name %s\n", argv[3]);
    char buf[BUF_SIZE];
    long size = 0;
    struct timeval globalStart;
    gettimeofday(&globalStart, NULL);
    puts("Start write file body");
    while (size < fSize) {
        struct timeval stop, start;
        gettimeofday(&start, NULL);
        gettimeofday(&stop, NULL);
        ssize_t bites = 0;
        while (getMs(&start, &stop) < TIME_TO_UPDATE && size < fSize) {
            size_t readed = read(file, buf, BUF_SIZE);
            if (readed < 0) {
                perror("Read error");
                exit(0);
            }
            writeBig(fdS, buf, readed);
            fsync(fdS);
            size += readed;
            bites += readed;
            gettimeofday(&stop, NULL);
        }
        printf("Speed: %llu Kb/s\n", bites * 1000000 / ( getMs(&start, &stop) * 1024 ));
    }
    struct timeval globalEnd;
    gettimeofday(&globalEnd, NULL);
    printf("Complete in: %llu ms\n", getMs(&globalStart, &globalEnd));
    close(fdS);
    close(file);
    return 0;
}
