// server.h - server implementation.

#include <assert.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "debug.h"
#include "server.h"
#include "protocol.h"
#include "dynamic_array.h"
#include "ring_buffer.h"

static thrd_t worker_tid;
static volatile atomic_bool working = false;
static DynamicArray *clients = NULL;
static DynamicArray *viewers = NULL;
static RingBuffer *client_requests = NULL;
static RingBuffer *viewer_requests = NULL;
static cnd_t have_new_request_signal;
static mtx_t have_new_request_mutex;

static int __server_worker(void *unused);
static void __clean(void);

static NetworkClient *__client_finder_by_address(const SOCKADDR *address, DynamicArray *a);
static NetworkClient *__client_registrator(const SOCKADDR *address,
                                           DynamicArray *a,
                                           size_t max_count,
                                           size_t client_size);
static bool __client_unregistrator(const SOCKADDR *address, DynamicArray *a);
static void __client_enqueuer(const NetworkClient *c, RingBuffer *rb);
static void __shutdown_notifier(DynamicArray *a);

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
    check(thrd_success == thrd_create(&worker_tid, __server_worker, NULL), "Failed to start server worker thread.", "");

    return true;
    error:
    thrd_detach(worker_tid);
    __clean();

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
    __clean();

    fprintf(stderr, "server_stop end.\n");
}

DynamicArray *server_get_clients(void)
{
    return clients;
}

static void __clean(void)
{
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

    dynamic_array_lock(a);
    size_t count = dynamic_array_count(a);

    for (size_t i = 0; i < count; i++)
    {
        NetworkClient *c = *DYNAMIC_ARRAY_GET(NetworkClient **, a, i);
        if (0 == memcmp(address, &c->address, sizeof(SOCKADDR)))
        {
            dynamic_array_unlock(a);
            return c;
        }
    }
    dynamic_array_unlock(a);

    return NULL;
}

Client *register_client(const SOCKADDR *address)
{
    return (Client *) __client_registrator(address, clients, MAX_CLIENTS, sizeof(Client));
}

ViewerClient *register_viewer(const SOCKADDR *address)
{
    return (ViewerClient *) __client_registrator(address, viewers, MAX_VIEWERS, sizeof(ViewerClient));
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

    dynamic_array_lock(a);
    check(dynamic_array_push(a, &c), "Failed to add new client.", "");
    dynamic_array_unlock(a);

    return c;
    error:
    dynamic_array_unlock(a);
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

    dynamic_array_lock(a);
    size_t count = dynamic_array_count(a);

    for (size_t i = 0; i < count; i++)
    {
        NetworkClient *c = *DYNAMIC_ARRAY_GET(NetworkClient **, a, i);
        if (0 == memcmp(address, &c->address, sizeof(SOCKADDR)))
        {
            dynamic_array_delete_at(a, i);
            free(c);
            dynamic_array_unlock(a);
            return true;
        }
    }
    dynamic_array_unlock(a);

    return false;
}

void enqueue_client(const Client *c)
{
    __client_enqueuer((const NetworkClient *) c, client_requests);
}

void enqueue_viewer(const ViewerClient *c)
{
    __client_enqueuer((const NetworkClient *) c, viewer_requests);
}

static void __client_enqueuer(const NetworkClient *c, RingBuffer *rb)
{
    assert(c && "Bad client pointer.");
    assert(rb && "Bad client request ring buffer pointer.");

    ring_buffer_write(rb, &c);
    check(thrd_success == cnd_signal(&have_new_request_signal), "Failed to signal request condition variable.", "");
    error:
    return;
}

void notify_viewers(NotViewerShellEvent *notification)
{
    assert(notification && "Bad notification pointer.");

    dynamic_array_lock(viewers);
    size_t viewer_count = dynamic_array_count(viewers);
    for (size_t i = 0; i < viewer_count; i++)
    {
        ViewerClient *c = *DYNAMIC_ARRAY_GET(ViewerClient **, viewers, i);
        respond((void *) notification, sizeof(NotViewerShellEvent), &c->network_client.address);
    }
    dynamic_array_unlock(viewers);
}

void notify_shutdown(void)
{
    __shutdown_notifier(clients);
    __shutdown_notifier(viewers);
}

static void __shutdown_notifier(DynamicArray *a)
{
    if (!a)
    {
        return;
    }

    dynamic_array_lock(a);
    while (dynamic_array_count(a))
    {
        NetworkClient *c = *DYNAMIC_ARRAY_GET(NetworkClient **, a, 0);
        dynamic_array_delete_at(a, 0);
        uint8_t response = req_bye;
        respond((char *) &response, 1, &c->address);
        free(c);
    }
    dynamic_array_unlock(a);
}

static int __server_worker(void *unused)
{
    #pragma ref unused

    log_info("start. tid: %u", GetCurrentThreadId());
    check(thrd_success == mtx_lock(&have_new_request_mutex), "Failed to lock request mutex.", "");

    RingBuffer *buffers_to_monitor[] = { client_requests, viewer_requests };
    const size_t buffers_to_monitor_count = sizeof(buffers_to_monitor) / sizeof(buffers_to_monitor[0]);

    while (working)
    {
        check(thrd_success == cnd_wait(&have_new_request_signal, &have_new_request_mutex), "Failed to wait request signal.", "");

        size_t processed_requests;
        do
        {
            processed_requests = 0;
            for (size_t buffer_counter = 0; buffer_counter < buffers_to_monitor_count; buffer_counter++)
            {
                if (ring_buffer_is_empty(buffers_to_monitor[buffer_counter]))
                {
                    continue;
                }

                NetworkClient *c = *RING_BUFFER_READ(NetworkClient **, buffers_to_monitor[buffer_counter]);
                assert(c && "Bad client pointer.");
                assert(c->current_packet_definition && "Client doesn't have pending packet.");
                assert(c->current_packet_definition->executor && "No executor in packet definition.");

                if (c->current_packet_definition->executor(c))
                {
                    c->current_packet_definition = NULL;
                }

                processed_requests++;
            }
        } while (processed_requests);
    }

    return 0;
    error:
    return -1;
}
