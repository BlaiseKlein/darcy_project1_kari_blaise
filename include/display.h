//
// Created by ds on 07/10/23.
//

#ifndef DISPLAY_H
    #define DISPLAY_HP
    #include "app_types.h"
    #include <stdbool.h>

void                    setupWindow(struct board_state *board, int *row, int *col);
void                    shutdownWindow(void);
void                    move_node(struct board_state *board, enum move_direction move, bool is_host);
void                    refresh_screen(struct board_state *board);
bool                    check_bound_collision(int x, int y, int row, int col);
static p101_fsm_state_t sync_nodes(const struct p101_env *env, struct p101_error *err, void *arg);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t sync_nodes(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context     *ctx   = (struct context *)arg;
    struct board_state *board = &(ctx->board);

    if((enum move_direction)ctx->input.direction != NONE)
    {
        move_node(board, (enum move_direction)ctx->input.direction, true);
    }
    if((enum move_direction)ctx->network.current_move != NONE)
    {
        move_node(board, (enum move_direction)ctx->network.current_move, false);
    }

    return REFRESH_SCREEN;
}

    #pragma GCC diagnostic pop

#endif    // DISPLAY_H
