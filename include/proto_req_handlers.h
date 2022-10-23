#ifndef __PROTO_REQ_HANDLERS_H__
#define __PROTO_REQ_HANDLERS_H__

#include <proto_objects.h>
#include <ut/uthash.h>
#include <ut/utlist.h>

struct proto_req_processor_t;
struct server_client_handlers_t;
struct request_handler_response_chain_t;

#define MAX_RECEIVING_OBJECTS (32)

/*
 * When handling a request, used to attach an object to response
 */
extern void server_request_add_response(struct request_handler_response_chain_t** response, ProtoObject* object);

/*
 * Called as a handler for particular request.
 * h: struct that holds request objects, see h->objects, h->objects_num.
 *     h->client_user holds data from server_handlers_init
 * response: could be filled up by server_request_add_response, to send response object(s)
 */
typedef const char* (*server_request_handler_cb)(struct server_client_handlers_t* h, struct request_handler_response_chain_t** response);

/*
 * A struct to hold a handler for request, filled up by server_handlers_handle_request.
 * A pointer `struct request_handler_t* handlers = NULL` should be allocated to hold all handlers,
 * as they will be extended using UT_hash_handle
 */
struct request_handler_t {
    const char* name;
    server_request_handler_cb cb;
    UT_hash_handle hh;
};

/*
 * A struct to handle a request from one user
 */
struct server_client_handlers_t
{
    ProtoObject* objects[MAX_RECEIVING_OBJECTS];
    uint8_t objects_num;
    struct request_handler_t* handlers;
    void* client_user;
};

/*
 * Chain of response objects, assigned through server_request_add_response
 */
struct request_handler_response_chain_t
{
    ProtoObject* response;
    struct request_handler_response_chain_t* next;
};

/*
 * Called to assign a request type to a callback.
 * A pointer `struct request_handler_t* handlers = NULL` should be allocated to hold all handlers,
 * as they will be extended using UT_hash_handle
 */
extern void server_handlers_handle_request(struct request_handler_t** handlers, const char* name, server_request_handler_cb handler);

/*
 * Should be called over proto_req_processor_t once to register internal functions for the processor.
 */
extern void server_processor_attach_handlers(struct proto_req_processor_t* processor, struct server_client_handlers_t* handlers);

/*
 * Should be called once for every client to init server_client_handlers_t.
 */
extern void server_handlers_init(struct server_client_handlers_t* h,
    struct request_handler_t* handlers, void* client_user);

#endif
