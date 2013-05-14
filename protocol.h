// protocol.h - protocol definition.

#pragma once
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

//#pragma message("__PROTOCOL_H__")

#include <stdint.h>
#include <stdbool.h>

#include <winsock2.h>

#include "morrigan.h"
#include "net.h"

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
    req_get_statistics   = 0x23,

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
    res_wait             = 0xf4,
    res_wait_shoot       = 0xf5,
    res_dead             = 0xf6
} Responses;

typedef enum Notifications
{
    not_tank_hit_bound        = 0x80,
    not_tank_collision        = 0x81,
    not_near_shoot            = 0x82,
    not_death                 = 0x83,
    not_win                   = 0x84,
    not_hit                   = 0x85,
    not_near_explosion        = 0x86,
    not_explosion_damage      = 0x87,
    not_viewer_shoot          = 0x88,
    not_viewer_explosion      = 0x89
} Notifications;

typedef bool (*packet_validation_handler)(const void *packet, size_t packet_size);
typedef bool (*packet_execution_handler)(void *c);

#pragma pack(push, 4)

typedef struct PacketDefinition
{
    uint8_t id;
    packet_validation_handler validator;
    packet_execution_handler executor;
    bool is_client_protocol;
} PacketDefinition;

#pragma pack(pop)

void handle_packet(const char *packet, size_t packet_size, const SOCKADDR *sender_address);

#pragma pack(push, 1)
#pragma warn(push)
#pragma warn(disable: 2185)

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

typedef struct ResGetStatistics
{
    uint8_t packet_id;
    unsigned long long ticks;
    size_t hp;
    size_t direct_hits;
    size_t hits;
    size_t got_direct_hits;
    size_t got_hits;
} ResGetStatistics;

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
    double orientation_x, orientation_y, orientation_z;
    double turret_x, turret_y, turret_z;
    double target_turret_x, target_turret_y, target_turret_z;
    double target_turn;
    double speed;
    uint8_t team;
    uint8_t hp;
} ResGetTanksTankRecord;

typedef struct NotViewerShellEvent
{
    uint8_t type;
    double x, y, z;
} NotViewerShellEvent;

#pragma warn(pop)
#pragma pack(pop)

#endif /* __PROTOCOL_H__ */
