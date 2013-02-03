// client_net.h - client network interface.

#pragma once
#ifndef __CLIENT_NET_H__
#define __CLIENT_NET_H__

#include <stdbool.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "landscape.h"

#define PORT 9000
#define PACKET_BUFFER 256
// In ms.
#define NET_TIMEOUT 500

bool client_net_start(void);
void client_net_stop(void);

bool client_connect(SOCKET *s, const char *address, bool is_client);
bool client_disconnect(SOCKET *s, bool is_client);

const Landscape *client_get_landscape(SOCKET *s);

#endif /* __CLIENT_NET_H__ */
