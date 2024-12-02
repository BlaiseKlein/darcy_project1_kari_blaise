//
// Created by ds on 07/10/23.
//

#ifndef DISPLAY_H
    #define DISPLAY_HP

    #include "app_types.h"
    #include <curses.h>
    #include <stdbool.h>

    #define ROW_SCALE_A 2
    #define ROW_SCALE_B 8
    #define COL_SCALE_A 9
    #define COL_SCALE_B 1
    #define SCALE_DIVISOR 10

// void setupWindow(struct board_state *board);
void shutdown_window(void);
void move_node(struct board_state *board, enum move_direction move, bool is_host);
// void                    refresh_screen(struct board_state *board);
bool                    check_bound_collision(int x, int y, int row, int col);
static p101_fsm_state_t sync_nodes(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t setup_window(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t refresh_screen(const struct p101_env *env, struct p101_error *err, void *arg);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t sync_nodes(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context     *ctx   = (struct context *)arg;
    struct board_state *board = &(ctx->board);

    if((enum move_direction)ctx->input.direction != NONE)
    {
        move_node(board, (enum move_direction)ctx->input.direction, TRUE);
    }
    if((enum move_direction)ctx->network.current_move != NONE)
    {
        move_node(board, (enum move_direction)ctx->network.current_move, FALSE);
    }

    return REFRESH_SCREEN;
}

    #pragma GCC diagnostic pop

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t setup_window(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context     *ctx   = (struct context *)arg;
    struct board_state *board = &(ctx->board);
    int                 row;
    int                 col;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);    //  hides the cursor
    getmaxyx(stdscr, row, col);

    board->length = row;
    board->width  = col;

    board->host_x = row - (row * ROW_SCALE_A / SCALE_DIVISOR);
    board->host_y = col - (col * COL_SCALE_A / SCALE_DIVISOR);
    board->net_x  = row - (row * ROW_SCALE_B / SCALE_DIVISOR);
    board->net_y  = col - (col * COL_SCALE_B / SCALE_DIVISOR);

    board->host_char = 'X';
    board->net_char  = 'O';

    mvaddch(board->host_x, board->host_y, (unsigned char)board->host_char);
    mvaddch(board->net_x, board->net_y, (unsigned char)board->net_char);

    refresh();

    return READ_INPUT;
}

    #pragma GCC diagnostic pop

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t refresh_screen(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context     *ctx   = (struct context *)arg;
    struct board_state *board = &(ctx->board);

    clear();
    mvaddch(board->host_x, board->host_y, (unsigned char)board->host_char);
    mvaddch(board->net_x, board->net_y, (unsigned char)board->net_char);
    refresh();

    return READ_INPUT;
}

    #pragma GCC diagnostic pop

#endif    // DISPLAY_H
