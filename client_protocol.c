// client_protocol.c - client network interface implementation.

#include <assert.h>
#include <time.h>

#include "debug.h"
#include "tank_defines.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "client_protocol.h"

static bool __recv_timeout(SOCKET *s, char *buf, int length, int flags, int timeout, int *received);
static bool __process_packet(ClientProtocol *cp, void *buf, size_t length);
static void __assert_client_protocol(ClientProtocol *cp);

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

static bool __recv_timeout(SOCKET *s, char *buf, int length, int flags, int timeout, int *received)
{
    assert(s && "Bad socket pointer.");
    assert(buf && "Bad buffer pointer.");
    assert(length && "Bad buffer length.");
    assert(received && "Bad result length pointer.");

    fd_set set;
    FD_ZERO(&set);
    FD_SET(*s, &set);

    unsigned long t = 1000 * timeout;
    struct timeval tv = {
        .tv_sec = (time_t) (t / 1000000),
        .tv_usec = (long) (t % 1000000)
    };

    switch (select(0, &set, NULL, NULL, &tv))
    {
        case SOCKET_ERROR:
            fprintf(stderr, "Socket error while select(). Error: %d", WSAGetLastError());
            return false;

        case 0:
            return false;

        case 1:
        {
            int r = recv(*s, buf, length, flags);
            if (SOCKET_ERROR != r)
            {
                *received = r;
                return true;
            }

            printf("recv() failed. Code: %d.\n", WSAGetLastError());
            return false;
        }
    }

    return false;
}

static void __assert_client_protocol(ClientProtocol *cp)
{
    assert(cp && "Bad client protocol pointer.");
    assert(cp->packets && "Bad packets pointer.");
    assert(cp->packet_count && "Bad packet count.");
    //assert(INVALID_SOCKET != cp->s && cp->connected && "Bad protocol state.");
    assert(INVALID_SOCKET != cp->s && "Bad protocol state.");
}

static bool __process_packet(ClientProtocol *cp, void *buf, size_t length)
{
    __assert_client_protocol(cp);
    assert(buf && "Bad buffer pointer.");
    assert(length && "Bad buffer length.");

    if (!length)
    {
        log_warning("Empty packet!", "");
        return false;
    }

    uint8_t packet_id = ((uint8_t *) buf)[0];
    const PacketDefinition *target_packet = find_packet_by_id(cp->packets, cp->packet_count, packet_id);
    if (!target_packet)
    {
        log_warning("Unknown packet id: %u.", packet_id);
        return false;
    }

    if (target_packet->validator && !target_packet->validator(buf, length))
    {
        log_warning("Bad packet. id: %u.", packet_id);
        return false;
    }

    if (target_packet->executor)
    {
        target_packet->executor(buf);
    }

    return true;
}

bool client_protocol_process_event(ClientProtocol *cp)
{
    __assert_client_protocol(cp);

    char buf[CLIENT_PACKET_BUFFER];
    size_t received = 0;
    bool result = __recv_timeout(&cp->s, buf, CLIENT_PACKET_BUFFER, 0, 0, (int *) &received);
    if (!result)
    {
        return false;
    }

    __process_packet(cp, buf, received);
    return true;
}

uint8_t client_protocol_wait_for(ClientProtocol *cp, uint8_t target_packet_id, void *packet, size_t *length)
{
    __assert_client_protocol(cp);

    size_t retries = NET_RETRIES, received;
    char buf[CLIENT_PACKET_BUFFER];

    do
    {
        received = 0;
        bool result = __recv_timeout(&cp->s, buf, CLIENT_PACKET_BUFFER, 0, NET_TIMEOUT, (int *) &received);
        if (!result || !received)
        {
            continue;
        }

        uint8_t packet_id = buf[0];
        if (target_packet_id == packet_id ||
            res_bad_request == packet_id ||
            res_too_many_clients == packet_id ||
            res_wait == packet_id ||
            res_wait_shoot == packet_id ||
            res_dead == packet_id ||
            req_bye == packet_id ||
            req_viewer_bye == packet_id)
        {
            if (packet && length && *length)
            {
                memcpy(packet, buf, min(received, *length));
                *length = received;
            }

            if (target_packet_id != packet_id)
            {
                log_warning("Unexpected packet. id: %u", packet_id);
            }

            __process_packet(cp, buf, received);
            return packet_id;
        }

        __process_packet(cp, buf, received);
    } while (--retries);

    return false;
}

bool client_connect(ClientProtocol *cp, const char *address, unsigned short port, bool is_client)
{
    assert(cp && "Bad client protocol pointer.");
    assert(cp->packets && "Bad packets pointer.");
    assert(cp->packet_count && "Bad packet count.");
    assert(address && "Bad address pointer.");

    if (!port)
    {
        port = PORT;
    }

    cp->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    check(INVALID_SOCKET != cp->s, "Failed to create socket. Error: %d.", WSAGetLastError());

    DWORD b = PACKET_BUFFER;
    check(SOCKET_ERROR != setsockopt(cp->s, SOL_SOCKET, SO_RCVBUF, (const char *) &b, sizeof(DWORD)),
          "Failed to socket set socket buffer. Error: %d.",
          WSAGetLastError());
    check(SOCKET_ERROR != setsockopt(cp->s, SOL_SOCKET, SO_SNDBUF, (const char *) &b, sizeof(DWORD)),
          "Failed to socket set socket buffer. Error: %d.",
          WSAGetLastError());

    struct addrinfo *result = NULL, hints = { .ai_family = AF_INET };
    check(0 == getaddrinfo(address, NULL, &hints, &result),
          "getaddrinfo() failed. Error: %d.",
          WSAGetLastError());

    ((SOCKADDR_IN *) result->ai_addr)->sin_port = htons(port);

    check(SOCKET_ERROR != connect(cp->s, result->ai_addr, result->ai_addrlen),
          "connect() failed. Error: %d.",
          WSAGetLastError());

    freeaddrinfo(result);
    result = NULL;

    uint8_t req = is_client ? req_hello : req_viewer_hello;

    check(SOCKET_ERROR != send(cp->s, (char *) &req, 1, 0), "send() failed. Error: %d.", WSAGetLastError());
    check(req == client_protocol_wait_for(cp, req, NULL, NULL), "Connecting failed (stage 1).", "");

    check(SOCKET_ERROR != send(cp->s, (char *) &req, 1, 0), "send() failed. Error: %d.", WSAGetLastError());
    check(req == client_protocol_wait_for(cp, req, NULL, NULL), "Connecting failed (stage 2).", "");

    cp->connected = true;
    return true;

    error:
    if (INVALID_SOCKET != cp->s)
    {
        closesocket(cp->s);
    }

    if (result)
    {
        freeaddrinfo(result);
    }

    return false;
}

bool client_disconnect(ClientProtocol *cp, bool is_client)
{
    assert(cp && "Bad client protocol pointer.");
    assert(cp->packets && "Bad packets pointer.");
    assert(cp->packet_count && "Bad packet count.");

    uint8_t req = is_client ? req_bye : req_viewer_bye;

    check(SOCKET_ERROR != send(cp->s, (char *) &req, 1, 0), "send() failed. Error: %d.", WSAGetLastError());
    cp->connected = false;
    closesocket(cp->s);

    return true;
    error:
    closesocket(cp->s);
    return false;
}

Landscape *client_get_landscape(ClientProtocol *cp)
{
    __assert_client_protocol(cp);

    uint8_t req = req_viewer_get_map;
    check(SOCKET_ERROR != send(cp->s, (char *) &req, 1, 0), "send() failed. Error: %d.", WSAGetLastError());

    char buf[CLIENT_PACKET_BUFFER];
    size_t received = CLIENT_PACKET_BUFFER;
    check(req == client_protocol_wait_for(cp, req, buf, &received), "Net timeout.", "");

    check(received > 1 + 2 * sizeof(size_t), "Bad map response.", "");

    check(req_viewer_get_map == (uint8_t) buf[0], "Bad map response.", "");

    size_t landscape_size = *((size_t *) &buf[1]),
           tile_size = *((size_t *) &buf[1 + sizeof(size_t)]);

    Landscape *l = landscape_create(landscape_size, tile_size, 1.0);
    check_mem(l);

    memcpy(l->height_map, &buf[1 + 2 * sizeof(size_t)], landscape_size * landscape_size * sizeof(double));
    return l;

    error:
    return NULL;
}

size_t client_get_tanks(ClientProtocol *cp, bool is_client, ResGetTanksTankRecord *tanks)
{
    __assert_client_protocol(cp);
    assert(tanks && "Bad tanks pointer.");

    uint8_t req = is_client ? req_get_tanks : req_viewer_get_tanks;
    check(SOCKET_ERROR != send(cp->s, (char *) &req, 1, 0), "send() failed. Error: %d.", WSAGetLastError());

    char buf[CLIENT_PACKET_BUFFER];
    size_t received = CLIENT_PACKET_BUFFER;

    check(req == client_protocol_wait_for(cp, req, buf, &received), "Net timeout.", "");

    check(received >= 2 * sizeof(uint8_t), "Bad tanks response.", "");

    check(req == (uint8_t) buf[0], "Bad tanks response.", "");
    size_t tanks_count = (uint8_t) buf[1];

    check(received == 2 * sizeof(uint8_t) + tanks_count * sizeof(ResGetTanksTankRecord), "Bad tanks response (stage 2).", "");

    memcpy(tanks, &buf[2 * sizeof(uint8_t)], tanks_count * sizeof(ResGetTanksTankRecord));

    return tanks_count;

    error:
    return 0;
}

bool set_engine_power(ClientProtocol *cp, int engine_power)
{
    __assert_client_protocol(cp);

    char req_buf[1 + sizeof(ReqSetEnginePower)];
    req_buf[0] = req_set_engine_power;
    ReqSetEnginePower *req_body = (ReqSetEnginePower *) &req_buf[1];
    req_body->engine_power = (int8_t) engine_power;
    check(SOCKET_ERROR != send(cp->s, req_buf, sizeof(req_buf), 0), "send() failed. Error: %d.", WSAGetLastError());

    char receive_buf[1];
    size_t received = 1;

    check(req_set_engine_power == client_protocol_wait_for(cp, req_set_engine_power, &receive_buf, &received), "Net timeout.", "");

    check(1 == received && req_set_engine_power == receive_buf[0], "Bad engine power response.", "");

    return true;
    error:
    return false;
}

bool turn(ClientProtocol *cp, double turn_angle)
{
    __assert_client_protocol(cp);

    char req_buf[1 + sizeof(ReqTurn)];
    req_buf[0] = req_turn;
    ReqTurn *req_body = (ReqTurn *) &req_buf[1];
    req_body->turn_angle = turn_angle;
    check(SOCKET_ERROR != send(cp->s, req_buf, sizeof(req_buf), 0), "send() failed. Error: %d.", WSAGetLastError());

    char receive_buf[1];
    size_t received = 1;

    check(req_turn == client_protocol_wait_for(cp, req_turn, &receive_buf, &received), "Net timeout.", "");

    check(1 == received && req_turn == receive_buf[0], "Bad turn response.", "");

    return true;
    error:
    return false;
}

bool look_at(ClientProtocol *cp, Vector *look_direction)
{
    __assert_client_protocol(cp);

    char req_buf[1 + sizeof(ReqLookAt)];
    req_buf[0] = req_look_at;
    ReqLookAt *req_body = (ReqLookAt *) &req_buf[1];
    req_body->x = look_direction->x;
    req_body->y = look_direction->y;
    req_body->z = look_direction->z;
    check(SOCKET_ERROR != send(cp->s, req_buf, sizeof(req_buf), 0), "send() failed. Error: %d.", WSAGetLastError());

    char receive_buf[1];
    size_t received = 1;

    check(req_look_at == client_protocol_wait_for(cp, req_look_at, &receive_buf, &received), "Net timeout.", "");

    check(1 == received && req_look_at == receive_buf[0], "Bad look at response.", "");

    return true;
    error:
    return false;
}

bool shoot(ClientProtocol *cp)
{
    __assert_client_protocol(cp);

    uint8_t req = req_shoot;
    check(SOCKET_ERROR != send(cp->s, (char *) &req, sizeof(req), 0), "send() failed. Error: %d.", WSAGetLastError());

    char receive_buf[1];
    size_t received = 1;

    check(req == client_protocol_wait_for(cp, req, &receive_buf, &received), "Net timeout.", "");

    check(1 == received && req_shoot == receive_buf[0], "Bad shoot response.", "");

    return true;
    error:
    return false;
}

double client_tank_get_heading(ClientProtocol *cp)
{
    __assert_client_protocol(cp);

    uint8_t req = req_get_heading;
    check(SOCKET_ERROR != send(cp->s, (char *) &req, sizeof(req), 0), "send() failed. Error: %d.", WSAGetLastError());

    ResGetHeading receive_buf;
    size_t received = sizeof(receive_buf);

    check(req == client_protocol_wait_for(cp, req, &receive_buf, &received), "Net timeout.", "");

    check(sizeof(receive_buf) == received && req_get_heading == receive_buf.packet_id, "Bad get heading response.", "");

    return receive_buf.heading;
    error:
    return 0.0;
}

double tank_get_speed(ClientProtocol *cp)
{
    __assert_client_protocol(cp);

    uint8_t req = req_get_speed;
    check(SOCKET_ERROR != send(cp->s, (char *) &req, sizeof(req), 0), "send() failed. Error: %d.", WSAGetLastError());

    ResGetSpeed receive_buf;
    size_t received = sizeof(receive_buf);

    check(req == client_protocol_wait_for(cp, req, &receive_buf, &received), "Net timeout.", "");

    check(sizeof(receive_buf) == received && req_get_speed == receive_buf.packet_id, "Bad get speed response.", "");

    return receive_buf.speed;
    error:
    return 0.0;
}

uint8_t tank_get_hp(ClientProtocol *cp)
{
    __assert_client_protocol(cp);

    uint8_t req = req_get_hp;
    check(SOCKET_ERROR != send(cp->s, (char *) &req, sizeof(req), 0), "send() failed. Error: %d.", WSAGetLastError());

    ResGetHP receive_buf;
    size_t received = sizeof(receive_buf);

    check(req == client_protocol_wait_for(cp, req, &receive_buf, &received), "Net timeout.", "");

    check(sizeof(receive_buf) == received && req_get_hp == receive_buf.packet_id, "Bad get hp response.", "");

    return receive_buf.hp;
    error:
    return 0.0;
}

bool tank_get_statistics(ClientProtocol *cp, ResGetStatistics *statistics)
{
    __assert_client_protocol(cp);
    assert(statistics && "Bad statistics pointer.");

    uint8_t req = req_get_statistics;
    check(SOCKET_ERROR != send(cp->s, (char *) &req, sizeof(req), 0), "send() failed. Error: %d.", WSAGetLastError());

    ResGetStatistics receive_buf;
    size_t received = sizeof(receive_buf);

    check(req == client_protocol_wait_for(cp, req, &receive_buf, &received), "Net timeout.", "");

    check(sizeof(receive_buf) == received && req_get_statistics == receive_buf.packet_id, "Bad get statistics response.", "");

    *statistics = receive_buf;
    return true;
    error:
    return false;
}

bool tank_get_map(ClientProtocol *cp, double *m)
{
    __assert_client_protocol(cp);
    assert(m && "Bad map pointer.");

    uint8_t req = req_get_map;
    check(SOCKET_ERROR != send(cp->s, (char *) &req, sizeof(req), 0), "send() failed. Error: %d.", WSAGetLastError());

    char buf[CLIENT_PACKET_BUFFER];
    size_t received = CLIENT_PACKET_BUFFER;

    check(req == client_protocol_wait_for(cp, req, buf, &received), "Net timeout.", "");

    check(sizeof(uint8_t) + sizeof(double) * TANK_OBSERVING_RANGE * TANK_OBSERVING_RANGE * sizeof(uint8_t) == received && req_get_map == buf[0], "Bad get hp response.", "");

    double scale = *((double *) &buf[1]);
    for (size_t i = 0; i < TANK_OBSERVING_RANGE * TANK_OBSERVING_RANGE; i++)
    {
        m[i] = scale * (*((uint8_t *) &buf[1 + sizeof(double) + i]));
    }

    return true;
    error:
    return false;
}

bool tank_get_normal(ClientProtocol *cp, Vector *normal)
{
    __assert_client_protocol(cp);
    assert(normal && "Bad normal vector pointer.");

    uint8_t req = req_get_normal;
    check(SOCKET_ERROR != send(cp->s, (char *) &req, sizeof(req), 0), "send() failed. Error: %d.", WSAGetLastError());

    ResGetNormal receive_buf;
    size_t received = sizeof(receive_buf);

    check(req == client_protocol_wait_for(cp, req, &receive_buf, &received), "Net timeout.", "");

    check(sizeof(receive_buf) == received && req_get_normal == receive_buf.packet_id, "Bad get hp response.", "");

    normal->x = receive_buf.x;
    normal->y = receive_buf.y;
    normal->z = receive_buf.z;

    return true;
    error:
    return false;
}

int tank_get_tanks(ClientProtocol *cp, ResGetTanksTankRecord *tanks, size_t tanks_count)
{
    __assert_client_protocol(cp);
    assert(tanks && "Bad tanks buffer pointer.");
    assert(tanks_count && "Bad tanks count.");

    uint8_t req = req_get_tanks;
    check(SOCKET_ERROR != send(cp->s, (char *) &req, sizeof(req), 0), "send() failed. Error: %d.", WSAGetLastError());

    char buf[CLIENT_PACKET_BUFFER];
    size_t received = CLIENT_PACKET_BUFFER;

    check(req == client_protocol_wait_for(cp, req, buf, &received), "Net timeout.", "");
    check(req_get_tanks == buf[0], "Bad get hp response.", "");
    if (received < sizeof(ResGetTanks))
    {
        return 0;
    }

    ResGetTanks *header = (ResGetTanks *) buf;
    ResGetTanksTankRecord *response_body = (ResGetTanksTankRecord *) (buf + sizeof(ResGetTanks));

    size_t tanks_count_to_copy = min(header->tanks_count, tanks_count);
    memcpy(tanks, response_body, tanks_count_to_copy * sizeof(ResGetTanksTankRecord));

    return tanks_count_to_copy;
    error:
    return 0;
}
