//
// Created by blaise-klein on 11/14/24.
//

#include "input.h"
#include <SDL2/SDL.h>
#include <curses.h>
#include <stdbool.h>
#include <stdio.h>

// SDL_GameController* setUpController(void){
//     SDL_GameController* controller;
//     if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
//         printf("SDL_Init Error: %s\n", SDL_GetError());
//         return NULL;
//         }
//     if (SDL_NumJoysticks() == 0) {
//         printf("No hame controllers connected\n");
//         SDL_Quit();
//         return NULL;
//     }
//     controller = SDL_GameControllerOpen(0);
//     if (!controller) {
//         printf("Could not open game controller: %s\n", SDL_GetError());
//         SDL_Quit();
//         return NULL;
//         }
//     return controller;
//     }
//
//
// enum move_direction getControllerInput(SDL_Event* event){
//         if(event->type == SDL_CONTROLLERBUTTONDOWN){
//             switch(event->cbutton.button){
//                 case 3: return UP;
//                 case 0: return DOWN;
//                 case 2: return LEFT;
//                 case 4: return RIGHT;
//                 default: return NONE;
//             }
//         }
//         return NONE;
//
//     }

enum move_direction getKeyboardInput(void)
{
    int ch = getch();
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
        default:
            return NONE;
    }
}
