// client_protocol.h - client network interface.

#pragma once
#ifndef __CLIENT_PROTOCOL_H__
#define __CLIENT_PROTOCOL_H__

#pragma message("__CLIENT_PROTOCOL_H__")

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

bool client_protocol_process_event(SOCKET *s, const PacketDefinition *packets, size_t packet_count);
int client_protocol_wait_for(SOCKET *s, const PacketDefinition *packets, size_t packet_count, uint8_t target_packet_id, void *packet, size_t *length);

// Connecting / disconnecting.
bool client_connect(SOCKET *s, const char *address, bool is_client, const PacketDefinition *packets, size_t packet_count);
bool client_disconnect(SOCKET *s, bool is_client);

// Viewer protocol.
Landscape *client_get_landscape(SOCKET *s, const PacketDefinition *packets, size_t packet_count);
size_t client_get_tanks(bool is_client, SOCKET *s, ResGetTanksTankRecord *tanks, const PacketDefinition *packets, size_t packet_count);

// Client protocol.
bool set_engine_power(SOCKET *s, int engine_power, const PacketDefinition *packets, size_t packet_count);
bool turn(SOCKET *s, double turn_angle, const PacketDefinition *packets, size_t packet_count);
bool look_at(SOCKET *s, Vector *look_direction, const PacketDefinition *packets, size_t packet_count);
bool shoot(SOCKET *s, const PacketDefinition *packets, size_t packet_count);

#endif /* __CLIENT_PROTOCOL_H__ */
