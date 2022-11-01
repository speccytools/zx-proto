#include <proto.h>
#include <proto_req.h>
#include <proto_req_handlers.h>
#include <sys/socket.h>
#include <unistd.h>
#include <printf.h>
#include <errno.h>

static struct request_handler_t* handler_functions = NULL;

const char* handle_command(struct server_client_handlers_t* h, struct request_handler_response_chain_t** response)
{
    printf("Hello is being handled (%d objects).\n", h->objects_num);

    ProtoObjectProperty* text = find_property(h->objects[0], 'C');
    if (text == NULL)
        return "No text supplied";

    char* command_text = copy_str_property(text);

    printf("You said: %s\n", command_text);
    char cmd[256];
    sprintf(cmd, "%s 2>&1", command_text);

    FILE *fp = popen(cmd, "r");

    char execution_result[256];
    ssize_t len = fread(execution_result, 1, sizeof(execution_result), fp);
    pclose(fp);
    execution_result[len] = '\0';
    free(command_text);

    declare_str_property_on_stack(result, 's', execution_result, NULL);
    server_request_add_response(response, proto_object_allocate(&result));

    return NULL;
}

/*
 * Bind various commands to particular callbacks.
 * UT_hash collection handler_functions will be updated
 */
void init_handlers()
{
    server_handlers_handle_request(&handler_functions, "command", handle_command);
}

int main()
{
    init_handlers();

    printf("Listening on 1339...\n");

    /*
     * proto_listen a socket to "accept" on
     */
    int accept_socket = proto_listen(1339);
    if (accept_socket < 0)
    {
        printf("Error: %d\n", accept_socket);
        return accept_socket;
    }

    /* Start an accept loop */
    while (1)
    {
        int client_socket = accept(accept_socket, (struct sockaddr*)NULL, NULL);

        printf("Connected: %d...\n", client_socket);

        struct proto_req_processor_t client_req_processor = {};
        struct proto_process_t client_proto = {};
        struct server_client_handlers_t client_handlers = {};
        uint8_t client_proto_buffer[512];

        /* supply a working buffer to the proto */
        proto_init(&client_proto, client_proto_buffer, sizeof(client_proto_buffer));

        /* init_handlers has generated us a list of command handles, so let's bind it to client_handlers */
        server_handlers_init(&client_handlers, handler_functions, NULL);

        /* attach proto_req_handlers callbacks to proto_req so one would use another */
        server_processor_attach_handlers(&client_req_processor, &client_handlers);

        uint8_t running = 1;

        while (running)
        {
            /* generic process loop */
            if (proto_server_process(client_socket, &client_proto, proto_req_new_request, proto_req_object_callback,
                proto_req_recv, &client_req_processor))
            {
                running = 0;
            }

            usleep(10000);
        }

        printf("Disconnected!\n");
    }
}