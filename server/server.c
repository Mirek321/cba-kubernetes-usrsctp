#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <usrsctp.h>

#define SCTP_PORT 9999
#define LOCAL_UDP_ENCAP_PORT 9899
#define PEER_UDP_ENCAP_PORT 9898

#define SERVER_IP "192.168.100.65"

#define BUFFER_SIZE 1024


int
main(void)
{
    struct socket *sock;
    struct socket *connection;

    struct sockaddr_in local_address;
    struct sctp_udpencaps remote_encaps;
    char buffer[BUFFER_SIZE + 1];

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Starting usrsctp server on %s:%d\n", SERVER_IP, SCTP_PORT);

    usrsctp_init(LOCAL_UDP_ENCAP_PORT, NULL, NULL);


    /*
     * Create SCTP socket.
     */
    sock = usrsctp_socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_SCTP,
        NULL,
        NULL,
        0,
        NULL
    );

    if (sock == NULL) {
        perror("usrsctp_socket");
        return 1;
    }

    memset(&remote_encaps, 0, sizeof(remote_encaps));
    remote_encaps.sue_address.ss_family = AF_INET;
    remote_encaps.sue_port = htons(PEER_UDP_ENCAP_PORT);
    if (usrsctp_setsockopt(sock, IPPROTO_SCTP,
                           SCTP_REMOTE_UDP_ENCAPS_PORT,
                           &remote_encaps, sizeof(remote_encaps)) < 0) {
        perror("usrsctp_setsockopt SCTP_REMOTE_UDP_ENCAPS_PORT");
        usrsctp_close(sock);
        return 1;
    }


    memset(&local_address, 0, sizeof(local_address));
    local_address.sin_family = AF_INET;
    local_address.sin_port = htons(SCTP_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &local_address.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IP address: %s\n", SERVER_IP);
        usrsctp_close(sock);
        return 1;
    }

    if (usrsctp_bind(sock, (struct sockaddr *)&local_address,
                     sizeof(local_address)) < 0) {
        perror("usrsctp_bind");
        usrsctp_close(sock);
        return 1;
    }

    printf("Server bound to %s:%d\n", SERVER_IP, SCTP_PORT);


    /*
     * Listen for SCTP associations.
     */
    if (usrsctp_listen(sock, 5) < 0) {

        perror("usrsctp_listen");
        return 1;
    }


    printf("SCTP server listening on %s:%d\n", SERVER_IP, SCTP_PORT);


    /* Accept a connection, then receive synchronously without callbacks. */
    connection = usrsctp_accept(sock, NULL, NULL);
    if (connection == NULL) {
        perror("usrsctp_accept");
        usrsctp_close(sock);
        return 1;
    }

    for (;;) {
        struct sockaddr_storage addr;
        struct sockaddr_in *peer = (struct sockaddr_in *)&addr;
        struct sctp_rcvinfo rcv_info;
        char peer_ip[INET_ADDRSTRLEN];
        socklen_t from_len = sizeof(addr);
        socklen_t infolen = sizeof(rcv_info);
        unsigned int infotype = SCTP_RECVV_RCVINFO;
        int flags = 0;
        ssize_t n;

        n = usrsctp_recvv(connection, buffer, BUFFER_SIZE,
                          (struct sockaddr *)&addr, &from_len,
                          &rcv_info, &infolen, &infotype, &flags);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            perror("usrsctp_recvv");
            break;
        }
        if (n == 0) {
            printf("SCTP connection closed\n");
            break;
        }
        if (flags & MSG_NOTIFICATION) {
            printf("SCTP notification received (%zd bytes)\n", n);
            continue;
        }

        if (inet_ntop(AF_INET, &peer->sin_addr,
                      peer_ip, sizeof(peer_ip)) != NULL) {
            printf("Received %zd bytes from %s:%u on stream %u: %.*s\n",
                   n, peer_ip, ntohs(peer->sin_port),
                   infotype == SCTP_RECVV_RCVINFO ? rcv_info.rcv_sid : 0,
                   (int)n, buffer);
        } else {
            printf("Received %zd bytes on stream %u: %.*s\n", n,
                   infotype == SCTP_RECVV_RCVINFO ? rcv_info.rcv_sid : 0,
                   (int)n, buffer);
        }

        if (usrsctp_sendv(connection, buffer, (size_t)n, NULL, 0,
                          NULL, 0, SCTP_SENDV_NOINFO, 0) < 0) {
            perror("usrsctp_sendv");
            break;
        }
        printf("Echoed %zd bytes to client\n", n);
    }

    usrsctp_close(connection);
    usrsctp_close(sock);

    while (usrsctp_finish() != 0) {
        sleep(1);
    }

    return 0;
}
