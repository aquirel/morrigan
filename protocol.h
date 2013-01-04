// protocol.h - protocol definition.

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdint.h>

#include <winsock2.h>

#include "net.h"

typedef enum Requests
{
    req_hello = 0x00,
    req_bye   = 0x01
} Requests;

typedef enum Responses
{
    res_hello            = 0x00,
    res_bye              = 0x01,
    res_too_many_clients = 0x03,
    res_wait             = 0x04,
    res_bad_request      = 0x80
} Responses;

typedef struct Client Client;

typedef bool (*packet_validation_handler)(const void *packet);
typedef void (*packet_execution_handler)(Client *c);

typedef struct PacketDefinition
{
    uint8_t id;
    packet_validation_handler validator;
    packet_execution_handler executor;
} PacketDefinition;

typedef enum ClientState
{
    cs_connected,
    cs_acknowledged,
    cs_in_game
} ClientState;

typedef struct Client
{
    ClientState state;
    SOCKADDR address;
    //tank.
    char current_packet_buffer[PACKET_BUFFER];
    PacketDefinition *current_packet_definition;
} Client;

void handle_packet(const char *packet, const SOCKADDR *sender_address);

#endif
