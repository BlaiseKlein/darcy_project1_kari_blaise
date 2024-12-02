//
// Created by blaise-klein on 11/14/24.
//

#ifndef NETWORK_H
#define NETWORK_H

#include "app_types.h"
#include <SDL2/SDL.h>
#include <arpa/inet.h>
#include <errno.h>
#include <input.h>
#include <inttypes.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define READYTOSENDMSG 2
#define CLOSE_CONNECTION_MESSAGE 7

in_port_t parse_in_port_t(const char *port_str, int *err);
void      convert_address(const char *address, struct sockaddr_storage *addr, socklen_t *addr_len, int *err);
int       socket_create(int domain, int type, int protocol, int *err);
void      socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port, int *err);
void      socket_close(int sockfd);
void      get_address_to_server(struct sockaddr_storage *addr, in_port_t port, int *err);
// int                     create_sending_stream(const char *address, const char *port, struct sockaddr_storage *addr, socklen_t *addr_len);
// int                     create_receiving_stream(const char *address, const char *port, struct sockaddr_storage *addr, socklen_t *addr_len);
// ssize_t                 handle_packet(int client_sockfd, struct sockaddr_storage *client_addr, void *buffer, ssize_t buffer_len, socklen_t client_addr_len);
// ssize_t                 send_packet(int server_fd, struct sockaddr_storage *server_addr, const void *message, ssize_t msg_len, socklen_t server_addr_len);
// ssize_t                 handle_dir_packet(int client_sockfd, struct sockaddr_storage *client_addr, uint16_t *direction, socklen_t client_addr_len);
// ssize_t                 send_dir_packet(int server_fd, struct sockaddr_storage *server_addr, uint16_t *direction, socklen_t server_addr_len);
static p101_fsm_state_t create_sending_stream(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t create_receiving_stream(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t send_packet(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t handle_packet(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t read_network(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t read_input(const struct p101_env *env, struct p101_error *err, void *arg);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t create_sending_stream(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context *ctx = (struct context *)arg;

    ctx->network.send_port = parse_in_port_t(ctx->arg.sys_port, &ctx->err);
    if(ctx->err < 0)
    {
        return ERROR;
    }
    convert_address(ctx->arg.sys_addr, ctx->network.send_addr, &ctx->network.send_addr_len, &ctx->err);
    if(ctx->err < 0)
    {
        return ERROR;
    }
    ctx->network.send_fd = socket_create(ctx->network.send_addr->ss_family, SOCK_DGRAM, 0, &ctx->err);
    if(ctx->err < 0)
    {
        return ERROR;
    }
    get_address_to_server(ctx->network.send_addr, ctx->network.send_port, &ctx->err);
    if(ctx->err < 0)
    {
        return ERROR;
    }

    return CREATE_RECEIVING_STREAM;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t create_receiving_stream(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context *ctx = (struct context *)arg;

    ctx->network.receive_port = parse_in_port_t(ctx->arg.target_port, &ctx->err);
    if(ctx->err < 0)
    {
        return ERROR;
    }
    convert_address(ctx->arg.target_addr, ctx->network.receive_addr, &ctx->network.receive_addr_len, &ctx->err);
    if(ctx->err < 0)
    {
        return ERROR;
    }
    ctx->network.receive_fd = socket_create(ctx->network.receive_addr->ss_family, SOCK_DGRAM, 0, &ctx->err);
    if(ctx->err < 0)
    {
        return ERROR;
    }
    socket_bind(ctx->network.receive_fd, ctx->network.receive_addr, ctx->network.receive_port, &ctx->err);
    if(ctx->err < 0)
    {
        return ERROR;
    }

    return SETUP_WINDOW;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t send_packet(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context        *ctx           = (struct context *)arg;
    ssize_t                bytes_sent    = 0;
    ssize_t                total_sent    = 0;
    const struct sockaddr *send_addr     = (struct sockaddr *)ctx->network.send_addr;
    const uint16_t         ready_message = htons(READYTOSENDMSG);
    char                  *sending       = (char *)malloc(sizeof(ready_message));
    memcpy(sending, &ready_message, sizeof(ready_message));
    ctx->input.direction = ntohs(ctx->input.direction);

    while((size_t)total_sent < sizeof(ready_message))
    {
        bytes_sent = sendto(ctx->network.send_fd, &sending[total_sent], sizeof(ready_message) - (size_t)total_sent, 0, send_addr, ctx->network.send_addr_len);

        if(bytes_sent == -1)
        {
            free(sending);
            return ERROR;
        }
        total_sent += bytes_sent;
    }

    total_sent = 0;
    free(sending);
    sending = (char *)malloc(sizeof(ctx->input.direction));
    memcpy(sending, &ctx->input.direction, sizeof(ctx->input.direction));

    while((size_t)total_sent < ctx->network.msg_size)
    {
        bytes_sent = sendto(ctx->network.send_fd, &sending[total_sent], (size_t)ctx->network.msg_size - (size_t)total_sent, 0, send_addr, ctx->network.send_addr_len);

        if(bytes_sent == -1)
        {
            free(sending);
            return ERROR;
        }
        total_sent += bytes_sent;
    }
    free(sending);

    if(ctx->net_rdy > 0)
    {
        return HANDLE_PACKET;
    }
    else
    {
        return SYNC_NODES;
    }
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t handle_packet(const struct p101_env *env, struct p101_error *err, void *arg)
{
    ssize_t         total_received = 0;
    struct context *ctx            = (struct context *)arg;
    char           *receiving      = (char *)malloc(sizeof(ctx->network.current_move));
    memcpy(receiving, &ctx->network.current_move, sizeof(ctx->network.current_move));

    while((size_t)total_received < ctx->network.msg_size)
    {
        ssize_t bytes_received = 0;
        bytes_received         = recvfrom(ctx->network.receive_fd, &receiving[total_received], ctx->network.msg_size, 0, (struct sockaddr *)&ctx->network.receive_addr, &ctx->network.receive_addr_len);

        if(bytes_received == -1)
        {
            free(receiving);
            return ERROR;
        }
        total_received += bytes_received;
    }
    memcpy(&ctx->network.current_move, receiving, ctx->network.msg_size);
    free(receiving);
    ctx->network.current_move = ntohs(ctx->network.current_move);

    return SYNC_NODES;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic pop    // END OF DEPRECATED

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t read_input(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context *ctx = (struct context *)arg;

    ctx->input_rdy = 0;
    ctx->net_rdy   = 0;

    if(ctx->input.type == KEYBOARD)
    {
        return READ_KEYBOARD;
    }
    else if(ctx->input.type == CONTROLLER)
    {
        return READ_CONTROLLER;
    }
    else
    {
        return ERROR;
    }
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t read_network(const struct p101_env *env, struct p101_error *err, void *arg)
{
    ssize_t         total_received = 0;
    struct context *ctx            = (struct context *)arg;
    uint16_t        received       = 0;
    char           *receiving      = (char *)malloc(sizeof(received));
    memcpy(receiving, &received, sizeof(received));

    while((size_t)total_received < ctx->network.msg_size)
    {
        ssize_t bytes_received = 0;
        bytes_received         = recvfrom(ctx->network.receive_fd, &receiving[total_received], sizeof(received), 0, (struct sockaddr *)&ctx->network.receive_addr, &ctx->network.receive_addr_len);

        if(bytes_received == -1)
        {
            free(receiving);
            return ERROR;
        }
        total_received += bytes_received;
    }
    memcpy(&received, receiving, sizeof(received));
    free(receiving);

    if(ntohs(received) == CLOSE_CONNECTION_MESSAGE)
    {
        return SAFE_CLOSE;
    }

    if(ntohs(received) == READYTOSENDMSG)
    {
        ctx->net_rdy = 1;
    }

    if(ctx->input_rdy == 0 && ctx->net_rdy == 0)
    {
        return READ_INPUT;
    }

    if(ctx->net_rdy == 0)
    {
        return HANDLE_PACKET;
    }

    if(ctx->input_rdy == 0)
    {
        return SEND_PACKET;
    }

    return ERROR;
}

#pragma GCC diagnostic pop

#endif    // NETWORK_H
