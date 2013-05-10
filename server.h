// server.h - server implementation.

#pragma once
#ifndef __SERVER_H__
#define __SERVER_H__

//#pragma message("__SERVER_H__")

#include <winsock2.h>

#include "morrigan.h"
#include "protocol.h"
#include "dynamic_array.h"
#include "tank.h"

#define MAX_CLIENTS 16
#define MAX_VIEWERS 4

typedef enum ClientState
{
    cs_connected,
    cs_acknowledged,
    cs_in_game
} ClientState;

#pragma pack(push, 8)

typedef struct NetworkClient
{
    ClientState state;
    SOCKADDR address;
    char current_packet_buffer[PACKET_BUFFER];
    size_t current_packet_size;
    const PacketDefinition *current_packet_definition;
} NetworkClient;

typedef struct Client
{
    NetworkClient network_client;
    Tank tank;
} Client;

typedef struct ViewerClient
{
    NetworkClient network_client;
} ViewerClient;

#pragma pack(pop)

bool server_start(void);
void server_stop(void);

DynamicArray *server_get_clients(void);

Client *find_client_by_address(const SOCKADDR *address);
NetworkClient *register_client(const SOCKADDR *address);
bool unregister_client(const SOCKADDR *address);
void enqueue_client(const NetworkClient *c);

ViewerClient *find_viewer_by_address(const SOCKADDR *address);
NetworkClient *register_viewer(const SOCKADDR *address);
bool unregister_viewer(const SOCKADDR *address);
void enqueue_viewer(const NetworkClient *c);

void notify_viewers(NotViewerShellEvent *notification);
void notify_shutdown(void);

#endif /* __SERVER_H__ */
