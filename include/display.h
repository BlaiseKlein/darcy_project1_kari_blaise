//
// Created by ds on 07/10/23.
//

#ifndef DISPLAY_H
#define DISPLAY_H
#include "app_types.h"
#include <stdbool.h>

void setupWindow(struct board_state *board, int *row, int *col);
void shutdownWindow(void);
void move_node(struct board_state *board, int move, bool is_host);
void refresh_screen(struct board_state *board);
bool check_bound_collision(int x, int y, int row, int col);

#endif    // DISPLAY_H
