// net.c - Network interface.

#include <stdbool.h>
#include <threads.h>
#include <stdatomic.h>

#include <winsock2.h>

#include "net.h"
#include "debug.h"
#include "protocol.h"
#include "server.h"

static thrd_t worker_tid;
static volatile atomic_bool working = false;
static SOCKET s = INVALID_SOCKET;

static int net_worker(void *unused);

bool net_start(void)
{
    WSADATA winsockData;
    check(0 == WSAStartup(MAKEWORD(2, 2), &winsockData), "Failed to initialize winsock. Error: %d.", WSAGetLastError());

    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    check(INVALID_SOCKET != s, "Failed to create socket. Error: %d.", WSAGetLastError());

    SOCKADDR_IN s_address = { .sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr.s_addr = htonl(INADDR_ANY) };
    check(SOCKET_ERROR != bind(s, (const SOCKADDR *) &s_address, sizeof(s_address)), "Failed to bind socket. Error: %d.", WSAGetLastError());

    working = true;
    check(thrd_success == thrd_create(&worker_tid, net_worker, NULL), "Failed to start network worker thread.", "");

    return true;

    error:
    WSACleanup();
    closesocket(s);
    thrd_detach(worker_tid);
    return false;
}

static int net_worker(void *unused)
{
    char buf[PACKET_BUFFER];
    SOCKADDR sender_address;
    int sender_address_size = sizeof(sender_address);

    while (working)
    {
        int res = recvfrom(s,
                           buf,
                           sizeof(buf),
                           0,
                           &sender_address,
                           &sender_address_size);

        if (!working)
        {
            break;
        }

        if (SOCKET_ERROR == res || 0 == res)
        {
            continue;
        }

        handle_packet(buf, res, &sender_address);
    }

    notify_shutdown();

    return 0;
}

void net_stop(void)
{
    working = false;
    shutdown(s, SD_RECEIVE);
    thrd_join(worker_tid, NULL);
    thrd_detach(worker_tid);
    closesocket(s);
    WSACleanup();
}

void respond(const char *data, size_t data_length, const SOCKADDR *to)
{
    check(SOCKET_ERROR != sendto(s, data, data_length, 0, to, sizeof(*to)),
          "Failed to send response to client. Error: %d.",
           WSAGetLastError());
    error:
    return;
}
