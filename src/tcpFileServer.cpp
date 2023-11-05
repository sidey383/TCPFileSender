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
#define TIME_TO_UPDATE 3000000

typedef struct {
    int port;
    char *ip;
    int help;
} args;


unsigned long getMs(struct timeval *start, struct timeval *end) {
    return ((end->tv_sec - start->tv_sec) * 1000000 + end->tv_usec - start->tv_usec);
}


class ConnectionAcceptor {
private:
    int socket;
    int file;
    struct {
        size_t fileSize;
        char fileName[NAME_SIZE];
    } header;

    int validateFileName(const char *str) {
        size_t strSize = 0;
        while (strSize < NAME_SIZE) {
            if (str[strSize] == 0) {
                break;
            }
            strSize++;
        }
        if (strSize == NAME_SIZE || strSize == 0) {
            std::cerr << "[" << getpid() << "] Wrong file name size\n";
            return 0;
        }
        if (strstr(str, "/.") != nullptr) {
            std::cerr << "[" << getpid() << "] Wrong file name, illegal substring \"/.\"\n";
            return 0;
        }
        if (str[0] == '/' || str[0] == '.') {
            std::cerr << "[" << getpid() << "] Wrong file name, illegal first character\n";
            return 0;
        }
        return 1;
    }

    int readHeader() {
        readSocket(socket, header.fileName, NAME_SIZE);
        if (!validateFileName(header.fileName)) {
            return STATUS_WRONG_PATH;
        }
        readSocket(socket, &(header.fileSize), sizeof(header.fileSize));
        if (header.fileSize > MAX_FILE_SIZE) {
            std::cerr << "[" << getpid() << "] File too log\n";
            return STATUS_FILE_TOO_LONG;
        }
        return STATUS_OK;
    }

    int readFile() {
        struct timeval globalStart{};
        gettimeofday(&globalStart, nullptr);
        size_t totalRead = 0;
        char buf[BUF_SIZE];
        while (totalRead < header.fileSize) {
            struct timeval stop{}, start{};
            gettimeofday(&start, nullptr);
            gettimeofday(&stop, nullptr);
            size_t periodRead = 0;
            while (getMs(&start, &stop) < TIME_TO_UPDATE && totalRead < header.fileSize) {
                size_t curRead = recv(socket, buf,
                                      header.fileSize - totalRead > BUF_SIZE ? BUF_SIZE : header.fileSize - totalRead,
                                      0);
                if (curRead == 0)
                    throw ConnectionException("Connection closed");
                if (curRead == -1)
                    throw ConnectionException(strerror(errno));
                size_t totalWrite = 0;
                while (totalWrite < curRead) {
                    size_t curWrite = write(file, buf + totalWrite, curRead - totalWrite);
                    if (curWrite == -1) {
                        std::cerr << "[" << getpid() << "] "
                                  << "File write error" << strerror(errno);
                        return 0;
                    }
                    totalWrite += curWrite;
                }

                periodRead += curRead;
                totalRead += curRead;
                gettimeofday(&stop, nullptr);
            }
            std::cout << "[" << getpid() << "] "
                      << "Status " << totalRead * 100 / header.fileSize << "% at " << stop.tv_sec - globalStart.tv_sec
                      << " sec\n"
                      << "Total speed: "
                      << (double) totalRead * 1000000.0 / ((double) getMs(&globalStart, &stop) * 1024)
                      << "Kb/s\n"
                      << "Current speed: "
                      << (double) periodRead * 1000000.0 / ((double) getMs(&start, &stop) * 1024)
                      << "Kb/s\n\n";
        }
        struct timeval globalEnd{};
        gettimeofday(&globalEnd, nullptr);
        std::cout << "[" << getpid() << "] "
                  << "Complete all\n\n";
        return 1;
    }

    int sendStatus(int status) {
        try {
            sendSocket(socket, &status, sizeof(int));
            return 1;
        } catch (ConnectionException &e) {
            std::cerr << "[" << getpid() << "] Send header status error: " << e.what();
            return 0;
        }
    }

    int openFile() {
        struct stat dirStat{};
        if (stat("uploads", &dirStat) != 0) {
            if (mkdir("uploads", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
                std::cerr << "[" << getpid() << "] Can't create directory for files\n";
                return STATUS_INTERNAL_ERROR;
            }
        } else {
            if (!S_ISDIR(dirStat.st_mode)) {
                std::cerr << "\"uploads\" is not directory\n";
                return STATUS_INTERNAL_ERROR;
            }
        }
        std::string path = "uploads/";
        path += header.fileName;
        file = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
        if (file < 0) {
            std::cerr << "[" << getpid() << "] File open error: " << strerror(errno) << "\n";
            return 0;
        }
        return STATUS_OK;
    }

public:
    ConnectionAcceptor(int socket) : socket(socket) {}

    void acceptFile() {
        int status;
        try {
            status = readHeader();
        } catch (ConnectionException &e) {
            std::cerr << "[" << getpid() << "] Read header error: " << e.what()  << "\n";
            return;
        }
        if (status != STATUS_OK) {
            sendStatus(status);
            return;
        }
        status = openFile();
        if(!sendStatus(status)) {
            close(file);
            return;
        }
        try {
            if (readFile()) {
                sendStatus(STATUS_OK);
            } else {
                sendStatus(STATUS_INTERNAL_ERROR);
            }
        } catch (ConnectionException &e) {
            std::cerr << "[" << getpid() << "] Read file error: " << e.what() << "\n";
            close(file);
            return;
        }
        close(file);
    }
};

class FileTCPServer : public TCPServer {
    struct timeval timeout;
private:
    void acceptor(int in, address_t address) override {
        std::cout << "[" << getpid() << "] Accept connection from " << toString(address) << "\n";
        //setSocketOptions(in, timeout);
        ConnectionAcceptor connectionAcceptor = ConnectionAcceptor(in);
        connectionAcceptor.acceptFile();
    }

public:
    FileTCPServer(char *ip, int port) : TCPServer(ip, port), timeout({1, 0}) {}

    FileTCPServer(char *ip, int port, struct timeval timeout) : TCPServer(ip, port), timeout(timeout) {}
};

char defaultIP[] = "127.0.0.1";

void parseArgs(int argc, char **argv, args *args) {
    args->help = 0;
    args->ip = defaultIP;
    args->port = DEFAULT_PORT;
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
        std::cerr << "TCP error: " << e.what() << "\n";
    }

}
