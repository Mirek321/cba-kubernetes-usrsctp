#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <usrsctp.h>

#define SCTP_PORT 9999

#define CLIENT_IP1 "192.168.100.102"
#define CLIENT_IP2 "192.168.200.102"

#define SERVER_IP "192.168.100.101"


static int
receive_cb(struct socket *sock,
           union sctp_sockstore addr,
           void *data,
           size_t datalen,
           struct sctp_rcvinfo rcv,
           int flags,
           void *ulp_info)
{
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

    struct sockaddr_in local_addresses[2];

    struct sockaddr_in server_address;

    const char *message =
        "Hello from usrsctp client!";


    printf("Starting usrsctp client...\n");


    /*
     * Initialize usrsctp.
     */
    usrsctp_init(0, NULL, NULL);


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


    /*
     * -------------------------------------------------
     * Local address #1
     * 192.168.100.103
     * -------------------------------------------------
     */

    memset(&local_addresses[0], 0,
           sizeof(struct sockaddr_in));

    local_addresses[0].sin_family = AF_INET;
    local_addresses[0].sin_port = htons(0);

    if (inet_pton(
            AF_INET,
            CLIENT_IP1,
            &local_addresses[0].sin_addr) != 1) {

        perror("inet_pton CLIENT_IP1");
        return 1;
    }


    /*
     * -------------------------------------------------
     * Local address #2
     * 192.168.200.103
     * -------------------------------------------------
     */

    memset(&local_addresses[1], 0,
           sizeof(struct sockaddr_in));

    local_addresses[1].sin_family = AF_INET;
    local_addresses[1].sin_port = htons(0);

    if (inet_pton(
            AF_INET,
            CLIENT_IP2,
            &local_addresses[1].sin_addr) != 1) {

        perror("inet_pton CLIENT_IP2");
        return 1;
    }


    /*
     * Add both local addresses.
     */
    if (usrsctp_bindx(
            sock,
            (struct sockaddr *)local_addresses,
            2,
            SCTP_BINDX_ADD_ADDR) < 0) {

        perror("usrsctp_bindx");
        return 1;
    }


    printf("Client bound to:\n");
    printf("  %s\n", CLIENT_IP1);
    printf("  %s\n", CLIENT_IP2);


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
