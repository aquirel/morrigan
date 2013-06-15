// genetic_client_net.cpp - genetic client protocol implementation.

#include <cassert>
#include <cstdio>
#include <string>

#include "genetic_client.hpp"
#include "genetic_client_net.hpp"

static bool __notification_executor(void *unused);

static bool __hit_bound_executor(void *unused);
static bool __tank_collision_executor(void *unused);
static bool __near_shoot_executor(void *packet_body);
static bool __near_explosion_executor(void *packet_body);
static bool __hit_executor(void *unused);
static bool __explosion_damage_executor(void *packet_body);

static bool __generic_executor(void *p);

PacketDefinition genetic_client_protocol_packets[] = {
    { .id = req_hello,            .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_bye,              .validator = NULL, .executor = __notification_executor,     .is_client_protocol = true },
    { .id = req_set_engine_power, .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_turn,             .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_look_at,          .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_shoot,            .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_get_heading,      .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_get_speed,        .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_get_hp,           .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_get_statistics,   .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_get_fire_delay,   .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_get_map,          .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_get_normal,       .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = req_get_tanks,        .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = res_bad_request,      .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = res_too_many_clients, .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = res_wait,             .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = res_wait_shoot,       .validator = NULL, .executor = NULL,                        .is_client_protocol = true },
    { .id = res_dead,             .validator = NULL, .executor = __notification_executor,     .is_client_protocol = true },
    { .id = not_tank_hit_bound,   .validator = NULL, .executor = __hit_bound_executor,        .is_client_protocol = true },
    { .id = not_tank_collision,   .validator = NULL, .executor = __tank_collision_executor,   .is_client_protocol = true },
    { .id = not_near_shoot,       .validator = NULL, .executor = __near_shoot_executor,       .is_client_protocol = true },
    { .id = not_death,            .validator = NULL, .executor = __notification_executor,     .is_client_protocol = true },
    { .id = not_win,              .validator = NULL, .executor = __notification_executor,     .is_client_protocol = true },
    { .id = not_hit,              .validator = NULL, .executor = __hit_executor,              .is_client_protocol = true },
    { .id = not_near_explosion,   .validator = NULL, .executor = __near_explosion_executor,   .is_client_protocol = true },
    { .id = not_explosion_damage, .validator = NULL, .executor = __explosion_damage_executor, .is_client_protocol = true }
};

ClientProtocol genetic_client_protocol = {
    .packets      = genetic_client_protocol_packets,
    .packet_count = sizeof(genetic_client_protocol_packets) / sizeof(genetic_client_protocol_packets[0]),
    .s            = INVALID_SOCKET,
    .connected    = false
};

static bool __notification_executor(void *unused)
{
    __generic_executor(unused);
    working = false;
    throw std::string("Execution terminated.");
    return false;
}

static bool __hit_bound_executor(void *unused)
{
    not_hit_bound_flag = true;
    return true;
}

static bool __tank_collision_executor(void *unused)
{
    not_tank_collision_flag = true;
    return true;
}

static bool __near_shoot_executor(void *packet_body)
{
    not_near_shoot_flag = true;
    NotViewerShellEvent *packet = (NotViewerShellEvent *) packet_body;
    not_near_shoot_position.x = packet->x;
    not_near_shoot_position.y = packet->y;
    not_near_shoot_position.z = packet->z;
    return true;
}

static bool __near_explosion_executor(void *packet_body)
{
    not_near_explosion_flag = true;
    NotViewerShellEvent *packet = (NotViewerShellEvent *) packet_body;
    not_near_explosion_position.x = packet->x;
    not_near_explosion_position.y = packet->y;
    not_near_explosion_position.z = packet->z;
    return true;
}

static bool __hit_executor(void *unused)
{
    not_hit_flag = true;
    return true;
}

static bool __explosion_damage_executor(void *packet_body)
{
    not_explosion_damage_flag = true;
    NotViewerShellEvent *packet = (NotViewerShellEvent *) packet_body;
    not_explosion_damage_position.x = packet->x;
    not_explosion_damage_position.y = packet->y;
    not_explosion_damage_position.z = packet->z;
    return true;
}

static bool __generic_executor(void *p)
{
    /*uint8_t packet_id = ((uint8_t *) p)[0];
    printf("Got packet. id: %u\n", (unsigned) packet_id);*/
    return true;
}
