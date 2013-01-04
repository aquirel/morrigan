// server.h - server implementation.

#include <stdbool.h>

#include "server.h"
#include "client.h"
#include "dynamic_array.h"

static DynamicArray *clients = NULL;

bool server_init(void)
{
    clients = DYNAMIC_ARRAY_CREATE(Client *, MAX_CLIENTS);
    check_mem(clients);
    return true;
    error:
    return false;
}

void server_shutdown(void)
{
    if (clients)
    {
        dynamic_array_destroy(clients);
        clients = NULL;
    }
}

Client *find_client_by_address(const SOCKADDR *address);
Client *register_client(const SOCKADDR *address);

void enqueue_client(const Client *c); // TODO: Add to ringbuffer.

void notify_shutdown(void); // TODO: Notify remaining clients about server shutdown.
