#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <usrsctp.h>

#define SCTP_PORT 9999

#define SERVER_IP1 "192.168.100.101"
#define SERVER_IP2 "192.168.200.101"

#define BUFFER_SIZE 1024


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

    printf("Starting usrsctp server...\n");

    /*
     * Initialize usrsctp.
     *
     * 0 = no UDP encapsulation,
     *     use native/direct SCTP.
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
     * 192.168.100.102
     * -------------------------------------------------
     */

    memset(&local_addresses[0], 0,
           sizeof(struct sockaddr_in));

    local_addresses[0].sin_family = AF_INET;
    local_addresses[0].sin_port = htons(SCTP_PORT);

    if (inet_pton(
            AF_INET,
            SERVER_IP1,
            &local_addresses[0].sin_addr) != 1) {

        perror("inet_pton SERVER_IP1");
        return 1;
    }


    /*
     * -------------------------------------------------
     * Local address #2
     * 192.168.200.102
     * -------------------------------------------------
     */

    memset(&local_addresses[1], 0,
           sizeof(struct sockaddr_in));

    local_addresses[1].sin_family = AF_INET;
    local_addresses[1].sin_port = htons(SCTP_PORT);

    if (inet_pton(
            AF_INET,
            SERVER_IP2,
            &local_addresses[1].sin_addr) != 1) {

        perror("inet_pton SERVER_IP2");
        return 1;
    }


    /*
     * Add both local addresses to the SCTP endpoint.
     */
    if (usrsctp_bindx(
            sock,
            (struct sockaddr *)local_addresses,
            2,
            SCTP_BINDX_ADD_ADDR) < 0) {

        perror("usrsctp_bindx");
        return 1;
    }


    printf("Server bound to:\n");
    printf("  %s:%d\n", SERVER_IP1, SCTP_PORT);
    printf("  %s:%d\n", SERVER_IP2, SCTP_PORT);


    /*
     * Listen for SCTP associations.
     */
    if (usrsctp_listen(sock, 5) < 0) {

        perror("usrsctp_listen");
        return 1;
    }


    printf("SCTP server listening...\n");


    /*
     * Keep the server running.
     *
     * receive_cb() receives incoming data.
     */
    while (1) {
        sleep(1);
    }


    usrsctp_close(sock);

    while (usrsctp_finish() != 0) {
        sleep(1);
    }

    return 0;
}
