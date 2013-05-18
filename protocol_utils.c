// protocol_utils.c - protocol helpers.

#include <assert.h>

#include "protocol_utils.h"

const PacketDefinition *find_packet_by_id(const PacketDefinition *protocol, size_t packet_count, uint8_t id)
{
    assert(protocol && packet_count && "Bad protocol definition.");

    for (size_t i = 0; i < packet_count; i++)
    {
        if (id == protocol[i].id)
        {
            return &protocol[i];
        }
    }

    return NULL;
}
