// server.h - server implementation.

#pragma once
#ifndef __SERVER_H__
#define __SERVER_H__

#include <winsock2.h>

#include "protocol.h"
#include "dynamic_array.h"

#define MAX_CLIENTS 16
#define MAX_VIEWERS 4

extern DynamicArray *clients;
extern DynamicArray *viewers;

bool server_start(void);
void server_stop(void);

Client *find_client_by_address(const SOCKADDR *address);
Client *register_client(const SOCKADDR *address);
bool unregister_client(const SOCKADDR *address);
void enqueue_client(const Client *c);
void enqueue_viewer(const ViewerClient *c);

ViewerClient *find_viewer_by_address(const SOCKADDR *address);
ViewerClient *register_viewer(const SOCKADDR *address);
bool unregister_viewer(const SOCKADDR *address);
void enqueue_viwer(const ViewerClient *c);

void notify_shutdown(void);

#endif /* __SERVER_H__ */
