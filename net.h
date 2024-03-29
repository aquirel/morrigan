﻿// net.h - Network interface.

#pragma once
#ifndef __NET_H__
#define __NET_H__

//#pragma message("__NET_H__")

#include <stdbool.h>

#include <winsock2.h>

#include "morrigan.h"

#define PORT 9000
#define PACKET_BUFFER 32768

bool net_start(unsigned short port);
void net_stop(void);

void respond(const char *data, size_t data_length, const SOCKADDR *to);

#endif /* __NET_H__ */
