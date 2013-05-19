// server.h - server implementation.

#include <assert.h>
#include <stdbool.h>
#include <threads.h>

#include "debug.h"
#include "server.h"
#include "protocol.h"
#include "dynamic_array.h"

static mtx_t global_mutex;
static DynamicArray *clients = NULL;
static DynamicArray *viewers = NULL;

static NetworkClient *__client_finder_by_address(const SOCKADDR *address, DynamicArray *a);
static NetworkClient *__client_registrator(const SOCKADDR *address,
                                           DynamicArray *a,
                                           size_t max_count,
                                           size_t client_size);
static bool __client_unregistrator(const SOCKADDR *address, DynamicArray *a);
static void __shutdown_notifier(DynamicArray *a, uint8_t message);
static void __clean(void);

bool server_start(void)
{
    clients = DYNAMIC_ARRAY_CREATE(Client *, MAX_CLIENTS);
    check_mem(clients);
    viewers = DYNAMIC_ARRAY_CREATE(ViewerClient *, MAX_VIEWERS);
    check_mem(viewers);

    check(thrd_success == mtx_init(&global_mutex, mtx_plain), "Failed to initialize global mutex.", "");

    return true;
    error:
    __clean();
    return false;
}

void server_stop(void)
{
    fprintf(stderr, "server_stop start.\n");
    __clean();
    fprintf(stderr, "server_stop end.\n");
}

DynamicArray *server_get_clients(void)
{
    return clients;
}

Client *find_client_by_address(const SOCKADDR *address)
{
    return (Client *) __client_finder_by_address(address, clients);
}

ViewerClient *find_viewer_by_address(const SOCKADDR *address)
{
    return (ViewerClient *) __client_finder_by_address(address, viewers);
}

static NetworkClient *__client_finder_by_address(const SOCKADDR *address, DynamicArray *a)
{
    assert(address && "Bad address pointer.");
    assert(a && "Bad collection pointer.");

    size_t count = dynamic_array_count(a);

    for (size_t i = 0; i < count; i++)
    {
        NetworkClient *c = *DYNAMIC_ARRAY_GET(NetworkClient **, a, i);
        if (0 == memcmp(address, &c->address, sizeof(SOCKADDR)))
        {
            return c;
        }
    }

    return NULL;
}

NetworkClient *register_client(const SOCKADDR *address)
{
    return __client_registrator(address, clients, MAX_CLIENTS, sizeof(Client));
}

NetworkClient *register_viewer(const SOCKADDR *address)
{
    return __client_registrator(address, viewers, MAX_VIEWERS, sizeof(ViewerClient));
}

static NetworkClient *__client_registrator(const SOCKADDR *address,
                                           DynamicArray *a,
                                           size_t max_count,
                                           size_t client_size)
{
    assert(address && "Bad address pointer.");
    assert(a && "Bad client array pointer.");
    assert(max_count && "Bad max available count.");
    assert(client_size && "Bad client size.");

    NetworkClient *c = __client_finder_by_address(address, a);
    if (c)
    {
        return c;
    }

    if (max_count == dynamic_array_count(a))
    {
        return NULL;
    }

    c = (NetworkClient *) calloc(1, client_size);
    check_mem(c);

    c->state = cs_connected;
    memcpy(&c->address, address, sizeof(SOCKADDR));

    check(dynamic_array_push(a, &c), "Failed to add new client.", "");

    return c;
    error:
    if (c)
    {
        free(c);
    }
    return NULL;
}

bool unregister_client(const SOCKADDR *address)
{
    return __client_unregistrator(address, clients);
}

bool unregister_viewer(const SOCKADDR *address)
{
    return __client_unregistrator(address, viewers);
}

static bool __client_unregistrator(const SOCKADDR *address, DynamicArray *a)
{
    assert(address && "Bad address pointer.");
    assert(a && "Bad client array pointer.");

    size_t count = dynamic_array_count(a);

    for (size_t i = 0; i < count; i++)
    {
        NetworkClient *c = *DYNAMIC_ARRAY_GET(NetworkClient **, a, i);
        if (0 == memcmp(address, &c->address, sizeof(SOCKADDR)))
        {
            dynamic_array_delete_at(a, i);
            free(c);
            return true;
        }
    }

    return false;
}

void notify_viewers(NotViewerShellEvent *notification)
{
    assert(notification && "Bad notification pointer.");

    size_t viewer_count = dynamic_array_count(viewers);
    for (size_t i = 0; i < viewer_count; i++)
    {
        ViewerClient *c = *DYNAMIC_ARRAY_GET(ViewerClient **, viewers, i);
        respond((void *) notification, sizeof(NotViewerShellEvent), &c->network_client.address);
    }
}

void notify_shutdown(void)
{
    __shutdown_notifier(clients, req_bye);
    __shutdown_notifier(viewers, req_viewer_bye);
}

static void __shutdown_notifier(DynamicArray *a, uint8_t message)
{
    if (!a)
    {
        return;
    }

    while (dynamic_array_count(a))
    {
        NetworkClient *c = *DYNAMIC_ARRAY_GET(NetworkClient **, a, 0);
        dynamic_array_delete_at(a, 0);
        uint8_t response = message;
        respond((char *) &response, 1, &c->address);
        free(c);
    }
}

void get_global_lock(void)
{
    check(thrd_success == mtx_lock(&global_mutex), "Failed to lock global mutex.", "");
    error:
    return;
}

void release_global_lock(void)
{
    check(thrd_success == mtx_unlock(&global_mutex), "Failed to unlock global mutex.", "");
    error:
    return;
}

static void __clean(void)
{
    if (clients)
    {
        dynamic_array_destroy(clients);
        clients = NULL;
    }

    if (viewers)
    {
        dynamic_array_destroy(viewers);
        viewers = NULL;
    }

    mtx_destroy(&global_mutex);
}
