// protocol.h - protocol definition.

#pragma once
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdint.h>
#include <stdbool.h>

#include <winsock2.h>

#include "morrigan.h"
#include "net.h"
#include "tank.h"

typedef enum Requests
{
    // Connecting.
    req_hello            = 0x00,
    req_bye              = 0x01,
    req_viewer_hello     = 0x03,
    req_viewer_bye       = 0x04,

    // Tank control.
    req_set_engine_power = 0x10,
    req_turn             = 0x11,
    req_look_at          = 0x12,
    req_shoot            = 0x13,

    // Tank telemetry.
    req_get_heading      = 0x20,
    req_get_speed        = 0x21,
    req_get_hp           = 0x22,

    // Observing.
    req_get_map          = 0x30,
    req_get_normal       = 0x31,
    req_get_tanks        = 0x32,

    // Viewing.
    req_viewer_get_map   = 0x40,
    req_viewer_get_tanks = 0x41
} Requests;

typedef enum Responses
{
    res_bad_request      = 0xf0,
    res_too_many_clients = 0xf3,
    res_wait             = 0xf4
} Responses;

typedef enum Notifications
{
    not_tank_hit_bound        = 0x80,
    not_tank_collision        = 0x81
} Notifications;

typedef bool (*packet_validation_handler)(const void *packet, size_t packet_size);
typedef bool (*packet_execution_handler)(void *c);

typedef struct PacketDefinition
{
    uint8_t id;
    packet_validation_handler validator;
    packet_execution_handler executor;
    bool is_client_protocol;
} PacketDefinition;

typedef enum ClientState
{
    cs_connected,
    cs_acknowledged,
    cs_in_game
} ClientState;

typedef struct Client
{
    ClientState state;
    SOCKADDR address;
    Tank tank;
    char current_packet_buffer[PACKET_BUFFER];
    size_t current_packet_size;
    PacketDefinition *current_packet_definition;
} Client;

typedef struct ViewerClient
{
    ClientState state;
    SOCKADDR address;
    char current_packet_buffer[PACKET_BUFFER];
    size_t current_packet_size;
    PacketDefinition *current_packet_definition;
} ViewerClient;

void handle_packet(const char *packet, size_t packet_size, const SOCKADDR *sender_address);

// Packet body definitions.
typedef struct ReqSetEnginePower
{
    int8_t engine_power;
} ReqSetEnginePower;

typedef struct ReqTurn
{
    double turn_angle;
} ReqTurn;

typedef struct ReqLookAt
{
    double x, y, z;
} ReqLookAt;

typedef struct ResGetHeading
{
    uint8_t packet_id;
    double heading;
} ResGetHeading;

typedef struct ResGetSpeed
{
    uint8_t packet_id;
    double speed;
} ResGetSpeed;

typedef struct ResGetHP
{
    uint8_t packet_id;
    uint8_t hp;
} ResGetHP;

typedef struct ResGetNormal
{
    uint8_t packet_id;
    double x, y, z;
} ResGetNormal;

typedef struct ResGetTanks
{
    uint8_t packet_id;
    uint8_t tanks_count;
} ResGetTanks;

typedef struct ResGetTanksTankRecord
{
    double x, y, z;
    double direction_x, direction_y, direction_z;
    double turret_x, turret_y, turret_z;
    double speed;
    uint8_t team;
} ResGetTanksTankRecord;

#endif /* __PROTOCOL_H__ */
