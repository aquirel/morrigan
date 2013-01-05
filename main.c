﻿// main.c - main().

#if !defined(DYNAMIC_ARRAY_TESTS) && !defined(RING_BUFFER_TESTS)

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "bstrlib.h"

#include "net.h"
#include "protocol.h"
#include "debug.h"

int main(int argc, const char *argv[], const char *envp[])
{
    puts("Starting morrigan.");

    check(net_start(), "Failed to start network interface.", "");
    check(protocol_start(), "Failed to start game.", "");

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
    protocol_stop();

    return EXIT_SUCCESS;

    error:
    net_stop();
    protocol_stop();
    return EXIT_FAILURE;
}

#endif
