#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <usrsctp.h>

#define SCTP_PORT 9999
#define LOCAL_UDP_ENCAP_PORT 9898
#define PEER_UDP_ENCAP_PORT 9899

#define CLIENT_IP "192.168.100.65"

#define SERVER_IP "192.168.100.65"


static int
receive_cb(struct socket *sock,
           union sctp_sockstore addr,
           void *data,
           size_t datalen,
           struct sctp_rcvinfo rcv,
           int flags,
           void *ulp_info)
{
    (void)sock;
    (void)addr;
    (void)rcv;
    (void)ulp_info;

    if (data == NULL) {
        return 1;
    }

    if (flags & MSG_NOTIFICATION) {

        printf("SCTP notification received\n");

    } else {

        printf("Received %zu bytes: %.*s\n",
               datalen,
               (int)datalen,
               (char *)data);
    }

    free(data);

    return 1;
}


int
main(void)
{
    struct socket *sock;

    struct sockaddr_in local_address;

    struct sockaddr_in server_address;
    struct sctp_udpencaps remote_encaps;

    const char *message =
        "Hello from usrsctp client!";


    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Starting usrsctp client from %s\n", CLIENT_IP);


    /*
     * Initialize usrsctp.
     */
    usrsctp_init(LOCAL_UDP_ENCAP_PORT, NULL, NULL);


    /*
     * Create SCTP socket.
     */
    sock = usrsctp_socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_SCTP,
        receive_cb,
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
    local_address.sin_port = htons(0);
    if (inet_pton(AF_INET, CLIENT_IP, &local_address.sin_addr) != 1) {
        fprintf(stderr, "Invalid client IP address: %s\n", CLIENT_IP);
        usrsctp_close(sock);
        return 1;
    }

    if (usrsctp_bind(sock, (struct sockaddr *)&local_address,
                     sizeof(local_address)) < 0) {
        perror("usrsctp_bind");
        usrsctp_close(sock);
        return 1;
    }

    printf("Client bound to %s\n", CLIENT_IP);


    /*
     * -------------------------------------------------
     * Server address
     * -------------------------------------------------
     */

    memset(&server_address, 0,
           sizeof(struct sockaddr_in));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SCTP_PORT);

    if (inet_pton(
            AF_INET,
            SERVER_IP,
            &server_address.sin_addr) != 1) {

        perror("inet_pton SERVER_IP");
        return 1;
    }


    printf("Connecting to %s:%d...\n",
           SERVER_IP,
           SCTP_PORT);


    /*
     * Establish SCTP association.
     */
    if (usrsctp_connect(
            sock,
            (struct sockaddr *)&server_address,
            sizeof(struct sockaddr_in)) < 0) {

        perror("usrsctp_connect");
        return 1;
    }


    printf("Connected!\n");


    /*
     * Send message.
     */
    if (usrsctp_sendv(
            sock,
            message,
            strlen(message),
            NULL,
            0,
            NULL,
            0,
            SCTP_SENDV_NOINFO,
            0) < 0) {

        perror("usrsctp_sendv");
    } else {

        printf("Message sent.\n");
    }


    /*
     * -------------------------------------------------
     * Show local SCTP addresses
     * -------------------------------------------------
     */

    struct sockaddr *addresses;

    int count;

    count = usrsctp_getladdrs(
        sock,
        0,
        &addresses
    );

    if (count < 0) {

        perror("usrsctp_getladdrs");

    } else {

        printf("\nLocal SCTP addresses: %d\n", count);

        /*
         * For this first version we know
         * that all addresses are IPv4.
         */

        struct sockaddr_in *addr =
            (struct sockaddr_in *)addresses;

        for (int i = 0; i < count; i++) {

            char ip[INET_ADDRSTRLEN];

            inet_ntop(
                AF_INET,
                &addr[i].sin_addr,
                ip,
                sizeof(ip)
            );

            printf("  %s\n", ip);
        }

        usrsctp_freeladdrs(addresses);
    }


    /*
     * -------------------------------------------------
     * Show peer SCTP addresses
     * -------------------------------------------------
     */

    count = usrsctp_getpaddrs(
        sock,
        0,
        &addresses
    );

    if (count < 0) {

        perror("usrsctp_getpaddrs");

    } else {

        printf("\nPeer SCTP addresses: %d\n", count);

        struct sockaddr_in *addr =
            (struct sockaddr_in *)addresses;

        for (int i = 0; i < count; i++) {

            char ip[INET_ADDRSTRLEN];

            inet_ntop(
                AF_INET,
                &addr[i].sin_addr,
                ip,
                sizeof(ip)
            );

            printf("  %s\n", ip);
        }

        usrsctp_freepaddrs(addresses);
    }


    sleep(5);


    usrsctp_close(sock);

    while (usrsctp_finish() != 0) {
        sleep(1);
    }

    return 0;
}
