//viewer_main.c - main() for viewer.

#include <stdio.h>

#include "debug.h"
#include "client_net.h"

void cleanup(Landscape *l);

int main(int argc, char *argv[])
{
    if (2 > argc)
    {
        fprintf(stderr, "Usage: %s server-address\n", argv[0]);
        return -1;
    }

    check(client_net_start(), "Failed to initialize net.", "");

    SOCKET s;
    check(client_connect(&s, argv[1], false), "Failed to connect.", "");

    puts("Connected to server.");
    getchar();

    Landscape *l = NULL;
    l = client_get_landscape(&s);
    puts("Loaded landscape.");
    getchar();

    check(client_disconnect(&s, false), "Failed to disconnect.", "");

    cleanup(l);
    return 0;

    error:
    cleanup(l); 
    return -1;
}

void cleanup(Landscape *l)
{
    client_net_stop();
    if (l)
    {
        landscape_destroy(l);
    }
}
