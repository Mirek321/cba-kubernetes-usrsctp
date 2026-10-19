#include "server.hpp"

Server::Server()
{
}

Server::~Server()
{
}

void Server::debugPrintf(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

int Server::run(int argc, char* argv[])
{
    ServerArguments args = parseArguments(argc, argv);

    if (args.serverPort < 1 || args.serverPort > 65535)
    {
        std::cerr << "Server port must be between 1 and 65535\n";
        return 1;
    }

    std::cout << "Server port: " << args.serverPort << '\n';

    for (const std::string& interface : args.interfaces)
    {
        std::cout << "Interface: " << interface << '\n';
    }

    std::vector<sockaddr_in> addresses =
        getInterfaceAddresses(
            args.interfaces,
            static_cast<uint16_t>(args.serverPort));

    usrsctp_init(0, nullptr, Server::debugPrintf);

    struct socket* sock = usrsctp_socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_SCTP,
        nullptr,
        nullptr,
        0,
        nullptr
    );

    if (sock == nullptr)
    {
        std::cerr << "Failed to create SCTP socket\n";
        return 1;
    }

    if (!bindAddresses(sock, addresses))
    {
        usrsctp_close(sock);
        return 1;
    }

    if (usrsctp_listen(sock, 5) < 0)
    {
        perror("usrsctp_listen");
        usrsctp_close(sock);
        return 1;
    }

    std::cout << "Local SCTP addresses:\n";

    for (const auto& address : addresses)
    {
        char ip[INET_ADDRSTRLEN];

        if (inet_ntop(
                AF_INET,
                &address.sin_addr,
                ip,
                sizeof(ip)) != nullptr)
        {
            std::cout << "  " << ip << ':' << args.serverPort << '\n';
        }
    }

    std::cout << "SCTP server listening\n";

    struct socket* connection = usrsctp_accept(sock, nullptr, nullptr);
    if (connection == nullptr)
    {
        perror("usrsctp_accept");
        usrsctp_close(sock);
        return 1;
    }

    bool received = receiveMessage(connection);

    usrsctp_close(connection);
    usrsctp_close(sock);

    return received ? 0 : 1;
}

ServerArguments Server::parseArguments(int argc, char* argv[])
{
    ServerArguments args;

    static struct option longOptions[] =
    {
        {"server-port", required_argument, nullptr, 'p'},
        {"interfaces",  required_argument, nullptr, 'i'},
        {"help",        no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int option;

    while ((option = getopt_long(
        argc,
        argv,
        "p:i:h",
        longOptions,
        nullptr)) != -1)
    {
        switch (option)
        {
            case 'p':
                args.serverPort = std::stoi(optarg);
                break;

            case 'i':
                args.interfaces.push_back(optarg);
                break;

            case 'h':
                std::cout
                    << "Usage: server [options]\n"
                    << "  -p, --server-port <port>\n"
                    << "  -i, --interfaces <interface>\n"
                    << "  -h, --help\n";

                std::exit(EXIT_SUCCESS);

            default:
                std::exit(EXIT_FAILURE);
        }
    }

    return args;
}

std::vector<sockaddr_in> Server::getInterfaceAddresses(
    const std::vector<std::string>& interfaces,
    uint16_t port)
{
    std::vector<sockaddr_in> addresses;

    struct ifaddrs* ifaddr = nullptr;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return addresses;
    }

    for (struct ifaddrs* ifa = ifaddr;
         ifa != nullptr;
         ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
            continue;

        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;

        if (interfaces.empty())
        {
            if (ifa->ifa_flags & IFF_LOOPBACK)
                continue;
        }
        else
        {
            bool selected = false;

            for (const auto& interface : interfaces)
            {
                if (interface == ifa->ifa_name)
                {
                    selected = true;
                    break;
                }
            }

            if (!selected)
                continue;
        }

        sockaddr_in address =
            *reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);

        address.sin_port = htons(port);
        addresses.push_back(address);
    }

    freeifaddrs(ifaddr);

    return addresses;
}

bool Server::bindAddresses(
    struct socket* sock,
    std::vector<sockaddr_in>& addresses)
{
    if (addresses.empty())
    {
        std::cerr << "No interface addresses to bind\n";
        return false;
    }

    if (usrsctp_bindx(
            sock,
            reinterpret_cast<struct sockaddr*>(addresses.data()),
            static_cast<int>(addresses.size()),
            SCTP_BINDX_ADD_ADDR) < 0)
    {
        perror("usrsctp_bindx");
        return false;
    }

    return true;
}

bool Server::receiveMessage(struct socket* sock)
{
    char buffer[1024];
    sockaddr_in address{};
    sctp_rcvinfo receiveInfo{};
    socklen_t addressLength = sizeof(address);
    socklen_t infoLength = sizeof(receiveInfo);
    unsigned int infoType = 0;
    int flags = 0;

    while (true)
    {
        addressLength = sizeof(address);
        infoLength = sizeof(receiveInfo);
        flags = 0;

        ssize_t received = usrsctp_recvv(
            sock,
            buffer,
            sizeof(buffer) - 1,
            reinterpret_cast<struct sockaddr*>(&address),
            &addressLength,
            &receiveInfo,
            &infoLength,
            &infoType,
            &flags);

        if (received < 0)
        {
            perror("usrsctp_recvv");
            return false;
        }

        if ((flags & MSG_NOTIFICATION) != 0)
        {
            std::cerr << "Received SCTP notification\n";
            continue;
        }

        if (received == 0)
        {
            std::cerr << "SCTP connection closed before receiving data\n";
            return false;
        }

        buffer[received] = '\0';
        std::cout << "Received " << received << " bytes: "
                  << buffer << '\n';
        return true;
    }
}
