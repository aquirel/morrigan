// server.h - server implementation.

#include "server.h"
#include "array.h"

bool server_init();
void server_shutdown();

Client *find_client_by_address(const SOCKADDR *address);
Client *register_client(const SOCKADDR *address);

void enqueue_client(const Client *c); // TODO: Add to ringbuffer.

void notify_shutdown(void); // TODO: Notify remaining clients about server shutdown.
