// net.h - Network interface.

#pragma once
#ifndef __NET_H__
#define __NET_H__

#include <stdbool.h>

#include <winsock2.h>

#define PORT 9000
#define PACKET_BUFFER 256

bool net_start(void);
void net_stop(void);

void respond(const char *data, size_t data_length, const SOCKADDR *to);

#endif /* __NET_H__ */
