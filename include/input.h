//
// Created by blaise-klein on 11/14/24.
//

#ifndef INPUT_H
#define INPUT_H

#include "app_types.h"
#include <SDL2/SDL.h>
#include <stdio.h>

void                setUpController(struct input_state *state);
enum move_direction getControllerInput(const SDL_Event *event);
enum move_direction getKeyboardInput(void);
enum move_direction wait_for_controller_input(void);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

static p101_fsm_state_t setup_controller(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context *ctx = (struct context *)arg;
    setUpController(&ctx->input);
    return CREATE_SENDING_STREAM;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

static p101_fsm_state_t read_controller(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context     *ctx = (struct context *)arg;
    enum move_direction direction;
    direction            = wait_for_controller_input();
    ctx->input.direction = direction;
    if(direction != NONE)
    {
        ctx->input_rdy = 1;
    }
    return READ_NETWORK;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

static p101_fsm_state_t read_keyboard(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context     *ctx = (struct context *)arg;
    enum move_direction direction;
    direction            = getKeyboardInput();
    ctx->input.direction = direction;
    if(direction != NONE)
    {
        ctx->input_rdy = 1;
    }
    return READ_NETWORK;
}

#pragma GCC diagnostic pop

#endif    // INPUT_H
