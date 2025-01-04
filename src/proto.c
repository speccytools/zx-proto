#include <proto.h>
#include <proto_objects.h>

#ifdef WIN32
#   include <Winsock2.h>
#else
#ifdef __SPECTRUM
#   include <sockpoll.h>
#   include <spectranet.h>
#else
#   include <poll.h>
#   include <arpa/inet.h>
#endif
#   include <sys/socket.h>
#   include <netdb.h>
#endif

#include <string.h>
#include <stdlib.h>

#ifndef __SPECTRUM
#ifdef WIN32
#   define sockclose closesocket
#   define MSG_DONTWAIT (0)
#else
#   define sockclose close
#   include <unistd.h>
#   include <sys/errno.h>
#endif
#endif

#ifdef PROTO_CLIENT
int client_socket = 0;
static disconnected_callback_f client_disconnected = NULL;
#endif

/*
 * On stackless implementation a global static struct is used, thus the compiler knows direct address of every
 * struct member (.), on implementation with stack, the proto object is reference by a pointer,
 * and thus struct members are resolved by an offset to that pointed (->)
 */

#ifdef STACKLESS_PROCESS
#define PROTO_MEMBER .
#else
#define PROTO_MEMBER ->
#endif

#ifdef STACKLESS_PROCESS
static int process_socket;
extern struct proto_process_t process_proto;
static proto_start_request_callback_f process_start_request;
static proto_next_object_callback_f process_next;
static proto_complete_request_callback_f process_complete_request;
static void* process_user;
#endif

void proto_init(
#ifndef STACKLESS_PROCESS
    struct proto_process_t* process_proto,
#endif
    uint8_t* process_buffer, uint16_t process_buffer_size)
{
#ifdef STACKLESS_PROCESS
    memset(&process_proto, 0, sizeof(struct proto_process_t));
#else
    memset(process_proto, 0, sizeof(struct proto_process_t));
#endif
    process_proto PROTO_MEMBER process_buffer = process_buffer;
    process_proto PROTO_MEMBER process_buffer_size = process_buffer_size;
}

#ifdef PROTO_SERVER
int proto_listen(int port)
{
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
    {
#ifdef WIN32
        return - WSAGetLastError();
#else
        return socketfd;
#endif
    }

#ifdef WIN32
    // make it non-blocking
    u_long mode = 1;
    ioctlsocket(socketfd, FIONBIO, &mode);
#endif

    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if(bind(socketfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            sockclose(socketfd);
            return -2000;
        }
    }

    if(listen(socketfd, 1) < 0)
    {
        sockclose(socketfd);
        return -3000;
    }

    return socketfd;
}
#endif

#ifdef PROTO_CLIENT
void proto_disconnect()
{
    if (client_socket)
    {
        sockclose(client_socket);
        client_socket = 0;
    }
}

int proto_connect(const char* host, int port, disconnected_callback_f disconnected)
{
    proto_disconnect();

    client_disconnected = disconnected;

    struct sockaddr_in remoteaddr;

#ifdef __SPECTRUM
    remoteaddr.sin_port = port;

    struct hostent *he = gethostbyname((char*)host);
    if(!he)
    {
        return -1;
    }

    remoteaddr.sin_addr.s_addr = (in_addr_t)he->h_addr;
#else
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_port = htons(port);
    remoteaddr.sin_addr.s_addr = inet_addr(host);
#endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return -2;
    }

    if(connect(sock, (struct sockaddr *)&remoteaddr, sizeof(struct sockaddr_in)) < 0)
    {
        sockclose(sock);
        client_disconnected = NULL;
        return -3;
    }

    client_socket = sock;

    return sock;
}
#endif

static uint8_t process(
#ifndef STACKLESS_PROCESS
    int process_socket,
    struct proto_process_t* process_proto,
    proto_start_request_callback_f process_start_request,
    proto_next_object_callback_f process_next,
    proto_complete_request_callback_f process_complete_request,
    void* process_user
#endif
)
{
    process_proto PROTO_MEMBER total_consumed = 0;
#ifdef STACKLESS_PROCESS
    static uint16_t available;
#else
    uint16_t
#endif
    available = process_proto PROTO_MEMBER total_received;
#ifdef STACKLESS_PROCESS
    static uint8_t* data;
#else
    uint8_t*
#endif
    data = (uint8_t*)process_proto PROTO_MEMBER process_buffer;

    while (1)
    {
        switch (process_proto PROTO_MEMBER state)
        {
            case proto_process_recv_header:
            {
                if (available >= 5)
                {
                    memcpy(&process_proto PROTO_MEMBER recv_flags, data, 5);
                    data += 5;
                    process_proto PROTO_MEMBER recv_consumed = 0;
                    process_proto PROTO_MEMBER recv_objects_num = 0;
                    process_proto PROTO_MEMBER total_consumed += 5;
                    available -= 5;
                    process_proto PROTO_MEMBER state = proto_process_recv_object_size;
                    process_start_request(
#ifndef PROTO_CLIENT
                        process_socket,
                        process_proto,
#endif
                        process_user);
                    /* fallthrough */
                }
                else
                {
                    return 0;
                }
            }
            case proto_process_recv_object_size:
            {
                if (available >= sizeof(uint16_t))
                {
                    process_proto PROTO_MEMBER recv_object_size = *(uint16_t*)data;
                    process_proto PROTO_MEMBER total_consumed += 2;
                    available -= 2;
                    data += 2;
                    process_proto PROTO_MEMBER recv_consumed += 2;
                    process_proto PROTO_MEMBER state = proto_process_recv_object;
                    /* fallthrough */
                }
                else
                {
                    return 0;
                }
            }
            case proto_process_recv_object:
            {
                if (available >= process_proto PROTO_MEMBER recv_object_size)
                {

                    {
#ifdef STACKLESS_PROCESS
                        static
#endif
                        uint8_t object_buffer[128];

                        ProtoObject* obj = (ProtoObject*)object_buffer;
                        proto_object_read(obj, 128, process_proto PROTO_MEMBER recv_object_size, data);
                        process_next(
#ifndef PROTO_CLIENT
                            process_socket,
                            process_proto,
#endif
                            obj, process_user);
                    }

                    process_proto PROTO_MEMBER recv_objects_num++;
                    process_proto PROTO_MEMBER total_consumed += process_proto PROTO_MEMBER recv_object_size;
                    available -= process_proto PROTO_MEMBER recv_object_size;
                    data += process_proto PROTO_MEMBER recv_object_size;
                    process_proto PROTO_MEMBER recv_consumed += process_proto PROTO_MEMBER recv_object_size;

                    if (process_proto PROTO_MEMBER recv_consumed >= process_proto PROTO_MEMBER recv_size)
                    {
                        const char* err = process_complete_request(
#ifndef PROTO_CLIENT
                            process_socket, process_proto,
#endif
                            process_user);

                        if (err)
                        {
                            declare_str_property_on_stack(error, OBJ_PROPERTY_ERROR, err, NULL);
                            declare_object_on_stack(response_object, 256, &error);

                            if (proto_send_one(
#ifndef PROTO_CLIENT
                                process_socket,
#endif
                                response_object, process_proto PROTO_MEMBER request_id,
                                PROTO_FLAG_ERROR | PROTO_FLAG_RESPONSE))
                            {
                                return -1;
                            }
                        }

                        process_proto PROTO_MEMBER state = proto_process_recv_header;
                    }
                    else
                    {
                        process_proto PROTO_MEMBER state = proto_process_recv_object_size;
                    }
                    break;
                }
                else
                {
                    return 0;
                }
            }
        }
    }
}

void atoh(uint8_t *ascii_ptr, char *hex_ptr,int len)
{
    const char* hex_data = "0123456789abcdef";
    for (int i = 0; i < len; i++)
    {
        char byte = ascii_ptr[i];
        *hex_ptr++ = hex_data[(byte & 0xF0) >> 4];
        *hex_ptr++ = hex_data[byte & 0x0F];
    }
}

static int recv_process(
#ifndef STACKLESS_PROCESS
    int process_socket,
    struct proto_process_t* process_proto,
    proto_start_request_callback_f process_start_request,
    proto_next_object_callback_f process_next,
    proto_complete_request_callback_f process_complete_request,
    void* process_user
#endif
)
{
    uint16_t afford = process_proto PROTO_MEMBER process_buffer_size - process_proto PROTO_MEMBER total_received;

#ifdef NONBLOCKING_RECV
    int n_received = recv(process_socket, process_proto PROTO_MEMBER process_buffer + process_proto PROTO_MEMBER total_received,
        afford, MSG_DONTWAIT);
#else
    int n_received = recv(process_socket, process_proto PROTO_MEMBER process_buffer + process_proto PROTO_MEMBER total_received,
        afford, 0);
#endif

    if (n_received > 0)
    {
        process_proto PROTO_MEMBER total_received += n_received;

        uint8_t res = process(
#ifndef STACKLESS_PROCESS
            process_socket, process_proto, process_start_request, process_next, process_complete_request, process_user
#endif
        );
        if (res)
        {
            return -1;
        }

        if (process_proto PROTO_MEMBER total_consumed == process_proto PROTO_MEMBER total_received)
        {
            process_proto PROTO_MEMBER total_consumed = 0;
            process_proto PROTO_MEMBER total_received = 0;
        }
        else
        {
            memmove(process_proto PROTO_MEMBER process_buffer, process_proto PROTO_MEMBER process_buffer + (uint16_t)process_proto PROTO_MEMBER total_consumed,
                process_proto PROTO_MEMBER total_received - (uint16_t)process_proto PROTO_MEMBER total_consumed);
            process_proto PROTO_MEMBER total_received -= (uint16_t)process_proto PROTO_MEMBER total_consumed;
        }
    }

#ifdef WIN32
    if(n_received == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
    {
        return -2;
    }
#endif

#ifdef NONBLOCKING_RECV
    if (n_received == 0)
    {
        return errno;
    }

    if (errno == EWOULDBLOCK)
    {
        return 0;
    }
    else if (n_received <= 0)
    {
        return errno;
    }

#else
    if (n_received <= 0)
    {
        return -2;
    }
#endif

    return 0;
}

#ifdef PROTO_CLIENT

void proto_client_process(
#ifndef STACKLESS_PROCESS
    struct proto_process_t* proto,
#endif
    proto_start_request_callback_f start_request,
    proto_next_object_callback_f next,
    proto_complete_request_callback_f complete_request,
    void* user)
{
#ifdef __SPECTRUM
    int polled;
    while ((polled = poll_fd(client_socket)) & POLLIN)
    {

#ifdef STACKLESS_PROCESS
        process_socket = client_socket;
        process_start_request = start_request;
        process_next = next;
        process_complete_request = complete_request;
        process_user = user;
#endif

        if (recv_process(
#ifndef STACKLESS_PROCESS
            process_socket, proto, process_start_request, process_next, process_complete_request, user
#endif
        ) < 0)
        {
            sockclose(client_socket);
            if (client_disconnected)
            {
                client_disconnected();
            }
            return;
        }
    }
    if (polled & (POLLHUP | POLLNVAL))
    {
        sockclose(client_socket);
        if (client_disconnected)
        {
            client_disconnected();
        }
    }
#else
    struct pollfd fds[1];
    fds[0].fd = client_socket;
    fds[0].events = POLLIN;

    int ret = poll((struct pollfd*)&fds, 1, 1);

    if (ret == -1)
    {
        return;
    }

    if ( fds[0].revents & POLLIN )
    {
#ifdef STACKLESS_PROCESS
        process_socket = client_socket;
        process_start_request = start_request;
        process_next = next;
        process_complete_request = complete_request;
#endif

        if (recv_process(
#ifndef STACKLESS_PROCESS
            client_socket, proto, start_request, next, complete_request
#endif
        ) < 0)
        {
            sockclose(client_socket);
            if (client_disconnected)
            {
                client_disconnected();
            }
            return;
        }
    }

    if ( fds[0].revents & POLLHUP )
    {
        sockclose(client_socket);
        if (client_disconnected)
        {
            client_disconnected();
        }
    }

    fds[0].revents = 0;

#endif
}
#endif

#ifdef PROTO_SERVER
int proto_server_process(int socket, struct proto_process_t* proto,
    proto_start_request_callback_f start_request,
    proto_next_object_callback_f next,
    proto_complete_request_callback_f complete_request,
    void* user)
{
    return recv_process(socket, proto, start_request, next, complete_request, user);
}

uint32_t proto_serialize_get_size(ProtoObject** objects, uint8_t amount)
{
    uint16_t req_size = 5;

    for (uint8_t i = 0; i < amount; i++)
    {
        req_size += objects[i]->object_size + 2;
    }

    return req_size;
}

uint8_t* proto_serialize(uint8_t* data, ProtoObject** objects, uint8_t amount, uint16_t request_id, uint8_t flags)
{
    uint16_t req_size = 0;

    for (uint8_t i = 0; i < amount; i++)
    {
        req_size += objects[i]->object_size + 2;
    }

    *data = flags; data++;
    memcpy(data, (uint8_t*)&req_size, 2); data += 2;
    memcpy(data, (uint8_t*)&request_id, 2); data += 2;

    for (uint8_t i = 0; i < amount; i++)
    {
        ProtoObject* object = objects[i];
        memcpy(data, (void*)proto_object_data_update_size(object), object->object_size + 2);
        data += object->object_size + 2;
    }

    return data;
}
#endif

#ifdef PROTO_SEND
int proto_send(int socket, ProtoObject** objects, uint8_t amount, uint16_t request_id, uint8_t flags)
{
    uint16_t req_size = 0;

    for (uint8_t i = 0; i < amount; i++)
    {
        req_size += objects[i]->object_size + 2;
    }

#ifdef COMBINE_OBJECTS_UPON_SENDING
    int write_size = 5;

    for (uint8_t i = 0; i < amount; i++)
    {
        ProtoObject* object = objects[i];
        write_size += object->object_size + 2;
    }

    uint8_t* data = malloc(write_size);

    *data = flags;
    memcpy(data + 1, (uint8_t*)&req_size, 2);
    memcpy(data + 3, (uint8_t*)&request_id, 2);
    write_size = 5;

    for (uint8_t i = 0; i < amount; i++)
    {
        ProtoObject* object = objects[i];
        memcpy(data + write_size, (void*)proto_object_data_update_size(object), object->object_size + 2);
        write_size += object->object_size + 2;
    }

    if (send(socket, (void*)data, write_size, 0) < 0)
    {
        free(data);
        return 1;
    }

    free(data);
#else

    uint8_t header[5];
    header[0] = flags;
    memcpy(header + 1, (uint8_t*)&req_size, 2);
    memcpy(header + 3, (uint8_t*)&request_id, 2);

    if (send(socket, (void*)header, 5, 0) < 0)
    {
        return 1;
    }

    for (uint8_t i = 0; i < amount; i++)
    {
        ProtoObject* object = objects[i];

        if (send(socket, (void*) proto_object_data_update_size(object), object->object_size + 2, 0) < 0)
        {
            return 3;
        }
    }
#endif

    return 0;
}
#endif

int proto_send_one(
#ifndef PROTO_CLIENT
    int client_socket,
#endif
    ProtoObject* object, uint16_t request_id, uint8_t flags)
{
    ProtoObjectRequestHeader* d = (ProtoObjectRequestHeader*)proto_object_data(object);
    d->flags = flags;
    d->object_size = object->object_size;
    d->request_size = object->object_size + 2;
    d->request_id = request_id;

    if (send(client_socket, (void*)d, object->object_size + sizeof(ProtoObjectRequestHeader), 0) < 0)
    {
        return 3;
    }

    return 0;
}

int proto_send_one_nf(
#ifndef PROTO_CLIENT
    int client_socket,
#endif
    ProtoObject* object) FASTCALL
{
    return proto_send_one(
#ifndef PROTO_CLIENT
               client_socket,
#endif
               object, 0, 0);
}
