//
// Created by blaise-klein on 11/14/24.
//

#include "network.h"

#define BASE_TEN 10

in_port_t parse_in_port_t(const char *str)
{
    char     *endptr;
    uintmax_t parsed_value;

    errno        = 0;
    parsed_value = strtoumax(str, &endptr, BASE_TEN);

    if(errno != 0)
    {
        perror("Error parsing in_port_t");
        exit(EXIT_FAILURE);
    }

    // Check if there are any non-numeric characters in the input string
    if(*endptr != '\0')
    {
        // usage(binary_name, EXIT_FAILURE, "Invalid characters in input.");
    }

    // Check if the parsed value is within the valid range for in_port_t
    if(parsed_value > UINT16_MAX)
    {
        // usage(binary_name, EXIT_FAILURE, "in_port_t value out of range.");
    }

    return (in_port_t)parsed_value;
}

void convert_address(const char *address, struct sockaddr_storage *addr, socklen_t *addr_len)
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
        fprintf(stderr, "%s is not an IPv4 or an IPv6 address\n", address);
        exit(EXIT_FAILURE);
    }
}

int socket_create(int domain, int type, int protocol)
{
    int sockfd;

    sockfd = socket(domain, type, protocol);

    if(sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port)
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
        fprintf(stderr, "Internal error: addr->ss_family must be AF_INET or AF_INET6, was: %d\n", addr->ss_family);
        exit(EXIT_FAILURE);
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        perror("inet_ntop");
        exit(EXIT_FAILURE);
    }

    printf("Binding to: %s:%u\n", addr_str, port);

    if(bind(sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        perror("Binding failed");
        fprintf(stderr, "Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("Bound to socket: %s:%u\n", addr_str, port);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// cppcheck-suppress constParameterPointer
ssize_t handle_packet(int client_sockfd, struct sockaddr_storage *client_addr, char *buffer, ssize_t buffer_len, socklen_t client_addr_len)
{
    ssize_t       total_received = 0;
    const ssize_t msg_size       = 7;

    while(total_received < msg_size)
    {
        ssize_t bytes_received = 0;
        sleep(1);
        bytes_received = recvfrom(client_sockfd, &buffer[total_received], (size_t)buffer_len, 0, (struct sockaddr *)&client_addr, &client_addr_len);

        if(bytes_received == -1)
        {
            perror("recvfrom");
            continue;
        }
        total_received += bytes_received;
    }
    buffer[(size_t)total_received] = '\0';
    printf("Sent %zu bytes: \"%s\"\n", (size_t)total_received, buffer);

    return total_received;
}

#pragma GCC diagnostic pop

void socket_close(int sockfd)
{
    if(close(sockfd) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

void get_address_to_server(struct sockaddr_storage *addr, in_port_t port)
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
}

int create_sending_stream(const char *address, const char *port, struct sockaddr_storage *addr, socklen_t *addr_len)
{
    in_port_t port_t;
    int       sockfd;

    port_t = parse_in_port_t(port);
    convert_address(address, addr, addr_len);
    sockfd = socket_create(addr->ss_family, SOCK_DGRAM, 0);
    get_address_to_server(addr, port_t);

    return sockfd;
}

int create_receiving_stream(const char *address, const char *port, struct sockaddr_storage *addr, socklen_t *addr_len)
{
    in_port_t port_t;
    int       sockfd;

    port_t = parse_in_port_t(port);
    convert_address(address, addr, addr_len);
    sockfd = socket_create(addr->ss_family, SOCK_DGRAM, 0);
    socket_bind(sockfd, addr, port_t);

    return sockfd;
}

ssize_t send_packet(int server_fd, struct sockaddr_storage *server_addr, const char *message, const ssize_t msg_len, socklen_t server_addr_len)
{
    ssize_t                bytes_sent = 0;
    ssize_t                total_sent = 0;
    const struct sockaddr *send_addr  = (struct sockaddr *)server_addr;

    while(total_sent < msg_len)
    {
        bytes_sent = sendto(server_fd, &message[total_sent], strlen(message), 0, send_addr, server_addr_len);

        if(bytes_sent == -1)
        {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
        total_sent += bytes_sent;
    }

    printf("Sent %zu bytes: \"%s\"\n", (size_t)bytes_sent, message);

    return bytes_sent;
}
