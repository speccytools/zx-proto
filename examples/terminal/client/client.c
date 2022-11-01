#ifdef __CLION_IDE__
#define __LIB__
#define __FASTCALL__
#define __SPECTRUM (1)
#endif

#include <spectranet.h>
#include <proto_req.h>
#include <proto.h>
#include <stdio.h>
#include <input.h>

static uint8_t client_proto_buffer[512];
static struct proto_process_t client_proto = {};
static struct proto_req_processor_t req_handle = {};

static void disconnected()
{
}

static char response[256];
static char input_string[256];
static int input_string_len = 0;
static uint8_t waiting_for_response = 0;

/*
 * Called when a new object arrives as a response to particular request
 * see send_command
 */
static void command_result_object(uint8_t index, ProtoObject* object)
{
    if (index == 0)
    {
        ProtoObjectProperty* response_str = find_property(object, 's');
        if (response_str)
        {
            memcpy(response, response_str->value, response_str->value_size);
            response[response_str->value_size] = '\0';
        }
        else
        {
            response[0] = '\0';
        }
    }
}

/*
 * Called when a particular request has succeeded (and all objects are received)
 */
static void command_result_success(struct proto_process_t* proto)
{
    printf("\nResponse:%s\n>_", response);
    waiting_for_response = 0;
}

/*
 * Called when a request failed
 */
static void command_result_error(const char* error)
{
    printf("\nError: %s\n>_", error);
    waiting_for_response = 0;
}

static void send_command()
{
    /*
     * Allocate two properties on a stack
     */
    declare_str_property_on_stack(id_, OBJ_PROPERTY_ID, "command", NULL);
    declare_variable_property_on_stack(text_, 'C', input_string, input_string_len, &id_);

    /*
     * Assign those two properties to an object that is also allocated on a stack
     */
    declare_object_on_stack(request, 256, &text_);

    waiting_for_response = 1;

    /*
     * Send off an object
     */
    proto_req_send_request(&req_handle, client_socket, request,
        command_result_object, command_result_success, command_result_error);
}

int main()
{
    /*
     * Clear screen
     */
    printf("\x0C");

    printf("Connect where to? [=127.0.0.1]\n");
    gets(input_string);

    /*
     * Bind callbacks if server sends US a request/command that is not a response to our own request,
     * but we are not interested in that, so we bind to nothing
     */
    proto_req_init_processor(&req_handle, NULL, NULL, NULL, NULL);

    /*
     * Enable spectranet
     */
    pagein();

    if (input_string[0] == 0)
    {
        strcpy(input_string, "127.0.0.1");
    }

    /* supply a working buffer to the proto */
    proto_init(&client_proto, client_proto_buffer, sizeof(client_proto_buffer));

    /*
     * Connect
     */
    printf("Connecting to %s...\n", input_string);
    int server_socket = proto_connect(input_string, 1339, disconnected);

    if (server_socket <= 0)
    {
        printf("Cannot connect.\n");
        pageout();
        return 1;
    }

    printf("Connected!\n");

    printf(">_");
    char waiting = 0;

    /* main loop */
    while (1)
    {
        /*
         * Called in a loop to keep the connection alive.
         * Nothing in your main loop can be blocked, as this function has to be called as often as possible
         */
        proto_req_client_process(&client_proto, &req_handle);

        if (waiting_for_response)
            continue;

        /* non-blockingly read for a prompt */
        char key = in_Inkey();

        if (waiting)
        {
            // we're waiting for key debounce
            if (key == 0)
            {
                waiting = 0;
            }
        }
        else
        {
            switch (key)
            {
                case 0:
                {
                    break;
                }
                case 12:
                {
                    if (input_string_len)
                    {
                        printf("\x08 \x08\x08_");
                        input_string_len--;
                    }
                    waiting = 1;
                    break;
                }
                case 13:
                {
                    // undo "_"
                    printf("\x08 ");

                    if (memcmp(input_string, "exit", 4) == 0)
                    {
                        proto_disconnect();
                        pageout();
                        return 1;
                    }

                    /* on Enter, send a command off */
                    send_command();
                    input_string_len = 0;
                    waiting = 1;
                    break;
                }
                default:
                {
                    printf("\x08%c_", key);
                    input_string[input_string_len++] = key;
                    waiting = 1;
                    break;
                }
            }
        }

    }

    pageout();
    return 0;
}