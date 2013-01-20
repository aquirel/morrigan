// server.h - server implementation.

#include <assert.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "server.h"
#include "protocol.h"
#include "dynamic_array.h"
#include "ring_buffer.h"
#include "debug.h"

static thrd_t worker_tid;
static volatile atomic_bool working = false;
DynamicArray *clients = NULL;
static RingBuffer *requests = NULL;

static int server_worker(void *unused);

bool server_start(void)
{
    clients = DYNAMIC_ARRAY_CREATE(Client *, MAX_CLIENTS);
    check_mem(clients);
    requests = RING_BUFFER_CREATE(Client *, MAX_CLIENTS);
    check_mem(requests);

    working = true;
    check(thrd_success == thrd_create(&worker_tid, server_worker, NULL), "Failed to start server worker thread.", "");

    return true;
    error:
    if (clients)
    {
        dynamic_array_destroy(clients);
    }

    if (requests)
    {
        ring_buffer_destroy(requests);
    }

    thrd_detach(worker_tid);

    return false;
}

void server_stop(void)
{
    fprintf(stderr, "server_stop start.\n");
    working = false;
    thrd_join(worker_tid, NULL);
    thrd_detach(worker_tid);

    if (clients)
    {
        dynamic_array_destroy(clients);
        clients = NULL;
    }

    if (requests)
    {
        ring_buffer_destroy(requests);
        requests = NULL;
    }

    fprintf(stderr, "server_stop end.\n");
}

Client *find_client_by_address(const SOCKADDR *address)
{
    dynamic_array_lock(clients);
    size_t client_count = dynamic_array_count(clients);

    for (size_t i = 0; i < client_count; i++)
    {
        Client *c = DYNAMIC_ARRAY_GET(Client *, clients, i);
        if (0 == memcmp(address, &c->address, sizeof(SOCKADDR)))
        {
            dynamic_array_unlock(clients);
            return c;
        }
    }
    dynamic_array_unlock(clients);

    return NULL;
}

Client *register_client(const SOCKADDR *address)
{
    Client *c = find_client_by_address(address);
    if (c)
    {
        return c;
    }

    if (MAX_CLIENTS == dynamic_array_count(clients))
    {
        return NULL;
    }

    c = (Client *) calloc(1, sizeof(Client));
    check_mem(c);

    c->state = cs_connected;
    memcpy(&c->address, address, sizeof(SOCKADDR));

    dynamic_array_lock(clients);
    check(dynamic_array_push(clients, c), "Failed to add new client.", "");
    dynamic_array_unlock(clients);

    return c;
    error:
    dynamic_array_unlock(clients);
    if (c)
    {
        free(c);
    }
    return NULL;
}

bool unregister_client(const SOCKADDR *address)
{
    dynamic_array_lock(clients);
    size_t client_count = dynamic_array_count(clients);

    for (size_t i = 0; i < client_count; i++)
    {
        Client *c = DYNAMIC_ARRAY_GET(Client *, clients, i);
        if (0 == memcmp(address, &c->address, sizeof(SOCKADDR)))
        {
            dynamic_array_delete_at(clients, i);
            free(c);
            dynamic_array_unlock(clients);
            return true;
        }
    }
    dynamic_array_unlock(clients);

    return false;
}

void enqueue_client(const Client *c)
{
    ring_buffer_write(requests, c);
}

void notify_shutdown(void)
{
    dynamic_array_lock(clients);
    while (dynamic_array_count(clients))
    {
        Client *c = DYNAMIC_ARRAY_GET(Client *, clients, 0);
        dynamic_array_delete_at(clients, 0);
        uint8_t response = req_bye;
        respond((char *) &response, 1, &c->address);
        free(c);
    }
    dynamic_array_unlock(clients);
}

static int server_worker(void *unused)
{
    while (working)
    {
        if (ring_buffer_is_empty(requests))
        {
            ring_buffer_wait_not_empty(requests);
        }

        if (ring_buffer_is_empty(requests))
        {
            continue;
        }

        Client *c = RING_BUFFER_READ(Client *, requests);
        assert(c && "Bad Client pointer.");
        assert(c->current_packet_definition && "Client doesn't have pending packet.");
        assert(c->current_packet_definition->executor && "No executor in packet definition.");

        c->current_packet_definition->executor(c);
        c->current_packet_definition = NULL;
    }

    return 0;
}
