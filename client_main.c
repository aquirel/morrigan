// client_main.c - main() for client.

#include <assert.h>
#include <process.h>
#include <threads.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "bstrlib.h"

#include "debug.h"
#include "client_protocol.h"
#include "client_net.h"
#include "vector.h"

typedef enum ClientCommand
{
    cmd_none,
    cmd_quit,
    cmd_power_inc,
    cmd_power_dec,
    cmd_halt,
    cmd_look_left,
    cmd_look_right,
    cmd_look_up,
    cmd_look_down,
    cmd_turn_left,
    cmd_turn_right,
    cmd_shoot
} ClientCommand;

static volatile ClientCommand command = cmd_none;

static thrd_t network_thread = { 0 };

static bstring input = NULL;

static int __network_worker(void *unused);
static void __cleanup(void);

static void __look_executor(Vector *current_look, const Vector *axis, double angle);

int main(int argc, char *argv[])
{
    if (2 > argc)
    {
        fprintf(stderr, "Usage: %s <server-address> [<port>]\n", argv[0]);
        return -1;
    }

    unsigned short port = 0;
    if (2 < argc)
    {
        port = (unsigned short) atoi(argv[2]);
        if (!port)
        {
            port = PORT;
        }
    }

    check(client_net_start(), "Failed to initialize net.", "");
    check(client_connect(&client_protocol, argv[1], port, true), "Failed to connect.", "");

    puts("Connected to server.");

    check(thrd_success == thrd_create(&network_thread, __network_worker, NULL), "thrd_create() failed.", "");

    do
    {
        printf(">");
        input = bgets(fgetc, stdin, '\n');

        if (NULL == input || 0 == strcmp("q\n", bdata(input)))
        {
            command = cmd_quit;
        }
        else if (0 == strcmp("pi\n", bdata(input)))
        {
            command = cmd_power_inc;
        }
        else if (0 == strcmp("pd\n", bdata(input)))
        {
            command = cmd_power_dec;
        }
        else if (0 == strcmp("h\n", bdata(input)))
        {
            command = cmd_halt;
        }
        else if (0 == strcmp("ll\n", bdata(input)))
        {
            command = cmd_look_left;
        }
        else if (0 == strcmp("lr\n", bdata(input)))
        {
            command = cmd_look_right;
        }
        else if (0 == strcmp("lu\n", bdata(input)))
        {
            command = cmd_look_up;
        }
        else if (0 == strcmp("ld\n", bdata(input)))
        {
            command = cmd_look_down;
        }
        else if (0 == strcmp("tl\n", bdata(input)))
        {
            command = cmd_turn_left;
        }
        else if (0 == strcmp("tr\n", bdata(input)))
        {
            command = cmd_turn_right;
        }
        else if (0 == strcmp("s\n", bdata(input)))
        {
            command = cmd_shoot;
        }
        else
        {
            command = cmd_none;
            puts("q  - quit,\n"
                 "pi - power inc,\n"
                 "pd - power dec,\n"
                 "h  - halt,\n"
                 "ll - look left,\n"
                 "lr - look right,\n"
                 "lu - look up,\n"
                 "ld - look down,\n"
                 "tl - turn left,\n"
                 "tr - turn right,\n"
                 "s  - shoot.");
        }

        bdestroy(input);
        input = NULL;
    } while(cmd_quit != command);

    thrd_join(network_thread, NULL);

    __cleanup();
    return 0;

    error:
    __cleanup();
    return -1;
}

static int __network_worker(void *unused)
{
    #pragma ref unused

    int current_power = 0;
    Vector current_look = { .x = 1, .y = 0, .z = 0 };

    while (cmd_quit != command)
    {
        if (cmd_none != command)
        {
            switch (command)
            {
                case cmd_power_inc:
                    current_power += 5;
                    set_engine_power(&client_protocol, current_power);
                    break;

                case cmd_power_dec:
                    current_power -= 5;
                    set_engine_power(&client_protocol, current_power);
                    break;

                case cmd_halt:
                    set_engine_power(&client_protocol, current_power = 0);
                    break;

                case cmd_look_left:
                    __look_executor(&current_look, &(Vector) { .x = 0, .y = 0, .z = 1 }, -2.5 / 180.0 * M_PI);
                    break;

                case cmd_look_right:
                    __look_executor(&current_look, &(Vector) { .x = 0, .y = 0, .z = 1 }, 2.5 / 180.0 * M_PI);
                    break;

                case cmd_look_up:
                    __look_executor(&current_look, &(Vector) { .x = 0, .y = 1, .z = 0 }, -2.5 / 180.0 * M_PI);
                    break;

                case cmd_look_down:
                    __look_executor(&current_look, &(Vector) { .x = 0, .y = 1, .z = 0 }, 2.5 / 180.0 * M_PI);
                    break;

                case cmd_turn_left:
                    turn(&client_protocol, -2.5 / 180.0 * M_PI);
                    break;

                case cmd_turn_right:
                    turn(&client_protocol, 2.5 / 180.0 * M_PI);
                    break;

                case cmd_shoot:
                    shoot(&client_protocol);
                    break;

                default:
                    assert(false && "Impossible!");
                    break;
            }

            command = cmd_none;
        }

        while (client_protocol_process_event(&client_protocol));

        struct timespec t = { .tv_nsec = 100000000 };
        thrd_sleep(&t, NULL);
    }

    return 0;
}

static void __look_executor(Vector *current_look, const Vector *axis, double angle)
{
    assert(current_look && "Bad current look pointer.");
    assert(axis && "Bad axis pointer.");

    VECTOR_ROTATE(current_look, axis, angle);
    VECTOR_NORMALIZE(current_look);
    look_at(&client_protocol, current_look);
}

static void __cleanup(void)
{
    thrd_detach(network_thread);

    if (client_protocol.connected)
    {
        check(client_disconnect(&client_protocol, true), "Failed to disconnect.", "");
    }

    static bool net_stopped = false;
    error:
    if (!net_stopped)
    {
        client_net_stop();
        net_stopped = true;
    }

    if (input)
    {
        bdestroy(input);
    }
}
