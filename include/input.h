//
// Created by blaise-klein on 11/14/24.
//

#ifndef INPUT_H
#define INPUT_H

#include "app_types.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-default"
#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wreserved-macro-identifier"
    #pragma clang diagnostic ignored "-Wreserved-identifier"
    #pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#endif
#include <SDL2/SDL.h>
#pragma GCC diagnostic pop
#ifdef __clang__
    #pragma clang diagnostic pop
#endif
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
void                setUpController(struct input_state *state);
enum move_direction getControllerInput(const SDL_Event *event);
enum move_direction getKeyboardInput(void);
enum move_direction getTimer(void);
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
    // int                 fd;
    struct context     *ctx = (struct context *)arg;
    enum move_direction direction;
    direction = wait_for_controller_input();
    if(direction == EXIT)
    {
        return SAFE_CLOSE;
    }
    ctx->input.direction = direction;

    // fd = open("/tmp/testing.fifo", O_RDONLY | O_WRONLY | O_CLOEXEC);
    //
    // if(fd == -1)
    // {
    //     return ERROR;
    // }
    // write(fd, "RC", 2);
    // close(fd);

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
    // int                 fd;
    struct context     *ctx = (struct context *)arg;
    enum move_direction direction;
    direction = getKeyboardInput();
    if(direction == EXIT)
    {
        return SAFE_CLOSE;
    }
    ctx->input.direction = direction;
    if(direction != NONE)
    {
        ctx->input_rdy = 1;
    }

    // fd = open("/tmp/testing.fifo", O_RDONLY | O_WRONLY | O_CLOEXEC);
    //
    // if(fd == -1)
    // {
    //     return ERROR;
    // }
    // write(fd, "RK", 2);
    // close(fd);

    return READ_NETWORK;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

static p101_fsm_state_t read_timer(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context     *ctx          = (struct context *)arg;
    time_t              current_time = time(NULL);
    enum move_direction direction;
    direction = getTimer();
    if(direction == EXIT)
    {
        return SAFE_CLOSE;
    }
    ctx->input.direction = direction;
    if(direction != NONE && current_time - ctx->input.last_send > 1)
    {
        ctx->input_rdy       = 1;
        ctx->input.last_send = current_time;
    }

    return READ_NETWORK;
}

#pragma GCC diagnostic pop

#endif    // INPUT_H
