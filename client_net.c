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

    struct addrinfo *result = NULL, hints = { .ai_family = AF_INET };
    check(0 == getaddrinfo(address, NULL, NULL, &result),
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

const Landscape *client_get_landscape(SOCKET *s);

bool __recv_timeout(SOCKET *s, char *buf, int len, int flags, int timeout, int *res)
{
    fd_set set;
    FD_ZERO(&set);
    FD_SET(*s, &set);

    struct timeval tv = { .tv_sec = 0, .tv_usec = 1000 * timeout };

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
