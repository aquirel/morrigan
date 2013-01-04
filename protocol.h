// protocol.h - protocol definition.

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <winsock2.h>

void handle_packet(const char *packet, const SOCKADDR *sender_address);

#endif
