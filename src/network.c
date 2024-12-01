//
// Created by blaise-klein on 11/14/24.
//

#include "network.h"
#include <SDL2/SDL.h>
#include <ncurses.h>
#include <input.h>

#define BASE_TEN 10
#define READYTOSENDMSG 2

in_port_t parse_in_port_t(const char *str, int *err)
{
    char     *endptr;
    uintmax_t parsed_value;

    errno        = 0;
    parsed_value = strtoumax(str, &endptr, BASE_TEN);

    if(errno != 0)
    {
        *err = errno;
    }

    // Check if there are any non-numeric characters in the input string
    if(*endptr != '\0')
    {
        // usage(binary_name, EXIT_FAILURE, "Invalid characters in input.");
        *err = -1;
    }

    // Check if the parsed value is within the valid range for in_port_t
    if(parsed_value > UINT16_MAX)
    {
        // usage(binary_name, EXIT_FAILURE, "in_port_t value out of range.");
        *err = -2;
    }

    return (in_port_t)parsed_value;
}

void convert_address(const char *address, struct sockaddr_storage *addr, socklen_t *addr_len, int *err)
{
    memset(addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        addr->ss_family = AF_INET;
        *addr_len       = sizeof(struct sockaddr_in);
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        addr->ss_family = AF_INET6;
        *addr_len       = sizeof(struct sockaddr_in6);
    }
    else
    {
        *err = -1;
    }
}

int socket_create(int domain, int type, int protocol, int *err)
{
    int sockfd;

    sockfd = socket(domain, type, protocol);

    if(sockfd == -1)
    {
        *err = -1;
    }

    return sockfd;
}

void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port, int *err)
{
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
    }
    else
    {
        *err = -1;
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        *err = -2;
    }

    printf("Binding to: %s:%u\n", addr_str, port);

    if(bind(sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        *err = -3;
    }

    printf("Bound to socket: %s:%u\n", addr_str, port);
}

void socket_close(int sockfd)
{
    if(close(sockfd) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

void get_address_to_server(struct sockaddr_storage *addr, in_port_t port, int *err)
{
    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr             = (struct sockaddr_in *)addr;
        ipv4_addr->sin_family = AF_INET;
        ipv4_addr->sin_port   = htons(port);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr              = (struct sockaddr_in6 *)addr;
        ipv6_addr->sin6_family = AF_INET6;
        ipv6_addr->sin6_port   = htons(port);
    }
    else
    {
        *err = -1;
    }
}

int create_sending_stream(const char *address, const char *port, struct sockaddr_storage *addr, socklen_t *addr_len)
{
    in_port_t port_t;
    int       sockfd;
    int err = 0;

    port_t = parse_in_port_t(port, &err);

    convert_address(address, addr, addr_len, &err);
    sockfd = socket_create(addr->ss_family, SOCK_DGRAM, 0, &err);
    get_address_to_server(addr, port_t, &err);

    return sockfd;
}

int create_receiving_stream(const char *address, const char *port, struct sockaddr_storage *addr, socklen_t *addr_len)
{
    in_port_t port_t;
    int       sockfd;
    int err = 0;

    port_t = parse_in_port_t(port, &err);
    convert_address(address, addr, addr_len, &err);
    sockfd = socket_create(addr->ss_family, SOCK_DGRAM, 0, &err);
    socket_bind(sockfd, addr, port_t, &err);

    return sockfd;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// cppcheck-suppress constParameterPointer
ssize_t handle_packet(int client_sockfd, struct sockaddr_storage *client_addr, void *buffer, ssize_t buffer_len, socklen_t client_addr_len)
{
    ssize_t       total_received = 0;
    const ssize_t msg_size       = 11;

    while(total_received < msg_size)
    {
        ssize_t bytes_received = 0;
        bytes_received         = recvfrom(client_sockfd, &((char *)buffer)[total_received], (size_t)buffer_len, 0, (struct sockaddr *)&client_addr, &client_addr_len);

        if(bytes_received == -1)
        {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }
        total_received += bytes_received;
    }

    return total_received;
}

#pragma GCC diagnostic pop

ssize_t send_packet(int server_fd, struct sockaddr_storage *server_addr, const void *message, const ssize_t msg_len, socklen_t server_addr_len)
{
    ssize_t                bytes_sent = 0;
    ssize_t                total_sent = 0;
    const struct sockaddr *send_addr  = (struct sockaddr *)server_addr;

    while(total_sent < msg_len)
    {
        bytes_sent = sendto(server_fd, &((const char *)message)[total_sent], (size_t)msg_len, 0, send_addr, server_addr_len);

        if(bytes_sent == -1)
        {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
        total_sent += bytes_sent;
    }

    return bytes_sent;
}

ssize_t handle_dir_packet(int client_sockfd, struct sockaddr_storage *client_addr, uint16_t *direction, socklen_t client_addr_len)
{
    ssize_t received = handle_packet(client_sockfd, client_addr, direction, sizeof(int), client_addr_len);

    *direction = ntohs(*direction);

    return received;
}

ssize_t send_dir_packet(int server_fd, struct sockaddr_storage *server_addr, uint16_t *direction, socklen_t server_addr_len)
{
    *direction = ntohs(*direction);

    return send_packet(server_fd, server_addr, direction, sizeof(int), server_addr_len);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t create_sending_stream(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context* ctx = arg;

    ctx->network.send_port = parse_in_port_t(ctx->arg.sys_port, &ctx->err);
    if (ctx->err < 0)
    {
        return ERROR;
    }
    convert_address(ctx->arg.sys_addr, ctx->network.send_addr, ctx->network.send_addr_len, &ctx->err);
    if (ctx->err < 0)
    {
        return ERROR;
    }
    ctx->network.send_fd = socket_create(ctx->network.send_addr->ss_family, SOCK_DGRAM, 0, &ctx->err);
    if (ctx->err < 0)
    {
        return ERROR;
    }
    get_address_to_server(ctx->network.send_addr, ctx->network.send_port, &ctx->err);
    if (ctx->err < 0)
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
    struct context* ctx = arg;

    ctx->network.receive_port = parse_in_port_t(ctx->arg.target_port, &ctx->err);
    if (ctx->err < 0)
    {
        return ERROR;
    }
    convert_address(ctx->arg.target_addr, ctx->network.receive_addr, ctx->network.receive_addr_len, &ctx->err);
    if (ctx->err < 0)
    {
        return ERROR;
    }
    ctx->network.receive_fd = socket_create(ctx->network.receive_addr->ss_family, SOCK_DGRAM, 0, &ctx->err);
    if (ctx->err < 0)
    {
        return ERROR;
    }
    socket_bind(ctx->network.receive_fd, ctx->network.receive_addr, ctx->network.receive_port, &ctx->err);
    if (ctx->err < 0)
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
    struct context* ctx = arg;
    ssize_t                bytes_sent = 0;
    ssize_t                total_sent = 0;
    const struct sockaddr *send_addr  = (struct sockaddr *)ctx->network.send_addr;
    const int ready_message = htons(READYTOSENDMSG);

    while(total_sent < sizeof(ready_message))
    {
        bytes_sent = sendto(ctx->network.send_fd, &((const char *)ready_message)[total_sent], sizeof(ready_message) - total_sent, 0, send_addr, ctx->network.send_addr_len);

        if(bytes_sent == -1)
        {
            return ERROR;
        }
        total_sent += bytes_sent;
    }

    total_sent = 0;

    while(total_sent < ctx->network.msg_size)
    {
        bytes_sent = sendto(ctx->network.send_fd, &((const char *)ctx->input.direction)[total_sent], (size_t)ctx->network.msg_size - total_sent, 0, send_addr, ctx->network.send_addr_len);

        if(bytes_sent == -1)
        {
            return ERROR;
        }
        total_sent += bytes_sent;
    }

    ctx->input.direction = ntohs(ctx->input.direction);

    return HANDLE_PACKET;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t handle_packet(const struct p101_env *env, struct p101_error *err, void *arg)
{
    ssize_t       total_received = 0;
    struct context* ctx = arg;

    while(total_received < ctx->network.msg_size)
    {
        ssize_t bytes_received = 0;
        bytes_received         = recvfrom(ctx->network.receive_fd, &((char *)ctx->network.current_move)[total_received],
            ctx->network.msg_size, 0, (struct sockaddr *)&ctx->network.receive_addr, &ctx->network.receive_addr_len);

        if(bytes_received == -1)
        {
            return ERROR;
        }
        total_received += bytes_received;
    }

    ctx->network.current_move = ntohs(ctx->network.current_move);

    return MOVE_NODE;
}

#pragma GCC diagnostic pop

//DEPRECATED DONT USE
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

//DEPRECATED DONT USE
static p101_fsm_state_t await_input(const struct p101_env *env, struct p101_error *err, void *arg)
{
    //DEPRECATED DONT USE
    SDL_Event event;
    int activity;
    fd_set readfds;
    struct context* ctx = arg;
    const int max_fds = 2;

    while(1)
    {
        enum move_direction key_move;

        //Reading keyboard

        key_move = getKeyboardInput();
        if (key_move != NONE)
        {
            return READ_KEYBOARD;
        }

        //Reading controller
        if(SDL_PollEvent(&event))
        {
            return READ_CONTROLLER;
        }

        //Reading network

        // Clear the socket set
        #ifndef __clang_analyzer__
        FD_ZERO(&readfds);
        #endif

        #if defined(__FreeBSD__) && defined(__GNUC__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wsign-conversion"
        #endif
        // Add the server socket to the set
        FD_SET(ctx->network.receive_fd, &readfds);
        #if defined(__FreeBSD__) && defined(__GNUC__)
        #pragma GCC diagnostic pop
        #endif

        activity = select(max_fds, &readfds, NULL, NULL, NULL);
        if (activity > 0)
        {
            return READ_NETWORK;
        }
    }
    return ERROR;
}

#pragma GCC diagnostic pop //END OF DEPRECATED


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t read_input(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context* ctx = arg;

    ctx->input_rdy = 0;
    ctx->net_rdy = 0;

    if (ctx->input.type == KEYBOARD)
    {
        return READ_KEYBOARD;
    } else if (ctx->input.type == CONTROLLER)
    {
        return READ_CONTROLLER;
    } else
    {
        return ERROR;
    }
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t read_network(const struct p101_env *env, struct p101_error *err, void *arg)
{
    ssize_t       total_received = 0;
    struct context* ctx = arg;
    uint16_t received = 0;

    ssize_t bytes_received = 0;
    bytes_received         = recvfrom(ctx->network.receive_fd, &received,
        sizeof(received), 0, (struct sockaddr *)&ctx->network.receive_addr, &ctx->network.receive_addr_len);

    if(bytes_received == -1)
    {
        return ERROR;
    }

    if (ntohs(received) == READYTOSENDMSG)
    {
        ctx->net_rdy = 1;
    }

    if (ctx->input_rdy == 0 && ctx->net_rdy == 0)
    {
        return READ_INPUT;
    }

    if (ctx->net_rdy == 0)
    {
        return HANDLE_PACKET;
    }

    if (ctx->input_rdy == 0)
    {
        return SEND_PACKET;
    }
}

#pragma GCC diagnostic pop

