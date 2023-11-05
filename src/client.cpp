#include "tcpFileClient.hpp"
#include <cstring>
#include <iostream>

typedef struct {
    int port;
    char *ip;
    char *file;
    char *sendName;
    int help;
    int timeout;
} args;

char defaultIP[] = "127.0.0.1";

void parseArgs(int argc, char **argv, args *args) {
    args->help = 0;
    args->ip = defaultIP;
    args->port = 383;
    args->file = nullptr;
    args->sendName = nullptr;
    args->timeout = 3;
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
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0) {
            i++;
            if (i < argc) {
                args->timeout = (int) strtol(argv[i], nullptr, 10);
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
        std::cout << "-p --port <port> server port\n"
                  << "-ip <ip> server ip\n"
                  << "-t --timeout <seconds> set server timeout\n"
                  << "-f --file <name> file to send\n"
                  << "-s --send <name> file name for server\n"
                  << "-h --help show this message\n";
        return 0;
    }
    if (args.file == nullptr) {
        std::cerr << "No file to send!\n";
        return 0;
    }
    try {
        FileTCPClient server = FileTCPClient(args.ip, args.port, {args.timeout, 0});
        std::cout << "Address: " << server.getAddressString() << "\n";
        server.sendFile(args.file, args.sendName);
    } catch (TCPError &e) {
        std::cerr << "TCP error: " << e.what();
    }

}