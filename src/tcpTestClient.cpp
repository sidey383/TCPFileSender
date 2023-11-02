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

class FileTCPClient : public TCPClient {
public:
    FileTCPClient(char *ip, int port) : TCPClient(ip, port) {}

    void sendFile(char *fName, char *sendName) {
        int fd = openConnection();
        if (fd < 0) {
            std::cerr << "Connect error: " << strerror(errno);
            return;
        }
        int err;

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        err = setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        if (err != 0) {
            std::cerr << "Connect option error: " << strerror(errno) << "\n";
            close(fd);
        }
        char str[] = "0123456789abcdef";
        char buf[1024];
        for (int i = 0; i < 1000; i++) {
            int len = 0;
            len = send(fd, str, 16, 0);
            if (len < 0) {
                if (errno == EAGAIN) {
                    std::cerr << "Socket read timeout\n";
                    break;
                }
                std::cerr << "Send error: " << errno << " " << strerror(errno) << "\n";
            } else {
                write(STDOUT_FILENO,"Send: ",sizeof("Send: "));
                write(STDOUT_FILENO, str, len);
                write(STDOUT_FILENO,"\n",1);
            };
            len = recv(fd, buf, sizeof(buf), 0);
            if (len < 0) {
                if (errno == EAGAIN) {
                    std::cerr << "Socket write timeout\n";
                    break;
                }
                std::cerr << "Send error: " << errno << " " << strerror(errno) << "\n";
            } else {
                write(STDOUT_FILENO,"Receive: ",sizeof("Receive: "));
                write(STDOUT_FILENO, buf, len);
                write(STDOUT_FILENO,"\n",1);
            }
            sleep(1);
        }

        close(fd);
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
                  "-h --help show this message\n";
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
