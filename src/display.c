#include "display.h"
#include <curses.h>
#include <stdbool.h>

// void setup_window(struct board_state *board)
// {
//     int row;
//     int col;
//
//     initscr();
//     cbreak();
//     noecho();
//     keypad(stdscr, TRUE);
//     curs_set(0);    //  hides the cursor
//     getmaxyx(stdscr, row, col);
//
//     board->length = row;
//     board->width  = col;
//
//     board->host_x = row - (row * ROW_SCALE_A / SCALE_DIVISOR);
//     board->host_y = col - (col * COL_SCALE_A / SCALE_DIVISOR);
//     board->net_x  = row - (row * ROW_SCALE_B / SCALE_DIVISOR);
//     board->net_y  = col - (col * COL_SCALE_B / SCALE_DIVISOR);
//
//     board->host_char = 'X';
//     board->net_char  = 'O';
//
//     mvaddch(board->host_x, board->host_y, (unsigned char)board->host_char);
//     mvaddch(board->net_x, board->net_y, (unsigned char)board->net_char);
//
//     refresh();
//     // getch();
// }

void shutdown_window(void)
{
    endwin();
}

void move_node(struct board_state *board, enum move_direction move, bool is_host)
{
    int *x = is_host ? &board->host_x : &board->net_x;
    int *y = is_host ? &board->host_y : &board->net_y;

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
    if(!is_host)
    {
        switch(move)
        {
            case UP:
                *x += 2;
                break;
            case DOWN:
                *x -= 2;
                break;
            case LEFT:
                *y += 2;
                break;
            case RIGHT:
                *y -= 2;
                break;
            case NONE:
                break;
            default:
                mvprintw(0, 0, "Invalid move detected");
                break;
        }
    }
    else
    {
        switch(move)
        {
            case UP:
                *x -= 2;
                break;
            case DOWN:
                *x += 2;
                break;
            case LEFT:
                *y -= 2;
                break;
            case RIGHT:
                *y += 2;
                break;
            case NONE:
                break;
            default:
                mvprintw(0, 0, "Invalid move detected");
                break;
        }
    }

#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

    if(check_bound_collision(*x, *y, board->length, board->width))
    {
        mvprintw(0, 0, "Collision detected! Player reset.");
        *x = board->length / 2;
        *y = board->width / 2;
    }
    // refresh_screen(board);
}

// void refresh_screen(struct board_state *board)
// {
//     clear();
//     mvaddch(board->host_x, board->host_y, (unsigned char)board->host_char);
//     mvaddch(board->net_x, board->net_y, (unsigned char)board->net_char);
//     refresh();
// }

bool check_bound_collision(int x, int y, int row, int col)
{
    return (x <= 0 || x >= row || y <= 0 || y >= col);
}
