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
#include "app_types.h"

in_port_t parse_in_port_t(const char *port_str, int *err);
void      convert_address(const char *address, struct sockaddr_storage *addr, socklen_t *addr_len, int *err);
int       socket_create(int domain, int type, int protocol, int *err);
void      socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port, int *err);
void      socket_close(int sockfd);
void      get_address_to_server(struct sockaddr_storage *addr, in_port_t port, int *err);
int       create_sending_stream(const char *address, const char *port, struct sockaddr_storage *addr, socklen_t *addr_len);
int       create_receiving_stream(const char *address, const char *port, struct sockaddr_storage *addr, socklen_t *addr_len);
ssize_t   handle_packet(int client_sockfd, struct sockaddr_storage *client_addr, void *buffer, ssize_t buffer_len, socklen_t client_addr_len);
ssize_t   send_packet(int server_fd, struct sockaddr_storage *server_addr, const void *message, ssize_t msg_len, socklen_t server_addr_len);
ssize_t   handle_dir_packet(int client_sockfd, struct sockaddr_storage *client_addr, uint16_t *direction, socklen_t client_addr_len);
ssize_t   send_dir_packet(int server_fd, struct sockaddr_storage *server_addr, uint16_t *direction, socklen_t server_addr_len);
static p101_fsm_state_t create_sending_stream(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t create_receiving_stream(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t send_packet(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t handle_packet(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t await_input(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t read_network(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t read_input(const struct p101_env *env, struct p101_error *err, void *arg);

#endif    // NETWORK_H
