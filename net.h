// net.h - Network interface.

#ifndef __NET_H__
#define __NET_H__

#define PORT 9000
#define PACKET_BUFFER 256

#include <stdbool.h>

#include <winsock2.h>

bool net_start(void);
void net_stop(void);

void respond(const char *data, size_t data_length, const SOCKADDR *to);

#endif
