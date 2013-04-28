// protocol_utils.h - protocol helpers.

#pragma once
#ifndef __PROTOCOL_UTILS_H__
#define __PROTOCOL_UTILS_H__

//#pragma message("__PROTOCOL_UTILS_H__")

#include "morrigan.h"
#include "protocol.h"

const PacketDefinition *find_packet_by_id(const PacketDefinition *protocol, size_t packet_count, uint8_t id);

#endif /* __PROTOCOL_UTILS_H__ */

