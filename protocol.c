﻿// protocol.c - protocol definition.

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <threads.h>
#include <stdatomic.h>

#include "protocol.h"
#include "protocol_utils.h"
#include "server.h"
#include "game.h"
#include "landscape.h"
#include "debug.h"

static void __packet_processor(NetworkClient *c,
                               const SOCKADDR *address,
                               NetworkClient *(*registrator)(const SOCKADDR *),
                               void *packet_buffer,
                               size_t packet_size,
                               const PacketDefinition *packet_definition,
                               void (*enqueuer)(const NetworkClient *),
                               uint8_t hello_packet);
static bool __check_double(double v, double min, double max);

// Connecting.
static bool __req_hello_executor(Client *c);
static bool __req_bye_executor(Client *c);
static bool __req_viewer_hello_executor(ViewerClient *c);
static bool __req_viewer_bye_executor(ViewerClient *c);

// Tank control.
static bool __req_set_engine_power_executor(Client *c);
static bool __req_set_engine_power_validator(const void *packet, size_t packet_size);
static bool __req_turn_executor(Client *c);
static bool __req_turn_validator(const void *packet, size_t packet_size);
static bool __req_look_at_executor(Client *c);
static bool __req_look_at_validator(const void *packet, size_t packet_size);
static bool __req_shoot_executor(Client *c);

// Tank telemetry.
static bool __req_get_heading_executor(Client *c);
static bool __req_get_speed_executor(Client *c);
static bool __req_get_hp_executor(Client *c);

// Observing.
static bool __req_get_map_executor(Client *c);
static bool __req_get_normal_executor(Client *c);
static bool __req_get_tanks_executor(Client *c);

// Viewing.
static bool __req_viewer_get_map_executor(ViewerClient *c);
static bool __req_viewer_get_tanks_executor(ViewerClient *c);

#pragma warn(push)
#pragma warn(disable: 2145)

static PacketDefinition RequestDefinitions[] =
{
    // Connecting.
    { .id = req_hello,            .validator = NULL,                             .executor = __req_hello_executor,            .is_client_protocol = true  },
    { .id = req_bye,              .validator = NULL,                             .executor = __req_bye_executor,              .is_client_protocol = true  },
    { .id = req_viewer_hello,     .validator = NULL,                             .executor = __req_viewer_hello_executor,     .is_client_protocol = false },
    { .id = req_viewer_bye,       .validator = NULL,                             .executor = __req_viewer_bye_executor,       .is_client_protocol = false },

    // Tank control.
    { .id = req_set_engine_power, .validator = __req_set_engine_power_validator, .executor = __req_set_engine_power_executor, .is_client_protocol = true  },
    { .id = req_turn,             .validator = __req_turn_validator,             .executor = __req_turn_executor,             .is_client_protocol = true  },
    { .id = req_look_at,          .validator = __req_look_at_validator,          .executor = __req_look_at_executor,          .is_client_protocol = true  },
    { .id = req_shoot,            .validator = NULL,                             .executor = __req_shoot_executor,            .is_client_protocol = true  },

    // Tank telemetry.
    { .id = req_get_heading,      .validator = NULL,                             .executor = __req_get_heading_executor,      .is_client_protocol = true  },
    { .id = req_get_speed,        .validator = NULL,                             .executor = __req_get_speed_executor,        .is_client_protocol = true  },
    { .id = req_get_hp,           .validator = NULL,                             .executor = __req_get_hp_executor,           .is_client_protocol = true  },

    // Observing.
    { .id = req_get_map,          .validator = NULL,                             .executor = __req_get_map_executor,          .is_client_protocol = true  },
    { .id = req_get_normal,       .validator = NULL,                             .executor = __req_get_normal_executor,       .is_client_protocol = true  },
    { .id = req_get_tanks,        .validator = NULL,                             .executor = __req_get_tanks_executor,        .is_client_protocol = true  },

    // Viewing.
    { .id = req_viewer_get_map,   .validator = NULL,                             .executor = __req_viewer_get_map_executor,   .is_client_protocol = false },
    { .id = req_viewer_get_tanks, .validator = NULL,                             .executor = __req_viewer_get_tanks_executor, .is_client_protocol = false }
};

#pragma warn(pop)

void handle_packet(const char *packet, size_t packet_size,  const SOCKADDR *sender_address)
{
    check(packet_size && PACKET_BUFFER >= packet_size, "Bad packet size.", "");

    uint8_t id = packet[0];
    const PacketDefinition *packet_definition = find_packet_by_id(RequestDefinitions, sizeof(RequestDefinitions) / sizeof(RequestDefinitions[0]), id);

    if ((NULL == packet_definition && 1 == packet_size) ||
        (NULL != packet_definition && NULL != packet_definition->validator && !packet_definition->validator(packet, packet_size)))
    {
        uint8_t response = res_bad_request;
        respond((char *) &response, 1, sender_address);
        return;
    }

    Client *c = find_client_by_address(sender_address);
    ViewerClient *vc = find_viewer_by_address(sender_address);

    if (c && vc ||
        packet_definition->is_client_protocol && NULL != vc ||
        !packet_definition->is_client_protocol && NULL != c)
    {
        uint8_t response = res_bad_request;
        respond((char *) &response, 1, sender_address);
        return;
    }

    if (packet_definition->is_client_protocol)
    {
        __packet_processor((NetworkClient *) c,
                           sender_address,
                           register_client,
                           (void *) packet,
                           packet_size,
                           packet_definition,
                           enqueue_client,
                           req_hello);
    }
    else
    {
        __packet_processor((NetworkClient *) vc,
                           sender_address,
                           register_viewer,
                           (void *) packet,
                           packet_size,
                           packet_definition,
                           enqueue_viewer,
                           req_viewer_hello);
    }

    error:
    return;
}

static void __packet_processor(NetworkClient *c,
                               const SOCKADDR *address,
                               NetworkClient *(*registrator)(const SOCKADDR *),
                               void *packet_buffer,
                               size_t packet_size,
                               const PacketDefinition *packet_definition,
                               void (*enqueuer)(const NetworkClient *),
                               uint8_t hello_packet)
{
    assert(address && "Bad address pointer.");
    assert(registrator && "Bad registrator callback.");
    assert(packet_buffer && "Bad packet buffer pointer.");
    assert(packet_size && "Bad packet size.");
    assert(packet_definition && "Bad packet definition pointer.");
    assert(enqueuer && "Bad enqueuer callback.");

    if (NULL == c)
    {
        c = registrator(address);
        if (NULL == c)
        {
            uint8_t response = res_too_many_clients;
            respond((char *) &response, 1, address);
            return;
        }

        uint8_t response = hello_packet;
        respond((char *) &response, 1, address);
        return;
    }

    if (c->current_packet_definition)
    {
        uint8_t response = res_wait;
        respond((char *) &response, 1, address);
        return;
    }

    memcpy(c->current_packet_buffer, packet_buffer, PACKET_BUFFER);
    c->current_packet_size = packet_size;
    c->current_packet_definition = packet_definition;
    enqueuer(c);
}

// Connecting.
static bool __req_hello_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (cs_connected == c->network_client.state)
    {
        c->network_client.state = cs_acknowledged;
    }

    uint8_t response = req_hello;
    respond((char *) &response, 1, &c->network_client.address);
    puts("Client connected.");
    return true;
}

static bool __req_bye_executor(Client *c)
{
    assert(c && "Bad client pointer.");
    uint8_t response = req_bye;
    respond((char *) &response, 1, &c->network_client.address);
    unregister_client(&c->network_client.address);
    puts("Client disconnected.");
    return false;
}

static bool __req_viewer_hello_executor(ViewerClient *c)
{
    assert(c && "Bad viewer client pointer.");

    if (cs_connected == c->network_client.state)
    {
        c->network_client.state = cs_acknowledged;
    }

    uint8_t response = req_viewer_hello;
    respond((char *) &response, 1, &c->network_client.address);
    puts("Viewer connected.");
    return true;
}

static bool __req_viewer_bye_executor(ViewerClient *c)
{
    assert(c && "Bad viewer client pointer.");
    uint8_t response = req_viewer_bye;
    respond((char *) &response, 1, &c->network_client.address);
    unregister_viewer(&c->network_client.address);
    puts("Viewer disconnected.");
    return false;
}

// Tank control.
static bool __req_set_engine_power_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->network_client.address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    tank_set_engine_power(&c->tank, ((ReqSetEnginePower *) (&c->network_client.current_packet_buffer[1]))->engine_power);
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");

    uint8_t response = req_set_engine_power;
    respond((char *) &response, 1, &c->network_client.address);
    error:
    return true;
}

static bool __req_set_engine_power_validator(const void *packet, size_t packet_size)
{
    assert(packet && "Bad packet data pointer.");
    return sizeof(ReqSetEnginePower) == packet_size - 1;
}

static bool __req_turn_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->network_client.address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    tank_turn(&c->tank, ((ReqTurn *) (&c->network_client.current_packet_buffer[1]))->turn_angle);
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");

    uint8_t response = req_turn;
    respond((char *) &response, 1, &c->network_client.address);
    error:
    return true;
}

static bool __req_turn_validator(const void *packet, size_t packet_size)
{
    assert(packet && "Bad packet data pointer.");

    if (sizeof(ReqTurn) != packet_size - 1)
    {
        return false;
    }

    double turn_angle = ((ReqTurn *) (& ((const char *) packet)[1]))->turn_angle;
    return __check_double(turn_angle, -M_PI, M_PI);
}

static bool __req_look_at_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->network_client.address);
        return true;
    }

    ReqLookAt *p = ((ReqLookAt *) (&c->network_client.current_packet_buffer[1]));
    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    tank_look_at(&c->tank, &(Vector) { .x = p->x, .y = p->y, .z = p->z });
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    uint8_t response = req_look_at;
    respond((char *) &response, 1, &c->network_client.address);
    error:
    return true;
}

static bool __req_look_at_validator(const void *packet, size_t packet_size)
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

static bool __req_shoot_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->network_client.address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    uint8_t response = tank_shoot(&c->tank) ? req_shoot : res_wait_shoot;
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    respond((char *) &response, 1, &c->network_client.address);
    error:
    return true;
}

// Tank telemetry.
static bool __req_get_heading_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->network_client.address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    ResGetHeading response = { .packet_id = req_get_heading, .heading = tank_get_heading(&c->tank) };
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    respond((char *) &response, sizeof(response), &c->network_client.address);
    error:
    return true;
}

static bool __req_get_speed_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->network_client.address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    ResGetSpeed response = { .packet_id = req_get_speed, .speed = c->tank.speed };
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    respond((char *) &response, sizeof(response), &c->network_client.address);
    error:
    return true;
}

static bool __req_get_hp_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->network_client.address);
        return true;
    }

    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    ResGetHP response = { .packet_id = req_get_hp, .hp = (uint8_t) c->tank.hp };
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    respond((char *) &response, sizeof(response), &c->network_client.address);
    error:
    return true;
}

// Observing.
static bool __req_get_map_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->network_client.address);
        return true;
    }

    char response[1 + TANK_OBSERVING_RANGE * TANK_OBSERVING_RANGE * sizeof(double)];
    memset(response, 0, sizeof(response));
    response[0] = (uint8_t) req_get_map;

    double (*response_height_map)[TANK_OBSERVING_RANGE][TANK_OBSERVING_RANGE] =
        (double (*)[TANK_OBSERVING_RANGE][TANK_OBSERVING_RANGE]) (&response[1]);

    size_t t_x, t_y;
    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    const Landscape *landscape = game_get_landscape();
    landscape_get_tile(landscape, c->tank.position.x, c->tank.position.y, &t_x, &t_y);
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");

    for (int i = -TANK_OBSERVING_RANGE / 2; i < TANK_OBSERVING_RANGE / 2; i++)
    {
        for (int j = -TANK_OBSERVING_RANGE / 2; j < TANK_OBSERVING_RANGE / 2; j++)
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

    respond(response, sizeof(response), &c->network_client.address);
    error:
    return true;
}

static bool __req_get_normal_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->network_client.address);
        return true;
    }

    ResGetNormal response = { .packet_id = req_get_normal };
    Vector t;
    check(thrd_success == mtx_lock(&c->tank.mtx), "Failed to lock tank mutex.", "");
    const Landscape *landscape = game_get_landscape();
    landscape_get_normal_at(landscape, c->tank.position.x, c->tank.position.y, &t);
    check(thrd_success == mtx_unlock(&c->tank.mtx), "Failed to unlock tank mutex.", "");
    response.x = t.x;
    response.y = t.y;
    response.z = t.z;
    respond((char *) &response, sizeof(response), &c->network_client.address);
    error:
    return true;
}

static bool __req_get_tanks_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (0 == c->tank.hp)
    {
        uint8_t response = res_dead;
        respond((char *) &response, 1, &c->network_client.address);
        return true;
    }

    char response[sizeof(ResGetTanks) + MAX_CLIENTS * sizeof(ResGetTanksTankRecord)];
    memset(response, 0, sizeof(response));

    ResGetTanks *response_header = (ResGetTanks *) response;
    response_header->packet_id = req_get_tanks;
    response_header->tanks_count = 0;

    ResGetTanksTankRecord *response_body = (ResGetTanksTankRecord *) (response + sizeof(ResGetTanks));

    const Landscape *landscape = game_get_landscape();
    DynamicArray *clients = server_get_clients();
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

        *response_body = (ResGetTanksTankRecord) {
            .x             = other_c->tank.position.x - c->tank.position.x,
            .y             = other_c->tank.position.y - c->tank.position.y,
            .z             = other_c->tank.position.z - c->tank.position.z,
            .direction_x   = other_c->tank.direction.x,
            .direction_y   = other_c->tank.direction.y,
            .direction_z   = other_c->tank.direction.z,
            .orientation_x = other_c->tank.orientation.x,
            .orientation_y = other_c->tank.orientation.y,
            .orientation_z = other_c->tank.orientation.z,
            .turret_x      = other_c->tank.turret_direction.x,
            .turret_y      = other_c->tank.turret_direction.y,
            .turret_z      = other_c->tank.turret_direction.z,
            .speed         = other_c->tank.speed,
            .team          = (uint8_t) other_c->tank.team
        };

        response_body++;
        response_header->tanks_count++;
    }
    dynamic_array_unlock(clients);

    respond((char *) &response,
            sizeof(ResGetTanks) + response_header->tanks_count * sizeof(ResGetTanksTankRecord),
            &c->network_client.address);
    return true;
}

static bool __req_viewer_get_map_executor(ViewerClient *c)
{
    assert(c && "Bad viewer client pointer.");
    const Landscape *landscape = game_get_landscape();
    const size_t ls = landscape->landscape_size;
    char response[1 + 2 * sizeof(size_t) + ls * ls * sizeof(double)];
    memset(response, 0, sizeof(response));
    response[0] = (uint8_t) req_viewer_get_map;

    *((size_t *) &response[1]) = ls;
    *((size_t *) &response[1 + sizeof(size_t)]) = landscape->tile_size;
    memcpy(&response[1 + 2 * sizeof(size_t)], landscape->height_map, ls * ls * sizeof(double));

    respond(response, sizeof(response), &c->network_client.address);
    return true;
}

static bool __req_viewer_get_tanks_executor(ViewerClient *c)
{
    assert(c && "Bad viewer client pointer.");
    char response[sizeof(ResGetTanks) + MAX_CLIENTS * sizeof(ResGetTanksTankRecord)];
    memset(response, 0, sizeof(response));

    ResGetTanks *response_header = (ResGetTanks *) response;
    response_header->packet_id = req_viewer_get_tanks;
    response_header->tanks_count = 0;

    ResGetTanksTankRecord *response_body = (ResGetTanksTankRecord *) (response + sizeof(ResGetTanks));

    DynamicArray *clients = server_get_clients();
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
            .team            = (uint8_t) other_c->tank.team,
            .hp              = (uint8_t) other_c->tank.hp
        };
    }
    dynamic_array_unlock(clients);

    respond((char *) &response,
            sizeof(ResGetTanks) + response_header->tanks_count * sizeof(ResGetTanksTankRecord),
            &c->network_client.address);
    return true;
}

static bool __check_double(double v, double min, double max)
{
    return isfinite(v) && min <= v && v <= max;
}
