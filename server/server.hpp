#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include <getopt.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdarg>
#include <cstdio>

#include "usrsctp.h"

struct ServerArguments
{
    std::vector<std::string> interfaces;
    int serverPort = 0;
};

class Server
{
public:
    Server();
    ~Server();

    int run(int argc, char* argv[]);

private:
    ServerArguments parseArguments(int argc, char* argv[]);
    static void debugPrintf(const char* format, ...);
    std::vector<sockaddr_in> getInterfaceAddresses(
        const std::vector<std::string>& interfaces,
        uint16_t port);
    bool bindAddresses(
        struct socket* sock,
        std::vector<sockaddr_in>& addresses);
    bool receiveMessage(struct socket* sock);
};
