#include "tcpFileServer.hpp"
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "timeUtil.hpp"

class ConnectionAcceptor {
private:
    int socket;
    int file{};
    struct {
        size_t fileSize;
        char fileName[NAME_SIZE];
    } header{};

    static int validateFileName(const char *str) {
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

    int readFile() const {
        struct timeval globalStart{};
        gettimeofday(&globalStart, nullptr);
        size_t totalRead = 0;
        char buf[BUF_SIZE];
        while (totalRead < header.fileSize) {
            struct timeval stop{}, start{};
            gettimeofday(&start, nullptr);
            gettimeofday(&stop, nullptr);
            size_t periodRead = 0;
            while (getMicroseconds(&start, &stop) < TIME_TO_UPDATE && totalRead < header.fileSize) {
                size_t curRead = recv(socket, buf, header.fileSize - totalRead > BUF_SIZE ? BUF_SIZE : header.fileSize - totalRead, 0);
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
                      << "Status " << totalRead * 100 / header.fileSize << "% at "
                      << getTimeDifStr(&globalStart, &stop) << " sec\n"
                      << "Total speed: "
                      << getSpeedStr(totalRead, &globalStart, &stop) << "\n"
                      << "Current speed: "
                      << getSpeedStr(periodRead, &globalStart, &stop) << "\n\n";
        }
        return 1;
    }

    int sendStatus(int status) const {
        try {
            sendSocket(socket, &status, sizeof(int));
            return 1;
        } catch (ConnectionException &e) {
            std::cerr << "[" << getpid() << "] Send status error: " << e.what() << "\n";
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
    explicit ConnectionAcceptor(int socket) : socket(socket), file(0), header({0, ""}) {}

    void acceptFile() {
        int status;
        try {
            status = readHeader();
        } catch (ConnectionException &e) {
            std::cerr << "[" << getpid() << "] Read header error: " << e.what() << "\n";
            return;
        }
        if (status != STATUS_OK) {
            sendStatus(status);
            return;
        }
        std::cout << "[" << getpid() << "] Receive file load request: " << header.fileSize << " " << header.fileName
                  << "\n";
        status = openFile();
        if (!sendStatus(status)) {
            close(file);
            return;
        }
        try {
            std::cout << "[" << getpid() << "] Start read file\n";
            if (readFile()) {
                std::cout <<  "[" << getpid() << "] Success receive file\n";
                sendStatus(STATUS_OK);
            } else {
                std::cout <<  "[" << getpid() << "] Receive file error\n";
                sendStatus(STATUS_INTERNAL_ERROR);
            }
        } catch (ConnectionException &e) {
            std::cerr << "[" << getpid() << "] Read file error: " << e.what() << "\n";
        }
        close(file);
    }
};

void FileTCPServer::acceptor(int in, address_t address) {
    std::cout << "[" << getpid()
              << "] Accept connection from " << toString(address)
              << "\n";

    setSocketTimeout(in, timeout);
    auto connectionAcceptor = ConnectionAcceptor(in);
    connectionAcceptor.acceptFile();
}

FileTCPServer::FileTCPServer(char *ip, int port) : TCPServer(ip, port), timeout({1, 0}) {}

FileTCPServer::FileTCPServer(char *ip, int port, struct timeval timeout) : TCPServer(ip, port), timeout(timeout) {}

