// client_net.h - client network interface.

#pragma once
#ifndef __CLIENT_NET_H__
#define __CLIENT_NET_H__

#include <stdbool.h>

#include <ws2tcpip.h>

#include "morrigan.h"
#include "landscape.h"
#include "protocol.h"

#define PORT 9000
#define CLIENT_PACKET_BUFFER 32768
#define MAX_CLIENTS 16
// In ms.
#define NET_TIMEOUT 10000

bool client_net_start(void);
void client_net_stop(void);

bool client_connect(SOCKET *s, const char *address, bool is_client);
bool client_disconnect(SOCKET *s, bool is_client);

// Viewer protocol.
Landscape *client_get_landscape(SOCKET *s);
size_t client_get_tanks(SOCKET *s, ResGetTanksTankRecord tanks[MAX_CLIENTS]);

// Client protocol.
bool set_engine_power(SOCKET *s, int engine_power);
bool turn(SOCKET *s, double turn_angle);
bool look_at(SOCKET *s, Vector *look_direction);

#endif /* __CLIENT_NET_H__ */
