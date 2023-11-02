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


class FileTCPServer : public TCPServer {
private:
    void acceptor(int fd, address_t address) override {
        printf("[%d] Accept connection from %s\n", getpid(), toString(address).c_str());

        int err;

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        if (err != 0) {
            std::cerr << "Connect option error: " << strerror(err);
        }
        char buf[16];

        while (1) {
            int len = recv(fd, buf, sizeof(buf), 0);
            if (len < 0) {
                if (errno == EAGAIN) {
                    std::cerr << "Socket timeout\n";
                    break;
                }
                std::cerr << "Receive error: " << strerror(errno) << "\n";
                continue;
            }
            write(STDOUT_FILENO,"Receive: ",sizeof("Receive :"));
            write(STDOUT_FILENO, buf, len);
            write(STDOUT_FILENO,"\n",1);
            int sendLen = 0;
            while (sendLen < len) {
                int curSend = send(fd, buf + sendLen, len - sendLen, 0);
                if (curSend < 0) {
                    if (errno == EAGAIN) {
                        std::cerr << "Socket timeout\n";
                        break;
                    }
                    std::cerr << "Send error: " << errno << " " << strerror(errno) << "\n";
                    continue;
                }
                write(STDOUT_FILENO,"Send: ",sizeof("Send: "));
                write(STDOUT_FILENO, buf, len);
                write(STDOUT_FILENO,"\n",1);
                sendLen+=curSend;
            }
        }

        close(fd);
    }

public:
    FileTCPServer(char *ip, int port) : TCPServer(ip, port) {}
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
        std::cerr << "TCP error: " << e.what() << "\n";
    }

}
