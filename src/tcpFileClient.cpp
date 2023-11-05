#include "tcpFileClient.hpp"
#include "timeUtil.hpp"
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

int getFileSize(char *filename, __off_t *size) {
    struct stat fStat{};
    if (stat(filename, &fStat) == -1) {
        std::cout << "File size read error: " << strerror(errno) << "\n";
        return 0;
    }
    if (!S_ISREG(fStat.st_mode)) {
        std::cout << "Not file: " << filename << "\n";
        return 0;
    }
    (*size) = fStat.st_size;
    return 1;
}

void receiveServerStatus(int fd, int *status) {
    readSocket(fd, status, sizeof(int));
}


FileTCPClient::FileTCPClient(char *ip, int port, struct timeval timeout) : TCPClient(ip, port), timeout(timeout) {}

FileTCPClient::FileTCPClient(char *ip, int port) : TCPClient(ip, port), timeout({1, 0}) {}

int FileTCPClient::acceptServerStatus(int status) {
    switch (status) {
        case STATUS_INTERNAL_ERROR:
            std::cerr << "Server internal error\n";
            return 0;
        case STATUS_WRONG_PATH:
            std::cerr << "Wrong file path\n";
            return 0;
        case STATUS_FILE_TOO_LONG:
            std::cerr << "File too long\n";
            return 0;
        case STATUS_OK:
            return 1;
        default:
            std::cerr << "Unknown status: " << status << "\n";
            return 0;
    }
}

int FileTCPClient::sendHeader(int fd, size_t fSize, char *sendName) {
    char sendNameRes[NAME_SIZE];
    strcpy(sendNameRes, sendName);
    try {
        sendSocket(fd, sendNameRes, NAME_SIZE);
    } catch (ConnectionException &e) {
        std::cerr << "Send file name error: " << e.what() << "\n";
        return 0;
    }

    std::cout << "Writ file name " << sendNameRes << "\n";

    try {
        sendSocket(fd, &fSize, sizeof(fSize));
    } catch (ConnectionException &e) {
        std::cerr << "Send file size error: " << e.what() << "\n";
        return 0;
    }
    std::cout << "Write file size " << fSize << "\n";
    return 1;
}

void FileTCPClient::writeFile(int socket, int file, size_t size) {
    char buf[BUF_SIZE];
    size_t totalSend = 0;
    puts("Start write file");
    struct timeval globalStart;
    gettimeofday(&globalStart, NULL);
    while (totalSend < size) {
        struct timeval stop, start;
        gettimeofday(&start, NULL);
        gettimeofday(&stop, NULL);
        ssize_t curSend = 0;
        while (getMicroseconds(&start, &stop) < TIME_TO_UPDATE && totalSend < size) {
            size_t curRead = read(file, buf, BUF_SIZE);
            sendSocket(socket, buf, curRead);
            totalSend += curRead;
            curSend += curRead;
            gettimeofday(&stop, NULL);
        }
        std::cout << "Status " << totalSend * 100 / size << "% at "
                  << getTimeDifStr(&globalStart, &stop) << " sec\n"
                  << "Total speed: "
                  << getSpeedStr(totalSend, &globalStart, &stop) << "\n"
                  << "Current speed: "
                  << getSpeedStr(curSend, &start, &stop) << "\n\n";
    }
}

void FileTCPClient::sendFile(char *fileName, char *sendName) {
    size_t size;
    __off_t originSize;
    int serverStatus;

    if (!getFileSize(fileName, &originSize)) {
        return;
    }
    size = originSize;
    int file = open(fileName, O_RDONLY);
    if (file < 0) {
        std::cerr << "File open error: " << strerror(file) << "\n";
        return;
    }
    std::cout << "Open file\n";
    int socket = openConnection();
    if (socket < 0) {
        std::cerr << "Connect error: " << strerror(errno) << "\n";
        close(file);
        return;
    }
    setSocketTimeout(socket, timeout);
    std::cout << "Open connection\n";

    if (!sendHeader(socket, size, sendName)) {
        close(file);
        close(socket);
        return;
    }
    try {
        receiveServerStatus(socket, &serverStatus);
    } catch (ConnectionException &e) {
        std::cerr << "Server status receive error: " << e.what() << "\n";
        close(file);
        close(socket);
        return;
    }

    if (!acceptServerStatus(serverStatus)) {
        close(file);
        close(socket);
        return;
    }
    try {
        writeFile(socket, file, size);
    } catch (ConnectionException &e) {
        std::cerr << "Write file error: " << e.what() << "\n";
        close(file);
        close(socket);
        return;
    }
    try {
        receiveServerStatus(socket, &serverStatus);
    } catch (ConnectionException &e) {
        std::cerr << "Send status receive error: " << e.what() << "\n";
        close(file);
        close(socket);
        return;
    }
    if (acceptServerStatus(serverStatus)) {
        std::cout << "Success!\n";
    }
    close(file);
    close(socket);
}

