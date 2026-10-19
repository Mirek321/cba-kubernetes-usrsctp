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

#include "usrsctp.h"

#include <cstdarg>
#include <cstdio>

struct Arguments
{

    std::vector<std::string> servers;
    std::vector<std::string> interfaces;

    int serverPort = 0;
};

class Client
{
public:
    Client();
    ~Client();

    int run(int argc, char* argv[]);

private:
 	Arguments parseArguments(int argc, char* argv[]);
	static void debugPrintf(const char* format, ...);
	std::vector<sockaddr_in> getInterfaceAddresses(const std::vector<std::string>& interfaces);
	bool bindAddresses(struct socket* sock, std::vector<sockaddr_in>& addresses);
	bool connectToServer(struct socket* sock, const std::vector<std::string>& servers, uint16_t port);
	bool sendMessage(struct socket* sock, const std::string& message);
	static int receiveCallback(struct socket* sock, union sctp_sockstore addr, void* data, size_t datalen, struct sctp_rcvinfo rcv, int flags, void* ulp_info);
};
