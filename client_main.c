// client_main.c - main() for client.

#include <process.h>
#include <threads.h>
#include <conio.h>

#include "debug.h"
#include "client_protocol.h"
#include "client_net.h"

SOCKET s = INVALID_SOCKET;
bool connected = false;

void cleanup(void);

int main(int argc, char *argv[])
{
    if (2 > argc)
    {
        fprintf(stderr, "Usage: %s server-address\n", argv[0]);
        return -1;
    }

    srand((unsigned) (time(NULL) ^ _getpid()));

    check(client_net_start(), "Failed to initialize net.", "");
    check(client_connect(&s, argv[1], true, client_protocol, sizeof(client_protocol) / sizeof(client_protocol[0])), "Failed to connect.", "");
    connected = true;

    puts("Connected to server.");

    char c = '\0';

    while (true)
    {
        while (client_protocol_process_event(&s, client_protocol, sizeof(client_protocol) / sizeof(client_protocol[0]))) ;

        if (_kbhit())
        {
            if ('q' == (c = (char) getchar()))
            {
                break;
            }

            switch (c)
            {
                case 'p':
                    set_engine_power(&s, rand() % 100 - 10, client_protocol, sizeof(client_protocol) / sizeof(client_protocol[0]));
                    break;

                case 'l':
                {
                    Vector look = { .x = -1.0 + 2.0 * (double) rand() / RAND_MAX,
                                    .y = -1.0 + 2.0 * (double) rand() / RAND_MAX,
                                    .z = -0.5 + (double) rand() / RAND_MAX };
                    VECTOR_NORMALIZE(&look);
                    look_at(&s, &look, client_protocol, sizeof(client_protocol) / sizeof(client_protocol[0]));
                    break;
                }

                case 't':
                    turn(&s, -M_PI + 2.0 * M_PI * (double) rand() / RAND_MAX, client_protocol, sizeof(client_protocol) / sizeof(client_protocol[0]));
                    break;

                case 's':
                    shoot(&s, client_protocol, sizeof(client_protocol) / sizeof(client_protocol[0]));
                    break;

                default:
                    printf("p - engine power; l - look; t - turn; s - shoot; q - quit.\n");
                    break;
            }
        }

        {
            struct timespec t = { .tv_nsec = 100000000 };
            thrd_sleep(&t, NULL);
        }
    }

    cleanup();
    return 0;

    error:
    cleanup();
    return -1;
}

void cleanup(void)
{
    if (connected)
    {
        check(client_disconnect(&s, true), "Failed to disconnect.", "");
        connected = true;
    }

    static bool net_stopped = false;
    error:
    if (!net_stopped)
    {
        client_net_stop();
        net_stopped = true;
    }
}
