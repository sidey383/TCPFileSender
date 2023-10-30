#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include "../tcp/tcp.h"

typedef struct {
    int port;
    char *ip;
    int help;
} args;

class FileTCPServer: public TCPServer {
private:
    void acceptor(int fd, address_t address) override {
        printf("[%d] Accept connection from %s\n", getpid(), toString(address).c_str());
        char buf[6];
        read(fd, buf, 6);
        std::cout<<buf;
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
        std::cout << "-p --port <port>" << "-ip <ip>" << "-h --help show this message\n";
        return 0;
    }
    try {
        FileTCPServer server = FileTCPServer(args.ip, args.port);
        std::cout<<server.getAddressString()<<"\n";
        server.bindAddress();
        server.startLister();
    } catch (TCPError& e) {
        std::cerr<<"TCP error: "<<e.what();
    }

}
