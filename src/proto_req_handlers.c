#include <proto_req_handlers.h>
#include <proto.h>
#include <proto_objects.h>
#include <proto_req.h>
#include <stdio.h>

void server_handlers_handle_request(struct request_handler_t** handlers, const char* name, server_request_handler_cb handler)
{
    struct request_handler_t* new_handler = calloc(1, sizeof(struct request_handler_t));
    new_handler->name = name;
    new_handler->cb = handler;
    HASH_ADD_STR(*handlers, name, new_handler);
}

void server_handlers_init(struct server_client_handlers_t* h,
    struct request_handler_t* handlers, void* client_user)
{
    h->objects_num = 0;
    h->handlers = handlers;
    h->client_user = client_user;
}

static void free_client_state_recv_objects(struct server_client_handlers_t* h)
{
    for (int i = 0; i < h->objects_num; i++)
    {
        free(h->objects[i]);
    }

    h->objects_num = 0;
}

static void client_request_new(int socket, struct proto_process_t* proto, void* user)
{
    free_client_state_recv_objects((struct server_client_handlers_t*)user);
}

static void client_request_new_object(int socket, struct proto_process_t* proto, ProtoObject* object, void* user)
{
    struct server_client_handlers_t* h = (struct server_client_handlers_t*)user;

    if (h->objects_num >= MAX_RECEIVING_OBJECTS)
    {
        return;
    }

    h->objects[h->objects_num++] = proto_object_copy(object);
}

static const char* client_request_complete(int socket, struct proto_process_t* proto, void* user)
{
    struct server_client_handlers_t* h = (struct server_client_handlers_t*)user;

    if (h->objects_num == 0)
    {
        return "Empty request";
    }

    ProtoObject* first_object = h->objects[0];
    ProtoObjectProperty* id = find_property(first_object, OBJ_PROPERTY_ID);
    if (id == NULL)
    {
        return "Unknown req id";
    }

    char* request_id = copy_str_property(id);

    struct request_handler_t* handler = NULL;
    HASH_FIND_STR(h->handlers, request_id, handler);
    free(request_id);

    if (handler == NULL)
    {
        return "Unknown request";
    }

    struct request_handler_response_chain_t* response_head = NULL;

    const char* result = handler->cb(h, &response_head);
    if (result)
    {
        return result;
    }

    int response_count = 0;

    {
        struct request_handler_response_chain_t* elem;
        LL_COUNT(response_head, elem, response_count);
    }

    if (response_count)
    {
        ProtoObject** as_array = calloc(response_count, sizeof(ProtoObject*));

        {
            struct request_handler_response_chain_t* elem;
            int counter = 0;
            LL_FOREACH(response_head, elem)
            {
                as_array[counter++] = elem->response;
            }
        }

        proto_send(socket, as_array, response_count, proto->request_id, PROTO_FLAG_RESPONSE);

        {
            struct request_handler_response_chain_t* elem;
            struct request_handler_response_chain_t* tmp;

            LL_FOREACH_SAFE(response_head, elem, tmp)
            {
                LL_DELETE(response_head, elem);
                free(elem->response);
                free(elem);
            }
        }

        free(as_array);
    }

    return NULL;
}

void server_processor_attach_handlers(struct proto_req_processor_t* processor,
    struct server_client_handlers_t* handlers)
{
    proto_req_init_processor(processor, client_request_new, client_request_new_object,
        client_request_complete, handlers);
}

void server_request_add_response(struct request_handler_response_chain_t** response, ProtoObject* object)
{
    struct request_handler_response_chain_t* entry = calloc(1, sizeof(struct request_handler_response_chain_t));
    entry->response = object;
    LL_APPEND(*response, entry);
}