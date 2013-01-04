// protocol.c - protocol definition.

#include <stdint.h>

#include "protocol.h"
#include "client.h"
#include "server.h"

typedef bool (*packet_validation_handler)(const void *packet);
typedef void (*packet_execution_handler)(Client *c);

typedef struct PacketDefinition
{
    uint8_t id;
    packet_validation_handler validator;
    packet_execution_handler executor;
} PacketDefinition;

typedef enum Requests
{
    req_hello = 0x00,
    req_bye   = 0x01
} Requests;

void req_hello_executor(Client *c);
void req_bye_executor(Client *c);

typedef enum Responses
{
    res_hello            = 0x00,
    res_bye              = 0x01,
    res_too_many_clients = 0x03,
    res_wait             = 0x04,
    res_bad_request      = 0x80
} Responses;

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
    }

    if (c->has_packet)
    {
        uint8_t response = res_wait;
        respond((char *) &response, 1, sender_address);
        return;
    }

    memcpy(c->current_packet, packet, PACKET_BUFFER);
    c->has_packet = true;
    enqueue_client(c);
}
