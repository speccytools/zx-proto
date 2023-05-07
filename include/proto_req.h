#ifndef __PROTO_REQ_H__
#define __PROTO_REQ_H__

#include <stdint.h>

#include "proto.h"
#include "proto_objects.h"

/*
 * A request handling sub-library, made to distinguish between request responses from the other party, or
 * new requests.
 */

typedef void (*proto_req_response_f)(struct proto_process_t* proto);
typedef void (*proto_req_object_callback_f)(uint8_t index, ProtoObject* object);
typedef void (*proto_req_error_callback_f)(const char* error);

struct proto_req_processor_t
{
    uint16_t current_request_id;
    uint8_t object_num;
    uint8_t notify_error;
    proto_req_object_callback_f request_object_callback;
    proto_req_response_f response_callback;
    proto_req_error_callback_f error_callback;

    proto_start_request_callback_f incoming_request;
    proto_next_object_callback_f incoming_object;
    proto_complete_request_callback_f incoming_complete;

    char error_str[64];
    void* user;
};

/*
 * Called to init `req_handle` to handle new requests. Request responses will not be caller here.
 * req_handle: a struct to init the callbacks with.
 *
 * Similarly to proto_client_process / proto_server_process,
 * incoming_request: will be called once a new request has started to be processed.
 * incoming_object: will be called for every new object received to a new request
 * incoming_complete: will be called once all objects have been received
 * user: user data that will be supplied to any of the above
 */
extern void proto_req_init_processor(
    struct proto_req_processor_t* req_processor,
    proto_start_request_callback_f incoming_request,
    proto_next_object_callback_f incoming_object,
    proto_complete_request_callback_f incoming_complete,
    void* user);

/*
 * Used to issue a particular request and receive a response *JUST* to that request. Any other responses will be
 * processed differently.
 *
 * req_processor: a struct to handle responses.
 * sockfd: server/client socket to process it on
 * object: requests typically only can have one object, so this is the request object.
 *
 * When response is received, the following callbacks will be called
 * object_callback: for each object of the response
 * cb: when the response has concluded (success)
 * err: when the response has concluded (error)
 */
extern uint8_t proto_req_send_request(
    struct proto_req_processor_t* req_processor,
    int sockfd, ProtoObject* object, proto_req_object_callback_f object_callback,
    proto_req_response_f cb, proto_req_error_callback_f err);

/*
 * Should be called instead of proto_client_process with custom callbacks, if you want the request-response system
 */
#ifdef STACKLESS_PROCESS
#define proto_req_client_process(handle) \
    proto_client_process(proto_req_new_request, proto_req_object_callback, proto_req_recv, handle)
#else
#define proto_req_client_process(proto, handle) \
    proto_client_process(proto, proto_req_new_request, proto_req_object_callback, proto_req_recv, handle)
#endif

/*
 * Should be called instead of proto_server_process with custom callbacks, if you want the request-response system
 */
#define proto_req_server_process(sockfd, proto, handle) \
    proto_server_process(sockfd, proto, proto_req_new_request, proto_req_object_callback, proto_req_recv, handle)

/*
 * These function should be passed to either
 *
 * proto_server_process(client_socket, &proto, proto_req_new_request, proto_req_object_callback,
 *     proto_req_recv, &req_handle)
 *
 * ... or ...
 *
 * proto_client_process(&proto, proto_req_new_request, proto_req_object_callback, proto_req_recv, &req_handle);
 *
 * to facilitate request handing on individual basis.
 */
extern void proto_req_new_request(int socket, struct proto_process_t* proto, void* user);
extern void proto_req_object_callback(int socket, struct proto_process_t* proto, ProtoObject* object, void* user);
extern const char* proto_req_recv(int socket, struct proto_process_t* proto, void* user);

#endif