//
// Created by blaise-klein on 11/14/24.
//

#ifndef NETWORK_H
#define NETWORK_H

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static in_port_t      parse_in_port_t(const char *binary_name, const char *port_str);
static void           convert_address(const char *address, struct sockaddr_storage *addr, socklen_t *addr_len);
static int            socket_create(int domain, int type, int protocol);
static void           socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port);
static int            handle_packet(int client_sockfd, struct sockaddr_storage *client_addr, const char *buffer, size_t bytes);
static void           socket_close(int sockfd);
static void           get_address_to_server(&addr, port);
static int            create_sending_stream(const char*address, const char* port);
static int            create_receiving_stream(const char*address, const char* port);
static int            send_packet(int server_fd, struct sockaddr_storage *server_addr, const char *msg, size_t bytes);

#endif //NETWORK_H
