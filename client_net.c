// client_net.c - client protocol implementation.

#include <assert.h>

#include "client_net.h"

PacketDefinition client_protocol[25] = {
    { .id = req_bye, .validator = NULL, .executor = NULL },
    { .id = req_set_engine_power, .validator = NULL, .executor = NULL },
    { .id = req_turn, .validator = NULL, .executor = NULL },
    { .id = req_look_at, .validator = NULL, .executor = NULL },
    { .id = req_shoot, .validator = NULL, .executor = NULL },
    { .id = req_get_heading, .validator = NULL, .executor = NULL },
    { .id = req_get_speed, .validator = NULL, .executor = NULL },
    { .id = req_get_hp, .validator = NULL, .executor = NULL },
    { .id = req_get_map, .validator = NULL, .executor = NULL },
    { .id = req_get_normal, .validator = NULL, .executor = NULL },
    { .id = req_get_tanks, .validator = NULL, .executor = NULL },
    { .id = res_bad_request, .validator = NULL, .executor = NULL },
    { .id = res_too_many_clients, .validator = NULL, .executor = NULL },
    { .id = res_wait, .validator = NULL, .executor = NULL },
    { .id = res_wait_shoot, .validator = NULL, .executor = NULL },
    { .id = res_dead, .validator = NULL, .executor = NULL },
    { .id = not_tank_hit_bound, .validator = NULL, .executor = NULL },
    { .id = not_tank_collision, .validator = NULL, .executor = NULL },
    { .id = not_near_shoot, .validator = NULL, .executor = NULL },
    { .id = not_death, .validator = NULL, .executor = NULL },
    { .id = not_hit, .validator = NULL, .executor = NULL },
    { .id = not_near_explosion, .validator = NULL, .executor = NULL },
    { .id = not_explosion_damage, .validator = NULL, .executor = NULL },
    { .id = not_viewer_shoot, .validator = NULL, .executor = NULL },
    { .id = not_viewer_explosion, .validator = NULL, .executor = NULL }
};

const PacketDefinition *find_packet_by_id(const PacketDefinition *protocol, size_t packet_count, uint8_t id)
{
    assert(protocol && packet_count && "Bad protocol definition.");

    for (int i = 0; i < packet_count; i++)
    {
        if (id == protocol[i].id)
        {
            return &protocol[i];
        }
    }

    return NULL;
}
