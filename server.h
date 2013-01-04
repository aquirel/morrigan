// server.h - server implementation.

#ifndef __SERVER_H__
#define __SERVER_H__

#include <winsock2.h>

#include "client.h"

bool server_init();
void server_shutdown();

Client *find_client_by_address(const SOCKADDR *address);
Client *register_client(const SOCKADDR *address);

void enqueue_client(const Client *c);

void notify_shutdown(void);

#endif
