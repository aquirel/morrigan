// client_main.c - main() for client.

#include "debug.h"
#include "client_protocol.h"

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

    srand(time(NULL));

    check(client_net_start(), "Failed to initialize net.", "");
    check(client_connect(&s, argv[1], true), "Failed to connect.", "");
    connected = true;

    puts("Connected to server.");

    char c = '\0';
    while ('q' != (c = getchar()))
    {
        switch (c)
        {
            case 'p':
                set_engine_power(&s, rand() % 100 - 10);
                break;

            case 'l':
            {
                Vector look = { .x = -1.0 + 2.0 * (double) rand() / RAND_MAX,
                                .y = -1.0 + 2.0 * (double) rand() / RAND_MAX,
                                .z = -0.5 + (double) rand() / RAND_MAX };
                VECTOR_NORMALIZE(&look);
                look_at(&s, &look);
                break;
            }

            case 't':
                turn(&s, -M_PI + 2.0 * M_PI * (double) rand() / RAND_MAX);
                break;

            default:
                printf("p - engine power; l - look; t - turn; q - quit.\n");
                break;
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
