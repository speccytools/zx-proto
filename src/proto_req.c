#include <proto_req.h>

#ifdef PROTO_CLIENT
#define PROTO_MEMBER .
extern struct proto_process_t process_proto;
#else
#define PROTO_MEMBER ->
#endif

void proto_req_new_request(
#ifndef PROTO_CLIENT
    int client_socket,
    struct proto_process_t* process_proto,
#endif
    void* user)
{
    struct proto_req_processor_t* req_handle = (struct proto_req_processor_t*)user;

    if ((process_proto PROTO_MEMBER recv_flags & PROTO_FLAG_RESPONSE) == 0)
    {
        if (req_handle->incoming_request)
            req_handle->incoming_request(
#ifndef PROTO_CLIENT
                client_socket, process_proto,
#endif
                req_handle->user);
    }
}

void proto_req_object_callback(
#ifndef PROTO_CLIENT
    int client_socket,
    struct proto_process_t* process_proto,
#endif
    ProtoObject* object, void* user)
{
    struct proto_req_processor_t* req_handle = (struct proto_req_processor_t*)user;

    if (process_proto PROTO_MEMBER recv_flags & PROTO_FLAG_RESPONSE)
    {
        if (req_handle->current_request_id != process_proto PROTO_MEMBER request_id)
        {
            return;
        }

        if (process_proto PROTO_MEMBER recv_flags & PROTO_FLAG_ERROR)
        {
            if (req_handle->object_num == 0)
            {
                ProtoObjectProperty* error = find_property(object, OBJ_PROPERTY_ERROR);
                if (error)
                {
                    memcpy(req_handle->error_str, error->value, error->value_size);
                    req_handle->error_str[error->value_size] = 0;
                    req_handle->notify_error = 1;
                    return;
                }
            }
        }
        else
        {
            if (req_handle->request_object_callback)
                req_handle->request_object_callback(req_handle->object_num++, object);
        }
    }
    else
    {
        if (req_handle->incoming_object)
            req_handle->incoming_object(
#ifndef PROTO_CLIENT
                client_socket,
                process_proto,
#endif
                object, req_handle->user);
    }
}

const char* proto_req_recv(
#ifndef PROTO_CLIENT
    int client_socket,
    struct proto_process_t* process_proto,
#endif
    void* user)
{
    struct proto_req_processor_t* req_handle = (struct proto_req_processor_t*)user;

    if (process_proto PROTO_MEMBER recv_flags & PROTO_FLAG_RESPONSE)
    {
        if (req_handle->current_request_id != process_proto PROTO_MEMBER request_id)
        {
            return "Unknown request id";
        }

        if (req_handle->notify_error)
        {
            if (req_handle->error_callback)
            {
                req_handle->error_callback(req_handle->error_str);
            }
            req_handle->notify_error = 0;
        }
        else
        {
            req_handle->response_callback(
#ifndef PROTO_CLIENT
                process_proto
#endif
            );
        }
    }
    else
    {
        return req_handle->incoming_complete(
#ifndef PROTO_CLIENT
                client_socket, process_proto,
#endif
                req_handle->user);
    }

    return NULL;
}

uint8_t proto_req_send_request(
#ifndef STATIC_PROCESSOR
    struct proto_req_processor_t* proto_req_processor,
#endif
#ifndef PROTO_CLIENT
    int sockfd,
#endif
    ProtoObject* object, proto_req_object_callback_f object_callback,
    proto_req_response_f cb, proto_req_error_callback_f err)
{
#ifdef STATIC_PROCESSOR
    proto_req_processor.object_num = 0;
    proto_req_processor.notify_error = 0;
    proto_req_processor.response_callback = cb;
    proto_req_processor.request_object_callback = object_callback;
    proto_req_processor.error_callback = err;
#else
    proto_req_processor->object_num = 0;
    proto_req_processor->notify_error = 0;
    proto_req_processor->response_callback = cb;
    proto_req_processor->request_object_callback = object_callback;
    proto_req_processor->error_callback = err;
#endif

    return proto_send_one(
#ifndef PROTO_CLIENT
               sockfd,
#endif
               object,
#ifdef STATIC_PROCESSOR
               ++proto_req_processor.current_request_id,
#else
               ++proto_req_processor->current_request_id,
#endif
    0);
}
