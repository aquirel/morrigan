﻿// main.c - main().

#if !defined(TESTS)

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "bstrlib.h"

#include "net.h"
#include "protocol.h"
#include "server.h"
#include "game.h"
#include "landscape.h"
#include "debug.h"

void stop(void);

static Landscape *l = NULL;
static bstring input = NULL;

int main(const int argc, const char const *argv[], const char const *envp[])
{
    puts("Starting morrigan.");

    check(SIG_ERR != signal(SIGINT, stop), "Failed to set signal handler.", "");
    check(SIG_ERR != signal(SIGTERM, stop), "Failed to set signal handler.", "");

    srand(time(NULL));

    l = landscape_load("land.dat", 0, 1.0);
    check(l, "Failed to load landscape.", "");
    check(net_start(), "Failed to start network interface.", "");
    check(server_start(), "Failed to start server.", "");
    check(game_start(l, clients), "Failed to start game.", "");

    do
    {
        printf(">");
        input = bgets(getchar, NULL, '\n');

        if (NULL == input || 0 == strcmp("exit\n", bdata(input)))
        {
            break;
        }

        bdestroy(input);
        input = NULL;
    } while(true);

    stop();
    return EXIT_SUCCESS;

    error:
    fprintf(stderr, "Error exit.\n");
    stop();
    return EXIT_FAILURE;
}

void stop(void)
{
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
    }
}

#endif
