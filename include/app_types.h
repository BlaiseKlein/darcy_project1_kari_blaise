//
// Created by blaise-klein on 11/28/24.
//

#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <p101_fsm/fsm.h>
#include <p101_posix/p101_unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include <SDL2/SDL.h>

enum application_states
{
    INIT = P101_FSM_USER_START,    // 2
    SETUP_CONTROLLER,
    CREATE_SENDING_STREAM,
    CREATE_RECEIVING_STREAM,
    SETUP_WINDOW,
    READ_INPUT,
    READ_CONTROLLER,
    READ_KEYBOARD,
    READ_NETWORK,
    SEND_PACKET,
    HANDLE_PACKET,
    SYNC_NODES,
    REFRESH_SCREEN,
    SAFE_CLOSE,
    ERROR,
};

enum controller_type{
    CONTROLLER,
    KEYBOARD,
    TIMER //May be unused
};
enum move_direction
{
    DOWN,
    RIGHT,
    LEFT,
    UP,
    NONE
};

struct arguments
{
    char *sys_addr;
    ssize_t sys_addr_len;
    char *sys_port;
    char *target_addr;
    ssize_t target_addr_len;
    char *target_port;
    // char    controller_type;
    // Controller type, joystick, keyboard, etc...
};

struct input_state
{
    //    enum controller_type controller;
    enum controller_type type;
    SDL_GameController *controller;
    enum move_direction direction;
};

struct network_state
{
    const size_t             msg_size;
    int                      send_fd;
    struct sockaddr_storage *send_addr;
    socklen_t                send_addr_len;
    in_port_t                send_port;
    int                      receive_fd;
    struct sockaddr_storage *receive_addr;
    socklen_t                receive_addr_len;
    in_port_t                receive_port;
    uint16_t                      current_move;
};

struct board_state
{
    int length;
    int  width;
    int  host_x;
    int  host_y;
    char host_char;
    int  net_x;
    int  net_y;
    char net_char;
};

struct context
{
    struct arguments     arg;
    struct input_state   input;
    struct network_state network;
    struct board_state   board;
    int err;
    int input_rdy;
    int net_rdy;
};

#endif //APP_TYPES_H
