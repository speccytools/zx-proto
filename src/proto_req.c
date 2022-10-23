#include <proto_req.h>

void proto_req_new_request(int socket, struct proto_process_t* proto, void* user)
{
    struct proto_req_processor_t* req_handle = (struct proto_req_processor_t*)user;

    if ((proto->recv_flags & PROTO_FLAG_RESPONSE) == 0)
    {
        if (req_handle->incoming_request)
            req_handle->incoming_request(socket, proto, req_handle->user);
    }
}

void proto_req_object_callback(int socket, struct proto_process_t* proto, ProtoObject* object, void* user)
{
    struct proto_req_processor_t* req_handle = (struct proto_req_processor_t*)user;

    if (proto->recv_flags & PROTO_FLAG_RESPONSE)
    {
        if (req_handle->current_request_id != proto->request_id)
        {
            return;
        }

        if (proto->recv_flags & PROTO_FLAG_ERROR)
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
            req_handle->incoming_object(socket, proto, object, req_handle->user);
    }
}

const char* proto_req_recv(int socket, struct proto_process_t* proto, void* user)
{
    struct proto_req_processor_t* req_handle = (struct proto_req_processor_t*)user;

    if (proto->recv_flags & PROTO_FLAG_RESPONSE)
    {
        if (req_handle->current_request_id != proto->request_id)
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
            req_handle->response_callback(proto);
        }
    }
    else
    {
        return req_handle->incoming_complete(socket, proto, req_handle->user);
    }

    return NULL;
}

void proto_req_init_processor(
    struct proto_req_processor_t* req_processor,
    proto_start_request_callback_f incoming_request,
    proto_next_object_callback_f incoming_object,
    proto_complete_request_callback_f incoming_complete,
    void* user)
{
    req_processor->incoming_request = incoming_request;
    req_processor->incoming_object = incoming_object;
    req_processor->incoming_complete = incoming_complete;
    req_processor->user = user;
}

uint8_t proto_req_send_request(
    struct proto_req_processor_t* req_processor,
    int sockfd, ProtoObject* object, proto_req_object_callback_f object_callback,
    proto_req_response_f cb, proto_req_error_callback_f err)
{
    req_processor->object_num = 0;
    req_processor->notify_error = 0;
    req_processor->response_callback = cb;
    req_processor->request_object_callback = object_callback;
    req_processor->error_callback = err;

    ProtoObject* objs[1];
    objs[0] = object;

    return proto_send(sockfd, objs, 1, ++req_processor->current_request_id, 0);
}
