#include "client.hpp"

Client::Client()
{



}

Client::~Client()
{
}

void Client::debugPrintf(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
int Client::run(int argc, char* argv[])
{
    Arguments args = parseArguments(argc, argv);

    std::cout << "Server port: " << args.serverPort << '\n';

    // Servers
    for (const std::string& server : args.servers)
    {
        std::cout << "Server: " << server << '\n';
    }

    // Interfaces
    for (const std::string& interface : args.interfaces)
    {
        std::cout << "Interface: " << interface << '\n';
    }

    std::vector<sockaddr_in> addresses =
    getInterfaceAddresses(args.interfaces);


    usrsctp_init(0, nullptr, Client::debugPrintf);

    struct socket* sock = usrsctp_socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_SCTP,
        Client::receiveCallback,
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
    	return 1;
    }
	if (!connectToServer(sock, args.servers, args.serverPort))
{
    return 1;
}
if (!sendMessage(sock, "Hello from client"))
{
    return 1;
}
std::cout << "Waiting for response...\n";

sleep(5);

usrsctp_close(sock);
usrsctp_finish();
    return 0;
}
Arguments Client::parseArguments(int argc, char* argv[])
{
    Arguments args;

    static struct option longOptions[] =
    {
        {"server",      required_argument, nullptr, 's'},
        {"server-port", required_argument, nullptr, 'p'},
        {"interfaces",  required_argument, nullptr, 'i'},
        {"help",        no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int option;

    while ((option = getopt_long(
        argc,
        argv,
        "s:p:i:h",
        longOptions,
        nullptr)) != -1)
    {
        switch (option)
        {
            case 's':
    		args.servers.push_back(optarg);
    		break;

	    case 'p':
    		args.serverPort = std::stoi(optarg);
    		break;

	    case 'i':
    		args.interfaces.push_back(optarg);
    		break;

            case 'h':
                std::cout
                    << "Usage: client [options]\n"
                    << "  -s, --server <address>\n"
                    << "  -p, --server-port <port>\n"
                    << "  -l, --local-port <port>\n"
                    << "  -i, --interfaces <net1,net2>\n"
                    << "  -h, --help\n";

                std::exit(EXIT_SUCCESS);

            default:
                std::exit(EXIT_FAILURE);
        }
    }

    return args;
}
std::vector<sockaddr_in> Client::getInterfaceAddresses(
    const std::vector<std::string>& interfaces)
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

        // No -i specified -> use all non-loopback IPv4 interfaces
        if (interfaces.empty())
        {
            if (ifa->ifa_flags & IFF_LOOPBACK)
                continue;
        }
        // -i specified -> use only requested interfaces
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

        sockaddr_in addr =
            *reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);

        addr.sin_port = htons(0);

        addresses.push_back(addr);
    }

    freeifaddrs(ifaddr);

    return addresses;
}
bool Client::bindAddresses(
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
bool Client::connectToServer(
    struct socket* sock,
    const std::vector<std::string>& servers,
    uint16_t port)
{
    if (servers.empty())
    {
        std::cerr << "No server addresses specified\n";
        return false;
    }

    std::vector<sockaddr_in> serverAddresses;

    for (const auto& server : servers)
    {
        sockaddr_in address{};

        address.sin_family = AF_INET;
        address.sin_port = htons(port);

        if (inet_pton(
                AF_INET,
                server.c_str(),
                &address.sin_addr) != 1)
        {
            std::cerr << "Invalid server address: "
                      << server << '\n';

            return false;
        }

        serverAddresses.push_back(address);
    }

    for (const auto& address : serverAddresses)
{
    char ip[INET_ADDRSTRLEN];

    inet_ntop(
        AF_INET,
        &address.sin_addr,
        ip,
        sizeof(ip));

    std::cout << "Remote SCTP address: "
              << ip << ':' << port << '\n';
}
sctp_assoc_t assocId;
int result = usrsctp_connect(
    sock,
    reinterpret_cast<struct sockaddr*>(&serverAddresses[0]),
    sizeof(sockaddr_in)
);

std::cout << "usrsctp_connectx returned: "
          << result << '\n';

if (result < 0)
{
    perror("usrsctp_connectx");
    return false;
}

std::cout << "SCTP connect initiated\n";
return true;
}
bool Client::sendMessage(
    struct socket* sock,
    const std::string& message)
{
    int sent = usrsctp_sendv(
        sock,
        message.c_str(),
        message.size(),
        nullptr,
        0,
        nullptr,
        0,
        SCTP_SENDV_NOINFO,
        0
    );

    if (sent < 0)
    {
        perror("usrsctp_sendv");
        return false;
    }

    std::cout << "Sent " << sent << " bytes\n";

    return true;
}
int Client::receiveCallback(
    struct socket* sock,
    union sctp_sockstore addr,
    void* data,
    size_t datalen,
    struct sctp_rcvinfo rcv,
    int flags,
    void* ulp_info)
{
    if (data == nullptr)
        return 1;

    if ((flags & MSG_NOTIFICATION) != 0)
    {
        std::cout << "SCTP notification received\n";
    }
    else
    {
        std::cout << "Received "
                  << datalen
                  << " bytes: "
                  << std::string(
                         static_cast<char*>(data),
                         datalen)
                  << '\n';
    }

    free(data);
    return 1;
}
