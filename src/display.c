#include "display.h"
#include <curses.h>
#include <stdbool.h>

#define ROW_SCALE_A 2
#define ROW_SCALE_B 8
#define COL_SCALE_A 9
#define COL_SCALE_B 1
#define SCALE_DIVISOR 10

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

void setupWindow(int row, int col)
{
    struct player a;
    struct player b;
    initscr();
    curs_set(0);    //  hides the cursor
    getmaxyx(stdscr, row, col);
    clear();

    a = (struct player){.x = row - (row * ROW_SCALE_A / SCALE_DIVISOR), .y = col - (col * COL_SCALE_A / SCALE_DIVISOR), .ch = "X"};

    b = (struct player){.x = row - (row * ROW_SCALE_B / SCALE_DIVISOR), .y = col - (col * COL_SCALE_B / SCALE_DIVISOR), .ch = "O"};
    mvprintw(a.x, a.y, "%s", a.ch);
    mvprintw(b.x, b.y, "%s", b.ch);
    refresh();
    getch();
}

void shutdownWindow(void)
{
    endwin();
}

void move_node(struct player *player, int move, int row, int col)
{
    if(move == UP)
    {
        player->y++;
    }
    else if(move == DOWN)
    {
        player->y--;
    }
    else if(move == LEFT)
    {
        player->x--;
    }
    else if(move == RIGHT)
    {
        player->x++;
    }
    if(check_bound_collision(player, row, col))
    {
        mvprintw(0, 0, "Collision detected! Player reset.");
        player->x = row / 2;
        player->y = col / 2;
        ;
    }
}

void refresh_screen(struct player a, struct player b)
{
    clear();
    mvprintw(a.x, a.y, "%s", a.ch);
    mvprintw(b.x, b.y, "%s", b.ch);
    refresh();
}

bool check_bound_collision(struct player *p, int row, int col)
{
    if(p->x < 0 || p->x >= row || p->y < 0 || p->y >= col)
    {
        return true;
    }
    return false;
}
