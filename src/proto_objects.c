#include <proto_objects.h>

#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <Winsock2.h>
#else
#include <sys/socket.h>
#ifndef __SPECTRUM
#include <arpa/inet.h>
#endif
#endif

void proto_object_assign(ProtoObject* obj, uint16_t buffer_available, ProtoStackObjectProperty* last_property) API_DECL
{
    uint16_t object_size = 0;
    ProtoStackObjectProperty* property = last_property;
    uint8_t number_of_properties = 0;

    while (property)
    {
        if (property->value_size)
        {
            object_size += property->value_size + sizeof(ProtoObjectProperty);
            number_of_properties++;
        }
        property = property->prev;
    }

    uint16_t offsets = sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*) +
        sizeof(ProtoObjectRequestHeader);

    uint8_t* raw_data = (uint8_t*)obj + offsets;
    obj->object_size = object_size;
    ProtoObjectProperty** res_prop = obj->properties;

    property = last_property;
    while (property)
    {
        if (property->value_size)
        {
            ProtoObjectProperty *target_property = (ProtoObjectProperty *) raw_data;
            *res_prop++ = target_property;

            target_property->key = property->key;
            target_property->value_size = property->value_size;
            memcpy(target_property->value, property->value, property->value_size);

            raw_data += target_property->value_size + sizeof(ProtoObjectProperty);
        }
        property = property->prev;
    }

    *res_prop = NULL;
}

#ifndef __SPECTRUM

/* NOTE: SPECTRUM HAS AN ASSEMBLER IMPLEMENTATION FOR THIS */
uint8_t proto_object_read(ProtoObject* obj, uint16_t buffer_available, uint16_t object_size, const uint8_t* buffer_from) API_DECL
{
    uint8_t number_of_properties = 0;
    {
        // find out number of properties
        const uint8_t* it = buffer_from;
        const uint8_t* end = it + object_size;
        while (it < end)
        {
            ++number_of_properties;
            uint16_t value_size = *(uint16_t*)it;
            it += sizeof(ProtoObjectProperty) + value_size;
        }

        if (it != end)
        {
            return 1;
        }
    }

    if (sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*) > buffer_available)
    {
        return 2;
    }

    obj->object_size = object_size;

    ProtoObjectProperty** properties = obj->properties;

    {
        // initialize pointers
        const uint8_t* it = buffer_from;
        const uint8_t* end = it + object_size;
        while (it < end)
        {
            *properties++ = (ProtoObjectProperty*)it;
            it += sizeof(ProtoObjectProperty) + ((ProtoObjectProperty*)it)->value_size;
        }
    }

    *properties = NULL;
    return 0;
}
#endif

#ifndef __SPECTRUM
ProtoObjectProperty* find_property(ProtoObject* o, uint8_t key) API_DECL
{
    ProtoObjectProperty** prop = o->properties;
    while (*prop)
    {
        if ((*prop)->key == key)
        {
            return *prop;
        }
        prop++;
    }

    return NULL;
}
#endif

ProtoObjectProperty* find_property_match(ProtoObject* o, uint8_t key, const char* match) API_DECL
{
    uint8_t len = strlen(match);

    ProtoObjectProperty** prop = o->properties;
    while (*prop)
    {
        ProtoObjectProperty* p = *prop;
        if (p->key == key && p->value_size >= len && (memcmp(p->value, match, len) == 0))
        {
            return p;
        }
        prop++;
    }

    return NULL;
}

#ifndef __SPECTRUM
uint16_t get_uint16_property(ProtoObject* o, uint8_t key, uint16_t def) API_DECL
{
    ProtoObjectProperty** prop = o->properties;
    while (*prop)
    {
        if ((*prop)->key == key && (*prop)->value_size == sizeof(uint16_t))
        {
            uint16_t result;
            memcpy(&result, (*prop)->value, sizeof(uint16_t));
            return result;
        }
        prop++;
    }

    return def;
}

void* get_property_ptr(ProtoObject* o, uint8_t key) API_DECL
{
    ProtoObjectProperty** prop = o->properties;
    while (*prop)
    {
        if ((*prop)->key == key
#ifdef SANITY_CHECKS
            && (*prop)->value_size == sizeof(uint8_t)
#endif
                ) {
            return (*prop)->value;
        }
        prop++;
    }

    return NULL;
}

uint8_t get_uint8_property(ProtoObject* o, uint8_t key, uint8_t def) API_DECL
{
    ProtoObjectProperty** prop = o->properties;
    while (*prop)
    {
        if ((*prop)->key == key
#ifdef SANITY_CHECKS
        && (*prop)->value_size == sizeof(uint8_t)
#endif
        ) {
            return *(*prop)->value;
        }
        prop++;
    }

    return def;
}

#endif

uint8_t get_str_property(ProtoObject* o, uint8_t key, char* target_buffer, uint16_t target_buffer_size) API_DECL
{
    ProtoObjectProperty** prop = o->properties;
    while (*prop)
    {
        if ((*prop)->key == key)
        {
            uint16_t max_size = (*prop)->value_size;
            if (max_size >= target_buffer_size)
            {
                // leave place for zero termination
                max_size = target_buffer_size - 1;
            }
            memcpy(target_buffer, (*prop)->value, max_size);
            target_buffer[max_size] = '\0';
            return 0;
        }
        prop++;
    }

    return 1;
}

uint8_t* proto_object_data(ProtoObject* o) API_DECL
{
    uint8_t number_of_properties = 0;

    while(o->properties[number_of_properties])
    {
        number_of_properties++;
    }

    return (uint8_t*)o + sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*);
}

uint8_t* proto_object_data_update_size(ProtoObject* o) API_DECL
{
    uint8_t* d = proto_object_data(o);
    /*
     * This little trick uses the 2 reserved bytes on allocations as a way to send object
     * at one call without reallocations. Those bytes are getting updated with object size prior to sending.
     * Thus, when sending, you should send object_size + 2
     */
    *(uint16_t*)d = o->object_size;
    return d;
}
