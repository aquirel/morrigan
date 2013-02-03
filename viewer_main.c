//viewer_main.c - main() for viewer.

#include <stdio.h>

#include "debug.h"
#include "client_net.h"

int main(int argc, char *argv[])
{
    if (2 > argc)
    {
        fprintf(stderr, "Usage: %s server-address\n", argv[0]);
        return -1;
    }

    check(client_net_start(), "Failed to initialize net.", "");

    SOCKET s;
    check(client_connect(&s, argv[1], true), "Failed to connect.", "");

    puts("Connected to server.");
    getchar();

    check(client_disconnect(&s, true), "Failed to disconnect.", "");

    client_net_stop();
    return 0;

    error:
    client_net_stop();
    return -1;
}
