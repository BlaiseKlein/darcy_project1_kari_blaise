//
// Created by ds on 07/10/23.
//

#ifndef DISPLAY_H
#define DISPLAY_H
#include "app_types.h"
#include <stdbool.h>

void setup_window(struct board_state *board);
void shutdown_window(void);
void refresh_screen(struct board_state *board);
bool check_bound_collision(int x, int y, int row, int col);
void move_node(struct board_state *board, enum move_direction move, bool is_host);

#endif    // DISPLAY_H
