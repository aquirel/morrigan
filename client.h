// client.h - client definition.

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "net.h"

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
    char current_packet[PACKET_BUFFER];
    bool has_packet;
} Client;

#endif
