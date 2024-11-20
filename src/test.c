//
// Created by blaise-klein on 11/19/24.
//
#include "network.h"

#define IP "127.0.0.1";
#define PORT "9999"
#define BUFFER_SIZE 1024

int main(void)
{
    const char             *ip;
    const char             *port;
    char                   *buffer;
    char                   *msg;
    struct sockaddr_storage addr_out;
    struct sockaddr_storage addr_in;
    socklen_t               addr_in_len;
    socklen_t               addr_out_len;
    int                     sock_out;
    int                     sock_in;
    const ssize_t           msg_size = 12;
    ip                               = IP;
    port                             = PORT;
    sock_out                         = create_sending_stream(ip, port, &addr_out, &addr_out_len);
    sock_in                          = create_receiving_stream(ip, port, &addr_in, &addr_in_len);

    buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    if(buffer == NULL)
    {
        exit(EXIT_FAILURE);
    }
    msg = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    if(msg == NULL)
    {
        exit(EXIT_FAILURE);
    }
    memset(buffer, 0, sizeof(char) * BUFFER_SIZE);
    memcpy(msg, "Hello World", (size_t)msg_size);
    send_packet(sock_out, &addr_out, msg, msg_size, addr_out_len);
    handle_packet(sock_in, &addr_out, buffer, BUFFER_SIZE, addr_out_len);
    free(buffer);
    free(msg);
    socket_close(sock_out);
    socket_close(sock_in);
}