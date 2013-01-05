// server.h - server implementation.

#ifndef __SERVER_H__
#define __SERVER_H__

#include <winsock2.h>

#include "protocol.h"

#define MAX_CLIENTS 16

bool server_start(void);
void server_stop(void);

Client *find_client_by_address(const SOCKADDR *address);
Client *register_client(const SOCKADDR *address);
bool unregister_client(const SOCKADDR *address);

void enqueue_client(const Client *c);

void notify_shutdown(void);

#endif
