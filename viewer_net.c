// viewer_net.c - viewer protocol implementation.

#include "debug.h"
#include "viewer.h"

static bool __bye_executor(const void *packet);
static bool __not_viewer_shell_event_validator(const void *packet, size_t packet_size);
static void __shell_event_executor(const void *packet, DynamicArray *notification_collection);
static bool __not_viewer_shoot_executor(const void *packet);
static bool __not_viewer_explosion_executor(const void *packet);

#pragma warn(push)
#pragma warn(disable: 2145)

PacketDefinition viewer_protocol_packets[] = {
    { .id = req_viewer_hello                                                                                                   },
    { .id = req_viewer_bye,                                                        .executor = __bye_executor                  },
    { .id = req_viewer_get_map                                                                                                 },
    { .id = req_viewer_get_tanks                                                                                               },
    { .id = not_viewer_shoot,     .validator = __not_viewer_shell_event_validator, .executor = __not_viewer_shoot_executor     },
    { .id = not_viewer_explosion, .validator = __not_viewer_shell_event_validator, .executor = __not_viewer_explosion_executor },
};

#pragma warn(pop)

ClientProtocol viewer_protocol = {
    .packets      = viewer_protocol_packets,
    .packet_count = sizeof(viewer_protocol_packets) / sizeof(viewer_protocol_packets[0]),
    .s            = INVALID_SOCKET,
    .connected    = false
};

static bool __bye_executor(const void *packet)
{
    #pragma ref unused
    working = false;
    return true;
}

static bool __not_viewer_shell_event_validator(const void *packet, size_t packet_size)
{
    assert(packet && "Bad packet body pointer.");

    if (sizeof(NotViewerShellEvent) != packet_size)
    {
        return false;
    }

    NotViewerShellEvent *p = (NotViewerShellEvent *) packet;

    return isnormal(p->x) && isnormal(p->y) && isnormal(p->z);
}

static bool __not_viewer_shoot_executor(const void *packet)
{
    assert(packet && "Bad packet body pointer.");
    printf("Got shoot!\n");
    __shell_event_executor(packet, viewer_get_shoots());
    return true;
}

static bool __not_viewer_explosion_executor(const void *packet)
{
    assert(packet && "Bad packet body pointer.");
    printf("Got explosion!\n");
    __shell_event_executor(packet, viewer_get_explosions());
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

    //printf("Shell event at: %lf; %lf; %lf\n", new_event->x, new_event->y, new_event->z);

    dynamic_array_push(notification_collection, &new_event);

    error:
    return;
}
