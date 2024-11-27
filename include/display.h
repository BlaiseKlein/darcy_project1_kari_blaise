//
// Created by ds on 07/10/23.
//

#ifndef DISPLAY_H
#define DISPLAY_H
#include <stdbool.h>

struct player
{
    int         x;
    int         y;
    const char *ch;
};

void setupWindow(int row, int col);
void shutdownWindow(void);
void move_node(void);
void refresh_screen(struct player a, struct player b);
bool check_bound_collision(struct player *p, int row, int col)

#endif    // DISPLAY_H
