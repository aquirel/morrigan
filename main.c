// main.c - main().

#if !defined(TESTS)

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <process.h>

#include "lib/bstrlib.h"

#include "net.h"
#include "protocol.h"
#include "server.h"
#include "game.h"
#include "landscape.h"
#include "debug.h"

static void __stop(int unused);

static Landscape *l = NULL;
static bstring input = NULL;

//int main(int argc, char *argv[], char *envp[])
int main(int argc, char *argv[])
{
    puts("Starting morrigan.");

    int port = 0;
    if (1 < argc)
    {
        port = atoi(argv[1]);
        if (!port)
        {
            port = PORT;
        }
        printf("Using port: %d.\n", port);
    }

    check(SIG_ERR != signal(SIGINT, __stop), "Failed to set signal handler.", "");
    check(SIG_ERR != signal(SIGTERM, __stop), "Failed to set signal handler.", "");

    srand((unsigned) (time(NULL) ^ _getpid()));

    l = landscape_load("land.dat", 32, 1.0);
    check(l, "Failed to load landscape.", "");
    check(net_start(port), "Failed to start network interface.", "");
    check(server_start(), "Failed to start server.", "");
    check(game_start(l, server_get_clients()), "Failed to start game.", "");

    do
    {
        printf(">");
        input = bgets(fgetc, stdin, '\n');

        if (NULL == input || 0 == strcmp("exit\n", bdata(input)))
        {
            break;
        }

        bdestroy(input);
        input = NULL;
    } while(true);

    __stop(0);
    return EXIT_SUCCESS;

    error:
    fprintf(stderr, "Error exit.\n");
    __stop(0);
    return EXIT_FAILURE;
}

static void __stop(int unused)
{
    #pragma ref unused

    puts("Stopping morrigan.");
    if (input)
    {
        bdestroy(input);
    }
    net_stop();
    game_stop();
    server_stop();
    if (l)
    {
        landscape_destroy(l);
        l = NULL;
    }
}

#endif
