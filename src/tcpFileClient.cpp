#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "../tcp/tcp.h"
#include <sys/time.h>
#include <sys/stat.h>
#include "tcpFileProtocol.hpp"

#define BUF_SIZE 1048576

#define TIME_TO_UPDATE 1000000

typedef struct {
    int port;
    char *ip;
    char *file;
    char *sendName;
    int help;
} args;

#define FILE_SIZE_OK 0
#define FILE_SIZE_ERROR -1
#define FILE_SIZE_NOT_FILE -2

unsigned long getMs(struct timeval *start, struct timeval *end) {
    return ((end->tv_sec - start->tv_sec) * 1000000 + end->tv_usec - start->tv_usec);
}

int getFileSize(char *filename, __off_t *size) {
    struct stat fStat{};
    if (stat(filename, &fStat) == -1) {
        return FILE_SIZE_ERROR;
    }
    if (S_ISREG(fStat.st_mode)) {
        (*size) = fStat.st_size;
        return FILE_SIZE_OK;
    }
    return FILE_SIZE_NOT_FILE;
}

void receiveServerStatus(int fd, int *status) {
    readSocket(fd, status, sizeof(int));
}

class FileTCPClient : public TCPClient {
    struct timeval timeout;
public:

    FileTCPClient(char *ip, int port, struct timeval timeout) : TCPClient(ip, port), timeout(timeout) {}

    FileTCPClient(char *ip, int port) : TCPClient(ip, port), timeout({1, 0}) {}

    void sendFile(char *fName, char *sendName) {
        size_t fSize;
        __off_t originSize;
        int err = getFileSize(fName, &originSize);
        if (err != FILE_SIZE_OK) {
            if (err == FILE_SIZE_ERROR)
                std::cout << "File size read error: " << strerror(errno) << "\n";
            if (err == FILE_SIZE_NOT_FILE)
                std::cout << "Not file: " << fName << "\n";
            return;
        }
        fSize = originSize;
        int file = open(fName, O_RDONLY);
        if (file < 0) {
            std::cerr << "File open error: " << strerror(file) << "\n";
            return;
        }
        std::cout << "Open file\n";
        int fd = openConnection();
        if (fd < 0) {
            std::cerr << "Connect error: " << strerror(errno) << "\n";
            close(file);
            return;
        }
        setSocketTimeout(fd, timeout);
        std::cout << "Open connection\n";
        char sendNameRes[NAME_SIZE];
        strcpy(sendNameRes, sendName);
        try {
            sendSocket(fd, sendNameRes, NAME_SIZE);
        } catch (ConnectionException &e) {
            std::cerr << "Send file name error: " << e.what() << "\n";
            close(file);
            close(fd);
            return;
        }

        std::cout << "Write name " << sendNameRes << "\n";

        try {
            sendSocket(fd, &fSize, sizeof(fSize));
        } catch (ConnectionException &e) {
            std::cerr << "Send file size error: " << e.what() << "\n";
            close(file);
            close(fd);
            return;
        }
        std::cout << "Write file size " << fSize << "\n";
        int serverStatus;
        try {
            receiveServerStatus(fd, &serverStatus);
        } catch (ConnectionException &e) {
            std::cerr << "Header status receive error: " << e.what() << "\n";
            close(file);
            close(fd);
            return;
        }
        switch (serverStatus) {
            case STATUS_INTERNAL_ERROR:
                std::cerr << "Server internal error\n";
                close(file);
                close(fd);
                return;
            case STATUS_WRONG_PATH:
                std::cerr << "Wrong file path\n";
                close(file);
                close(fd);
                return;
            case STATUS_FILE_TOO_LONG:
                std::cerr << "File too long\n";
                close(file);
                close(fd);
                return;
            case STATUS_OK:
                break;
            default:
                std::cerr << "Unknown status: " << serverStatus << "\n";
                close(file);
                close(fd);
                return;
        }

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
                try {
                    sendSocket(fd, buf, readed);
                } catch (ConnectionException &e) {
                    std::cerr << "File data write error: " << e.what() << "\n";
                    close(file);
                    close(fd);
                    return;
                }
                size += readed;
                bites += readed;
                gettimeofday(&stop, NULL);
                fsync(fd);
            }
            std::cout << "Status: " << size * 100 / fSize << "% loaded Speed: "
                      << bites * 1000000 / (getMs(&start, &stop) * 1024) << "kB/s\n";
        }
        struct timeval globalEnd;
        gettimeofday(&globalEnd, NULL);
        try {
            receiveServerStatus(fd, &serverStatus);
        } catch (ConnectionException &e) {
            std::cerr << "Send status receive error: " << e.what() << "\n";
            close(file);
            close(fd);
            return;
        }
        switch (serverStatus) {
            case STATUS_INTERNAL_ERROR:
                std::cerr << "Server internal error\n";
                return;
            case STATUS_WRONG_PATH:
                std::cerr << "Wrong file path\n";
                return;
            case STATUS_FILE_TOO_LONG:
                std::cerr << "File too long\n";
                return;
            case STATUS_OK:
                std::cout << "Complete in: " << globalEnd.tv_sec - globalStart.tv_sec << " sec\n";
                break;
            default:
                std::cerr << "Unknown status: " << serverStatus << "\n";
                return;
        }
        close(fd);
        close(file);

    }

};

char defaultIP[] = "127.0.0.1";

void parseArgs(int argc, char **argv, args *args) {
    args->help = 0;
    args->ip = defaultIP;
    args->port = 383;
    args->file = nullptr;
    args->sendName = nullptr;
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
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) {
            i++;
            if (i < argc) {
                args->file = argv[i];
            }
        }
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--send") == 0) {
            i++;
            if (i < argc) {
                args->sendName = argv[i];
            }
        }
    }
    if (args->sendName == nullptr) {
        args->sendName = args->file;
    }
}

int main(int argc, char **argv) {
    args args;
    parseArgs(argc, argv, &args);
    if (args.help) {
        std::cout <<
                  "-p --port <port> server port\n" <<
                  "-ip <ip> server ip\n" <<
                  "-f --file <name> file to send\n" <<
                  "-s --send <name> file name for server\n" <<
                  "-h --help show this message\n";
        return 0;
    }
    if (args.file == nullptr) {
        std::cerr << "No file to send!\n";
        return 0;
    }
    try {
        FileTCPClient server = FileTCPClient(args.ip, args.port);
        std::cout << server.getAddressString() << "\n";
        server.sendFile(args.file, args.sendName);
    } catch (TCPError &e) {
        std::cerr << "TCP error: " << e.what();
    }

}
