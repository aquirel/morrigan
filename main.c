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
#include "debug.h"

int main(int argc, const char *argv[], const char *envp[])
{
    puts("Starting morrigan.");

    srand(time(NULL));

    check(net_start(), "Failed to start network interface.", "");
    check(server_start(), "Failed to start server.", "");

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
    server_stop();

    return EXIT_SUCCESS;

    error:
    net_stop();
    server_stop();
    return EXIT_FAILURE;
}

#endif
