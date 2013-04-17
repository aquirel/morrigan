// viewer_net.c - viewer protocol implementation.

#include "debug.h"
#include "viewer.h"

bool not_viewer_shell_event_validator(const void *packet, size_t packet_size);
bool not_viewer_shoot_executor(const void *packet);
bool not_viewer_explosion_executor(const void *packet);

static void __shell_event_executor(const void *packet, DynamicArray *notification_collection);

#pragma warn(push)
#pragma warn(disable: 2145)

PacketDefinition viewer_protocol[6] = {
    { .id = req_viewer_hello },
    { .id = req_viewer_bye },
    { .id = req_viewer_get_map },
    { .id = req_viewer_get_tanks },
    { .id = not_viewer_shoot, .validator = not_viewer_shell_event_validator, .executor = not_viewer_shoot_executor },
    { .id = not_viewer_explosion, .validator = not_viewer_shell_event_validator, .executor = not_viewer_explosion_executor }
};

#pragma warn(pop)

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

bool not_viewer_shell_event_validator(const void *packet, size_t packet_size)
{
    if (sizeof(NotViewerShellEvent) != packet_size)
    {
        return false;
    }

    NotViewerShellEvent *p = (NotViewerShellEvent *) packet;

    return isnormal(p->x) && isnormal(p->y) && isnormal(p->z);
}

bool not_viewer_shoot_executor(const void *packet)
{
    printf("Got shoot!\n");
    __shell_event_executor(packet, shoots);
    return true;
}

bool not_viewer_explosion_executor(const void *packet)
{
    printf("Got explosion!\n");
    __shell_event_executor(packet, explosions);
    return true;
}

static void __shell_event_executor(const void *packet, DynamicArray *notification_collection)
{
    assert(packet && "Bad packet body pointer.");
    assert(notification_collection && "Bad notification collection pointer.");

    NotViewerShellEvent *p = (NotViewerShellEvent *) packet,
                        *new_event = NULL;

    check_mem(new_event = calloc(1, sizeof(NotViewerShellEvent)));
    new_event->x = p->x;
    new_event->y = p->y;
    new_event->z = p->z;

    printf("Shell event at: %lf; %lf; %lf\n", new_event->x, new_event->y, new_event->z);

    dynamic_array_push(notification_collection, &new_event);

    error:
    return;
}
