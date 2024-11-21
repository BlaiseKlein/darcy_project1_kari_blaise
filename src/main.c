#include <p101_fsm/fsm.h>
#include <p101_posix/p101_unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "input.h"
#include "network.h"

static void             parse_arguments(const struct p101_env *env, int argc, char *argv[], bool *bad, bool *will, bool *did);
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
static p101_fsm_state_t usage(const struct p101_env *env, struct p101_error *err, void *arg);
static p101_fsm_state_t shutdown(const struct p101_env *env, struct p101_error *err, void *arg);

enum application_states
{
    INIT = P101_FSM_USER_START,    // 2
    INPUT_SETUP,    // 2
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
    USAGE,
    SHUTDOWN,
    ERROR,
};

struct arguments
{
	char *sys_addr;
    ssize_t sys_addr_len;
    char *sys_port;
    char *target_addr;
    ssize_t target_addr_len;
    char *target_port;
    char controller_type;
    //Controller type, joystick, keyboard, etc...
};

struct input_state
{
    enum controller_type controller;
};

struct network_state
{
	int send_fd;
    struct sockaddr_storage *send_addr;
    socklen_t send_addr_len;
    in_port_t send_port;
    int receive_fd;
    struct sockaddr_storage *receive_addr;
    socklen_t receive_addr_len;
    in_port_t receive_port;
    int current_move;
};

struct board_state
{
	int length;
    int width;
    int host_x;
    int host_y;
    char host_char;
    int net_x;
    int net_y;
    char net_char;
};

struct context
{
	struct arguments arg;
	struct input_state input;
    struct network_state network;
    struct board_state board;
};

#define UNKNOWN_OPTION_MESSAGE_LEN 24

int main(int argc, char *argv[])
{
    struct p101_error    *error;
    struct p101_env      *env;
    bool                  bad;
    bool                  will;
    bool                  did;
    struct p101_error    *fsm_error;
    struct p101_env      *fsm_env;
    struct p101_fsm_info *fsm;

    error = p101_error_create(false);
    env   = p101_env_create(error, true, NULL);
    bad   = false;
    will  = false;
    did   = false;
    parse_arguments(env, argc, argv, &bad, &will, &did);
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
            {P101_FSM_INIT, INIT,          init               },
            {INIT,          INPUT_SETUP,   setup_input_source },
            {INPUT_SETUP,   C,             c                  },
            {C,             A,             a          },
            {C,             ERROR,         state_error},
            {ERROR,         P101_FSM_EXIT, NULL       }
        };
        p101_fsm_state_t from_state;
        p101_fsm_state_t to_state;
        int              count;

        count = 0;
        p101_fsm_run(fsm, &from_state, &to_state, &count, transitions, sizeof(transitions));
        p101_fsm_info_destroy(env, &fsm);
    }

    free(fsm_env);
    free(env);
    p101_error_reset(error);
    free(error);

    return EXIT_SUCCESS;
}

static void parse_arguments(const struct p101_env *env, int argc, char *argv[], bool *bad, bool *will, bool *did)
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
                //save own ip address
                break;
            }
            case 'I':
            {
                //save target ip address
                break;
            }
            case 'p':
            {
                //save sending port
                break;
            }
            case 'P':
            {
                //save receiving port
                break;
            }
            case 'c':
            {
                //save controller type
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

static p101_fsm_state_t a(const struct p101_env *env, struct p101_error *err, void *arg)
{
    int *count;

    P101_TRACE(env);
    count = ((int *)arg);
    printf("a called with %d\n", *count);
    *count += 1;

    return B;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t b(const struct p101_env *env, struct p101_error *err, void *arg)
{
    int *count;

    P101_TRACE(env);
    count = ((int *)arg);
    printf("b called with %d\n", *count);
    *count += 1;

    return C;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static p101_fsm_state_t c(const struct p101_env *env, struct p101_error *err, void *arg)
{
    int             *count;
    p101_fsm_state_t next_state;

    P101_TRACE(env);
    count = ((int *)arg);
    printf("c called with %d\n", *count);
    *count += 1;

    if(*count > 3)
    {
        next_state = ERROR;
    }
    else
    {
        next_state = A;
    }

    return next_state;
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