// server.h - server implementation.

#pragma once
#ifndef __SERVER_H__
#define __SERVER_H__

//#pragma message("__SERVER_H__")

#include <winsock2.h>

#include "morrigan.h"
#include "protocol.h"
#include "dynamic_array.h"

#define MAX_CLIENTS 16
#define MAX_VIEWERS 4

bool server_start(void);
void server_stop(void);

DynamicArray *server_get_clients(void);

Client *find_client_by_address(const SOCKADDR *address);
Client *register_client(const SOCKADDR *address);
bool unregister_client(const SOCKADDR *address);
void enqueue_client(const Client *c);

ViewerClient *find_viewer_by_address(const SOCKADDR *address);
ViewerClient *register_viewer(const SOCKADDR *address);
bool unregister_viewer(const SOCKADDR *address);
void enqueue_viewer(const ViewerClient *c);

void notify_viewers(NotViewerShellEvent *notification);
void notify_shutdown(void);

#endif /* __SERVER_H__ */
