#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "../tcp/tcp.h"
#include <sys/time.h>

#define BUF_SIZE 1048576

#define TIME_TO_UPDATE 1000000

typedef struct {
    int port;
    char *ip;
    char *file;
    char *sendName;
    int help;
} args;

unsigned long getMs(struct timeval* start, struct timeval* end) {
    return ((end->tv_sec - start->tv_sec) * 1000000 + end->tv_usec - start->tv_usec);
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

void writeBig(int fd, void* data, size_t size) {
    size_t writed = 0;
    while (writed < size) {
        ssize_t w = write(fd, ((char*)data) + writed, size - writed);
        if (w < 0) {
            std::cerr << "Write error: " << strerror(errno);
            exit(0);
        }
        writed += w;
    }
}

class FileTCPClient : public TCPClient {
public:
    FileTCPClient(char *ip, int port) : TCPClient(ip, port) {}

    void sendFile(char *fName, char *sendName) {
        size_t fSize;
        int err = getFileSize(fName, &fSize);
        if (err != 0) {
            std::cout<<"File open error\n";
            return;
        }
        int file = open(fName, O_RDONLY);
        if (file < 0) {
            std::cerr << "File open error: " << strerror(file);
            return;
        }
        std::cout<<"Open file\n";
        int fd = openConnection();
        if (fd < 0) {
            std::cerr << "Connect error: " << strerror(fd);
            return;
        }
        std::cout<<"Open connection\n";
        size_t headerSize = strlen(sendName);
        write(fd, &headerSize, sizeof(headerSize));
        write(fd, sendName, headerSize);
        printf("Write name %s\n", sendName);

        write(fd, &fSize, sizeof(fSize));
        printf("Write file size %llu\n", fSize);
        fsync(fd);

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
                writeBig(fd, buf, readed);
                fsync(fd);
                size += readed;
                bites += readed;
                gettimeofday(&stop, NULL);
            }
            printf("Complete: %d%% Speed: %llu Kb/s\n", size * 100 /fSize , bites * 1000000 / ( getMs(&start, &stop) * 1024 ));
        }
        struct timeval globalEnd;
        gettimeofday(&globalEnd, NULL);
        printf("Complete in: %llu ms\n", getMs(&globalStart, &globalEnd));
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
