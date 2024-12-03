#include "app_types.h"
#include "display.h"
#include "input.h"
#include "network.h"
#include <SDL2/SDL.h>
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <p101_fsm/fsm.h>
#include <p101_posix/p101_unistd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define UNKNOWN_OPTION_MESSAGE_LEN 24

void                    handle_signal(int signal);
static void             parse_arguments(const struct p101_env *env, int argc, char *argv[], struct context *ctx);
_Noreturn static void   usage(const char *program_name, int exit_code, const char *message);
static p101_fsm_state_t init(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t safe_close(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t error_state(const struct p101_env *env, struct p101_error *err, void *arg);

int main(int argc, char *argv[])
{
    struct p101_error    *error;
    struct p101_env      *env;
    struct p101_error    *fsm_error;
    struct p101_env      *fsm_env;
    struct p101_fsm_info *fsm;
    struct context       *ctx;

    error = p101_error_create(false);
    env   = p101_env_create(error, true, NULL);
    ctx   = (struct context *)malloc(sizeof(struct context));

    if(ctx == NULL)
    {
        usage(argv[0], EXIT_FAILURE, "Context malloc failed");
    }

    ctx->network.receive_addr = NULL;
    ctx->network.send_addr    = NULL;
    parse_arguments(env, argc, argv, ctx);
    fsm_error = p101_error_create(false);
    fsm_env   = p101_env_create(error, true, NULL);
    fsm       = p101_fsm_info_create(env, error, "game", fsm_env, fsm_error, NULL);

    if(signal(SIGINT, handle_signal) == SIG_ERR)
    {
        usage(argv[0], EXIT_FAILURE, "signal handler could not be setup");
    }

    if(p101_error_has_error(error))
    {
        fprintf(stderr, "Error creating FSM: %s\n", p101_error_get_message(error));
        free(ctx);
        ctx = NULL;
    }
    else
    {
        static struct p101_fsm_transition transitions[] = {
            {P101_FSM_INIT,           INIT,                    init                   },
            {INIT,                    SETUP_CONTROLLER,        setup_controller       },
            {INIT,                    CREATE_SENDING_STREAM,   create_sending_stream  },
            {SETUP_CONTROLLER,        CREATE_SENDING_STREAM,   create_sending_stream  },
            {CREATE_SENDING_STREAM,   CREATE_RECEIVING_STREAM, create_receiving_stream},
            {CREATE_RECEIVING_STREAM, SETUP_WINDOW,            setup_window           },
            {SETUP_WINDOW,            READ_INPUT,              read_input             },
            {INIT,                    ERROR,                   error_state            },
            {SETUP_CONTROLLER,        ERROR,                   error_state            },
            {CREATE_SENDING_STREAM,   ERROR,                   error_state            },
            {CREATE_RECEIVING_STREAM, ERROR,                   error_state            },
            {SETUP_WINDOW,            ERROR,                   error_state            },
            {READ_INPUT,              READ_CONTROLLER,         read_controller        },
            {READ_INPUT,              READ_KEYBOARD,           read_keyboard          },
            {READ_KEYBOARD,           READ_NETWORK,            read_network           },
            {READ_KEYBOARD,           SAFE_CLOSE,              safe_close             },
            {READ_CONTROLLER,         READ_NETWORK,            read_network           },
            {READ_CONTROLLER,         SAFE_CLOSE,              safe_close             },
            {READ_NETWORK,            HANDLE_PACKET,           handle_packet          },
            {READ_NETWORK,            SEND_PACKET,             send_packet            },
            {READ_NETWORK,            READ_INPUT,              read_input             },
            {SEND_PACKET,             HANDLE_PACKET,           handle_packet          },
            {SEND_PACKET,             SYNC_NODES,              sync_nodes             },
            {HANDLE_PACKET,           SYNC_NODES,              sync_nodes             },
            {SYNC_NODES,              REFRESH_SCREEN,          refresh_screen         },
            {REFRESH_SCREEN,          READ_INPUT,              read_input             },
            {READ_INPUT,              SAFE_CLOSE,              safe_close             },
            {ERROR,                   P101_FSM_EXIT,           NULL                   },
            {SAFE_CLOSE,              P101_FSM_EXIT,           NULL                   }
        };
        p101_fsm_state_t from_state;
        p101_fsm_state_t to_state;

        p101_fsm_run(fsm, &from_state, &to_state, ctx, transitions, sizeof(transitions));
        p101_fsm_info_destroy(env, &fsm);
    }

    if(ctx != NULL)
    {
        if(ctx->network.send_addr != NULL)
        {
            free(ctx->network.send_addr);
            ctx->network.send_addr = NULL;
        }
        if(ctx->network.receive_addr != NULL)
        {
            free(ctx->network.receive_addr);
            ctx->network.receive_addr = NULL;
        }
        free(ctx);
        ctx = NULL;
    }
    free(fsm_env);
    free(env);
    p101_error_reset(error);
    free(error);

    return EXIT_SUCCESS;
}

static void parse_arguments(const struct p101_env *env, int argc, char *argv[], struct context *ctx)
{
    int       opt;
    const int help_args = 2;
    const int all_args  = 11;

    opterr = 0;

    while((opt = p101_getopt(env, argc, argv, "i:I:p:P:c:")) != -1)
    {
        switch(opt)
        {
            case 'h':
            {
                usage(argv[0], EXIT_SUCCESS, NULL);
            }
            case 'i':
            {
                // save own ip address
                ctx->arg.sys_addr     = optarg;
                ctx->arg.sys_addr_len = (ssize_t)strlen(optarg);
                break;
            }
            case 'I':
            {
                // save target ip address
                ctx->arg.target_addr     = optarg;
                ctx->arg.target_addr_len = (ssize_t)strlen(optarg);
                break;
            }
            case 'p':
            {
                // save sending port
                ctx->arg.sys_port = optarg;
                break;
            }
            case 'P':
            {
                // save receiving port
                ctx->arg.target_port = optarg;
                break;
            }
            case 'c':
            {
                // save controller type
                const char *input_type = optarg;
                if(input_type == NULL)
                {
                    usage(argv[0], EXIT_SUCCESS, "-c requires either \"keyboard\" or \"controller\" as an input");
                }
                if(strcmp(input_type, "keyboard") == 0)
                {
                    ctx->input.type = KEYBOARD;
                }
                if(strcmp(input_type, "controller") == 0)
                {
                    ctx->input.type = CONTROLLER;
                }
                break;
            }
            case '?':
            {
                if(optopt == 'c')
                {
                    usage(argv[0], EXIT_FAILURE, "Option '-c' requires a value.");
                }
                else
                {
                    char message[UNKNOWN_OPTION_MESSAGE_LEN];

                    snprintf(message, sizeof(message), "Unknown option '-%c'.", optopt);
                    usage(argv[0], EXIT_FAILURE, message);
                }
            }
            default:
            {
                usage(argv[0], EXIT_FAILURE, NULL);
            }
        }
    }

    if(argc != help_args && argc != all_args)
    {
        usage(argv[0], EXIT_FAILURE, "Invalid argument count.");
    }
}

_Noreturn static void usage(const char *program_name, int exit_code, const char *message)
{
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }

    fprintf(stderr, "Usage: %s -i <sending_ip> -I <receiving_ip> -p <sending_port> -P <receiving_port>\n", program_name);
    fputs("Options:\n", stderr);
    fputs("  -i  <sending_ip> The IP address of this system\n", stderr);
    fputs("  -I  <receiving_ip> The IP address of the other connected system\n", stderr);
    fputs("  -p  <sending_port> The port used to send updates from this system.\n", stderr);
    fputs("  -P  <receiving_port> The port used to receive updates from the paired system\n", stderr);
    fputs("  -c  <device type> The device to use: controller or keyboard\n", stderr);
    exit(exit_code);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t init(const struct p101_env *env, struct p101_error *err, void *arg)
{
    const struct context *ctx = (struct context *)arg;

    printf("Press enter button on keyboard to start, \n'q' to exit on keyboard, or the back button to quit on controller\n");
    getchar();

    if(ctx->input.type == KEYBOARD)
    {
        return CREATE_SENDING_STREAM;
    }
    return SETUP_CONTROLLER;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t safe_close(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context *ctx = (struct context *)arg;
    endwin();
    if(ctx != NULL)
    {
        if(ctx->network.send_addr != NULL)
        {
            free(ctx->network.send_addr);
            ctx->network.send_addr = NULL;
        }
        if(ctx->network.receive_addr != NULL)
        {
            free(ctx->network.receive_addr);
            ctx->network.receive_addr = NULL;
        }
    }

    return P101_FSM_EXIT;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t error_state(const struct p101_env *env, struct p101_error *err, void *arg)
{
    struct context *ctx = (struct context *)arg;

    if(ctx != NULL)
    {
        if(ctx->network.send_addr != NULL)
        {
            free(ctx->network.send_addr);
            ctx->network.send_addr = NULL;
        }
        if(ctx->network.receive_addr != NULL)
        {
            free(ctx->network.receive_addr);
            ctx->network.receive_addr = NULL;
        }
        free(ctx);
        ctx = NULL;
    }
    return P101_FSM_EXIT;
}

#pragma GCC diagnostic pop

void handle_signal(int signal)
{
    if(signal == SIGINT)
    {
        abort();
    }
}
