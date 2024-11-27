#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <p101_fsm/fsm.h>
#include <p101_posix/p101_unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// #include "input.h"
// #include "network.h"

enum application_states
{
    INIT = P101_FSM_USER_START,    // 2
    INPUT_SETUP,                   // 2
    SETUP_CONTROLLER,
    SETUP_KEYBOARD,
    CREATE_SENDING_STREAM,
    CREATE_RECEIVING_STREAM,
    SETUP_WINDOW,
    AWAIT_INPUT,
    READ_CONTROLLER,
    READ_NETWORK,
    SEND_PACKET,
    HANDLE_PACKET,
    MOVE_NODE,
    REFRESH_SCREEN,
    SAFE_CLOSE,
    ERROR,
};

struct arguments
{
    char *sys_addr;
    // ssize_t sys_addr_len;
    char *sys_port;
    char *target_addr;
    // ssize_t target_addr_len;
    char *target_port;
    // char    controller_type;
    // Controller type, joystick, keyboard, etc...
};

struct input_state
{
    //    enum controller_type controller;
    char *type;
};

struct network_state
{
    int send_fd;
    // struct sockaddr_storage *send_addr;
    // socklen_t                send_addr_len;
    // in_port_t                send_port;
    // int                      receive_fd;
    // struct sockaddr_storage *receive_addr;
    // socklen_t                receive_addr_len;
    // in_port_t                receive_port;
    // int                      current_move;
};

struct board_state
{
    int length;
    // int  width;
    // int  host_x;
    // int  host_y;
    // char host_char;
    // int  net_x;
    // int  net_y;
    // char net_char;
};

struct context
{
    struct arguments     arg;
    struct input_state   input;
    struct network_state network;
    struct board_state   board;
};

#define UNKNOWN_OPTION_MESSAGE_LEN 24

static void             parse_arguments(const struct p101_env *env, int argc, char *argv[], struct context *ctx);
_Noreturn static void   usage(const char *program_name, int exit_code, const char *message);
static p101_fsm_state_t init(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t setup_input_source(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t setup_controller(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t setup_keyboard(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t create_sending_stream(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t create_receiving_stream(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t setup_window(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t await_input(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t read_controller(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t read_network(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t send_packet(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t handle_packet(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t move_node(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t refresh_screen(const struct p101_env *env, struct p101_error *err, void *arg);
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
    parse_arguments(env, argc, argv, ctx);
    fsm_error = p101_error_create(false);
    fsm_env   = p101_env_create(error, true, NULL);
    fsm       = p101_fsm_info_create(env, error, "test-fsm", fsm_env, fsm_error, NULL);

    if(p101_error_has_error(error))
    {
        fprintf(stderr, "Error creating FSM: %s\n", p101_error_get_message(error));
    }
    else
    {
        static struct p101_fsm_transition transitions[] = {
            {P101_FSM_INIT,           INIT,                    init                   },
            {INIT,                    INPUT_SETUP,             setup_input_source     },
            {INPUT_SETUP,             SETUP_CONTROLLER,        setup_controller       },
            {INPUT_SETUP,             SETUP_KEYBOARD,          setup_keyboard         },
            {SETUP_CONTROLLER,        CREATE_SENDING_STREAM,   create_sending_stream  },
            {SETUP_KEYBOARD,          CREATE_SENDING_STREAM,   create_sending_stream  },
            {CREATE_SENDING_STREAM,   CREATE_RECEIVING_STREAM, create_receiving_stream},
            {CREATE_RECEIVING_STREAM, SETUP_WINDOW,            setup_window           },
            {SETUP_WINDOW,            AWAIT_INPUT,             await_input            },
            {INIT,                    ERROR,                   error_state            },
            {INPUT_SETUP,             ERROR,                   error_state            },
            {INPUT_SETUP,             ERROR,                   error_state            },
            {SETUP_CONTROLLER,        ERROR,                   error_state            },
            {SETUP_KEYBOARD,          ERROR,                   error_state            },
            {CREATE_SENDING_STREAM,   ERROR,                   error_state            },
            {CREATE_RECEIVING_STREAM, ERROR,                   error_state            },
            {SETUP_WINDOW,            ERROR,                   error_state            },
            {AWAIT_INPUT,             READ_CONTROLLER,         read_controller        },
            {AWAIT_INPUT,             READ_NETWORK,            read_network           },
            {READ_CONTROLLER,         SEND_PACKET,             send_packet            },
            {READ_NETWORK,            HANDLE_PACKET,           handle_packet          },
            {SEND_PACKET,             MOVE_NODE,               move_node              },
            {HANDLE_PACKET,           MOVE_NODE,               move_node              },
            {MOVE_NODE,               REFRESH_SCREEN,          refresh_screen         },
            {REFRESH_SCREEN,          AWAIT_INPUT,             await_input            },
            {AWAIT_INPUT,             SAFE_CLOSE,              safe_close             },
            {ERROR,                   P101_FSM_EXIT,           NULL                   },
            {SAFE_CLOSE,              P101_FSM_EXIT,           NULL                   }
        };
        p101_fsm_state_t from_state;
        p101_fsm_state_t to_state;

        p101_fsm_run(fsm, &from_state, &to_state, ctx, transitions, sizeof(transitions));
        p101_fsm_info_destroy(env, &fsm);
    }

    free(ctx);
    free(fsm_env);
    free(env);
    p101_error_reset(error);
    free(error);

    return EXIT_SUCCESS;
}

static void parse_arguments(const struct p101_env *env, int argc, char *argv[], struct context *ctx)
{
    int opt;

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
                ctx->arg.sys_addr = optarg;
                break;
            }
            case 'I':
            {
                // save target ip address
                ctx->arg.target_addr = optarg;
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
                ctx->input.type = optarg;
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

    if(optind < argc)
    {
        usage(argv[0], EXIT_FAILURE, "Too many arguments.");
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
    printf("Init");

    return INPUT_SETUP;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t setup_input_source(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("setup_input_source");

    return SETUP_CONTROLLER;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t setup_controller(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("setup_controller");

    return CREATE_SENDING_STREAM;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t setup_keyboard(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("setup_keyboard");

    return CREATE_SENDING_STREAM;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t create_sending_stream(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("create_sending_stream");

    ((struct context *)arg)->network.send_fd = 1;

    return CREATE_RECEIVING_STREAM;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t create_receiving_stream(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("create_receiving_stream");

    return SETUP_WINDOW;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t setup_window(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("setup_window");

    ((struct context *)arg)->board.length = 1;

    return AWAIT_INPUT;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t await_input(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("await_input");

    return READ_CONTROLLER;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t read_controller(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("read_controller");

    return SEND_PACKET;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t read_network(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("read_network");

    return HANDLE_PACKET;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t send_packet(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("send_packet");

    return MOVE_NODE;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t handle_packet(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("handle_packet");

    return MOVE_NODE;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t move_node(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("move_node");

    return REFRESH_SCREEN;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t refresh_screen(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("refresh_screen");

    return AWAIT_INPUT;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t safe_close(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("safe_close");

    return P101_FSM_EXIT;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t error_state(const struct p101_env *env, struct p101_error *err, void *arg)
{
    printf("error_state");

    return P101_FSM_EXIT;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t state_error(const struct p101_env *env, struct p101_error *err, void *arg)
{
    P101_TRACE(env);

    return P101_FSM_EXIT;
}

#pragma GCC diagnostic pop