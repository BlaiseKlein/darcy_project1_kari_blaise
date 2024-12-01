//
// Created by blaise-klein on 11/14/24.
//

#ifndef INPUT_H
#define INPUT_H

#include "app_types.h"
#include <SDL2/SDL.h>
#include <stdio.h>

SDL_GameController *setUpController(void);
enum move_direction getControllerInput(SDL_Event *event);
enum move_direction getKeyboardInput(void);
#endif    // INPUT_H
