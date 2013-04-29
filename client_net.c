// client_net.c - client protocol implementation.

#include <assert.h>

#include "client_net.h"

PacketDefinition client_protocol_packets[] = {
    { .id = req_bye              },
    { .id = req_set_engine_power },
    { .id = req_turn             },
    { .id = req_look_at          },
    { .id = req_shoot            },
    { .id = req_get_heading      },
    { .id = req_get_speed        },
    { .id = req_get_hp           },
    { .id = req_get_map          },
    { .id = req_get_normal       },
    { .id = req_get_tanks        },
    { .id = res_bad_request      },
    { .id = res_too_many_clients },
    { .id = res_wait             },
    { .id = res_wait_shoot       },
    { .id = res_dead             },
    { .id = not_tank_hit_bound   },
    { .id = not_tank_collision   },
    { .id = not_near_shoot       },
    { .id = not_death            },
    { .id = not_hit              },
    { .id = not_near_explosion   },
    { .id = not_explosion_damage },
    { .id = not_viewer_shoot     },
    { .id = not_viewer_explosion }
};

ClientProtocol client_protocol = {
    .packets      = client_protocol_packets,
    .packet_count = sizeof(client_protocol_packets) / sizeof(client_protocol_packets[0]),
    .s            = INVALID_SOCKET,
    .connected    = false
};
