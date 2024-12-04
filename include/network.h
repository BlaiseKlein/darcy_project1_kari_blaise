//
// Created by blaise-klein on 11/14/24.
//

#ifndef NETWORK_H
#define NETWORK_H

#include "app_types.h"
#include <SDL2/SDL.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <input.h>
#include <inttypes.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define READYTOSENDMSG 12
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
#pragma GCC diagnostic ignored "-Wunused-function"

static p101_fsm_state_t create_sending_stream(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context *ctx    = (struct context *)arg;
    ctx->network.send_addr = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));

    ctx->network.send_port = parse_in_port_t(ctx->arg.target_port, &ctx->err);
    if(ctx->err < 0)
    {
        return ERROR;
    }
    convert_address(ctx->arg.target_addr, ctx->network.send_addr, &ctx->network.send_addr_len, &ctx->err);
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
#pragma GCC diagnostic ignored "-Wunused-function"

static p101_fsm_state_t create_receiving_stream(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context *ctx       = (struct context *)arg;
    ctx->network.receive_addr = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));

    ctx->network.receive_port = parse_in_port_t(ctx->arg.sys_port, &ctx->err);
    if(ctx->err < 0)
    {
        return ERROR;
    }
    convert_address(ctx->arg.sys_addr, ctx->network.receive_addr, &ctx->network.receive_addr_len, &ctx->err);
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

    ctx->network.msg_size = sizeof(uint16_t);

    return SETUP_WINDOW;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

static p101_fsm_state_t send_packet(const struct p101_env *env, struct p101_error *err, void *arg)
{
    // int                    fd;
    struct context        *ctx = (struct context *)arg;
    ssize_t                bytes_sent;
    ssize_t                total_sent     = 0;
    uint16_t               send_direction = 0;
    const struct sockaddr *send_addr      = (struct sockaddr *)ctx->network.send_addr;
    const uint16_t         ready_message  = htons(READYTOSENDMSG);
    size_t                 msg_size       = sizeof(ready_message);
    size_t                 msg2_size      = sizeof(send_direction);
    char                  *sending        = (char *)malloc(msg_size);
    if(sending == NULL)
    {
        return ERROR;
    }
    memcpy(sending, &ready_message, msg_size);
    send_direction = htons((uint16_t)ctx->input.direction);

    // fd = open("/tmp/testing.fifo", O_RDONLY | O_WRONLY | O_CLOEXEC);
    //
    // if(fd == -1)
    // {
    //     free(sending);
    //     return ERROR;
    // }
    // write(fd, "SP", 2);
    // close(fd);

    while((size_t)total_sent < msg_size)
    {
        bytes_sent = sendto(ctx->network.send_fd, &sending[total_sent], msg_size - (size_t)total_sent, 0, send_addr, ctx->network.send_addr_len);

        if(bytes_sent == -1)
        {
            free(sending);
            return ERROR;
        }
        total_sent += bytes_sent;
    }

    total_sent = 0;
    free(sending);
    sending = (char *)malloc(msg2_size);
    if(sending == NULL)
    {
        return ERROR;
    }
    memcpy(sending, &send_direction, msg2_size);

    while((size_t)total_sent < ctx->network.msg_size)
    {
        bytes_sent = sendto(ctx->network.send_fd, &sending[total_sent], ctx->network.msg_size - (size_t)total_sent, 0, send_addr, ctx->network.send_addr_len);

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
    return SYNC_NODES;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

static p101_fsm_state_t handle_packet(const struct p101_env *env, struct p101_error *err, void *arg)
{
    // int             fd;
    ssize_t         total_received = 0;
    struct context *ctx            = (struct context *)arg;
    size_t          msg_size       = sizeof(uint16_t);
    char           *receiving      = NULL;
    receiving                      = (char *)malloc(msg_size);
    if(receiving == NULL)
    {
        return ERROR;
    }
    memset(receiving, ntohs((uint16_t)ctx->input.direction), msg_size);

    // fd = open("/tmp/testing.fifo", O_RDONLY | O_WRONLY | O_CLOEXEC);
    //
    // if(fd == -1)
    // {
    //     free(receiving);
    //     return ERROR;
    // }
    // write(fd, "HP", 2);
    // close(fd);

    while((size_t)total_received < msg_size)
    {
        ssize_t bytes_received = 0;
        bytes_received         = recvfrom(ctx->network.receive_fd, &receiving[total_received], ctx->network.msg_size - (size_t)total_received, 0, (struct sockaddr *)ctx->network.receive_addr, &ctx->network.receive_addr_len);

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
    if (ctx->network.current_move == READYTOSENDMSG)
    {
        ctx->net_rdy = 0;
    }

    return SYNC_NODES;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

static p101_fsm_state_t read_input(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context *ctx = (struct context *)arg;
    // int             fd  = open("/tmp/testing.fifo", O_RDONLY | O_WRONLY | O_CLOEXEC);
    // if(fd == -1)
    // {
    //     return ERROR;
    // }
    // write(fd, "\nRI", 3);
    // close(fd);

    ctx->input_rdy = 0;
    ctx->net_rdy   = 0;

    if(ctx->input.type == KEYBOARD)
    {
        return READ_KEYBOARD;
    }
    if(ctx->input.type == CONTROLLER)
    {
        return READ_CONTROLLER;
    }
    return ERROR;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

static p101_fsm_state_t read_network(const struct p101_env *env, struct p101_error *err, void *arg)
{
    // int             fd;
    const int       max_count = 10;
    struct context *ctx       = (struct context *)arg;
    uint16_t        received  = 0;
    // int             test      = 0;
    size_t        msg_size  = sizeof(received);
    struct pollfd fds       = {ctx->network.receive_fd, POLLIN, 0};
    char         *receiving = (char *)malloc(msg_size);

    if(receiving == NULL)
    {
        return ERROR;
    }

    // fd = open("/tmp/testing.fifo", O_RDONLY | O_WRONLY | O_CLOEXEC);
    //
    // if(fd == -1)
    // {
    //     free(receiving);
    //     return ERROR;
    // }
    // write(fd, "RN", 2);
    // test = poll(&fds, 1, max_count);
    // for(int i = 0; i <= test; i++)
    // {
    //     write(fd, "RUN", 3);
    // }
    // close(fd);

    if(poll(&fds, 1, max_count) > 0)
    {
        ssize_t total_received = 0;
        int     count          = 0;

        memcpy(receiving, &received, msg_size);

        while((size_t)total_received < ctx->network.msg_size && count != max_count)
        {
            ssize_t bytes_received = 0;

            bytes_received = recvfrom(ctx->network.receive_fd, &receiving[total_received], sizeof(received) - (size_t)total_received, 0, (struct sockaddr *)ctx->network.receive_addr, &ctx->network.receive_addr_len);

            if(bytes_received == -1)
            {
                free(receiving);
                return ERROR;
            }
            total_received += bytes_received;
            count++;
            if(count == max_count && total_received == 0)
            {
                break;
            }
        }
        memcpy(&received, receiving, msg_size);
    }
    free(receiving);
    // fd = open("/tmp/testing.fifo", O_RDONLY | O_WRONLY | O_CLOEXEC);
    //
    // if(fd == -1)
    // {
    //     return ERROR;
    // }
    // write(fd, "YE", 2);
    // close(fd);
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

    if(ctx->net_rdy == 1)
    {
        return HANDLE_PACKET;
    }

    if(ctx->input_rdy == 1)
    {
        return SEND_PACKET;
    }

    return ERROR;
}

#pragma GCC diagnostic pop

#endif    // NETWORK_H
