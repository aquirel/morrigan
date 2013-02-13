// client_net.c - client network interface implementation.

#include <assert.h>
#include <stdint.h>

#include "debug.h"
#include "client_net.h"
#include "protocol.h"

bool __recv_timeout(SOCKET *s, char *buf, int len, int flags, int timeout, int *res);

bool client_net_start(void)
{
    WSADATA winsockData;
    check(0 == WSAStartup(MAKEWORD(2, 2), &winsockData), "Failed to initialize winsock. Error: %d.", WSAGetLastError());

    return true;

    error:
    WSACleanup();
    return false;
}

void client_net_stop(void)
{
    WSACleanup();
}

bool client_connect(SOCKET *s, const char *address, bool is_client)
{
    assert(s && "Bad socket pointer.");
    assert(address && "Bad address pointer.");

    *s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    check(INVALID_SOCKET != *s, "Failed to create socket. Error: %d.", WSAGetLastError());

    DWORD b = PACKET_BUFFER;
    check(SOCKET_ERROR != setsockopt(*s, SOL_SOCKET, SO_RCVBUF, (const char *) &b, sizeof(DWORD)),
          "Failed to socket set socket buffer. Error: %d.",
          WSAGetLastError());
    check(SOCKET_ERROR != setsockopt(*s, SOL_SOCKET, SO_SNDBUF, (const char *) &b, sizeof(DWORD)),
          "Failed to socket set socket buffer. Error: %d.",
          WSAGetLastError());

    struct addrinfo *result = NULL, hints = { .ai_family = AF_INET };
    check(0 == getaddrinfo(address, NULL, &hints, &result),
          "getaddrinfo() failed. Error: %d.",
          WSAGetLastError());

    ((SOCKADDR_IN *) result->ai_addr)->sin_port = htons(PORT);

    check(SOCKET_ERROR != connect(*s, result->ai_addr, result->ai_addrlen),
          "connect() failed. Error: %d.",
          WSAGetLastError());

    freeaddrinfo(result);
    result = NULL;

    uint8_t req, res;
    req = is_client ? req_hello : req_viewer_hello;

    check(SOCKET_ERROR != send(*s, &req, 1, 0), "send() failed. Error: %d.", WSAGetLastError());
    int received = 0;
    check(__recv_timeout(s, &res, 1, 0, NET_TIMEOUT, &received), "Net timeout.", "");
    check(1 == received && res == req, "Connecting failed (stage 1).", "");

    check(SOCKET_ERROR != send(*s, &req, 1, 0), "send() failed. Error: %d.", WSAGetLastError());
    received = 0;
    check(__recv_timeout(s, &res, 1, 0, NET_TIMEOUT, &received), "Net timeout.", "");
    check(1 == received && res == req, "Connecting failed (stage 2).", "");

    return true;

    error:
    if (INVALID_SOCKET != *s)
    {
        closesocket(*s);
    }

    if (!result)
    {
        freeaddrinfo(result);
    }

    return false;
}

bool client_disconnect(SOCKET *s, bool is_client)
{
    assert(s && "Bad socket pointer.");

    uint8_t req;
    req = is_client ? req_bye : req_viewer_bye;

    check(SOCKET_ERROR != send(*s, &req, 1, 0), "send() failed. Error: %d.", WSAGetLastError());
    closesocket(*s);

    return true;
    error:
    closesocket(*s);
    return false;
}

Landscape *client_get_landscape(SOCKET *s)
{
    assert(s && "Bad socket pointer.");

    uint8_t req = req_viewer_get_map;
    check(SOCKET_ERROR != send(*s, &req, 1, 0), "send() failed. Error: %d.", WSAGetLastError());

    char buf[CLIENT_PACKET_BUFFER];
    int received;
    check(__recv_timeout(s, buf, PACKET_BUFFER, 0, NET_TIMEOUT, &received), "Net timeout.", "");
    check(received > 1 + 2 * sizeof(size_t), "Bad map response.", "");

    check(req_viewer_get_map == (uint8_t) buf[0], "Bad map response.", "");

    size_t landscape_size = *((size_t *) &buf[1]),
           tile_size = *((size_t *) &buf[1 + sizeof(size_t)]);

    Landscape *l = landscape_create(landscape_size, tile_size);
    check_mem(l);

    memcpy(l->height_map, &buf[1 + 2 * sizeof(size_t)], landscape_size * landscape_size * sizeof(double));
    return l;

    error:
    return NULL;
}

size_t client_get_tanks(SOCKET *s, ResGetTanksTankRecord tanks[MAX_CLIENTS])
{
    assert(s && "Bad socket pointer.");

    uint8_t req = req_viewer_get_tanks;
    check(SOCKET_ERROR != send(*s, &req, 1, 0), "send() failed. Error: %d.", WSAGetLastError());

    char buf[CLIENT_PACKET_BUFFER];
    int received;
    check(__recv_timeout(s, buf, PACKET_BUFFER, 0, NET_TIMEOUT, &received), "Net timeout.", "");
    check(received >= 2 * sizeof(uint8_t), "Bad tanks response.", "");

    check(req_viewer_get_tanks == (uint8_t) buf[0], "Bad tanks response.", "");
    size_t tanks_count = (uint8_t) buf[1];

    check(received == 2 * sizeof(uint8_t) + tanks_count * sizeof(ResGetTanksTankRecord), "Bad tanks response (stage 2).", "");

    memcpy(tanks, &buf[2 * sizeof(uint8_t)], tanks_count * sizeof(ResGetTanksTankRecord));

    return tanks_count;

    error:
    return 0;
}

bool __recv_timeout(SOCKET *s, char *buf, int len, int flags, int timeout, int *res)
{
    fd_set set;
    FD_ZERO(&set);
    FD_SET(*s, &set);

    unsigned long long t = 1000 * timeout;

    struct timeval tv = { .tv_sec = t % 1000000, .tv_usec = t / 1000000 };

    switch (select(0, &set, NULL, NULL, &tv))
    {
        case SOCKET_ERROR:
            fprintf(stderr, "Socket error while select(). Error: %d", WSAGetLastError());
            return false;

        case 0:
            return false;
    }

    *res = recv(*s, buf, len, flags);
    return true;
}
