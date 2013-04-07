﻿// protocol.c - protocol definition.

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <threads.h>
#include <stdatomic.h>

#include "protocol.h"
#include "server.h"
#include "game.h"
#include "landscape.h"
#include "debug.h"

static inline bool __check_double(double v, double min, double max);

// Connecting.
static bool req_hello_executor(Client *c);
static bool req_bye_executor(Client *c);
static bool req_viewer_hello_executor(ViewerClient *c);
static bool req_viewer_bye_executor(ViewerClient *c);

// Tank control.
static bool req_set_engine_power_executor(Client *c);
static bool req_set_engine_power_validator(const void *packet, size_t packet_size);
static bool req_turn_executor(Client *c);
static bool req_turn_validator(const void *packet, size_t packet_size);
static bool req_look_at_executor(Client *c);
static bool req_look_at_validator(const void *packet, size_t packet_size);
static bool req_shoot_executor(Client *c);

// Tank telemetry.
static bool req_get_heading_executor(Client *c);
static bool req_get_speed_executor(Client *c);
static bool req_get_hp_executor(Client *c);

// Observing.
static bool req_get_map_executor(Client *c);
static bool req_get_normal_executor(Client *c);
static bool req_get_tanks_executor(Client *c);

// Viewing.
static bool req_viewer_get_map_executor(ViewerClient *c);
static bool req_viewer_get_tanks_executor(ViewerClient *c);

static PacketDefinition RequestDefinitions[] =
{
    // Connecting.
    { .id = req_hello, .validator = NULL, .executor = req_hello_executor, .is_client_protocol = true },
    { .id = req_bye, .validator = NULL, .executor = req_bye_executor, .is_client_protocol = true },
    { .id = req_viewer_hello, .validator = NULL, .executor = req_viewer_hello_executor, .is_client_protocol = false },
    { .id = req_viewer_bye, .validator = NULL, .executor = req_viewer_bye_executor, .is_client_protocol = false },

    // Tank control.
    { .id = req_set_engine_power, .validator = req_set_engine_power_validator, .executor = req_set_engine_power_executor, .is_client_protocol = true },
    { .id = req_turn, .validator = req_turn_validator, .executor = req_turn_executor, .is_client_protocol = true },
    { .id = req_look_at, .validator = req_look_at_validator, .executor = req_look_at_executor, .is_client_protocol = true },
    { .id = req_shoot, .validator = NULL, .executor = req_shoot_executor, .is_client_protocol = true },

    // Tank telemetry.
    { .id = req_get_heading, .validator = NULL, .executor = req_get_heading_executor, .is_client_protocol = true },
    { .id = req_get_speed, .validator = NULL, .executor = req_get_speed_executor, .is_client_protocol = true },
    { .id = req_get_hp, .validator = NULL, .executor = req_get_hp_executor, .is_client_protocol = true },

    // Observing.
    { .id = req_get_map, .validator = NULL, .executor = req_get_map_executor, .is_client_protocol = true },
    { .id = req_get_normal, .validator = NULL, .executor = req_get_normal_executor, .is_client_protocol = true },
    { .id = req_get_tanks, .validator = NULL, .executor = req_get_tanks_executor, .is_client_protocol = true },

    // Viewing.
    { .id = req_viewer_get_map, .validator = NULL, .executor = req_viewer_get_map_executor, .is_client_protocol = false },
    { .id = req_viewer_get_tanks, .validator = NULL, .executor = req_viewer_get_tanks_executor, .is_client_protocol = false }
};

static PacketDefinition *find_packet_by_id(uint8_t id)
{
    for (int i = 0; i < sizeof(RequestDefinitions) / sizeof(RequestDefinitions[0]); i++)
    {
        if (id == RequestDefinitions[i].id)
        {
            return &RequestDefinitions[i];
        }
    }

    return NULL;
}

void handle_packet(const char *packet, size_t packet_size,  const SOCKADDR *sender_address)
{
    check(packet_size && PACKET_BUFFER >= packet_size, "Bad packet size.", "");

    uint8_t id = packet[0];
    PacketDefinition *packet_definition = find_packet_by_id(id);

    if ((NULL == packet_definition && 1 == packet_size) ||
        (NULL != packet_definition && NULL != packet_definition->validator && !packet_definition->validator(packet, packet_size)))
    {
        uint8_t response = res_bad_request;
        respond((char *) &response, 1, sender_address);
        return;
    }

    Client *c = find_client_by_address(sender_address);
    ViewerClient *vc = find_viewer_by_address(sender_address);

    if (packet_definition->is_client_protocol)
    {
        if (NULL != vc)
        {
            uint8_t response = res_bad_request;
            respond((char *) &response, 1, sender_address);
            return;
        }

        if (NULL == c)
        {
            c = register_client(sender_address);
            if (NULL == c)
            {
                uint8_t response = res_too_many_clients;
                respond((char *) &response, 1, sender_address);
                return;
            }

            uint8_t response = req_hello;
            respond((char *) &response, 1, sender_address);
            return;
        }

        if (c->current_packet_definition)
        {
            uint8_t response = res_wait;
            respond((char *) &response, 1, sender_address);
            return;
        }

        memcpy(c->current_packet_buffer, packet, PACKET_BUFFER);
        c->current_packet_size = packet_size;
        c->current_packet_definition = packet_definition;
        enqueue_client(c);
    }
    else
    {
        if (NULL != c)
        {
            uint8_t response = res_bad_request;
            respond((char *) &response, 1, sender_address);
            return;
        }

        if (NULL == vc)
        {
            vc = register_viewer(sender_address);
            if (NULL == vc)
            {
                uint8_t response = res_too_many_clients;
                respond((char *) &response, 1, sender_address);
                return;
            }

            uint8_t response = req_viewer_hello;
            respond((char *) &response, 1, sender_address);
            return;
        }

        if (vc->current_packet_definition)
        {
            uint8_t response = res_wait;
            respond((char *) &response, 1, sender_address);
            return;
        }

        memcpy(vc->current_packet_buffer, packet, PACKET_BUFFER);
        vc->current_packet_size = packet_size;
        vc->current_packet_definition = packet_definition;
        enqueue_viewer(vc);
    }

    error:
    return;
}

// Connecting.
static bool req_hello_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (cs_connected == c->state)
    {
        c->state = cs_acknowledged;
    }

    uint8_t response = req_hello;
    respond((char *) &response, 1, &c->address);
    puts("Client connected.");
    return true;
}

static bool req_bye_executor(Client *c)
{
    assert(c && "Bad client pointer.");
    unregister_client(&c->address);
    uint8_t response = req_bye;
    respond((char *) &response, 1, &c->address);
    puts("Client disconnected.");
    return false;
}

static bool req_viewer_hello_executor(ViewerClient *c)
{
    assert(c && "Bad viewer client pointer.");

    if (cs_connected == c->state)
    {
        c->state = cs_acknowledged;
    }

    uint8_t response = req_viewer_hello;
    respond((char *) &response, 1, &c->address);
    puts("Viewer connected.");
    return true;
}

static bool req_viewer_bye_executor(ViewerClient *c)
{
    assert(c && "Bad viewer client pointer.");
    unregister_viewer(&c->address);
    uint8_t response = req_viewer_bye;
    respond((char *) &response, 1, &c->address);
    puts("Viewer disconnected.");
    return false;
}

// Tank control.
static bool req_set_engine_power_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    tank_set_engine_power(&c->tank, ((ReqSetEnginePower *) (&c->current_packet_buffer[1]))->engine_power);
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");

    uint8_t response = req_set_engine_power;
    respond((char *) &response, 1, &c->address);
    error:
    return true;
}

static bool req_set_engine_power_validator(const void *packet, size_t packet_size)
{
    assert(packet && "Bad packet data pointer.");
    return sizeof(ReqSetEnginePower) == packet_size - 1;
}

static bool req_turn_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    tank_turn(&c->tank, ((ReqTurn *) (&c->current_packet_buffer[1]))->turn_angle);
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");

    uint8_t response = req_turn;
    respond((char *) &response, 1, &c->address);
    error:
    return true;
}

static bool req_turn_validator(const void *packet, size_t packet_size)
{
    assert(packet && "Bad packet data pointer.");

    if (sizeof(ReqTurn) != packet_size - 1)
    {
        return false;
    }

    double turn_angle = ((ReqTurn *) (& ((const char *) packet)[1]))->turn_angle;
    return __check_double(turn_angle, -M_PI, M_PI);
}

static bool req_look_at_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->address);
        return true;
    }

    ReqLookAt *p = ((ReqLookAt *) (&c->current_packet_buffer[1]));
    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    tank_look_at(&c->tank, &(Vector) { .x = p->x, .y = p->y, .z = p->z });
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    uint8_t response = req_look_at;
    respond((char *) &response, 1, &c->address);
    error:
    return true;
}

static bool req_look_at_validator(const void *packet, size_t packet_size)
{
    assert(packet && "Bad packet data pointer.");

    if (sizeof(ReqLookAt) != packet_size - 1)
    {
        return false;
    }

    ReqLookAt *p = ((ReqLookAt *) (& ((const char *) packet)[1]));
    return __check_double(p->x, -1.0, 1.0) &&
           __check_double(p->y, -1.0, 1.0) &&
           __check_double(p->z, -1.0, 1.0);
}

static bool req_shoot_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    uint8_t response = tank_shoot(&c->tank) ? req_shoot : res_wait_shoot;
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    respond((char *) &response, 1, &c->address);
    error:
    return true;
}

// Tank telemetry.
static bool req_get_heading_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    ResGetHeading response = { .packet_id = req_get_heading, .heading = tank_get_heading(&c->tank) };
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    respond((char *) &response, sizeof(response), &c->address);
    error:
    return true;
}

static bool req_get_speed_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    ResGetSpeed response = { .packet_id = req_get_speed, .speed = c->tank.speed };
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    respond((char *) &response, sizeof(response), &c->address);
    error:
    return true;
}

static bool req_get_hp_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    ResGetHP response = { .packet_id = req_get_hp, .hp = c->tank.hp };
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    respond((char *) &response, sizeof(response), &c->address);
    error:
    return true;
}

// Observing.
static bool req_get_map_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->address);
        return true;
    }

    char response[1 + TANK_OBSERVING_RANGE * TANK_OBSERVING_RANGE * sizeof(double)];
    memset(response, 0, sizeof(response));
    response[0] = (uint8_t) req_get_map;

    double (*response_height_map)[TANK_OBSERVING_RANGE][TANK_OBSERVING_RANGE] =
        (double (*)[TANK_OBSERVING_RANGE][TANK_OBSERVING_RANGE]) (&response[1]);

    int t_x, t_y;
    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    landscape_get_tile(landscape, c->tank.position.x, c->tank.position.y, &t_x, &t_y);
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");

    for (int i = -TANK_OBSERVING_RANGE / 2; i < TANK_OBSERVING_RANGE / 2; i++)
    {
        for (int j = -TANK_OBSERVING_RANGE / 2; j < TANK_OBSERVING_RANGE / TANK_OBSERVING_RANGE; j++)
        {
            int l_y = t_y + i, l_x = t_x + j;
            if (l_y < 0 || l_y >= landscape->landscape_size ||
                l_x < 0 || l_x >= landscape->landscape_size)
            {
                continue;
            }

            (*response_height_map)[l_y][l_x] = landscape_get_height_at_node(landscape, l_y, l_x);
        }
    }

    respond(response, sizeof(response), &c->address);
    error:
    return true;
}

static bool req_get_normal_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->address);
        return true;
    }

    ResGetNormal response = { .packet_id = req_get_normal };
    Vector t;
    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    landscape_get_normal_at(landscape, c->tank.position.x, c->tank.position.y, &t);
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    response.x = t.x;
    response.y = t.y;
    response.z = t.z;
    respond((char *) &response, sizeof(response), &c->address);
    error:
    return true;
}

static bool req_get_tanks_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->address);
        return true;
    }

    char response[sizeof(ResGetTanks) + MAX_CLIENTS * sizeof(ResGetTanksTankRecord)];
    memset(response, 0, sizeof(response));

    ResGetTanks *response_header = (ResGetTanks *) response;
    response_header->packet_id = req_get_tanks;
    response_header->tanks_count = 0;

    ResGetTanksTankRecord *response_body = (ResGetTanksTankRecord *) (response + sizeof(ResGetTanks));

    dynamic_array_lock(clients);
    size_t clients_count = dynamic_array_count(clients);
    for (size_t i = 0; i < clients_count; i++)
    {
        Client *other_c = *DYNAMIC_ARRAY_GET(Client **, clients, i);

        if (other_c == c ||
            TANK_OBSERVING_RANGE * landscape->tile_size < vector_distance(&c->tank.position, &other_c->tank.position))
        {
            continue;
        }

        response_body->x = other_c->tank.position.x - c->tank.position.x;
        response_body->y = other_c->tank.position.y - c->tank.position.y;
        response_body->z = other_c->tank.position.z - c->tank.position.z;
        response_body->direction_x = other_c->tank.direction.x;
        response_body->direction_y = other_c->tank.direction.y;
        response_body->direction_z = other_c->tank.direction.z;
        response_body->orientation_x = other_c->tank.orientation.x;
        response_body->orientation_y = other_c->tank.orientation.y;
        response_body->orientation_z = other_c->tank.orientation.z;
        response_body->turret_x = other_c->tank.turret_direction.x;
        response_body->turret_y = other_c->tank.turret_direction.y;
        response_body->turret_z = other_c->tank.turret_direction.z;
        response_body->speed = other_c->tank.speed;
        response_body->target_turn = other_c->tank.turn_angle_target;
        response_body->team = other_c->tank.team;

        response_body++;
        response_header->tanks_count++;
    }
    dynamic_array_unlock(clients);

    respond((char *) &response, sizeof(ResGetTanks) + response_header->tanks_count * sizeof(ResGetTanksTankRecord), &c->address);
    return true;
}

static bool req_viewer_get_map_executor(ViewerClient *c)
{
    assert(c && "Bad viewer client pointer.");
    const size_t ls = landscape->landscape_size;
    char response[1 + 2 * sizeof(size_t) + ls * ls * sizeof(double)];
    memset(response, 0, sizeof(response));
    response[0] = (uint8_t) req_viewer_get_map;

    *((size_t *) &response[1]) = ls;
    *((size_t *) &response[1 + sizeof(size_t)]) = landscape->tile_size;
    memcpy(&response[1 + 2 * sizeof(size_t)], landscape->height_map, ls * ls * sizeof(double));

    respond(response, sizeof(response), &c->address);
    return true;
}

static bool req_viewer_get_tanks_executor(ViewerClient *c)
{
    assert(c && "Bad viewer client pointer.");
    char response[sizeof(ResGetTanks) + MAX_CLIENTS * sizeof(ResGetTanksTankRecord)];
    memset(response, 0, sizeof(response));

    ResGetTanks *response_header = (ResGetTanks *) response;
    response_header->packet_id = req_viewer_get_tanks;
    response_header->tanks_count = 0;

    ResGetTanksTankRecord *response_body = (ResGetTanksTankRecord *) (response + sizeof(ResGetTanks));

    dynamic_array_lock(clients);
    size_t clients_count = dynamic_array_count(clients);
    for (size_t i = 0; i < clients_count; i++, response_body++, response_header->tanks_count++)
    {
        Client *other_c = *DYNAMIC_ARRAY_GET(Client **, clients, i);

        *response_body = (ResGetTanksTankRecord) {
            .x               = other_c->tank.position.x,
            .y               = other_c->tank.position.y,
            .z               = other_c->tank.position.z,
            .direction_x     = other_c->tank.direction.x,
            .direction_y     = other_c->tank.direction.y,
            .direction_z     = other_c->tank.direction.z,
            .orientation_x   = other_c->tank.orientation.x,
            .orientation_y   = other_c->tank.orientation.y,
            .orientation_z   = other_c->tank.orientation.z,
            .turret_x        = other_c->tank.turret_direction.x,
            .turret_y        = other_c->tank.turret_direction.y,
            .turret_z        = other_c->tank.turret_direction.z,
            .target_turret_x = other_c->tank.turret_direction_target.x,
            .target_turret_y = other_c->tank.turret_direction_target.y,
            .target_turret_z = other_c->tank.turret_direction_target.z,
            .target_turn     = other_c->tank.turn_angle_target,
            .speed           = other_c->tank.speed,
            .team            = other_c->tank.team
        };
    }
    dynamic_array_unlock(clients);

    respond((char *) &response, sizeof(ResGetTanks) + response_header->tanks_count * sizeof(ResGetTanksTankRecord), &c->address);
    return true;
}

static inline bool __check_double(double v, double min, double max)
{
    return isfinite(v) && min <= v && v <= max;
}
