// client_protocol.h - client network interface.

#pragma once
#ifndef __CLIENT_PROTOCOL_H__
#define __CLIENT_PROTOCOL_H__

//#pragma message("__CLIENT_PROTOCOL_H__")

#include <stdbool.h>
#include <stdint.h>

#pragma warn(push)
#pragma warn(disable: 2185)
#include <ws2tcpip.h>
#pragma warn(pop)

#include "protocol.h"
#include "landscape.h"

#define PORT 9000
#define CLIENT_PACKET_BUFFER 65535
#define MAX_CLIENTS 16
// In ms.
#define NET_TIMEOUT 10000
#define NET_RETRIES 32

bool client_net_start(void);
void client_net_stop(void);

#pragma pack(push, 4)

typedef struct ClientProtocol
{
    const PacketDefinition *packets;
    size_t packet_count;
    SOCKET s;
    bool connected;
} ClientProtocol;

#pragma pack(pop)

bool client_protocol_process_event(ClientProtocol *cp);
uint8_t client_protocol_wait_for(ClientProtocol *cp, uint8_t target_packet_id, void *packet, size_t *length);

// Connecting / disconnecting.
bool client_connect(ClientProtocol *cp, const char *address, int port, bool is_client);
bool client_disconnect(ClientProtocol *cp, bool is_client);

// Viewer protocol.
Landscape *client_get_landscape(ClientProtocol *cp);
size_t client_get_tanks(ClientProtocol *cp, bool is_client, ResGetTanksTankRecord *tanks);

// Client protocol.
bool set_engine_power(ClientProtocol *cp, int engine_power);
bool turn(ClientProtocol *cp, double turn_angle);
bool look_at(ClientProtocol *cp, Vector *look_direction);
bool shoot(ClientProtocol *cp);

#endif /* __CLIENT_PROTOCOL_H__ */
