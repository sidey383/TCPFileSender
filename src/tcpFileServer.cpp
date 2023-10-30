#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "../tcp/tcp.h"

#define BUF_SIZE 1048576

typedef struct {
    int port;
    char *ip;
    int help;
} args;

int readToBuff(int fd, void* val, size_t size) {
    size_t readBytes = 0;
    size_t totalBytes = 0;
    while ((readBytes = read(fd, ((char*)val) + totalBytes, size - totalBytes)) >= 0 && totalBytes < size) {
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

class FileTCPServer : public TCPServer {
private:
    void acceptor(int in, address_t address) override {
        printf("[%d] Accept connection from %s\n", getpid(), toString(address).c_str());

        size_t headerSize = 0;
        readSize(in, &headerSize);

        char name[headerSize + 1];
        readToBuff(in, name, headerSize);
        name[headerSize] = 0;
        printf("[%d] File name %s\n", getpid(), name);

        size_t fSize = 0;

        readSize(in, &fSize);
        printf("[%d] File Size %llu\n", getpid(), fSize);

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

public:
    FileTCPServer(char *ip, int port) : TCPServer(ip, port) {
    }

};

char defaultIP[] = "127.0.0.1";

void parseArgs(int argc, char **argv, args *args) {
    args->help = 0;
    args->ip = defaultIP;
    args->port = 383;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            i++;
            if (i < argc) {
                args->port = (int) strtol(argv[i], nullptr, 10);
            }
        }
        if (strcmp(argv[i], "-ip") == 0) {
            i++;
            if (i < argc) {
                args->ip = argv[i];
            }
        }
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            args->help = 1;
        }
    }
}

int main(int argc, char **argv) {
    args args;
    parseArgs(argc, argv, &args);
    if (args.help) {
        std::cout <<
                  "-p --port <port> server port\n" <<
                  "-ip <ip> server ip\n" <<
                  "-h --help show this message\n";
        return 0;
    }
    try {
        FileTCPServer server = FileTCPServer(args.ip, args.port);
        std::cout << server.getAddressString() << "\n";
        server.bindAddress();
        server.startLister();
    } catch (TCPError &e) {
        std::cerr << "TCP error: " << e.what();
    }

}
