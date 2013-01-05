// protocol.c - protocol definition.

#include <stdint.h>
#include <stdbool.h>
#include <threads.h>
#include <stdatomic.h>

#include "protocol.h"
#include "server.h"

static void req_hello_executor(Client *c);
static void req_bye_executor(Client *c);

static PacketDefinition RequestDefinitions[] =
{
    { .id = req_hello, .validator = NULL, .executor = req_hello_executor },
    { .id = req_bye, .validator = NULL, .executor = req_bye_executor }
};

static PacketDefinition *find_packet_by_id(uint8_t id)
{
    for (int i = 0; i < sizeof(RequestDefinitions) / sizeof(RequestDefinitions[0]); i++)
    {
        if (id == RequestDefinitions[i].id)
        {
            return &RequestDefinitions[i];
        }
    }

    return NULL;
}

void handle_packet(const char *packet, const SOCKADDR *sender_address)
{
    uint8_t id = packet[0];
    PacketDefinition *packet_definition = find_packet_by_id(id);

    if (NULL == packet_definition ||
        (NULL != packet_definition && NULL != packet_definition->validator && !packet_definition->validator(packet)))
    {
        uint8_t response = res_bad_request;
        respond((char *) &response, 1, sender_address);
        return;
    }

    Client *c = find_client_by_address(sender_address);
    if (NULL == c)
    {
        c = register_client(sender_address);
        if (NULL == c)
        {
            uint8_t response = res_too_many_clients;
            respond((char *) &response, 1, sender_address);
            return;
        }

        uint8_t response = res_hello;
        respond((char *) &response, 1, sender_address);
    }

    if (c->current_packet_definition)
    {
        uint8_t response = res_wait;
        respond((char *) &response, 1, sender_address);
        return;
    }

    memcpy(c->current_packet_buffer, packet, PACKET_BUFFER);
    c->current_packet_definition = packet_definition;
    enqueue_client(c);
}

void req_hello_executor(Client *c)
{
    if (cs_connected == c->state)
    {
        c->state = cs_acknowledged;
    }

    uint8_t response = res_hello;
    respond((char *) &response, 1, sender_address);
}

void req_bye_executor(Client *c)
{
    unregister_client(&c->address);
}
