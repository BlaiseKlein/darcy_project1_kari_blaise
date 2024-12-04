//
// Created by blaise-klein on 11/14/24.
//

#include "input.h"
#include "app_types.h"
#include <SDL2/SDL.h>
#include <curses.h>
#include <stdbool.h>
#include <stdio.h>

void setUpController(struct input_state *state)
{
    SDL_GameController *controller;
    if(SDL_Init(SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        state->controller = NULL;
    }
    if(SDL_NumJoysticks() == 0)
    {
        printf("No hame controllers connected\n");
        SDL_Quit();
        state->controller = NULL;
    }
    controller = SDL_GameControllerOpen(0);
    if(!controller)
    {
        printf("Could not open game controller: %s\n", SDL_GetError());
        SDL_Quit();
        state->controller = NULL;
    }
    state->controller = controller;
}

//
// enum move_direction getControllerInput(const SDL_Event *event)
// {
//     const int quit = 6;
//     if(event->type == SDL_CONTROLLERBUTTONDOWN)
//     {
//         switch(event->cbutton.button)
//         {
//             case UP:
//                 return UP;
//             case DOWN:
//                 return DOWN;
//             case LEFT:
//                 return LEFT;
//             case RIGHT:
//                 return RIGHT;
//             case NONE:
//                 return NONE;
//             case quit:
//                 return EXIT;
//             default:
//                 return NONE;
//         }
//     }
//     return NONE;
// }
enum move_direction getControllerInput(const SDL_Event *event)
{
    if(event->type == SDL_CONTROLLERBUTTONDOWN)
    {
        switch(event->cbutton.button)
        {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                return UP;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                return DOWN;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                return LEFT;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                return RIGHT;
            case SDL_CONTROLLER_BUTTON_BACK:    // Assign the back button to quit
                return EXIT;
            default:
                return NONE;
        }
    }
    return NONE;
}

enum move_direction wait_for_controller_input(void)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        if(event.type == SDL_CONTROLLERBUTTONDOWN)
        {
            return getControllerInput(&event);
        }
    }
    return NONE;
}

enum move_direction getKeyboardInput(void)
{
    // int fd;
    int ch;
    nodelay(stdscr, TRUE);

    ch = getch();
    // fd                     = open("/tmp/testing.fifo", O_RDONLY | O_WRONLY | O_CLOEXEC);
    //
    // if(fd == -1)
    // {
    //     return ERROR;
    // }
    // write(fd, &ch, 1);
    // close(fd);
    switch(ch)
    {
        case KEY_UP:
            return UP;
        case KEY_DOWN:
            return DOWN;
        case KEY_LEFT:
            return LEFT;
        case KEY_RIGHT:
            return RIGHT;
        case 'q':
            return EXIT;
        default:
            return NONE;
    }
}

enum move_direction getTimer(void)

{
    int ch;

    const int max           = 99;
    const int direction_max = 5;
    int       randomNumber  = rand() % max;    // Generate a number between 0 and 5
    nodelay(stdscr, TRUE);
    ch = getch();
    if(ch == 'q')
    {
        return EXIT;
    }
    if(randomNumber < direction_max)
    {
        switch(randomNumber)
        {
            case 0:
                return UP;
            case 1:
                return DOWN;
            case 2:
                return LEFT;
            case 3:
                return RIGHT;
            default:
                return NONE;    // Fallback
        }
    }
    else
    {
        return NONE;
    }
}
