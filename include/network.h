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

in_port_t parse_in_port_t(const char *port_str);
void      convert_address(const char *address, struct sockaddr_storage *addr, socklen_t *addr_len);
int       socket_create(int domain, int type, int protocol);
void      socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port);
ssize_t   handle_packet(int client_sockfd, struct sockaddr_storage *client_addr, char *buffer, ssize_t buffer_len, socklen_t client_addr_len);
void      socket_close(int sockfd);
void      get_address_to_server(struct sockaddr_storage *addr, in_port_t port);
int       create_sending_stream(const char *address, const char *port, struct sockaddr_storage *addr, socklen_t *addr_len);
int       create_receiving_stream(const char *address, const char *port, struct sockaddr_storage *addr, socklen_t *addr_len);
ssize_t   send_packet(int server_fd, struct sockaddr_storage *server_addr, const char *message, ssize_t msg_len, socklen_t server_addr_len);

#endif    // NETWORK_H
