// genetic_client_net.cpp - genetic client protocol implementation.

#include <cassert>

#include "genetic_client.hpp"
#include "genetic_client_net.hpp"

static bool __notification_executor(void *unused);

// TODO: Set validators and executors.
PacketDefinition genetic_client_protocol_packets[] = {
    { .id = req_hello,            .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = req_bye,              .validator = NULL, .executor = __notification_executor, .is_client_protocol = true },
    { .id = req_set_engine_power, .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = req_turn,             .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = req_look_at,          .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = req_shoot,            .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = req_get_heading,      .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = req_get_speed,        .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = req_get_hp,           .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = req_get_map,          .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = req_get_normal,       .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = req_get_tanks,        .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = res_bad_request,      .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = res_too_many_clients, .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = res_wait,             .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = res_wait_shoot,       .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = res_dead,             .validator = NULL, .executor = __notification_executor, .is_client_protocol = true },
    { .id = not_tank_hit_bound,   .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = not_tank_collision,   .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = not_near_shoot,       .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = not_death,            .validator = NULL, .executor = __notification_executor, .is_client_protocol = true },
    { .id = not_win,              .validator = NULL, .executor = __notification_executor, .is_client_protocol = true },
    { .id = not_hit,              .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = not_near_explosion,   .validator = NULL, .executor = NULL,                    .is_client_protocol = true },
    { .id = not_explosion_damage, .validator = NULL, .executor = NULL,                    .is_client_protocol = true }
};

ClientProtocol genetic_client_protocol = {
    .packets      = genetic_client_protocol_packets,
    .packet_count = sizeof(genetic_client_protocol_packets) / sizeof(genetic_client_protocol_packets[0]),
    .s            = INVALID_SOCKET,
    .connected    = false
};

static bool __notification_executor(void *unused)
{
    working = false;
    return false;
}
