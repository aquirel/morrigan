// protocol.c - protocol definition.

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <threads.h>
#include <stdatomic.h>

#include "protocol.h"
#include "server.h"
#include "debug.h"

// Connecting.
static void req_hello_executor(Client *c);
static void req_bye_executor(Client *c);

// Tank control.
static void req_set_engine_power_executor(Client *c);
static bool req_set_engine_power_validator(const void *packet, size_t packet_size);
static void req_turn_executor(Client *c);
static bool req_turn_validator(const void *packet, size_t packet_size);
static void req_look_at_executor(Client *c);
static bool req_look_at_validator(const void *packet, size_t packet_size);
static void req_shoot_executor(Client *c);

// Tank telemetry.
static void req_get_heading_executor(Client *c);
static void req_get_speed_executor(Client *c);
static void req_get_hp_executor(Client *c);

// Observing.
static void req_get_map_executor(Client *c);
static void req_get_tanks_executor(Client *c);

static PacketDefinition RequestDefinitions[] =
{
    // Connecting.
    { .id = req_hello, .validator = NULL, .executor = req_hello_executor },
    { .id = req_bye, .validator = NULL, .executor = req_bye_executor },

    // Tank control.
    { .id = req_set_engine_power, .validator = req_set_engine_power_validator, .executor = req_set_engine_power_executor },
    { .id = req_turn, .validator = req_turn_validator, .executor = req_turn_executor },
    { .id = req_look_at, .validator = req_look_at_validator, .executor = req_look_at_executor },
    { .id = req_shoot, .validator = NULL, .executor = req_shoot_executor },

    // Tank telemetry.
    { .id = req_get_heading, .validator = NULL, .executor = req_get_heading_executor },
    { .id = req_get_speed, .validator = NULL, .executor = req_get_speed_executor },
    { .id = req_get_hp, .validator = NULL, .executor = req_get_hp_executor },

    // Observing.
    { .id = req_get_map, .validator = NULL, .executor = req_get_map_executor },
    { .id = req_get_tanks, .validator = NULL, .executor = req_get_tanks_executor }
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

    error:
    return;
}

// Connecting.
static void req_hello_executor(Client *c)
{
    assert(c && "Bad client pointer.");

    if (cs_connected == c->state)
    {
        c->state = cs_acknowledged;
    }

    uint8_t response = req_hello;
    respond((char *) &response, 1, &c->address);
}

static void req_bye_executor(Client *c)
{
    assert(c && "Bad client pointer.");
    unregister_client(&c->address);
    uint8_t response = req_bye;
    respond((char *) &response, 1, &c->address);
}

// Tank control.
static void req_set_engine_power_executor(Client *c)
{
    assert(c && "Bad client pointer.");
    tank_set_engine_power(&c->tank, ((ReqSetEnginePower *) (&c->current_packet_buffer[1]))->engine_power);
    uint8_t response = req_set_engine_power;
    respond((char *) &response, 1, &c->address);
}

static bool req_set_engine_power_validator(const void *packet, size_t packet_size)
{
    assert(packet && "Bad packet data pointer.");
    return sizeof(ReqSetEnginePower) == packet_size - 1;
}

static void req_turn_executor(Client *c)
{
    assert(c && "Bad client pointer.");
    tank_turn(&c->tank, ((ReqTurn *) (&c->current_packet_buffer[1]))->turn_angle);
    uint8_t response = req_turn;
    respond((char *) &response, 1, &c->address);
}

static bool req_turn_validator(const void *packet, size_t packet_size)
{
    assert(packet && "Bad packet data pointer.");
    return sizeof(ReqTurn) == packet_size - 1;
}

static void req_look_at_executor(Client *c)
{
    assert(c && "Bad client pointer.");
    ReqLookAt *p = ((ReqLookAt *) (&c->current_packet_buffer[1]));
    tank_look_at(&c->tank, &(Vector) { .x = p->x, .y = p->y, .z = p->z });
    uint8_t response = req_look_at;
    respond((char *) &response, 1, &c->address);
}

static bool req_look_at_validator(const void *packet, size_t packet_size)
{
    assert(packet && "Bad packet data pointer.");
    return sizeof(ReqLookAt) == packet_size - 1;
}

static void req_shoot_executor(Client *c)
{
    assert(c && "Bad client pointer.");
    uint8_t response = req_shoot;
    respond((char *) &response, 1, &c->address);
}

// Tank telemetry.
static void req_get_heading_executor(Client *c)
{
    assert(c && "Bad client pointer.");
    ResGetHeading response = { .packet_id = req_get_heading, .heading = tank_get_heading(&c->tank) };
    respond((char *) &response, sizeof(response), &c->address);
}

static void req_get_speed_executor(Client *c)
{
    assert(c && "Bad client pointer.");
    ResGetSpeed response = { .packet_id = req_get_speed, .speed = c->tank.speed };
    respond((char *) &response, sizeof(response), &c->address);
}

static void req_get_hp_executor(Client *c)
{
    assert(c && "Bad client pointer.");
    ResGetHP response = { .packet_id = req_get_hp, .hp = c->tank.hp };
    respond((char *) &response, sizeof(response), &c->address);
}

// Observing.
static void req_get_map_executor(Client *c)
{
    assert(c && "Bad client pointer.");
}

static void req_get_tanks_executor(Client *c)
{
    assert(c && "Bad client pointer.");
}
