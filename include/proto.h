#ifndef __PROTO_H__
#define __PROTO_H__

#include <stdint.h>
#include <proto_objects.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __SPECTRUM
#define PROTO_CLIENT (1)
#else
#define PROTO_SERVER (1)
#define SANITY_CHECKS (1)
#endif

#ifdef PROTO_SERVER
#define NONBLOCKING_RECV (1)
#endif

#ifdef __SPECTRUM
#define STACKLESS_PROCESS (1)
#else
#define COMBINE_OBJECTS_UPON_SENDING (1)
#endif

enum proto_process_state_t
{
    proto_process_recv_header = 0,
    proto_process_recv_object_size,
    proto_process_recv_object,
};

#define PROTO_FLAG_ERROR (0x01)
#define PROTO_FLAG_RESPONSE (0x02)


#pragma pack(push)
#pragma pack(1)

struct proto_process_t
{
    enum proto_process_state_t state;
    uint8_t recv_flags;
    uint16_t recv_size;
    uint16_t request_id;
    uint16_t recv_consumed;
    uint16_t recv_object_size;
    uint16_t total_received;
    uint16_t total_consumed;
    uint8_t recv_objects_num;

    uint8_t* process_buffer;
    uint16_t process_buffer_size;
};

#pragma pack(pop)

typedef void (*disconnected_callback_f)(void);
typedef void (*proto_start_request_callback_f)(int socket, struct proto_process_t* proto, void* user);
typedef void (*proto_next_object_callback_f)(int socket, struct proto_process_t* proto, ProtoObject* object, void* user);
// return NULL in case of success, or a text explaining the error
typedef const char* (*proto_complete_request_callback_f)(int socket, struct proto_process_t* proto, void* user);

/*
 * Initialize a proto with a working buffer provided. Has to be called ones
 * On STACKLESS proto implementation, an "extern struct proto_process_t process_proto" has to be declared somehwere.
 */
extern void proto_init(
#ifndef STACKLESS_PROCESS
    struct proto_process_t* process_proto,
#endif
    uint8_t* process_buffer, uint16_t process_buffer_size);

#ifdef PROTO_CLIENT
/*
 * Socket for the connection, useful if tweaks are needed. 0 = disconnected
 */
extern int client_socket;

/*
 * Connect to a proto server. disconnected will be called in an event the disconnect happens
 */
extern int proto_connect(const char* host, int port, disconnected_callback_f disconnected);

/*
 * Disconnect from client side.
 */
extern void proto_disconnect();

/*
 * Main loop function that should be called as part of main loop of your program, as often as possible.
 * proto: a struct for handling the connection, should be allocated somewhere and zeroed out prior to use.
 * start_request: a callback that will be called, should a server trigger a request to the client.
 * next: a callback that will be called for every object on the incoming request.
 * complete_request: a callback that will be called when all object have been processed.
 * user: a variable that will be passed to start_request, next and complete_request, for user purposes.
 *
 * On STACKLESS proto implementation, an "extern struct proto_process_t process_proto" has to be declared somehwere.
 *
 * When a proto_req.h sub-library is used, should be called as follows:
 * proto_req_client_process(&proto, &req_handle);
 */
extern void proto_client_process(
#ifndef STACKLESS_PROCESS
    struct proto_process_t* proto,
#endif
    proto_start_request_callback_f start_request,
    proto_next_object_callback_f next,
    proto_complete_request_callback_f complete_request,
    void* user);
#endif

#ifdef PROTO_SERVER

/*
 * Start listening on new sockets. Returns 'accept' socket, which you should call 'accept' upon in a loop.
 *
 * int accept_socket = proto_listen(...);
 *
 * fd_set rfds;
 * FD_ZERO(&rfds);
 * FD_SET(accept_socket, &rfds);
 * int accepted = select(accept_socket + 1, &rfds, NULL, NULL, &tv);
 * if(accepted <= 0)
 *  continue;
 * int new_client = accept(accept_socket, (struct sockaddr*)NULL, NULL);
 *
 * ... start a thread for a new client with socket new_client
 */
extern int proto_listen(int port);

/*
 * Process a client once. Should be called on a separate thread on a socket that has been 'accepted'.
 * socket: a struct for handling the connection with particular client, should be allocated somewhere and zeroed out prior to use.
 * start_request: a callback that will be called, should a client trigger a request to the server.
 * next: a callback that will be called for every object on the incoming request.
 * complete_request: a callback that will be called when all object have been processed.
 * user: a variable that will be passed to start_request, next and complete_request, for user purposes.
 */
extern int proto_server_process(int socket, struct proto_process_t* proto,
    proto_start_request_callback_f start_request,
    proto_next_object_callback_f next,
    proto_complete_request_callback_f complete_request,
    void* user);

/*
 * Returns the size of all objects combined on a buffer would take.
 */
extern uint32_t proto_serialize_get_size(ProtoObject** objects, uint8_t amount);

/*
 * Serializes a set of objects on a buffer 'data'. Similar to `proto_send`, but instead you should send the data
 * yourself (e.g. send(client_socket, buffer, buffer_size))
 */
extern uint8_t* proto_serialize(uint8_t* data, ProtoObject** objects, uint8_t amount, uint16_t request_id, uint8_t flags);
#endif

/*
 * Launch a request from one side to another.
 * socket: a socket send the request to
 * objects: an array of pointers to allocated objects to send
 * amount: amount of such objects
 * flags: (e.g. PROTO_FLAG_ERROR)
 * request_id: a request id to distinguish the progress. easiest implentation is keep a counter. Pass 0 if you
 *     expect no response.
 *
 * When a proto_req.h sub-library is used, should be called as follows:
 * proto_server_process(client_socket, &proto, proto_req_new_request, proto_req_object_callback,
 *     proto_req_recv, &req_handle)
 */
extern int proto_send(int socket, ProtoObject** objects, uint8_t amount, uint16_t request_id, uint8_t flags);

#ifdef __cplusplus
}
#endif

#endif