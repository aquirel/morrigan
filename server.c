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
DynamicArray *viewers = NULL;
static RingBuffer *client_requests = NULL;
static RingBuffer *viewer_requests = NULL;
static cnd_t have_new_request_signal;
static mtx_t have_new_request_mutex;

static int server_worker(void *unused);

bool server_start(void)
{
    clients = DYNAMIC_ARRAY_CREATE(Client *, MAX_CLIENTS);
    check_mem(clients);
    viewers = DYNAMIC_ARRAY_CREATE(ViewerClient *, MAX_VIEWERS);
    check_mem(viewers);
    client_requests = RING_BUFFER_CREATE(Client *, MAX_CLIENTS);
    check_mem(client_requests);
    viewer_requests = RING_BUFFER_CREATE(ViewerClient *, MAX_VIEWERS);
    check_mem(viewer_requests);
    check(thrd_success == cnd_init(&have_new_request_signal), "Failed to initialize request signal.", "");
    check(thrd_success == mtx_init(&have_new_request_mutex, mtx_plain), "Failed to initialize request mutex.", "");

    working = true;
    check(thrd_success == thrd_create(&worker_tid, server_worker, NULL), "Failed to start server worker thread.", "");

    return true;
    error:
    if (clients)
    {
        dynamic_array_destroy(clients);
    }

    if (client_requests)
    {
        ring_buffer_destroy(client_requests);
    }

    if (viewers)
    {
        dynamic_array_destroy(viewers);
    }

    if (viewer_requests)
    {
        ring_buffer_destroy(viewer_requests);
    }

    thrd_detach(worker_tid);
    cnd_destroy(&have_new_request_signal);
    mtx_destroy(&have_new_request_mutex);

    return false;
}

void server_stop(void)
{
    fprintf(stderr, "server_stop start.\n");
    if (working)
    {
        working = false;
        check(thrd_success == cnd_signal(&have_new_request_signal), "Failed to signal request condition variable.", "");
    }
    error:
    thrd_join(worker_tid, NULL);
    thrd_detach(worker_tid);

    if (clients)
    {
        dynamic_array_destroy(clients);
        clients = NULL;
    }

    if (client_requests)
    {
        ring_buffer_destroy(client_requests);
        client_requests = NULL;
    }

    if (viewers)
    {
        dynamic_array_destroy(viewers);
        viewers = NULL;
    }

    if (viewer_requests)
    {
        ring_buffer_destroy(viewer_requests);
        viewer_requests = NULL;
    }

    cnd_destroy(&have_new_request_signal);
    mtx_destroy(&have_new_request_mutex);

    fprintf(stderr, "server_stop end.\n");
}

Client *find_client_by_address(const SOCKADDR *address)
{
    dynamic_array_lock(clients);
    size_t client_count = dynamic_array_count(clients);

    for (size_t i = 0; i < client_count; i++)
    {
        Client *c = *DYNAMIC_ARRAY_GET(Client **, clients, i);
        if (0 == memcmp(address, &c->address, sizeof(SOCKADDR)))
        {
            dynamic_array_unlock(clients);
            return c;
        }
    }
    dynamic_array_unlock(clients);

    return NULL;
}

ViewerClient *find_viewer_by_address(const SOCKADDR *address)
{
    dynamic_array_lock(viewers);
    size_t viewer_count = dynamic_array_count(viewers);

    for (size_t i = 0; i < viewer_count; i++)
    {
        ViewerClient *c = *DYNAMIC_ARRAY_GET(ViewerClient **, viewers, i);
        if (0 == memcmp(address, &c->address, sizeof(SOCKADDR)))
        {
            dynamic_array_unlock(viewers);
            return c;
        }
    }
    dynamic_array_unlock(viewers);

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
    check(dynamic_array_push(clients, &c), "Failed to add new client.", "");
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

ViewerClient *register_viewer(const SOCKADDR *address)
{
    ViewerClient *c = find_viewer_by_address(address);
    if (c)
    {
        return c;
    }

    if (MAX_VIEWERS == dynamic_array_count(viewers))
    {
        return NULL;
    }

    c = (ViewerClient *) calloc(1, sizeof(ViewerClient));
    check_mem(c);

    c->state = cs_connected;
    memcpy(&c->address, address, sizeof(SOCKADDR));

    dynamic_array_lock(viewers);
    check(dynamic_array_push(viewers, &c), "Failed to add new viewer.", "");
    dynamic_array_unlock(viewers);

    return c;
    error:
    dynamic_array_unlock(viewers);
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
        Client *c = *DYNAMIC_ARRAY_GET(Client **, clients, i);
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

bool unregister_viewer(const SOCKADDR *address)
{
    dynamic_array_lock(viewers);
    size_t viewer_count = dynamic_array_count(viewers);

    for (size_t i = 0; i < viewer_count; i++)
    {
        ViewerClient *c = *DYNAMIC_ARRAY_GET(ViewerClient **, viewers, i);
        if (0 == memcmp(address, &c->address, sizeof(SOCKADDR)))
        {
            dynamic_array_delete_at(viewers, i);
            free(c);
            dynamic_array_unlock(viewers);
            return true;
        }
    }
    dynamic_array_unlock(viewers);

    return false;
}

void enqueue_client(const Client *c)
{
    ring_buffer_write(client_requests, &c);
    check(thrd_success == cnd_signal(&have_new_request_signal), "Failed to signal request condition variable.", "");
    error:
    return;
}

void enqueue_viewer(const ViewerClient *c)
{
    ring_buffer_write(viewer_requests, &c);
    check(thrd_success == cnd_signal(&have_new_request_signal), "Failed to signal request condition variable.", "");
    error:
    return;
}

void notify_shutdown(void)
{
    dynamic_array_lock(clients);
    while (dynamic_array_count(clients))
    {
        Client *c = *DYNAMIC_ARRAY_GET(Client **, clients, 0);
        dynamic_array_delete_at(clients, 0);
        uint8_t response = req_bye;
        respond((char *) &response, 1, &c->address);
        free(c);
    }
    dynamic_array_unlock(clients);

    dynamic_array_lock(viewers);
    while (dynamic_array_count(viewers))
    {
        ViewerClient *c = *DYNAMIC_ARRAY_GET(ViewerClient **, viewers, 0);
        dynamic_array_delete_at(viewers, 0);
        uint8_t response = req_bye;
        respond((char *) &response, 1, &c->address);
        free(c);
    }
    dynamic_array_unlock(viewers);
}

static int server_worker(void *unused)
{
    check(thrd_success == mtx_lock(&have_new_request_mutex), "Failed to lock request mutex.", "");

    while (working)
    {
        if (ring_buffer_is_empty(client_requests) &&
            ring_buffer_is_empty(viewer_requests))
        {
            check(thrd_success == cnd_wait(&have_new_request_signal, &have_new_request_mutex), "Failed to wait request signal.", "");

            if (!working)
            {
                break;
            }
        }

        bool no_client_request = ring_buffer_is_empty(client_requests),
             no_viewer_request = ring_buffer_is_empty(viewer_requests);
        if (no_client_request && no_viewer_request)
        {
            continue;
        }

        if (!no_client_request)
        {
            Client *c = *RING_BUFFER_READ(Client **, client_requests);
            assert(c && "Bad Client pointer.");
            assert(c->current_packet_definition && "Client doesn't have pending packet.");
            assert(c->current_packet_definition->executor && "No executor in packet definition.");

            if (c->current_packet_definition->executor(c))
            {
                c->current_packet_definition = NULL;
            }
            else
            {
                free(c);
            }
        }

        if (!no_viewer_request)
        {
            ViewerClient *c = *RING_BUFFER_READ(ViewerClient **, viewer_requests);
            assert(c && "Bad ViewerClient pointer.");
            assert(c->current_packet_definition && "ViewerClient doesn't have pending packet.");
            assert(c->current_packet_definition->executor && "No executor in packet definition.");

            if (c->current_packet_definition->executor(c))
            {
                c->current_packet_definition = NULL;
            }
            else
            {
                free(c);
            }
        }
    }

    return 0;
    error:
    return -1;
}
