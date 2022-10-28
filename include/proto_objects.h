#ifndef PROTO_OBJECTS_H
#define PROTO_OBJECTS_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OBJ_PROPERTY_PAYLOAD        = 0x00,
    OBJ_PROPERTY_ERROR          = 0x01,
    OBJ_PROPERTY_ID             = 0x02,
    OBJ_PROPERTY_TITLE          = 0x03,
    OBJ_PROPERTY_COMMENT        = 0x04,
    OBJ_PROPERTY_KEYVALUE       = 0x05
} ProtoObjectPropertyKey;

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    uint16_t value_size;
    uint8_t key;
    char value[];
} ProtoObjectProperty;

typedef struct ProtoStackObjectProperty
{
    uint8_t key;
    const char* value;
    uint16_t value_size;
    struct ProtoStackObjectProperty* prev;
} ProtoStackObjectProperty;

/*
 * A very space efficient generic object witch abuses C flexible array members.
 *
 * Memory allocated to this object is always bigger than the struct itself. The actual struct size is just 2 bytes
 * (uint16_y), the rest is an array of pointers to data that comes immediately after. That way we can iterate
 * over this object very quickly, but the object was created with only one allocation.
 *
 * Layout:
 * - 2 bytes - object_size
 * - N + 1 pointers - amount of ChannelObjectProperty* for each property, and NULL for last one
 * - 2 bytes - reserved for convenient send
 * - property data - pointers above point to this data. each property comes immediately after previous one
 *
 * Freeing such an object is just a matter of one free() even though it has dynamic amount of properties,
 * they're all allocated in same blob of data.
 */

typedef struct
{
    uint16_t object_size;
    ProtoObjectProperty* properties[];
} ProtoObject;

#pragma pack(pop)

/*
 * Declare a C-string property. Must be zero-terminated, as strlen is used.
 */
#define declare_str_property_on_stack(name, property_key, property_value, property_prev) \
    ProtoStackObjectProperty name;                                  \
    {                                                                 \
        name.key = property_key;                                      \
        name.value = (const char*)property_value;                     \
        name.value_size = strlen(name.value);                         \
        name.prev = property_prev;                                    \
    }

/*
 * Declare a static-size property, like uint8_t or uint16_t. Can be anything really.
 */
#define declare_arg_property_on_stack(name, property_key, property, property_prev) \
    ProtoStackObjectProperty name;                                    \
    {                                                                 \
        name.key = property_key;                                      \
        name.value = (const char*)&property;                          \
        name.value_size = sizeof(property);                           \
        name.prev = property_prev;                                    \
    }

/*
 * Declare a property with length of anything you want. Can be a non-C-style string without 0-termination.
 */
#define declare_variable_property_on_stack(name, property_key, property_ptr, property_size, property_prev) \
    ProtoStackObjectProperty name;                                    \
    {                                                                 \
        name.key = property_key;                                      \
        name.value = (const char*)property_ptr;                       \
        name.value_size = property_size;                              \
        name.prev = property_prev;                                    \
    }

/*
 * Declear an object to be ready to be sent off. You have to make sure the buffer size fits all the properties. It's not a good
 * idea to have a buffer more than 512 bytes, as this is stack. Alternatively, you can just proto_object_assign to a static location yourself.
 * last_property is a last property of a daisy-chain of properties. Every property declared, except the first one, has a previous property.
 * Thus the method can reconstruct the whole object wihout having to allocate a separate list.
 */
#define declare_object_on_stack(name, buffer_size, last_property)     \
    uint8_t name ## _buffer [buffer_size];                            \
    ProtoObject* name = (ProtoObject*)name ## _buffer;                \
    proto_object_assign(name, buffer_size, last_property);

/*
 * Construct an object from a single-linked list of properties (usually declared on stack via declare_property_on_stack)
 * Empty properties (zero size) are ignored.
 * This object is required to be free()'d
 */
extern ProtoObject* proto_object_allocate(ProtoStackObjectProperty* last_property);

/*
 * Make a copy of an existing object. A new blob of data is allocated and all pointers are recalculated.
 * This object is required to be free()'d
 */
extern ProtoObject* proto_object_copy(ProtoObject* obj);

/*
 * Initialize an object with properties from a single-linked list. An obj could be just a pointer to a buffer that
 * has enough data to fit whole object. No allocations happen in the process so the object can live on stack.
 * Empty properties (zero size) are ignored.
 * This is used by declare_object_on_stack
 */
extern void proto_object_assign(ProtoObject* obj, uint16_t buffer_available, ProtoStackObjectProperty* last_property);

/*
 * Read up an object by known size from some buffer. The obj could be initialized, but no data is actually copied
 * from the buffer_from (needs to be of size just to fit N + 1 property references) - the data is only referenced.
 * No allocations happen so obj could be on stack.
 * If object is required to be used past buffer_from lifetime, a copy has to be made
 */
extern uint8_t proto_object_read(ProtoObject* obj, uint16_t buffer_available, uint16_t object_size, const uint8_t* buffer_from);

/*
 * Find a property by a given key. An object can contain multiple properties with the same key,
 * this function returns only the first match.
 */
extern ProtoObjectProperty* find_property(ProtoObject* o, uint8_t key);

/*
 * Find a property by a given key whose value is also partially matches.
 * An object can contain multiple properties with the same key, this function returns only the first match.
 */
extern ProtoObjectProperty* find_property_match(ProtoObject* o, uint8_t key, const char* match);

/*
 * Get uint16_t property value by a given key. If no property has been found, the default value is returned.
 * An object can contain multiple properties with the same key, this function returns only the first match.
 */
extern uint16_t get_uint16_property(ProtoObject* o, uint8_t key, uint16_t def);

/*
 * Get uint8_t property value by a given key. If no property has been found, the default value is returned.
 * An object can contain multiple properties with the same key, this function returns only the first match.
 */
extern uint8_t get_uint8_property(ProtoObject* o, uint8_t key, uint8_t def);

/*
 * Get C-string property value by a given key into target_buffer. If no property has been found, 1 is returned, 0 on success.
 * target_buffer has to be sufficient fot the value size.
 * An object can contain multiple properties with the same key, this function returns only the first match.
 */
extern uint8_t get_str_property(ProtoObject* o, uint8_t key, char* target_buffer, uint16_t target_buffer_size);

/*
 * Get string value of a given property. A copy is returned (heap allocated), and it is automatically null-terminated.
 */
extern char* copy_str_property(ProtoObjectProperty* property);

/*
 * Returns a pointer to char[object_size + 2] buffer containing the whole object including its properties and size.
 * When sending this over the net, send object_size + 2 bytes
 */
extern uint8_t* proto_object_data(ProtoObject* o);

#ifdef __cplusplus
}
#endif

#endif
