// client_main.c - main() for client.

#include "debug.h"
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

    check(client_net_start(), "Failed to initialize net.", "");
    check(client_connect(&s, argv[1], true), "Failed to connect.", "");
    connected = true;

    puts("Connected to server.");

    while (EOF != getchar())
    {
        set_engine_power(&s, rand() % 100 - 10);
        Vector look = { .x = (double) rand() / RAND_MAX, .y = (double) rand() / RAND_MAX, .z = (double) rand() / (4.0 * RAND_MAX) };
        VECTOR_NORMALIZE(&look);
        look_at(&s, &look);
        turn(&s, -M_PI + ((double) rand() / RAND_MAX * 2.0 * M_PI));
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
