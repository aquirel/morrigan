// main.c - main().

#if !defined(TESTS)

#include <stdlib.h>
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

int main(int argc, const char *argv[], const char *envp[])
{
    puts("Starting morrigan.");

    srand(time(NULL));

    Landscape *l = NULL;
    // TODO: Create & load landscape. Add load_landscape function.
    check(net_start(), "Failed to start network interface.", "");
    check(server_start(), "Failed to start server.", "");
    check(game_start(l, clients), "Failed to start game.", "");

    do
    {
        printf(">");
        bstring input = bgets(getchar, NULL, '\n');

        if (NULL == input || 0 == strcmp("exit\n", bdata(input)))
        {
            break;
        }

        bdestroy(input);
    } while(true);

    puts("Stopping morrigan.\n");

    net_stop();
    game_stop();
    server_stop();
    landscape_destroy(l);

    return EXIT_SUCCESS;

    error:
    net_stop();
    game_stop();
    server_stop();
    if (l)
    {
        landscape_destroy(l);
    }
    return EXIT_FAILURE;
}

#endif
