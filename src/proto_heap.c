#include <proto_objects.h>

#include <stdlib.h>
#include <string.h>

#ifndef __SPECTRUM
static uint16_t proto_property_value_size(const ProtoObjectProperty* property)
{
    uint16_t value_size;
    memcpy(&value_size, &property->value_size, sizeof(value_size));
    return value_size;
}

static void proto_property_write(uint8_t* target, uint8_t key, const char* value, uint16_t value_size)
{
    memcpy(target, &value_size, sizeof(value_size));
    target[sizeof(value_size)] = key;
    memcpy(target + sizeof(ProtoObjectProperty), value, value_size);
}
#else
#define proto_property_value_size(property) ((property)->value_size)
#endif

ProtoObject* proto_object_allocate(ProtoStackObjectProperty* last_property) API_DECL
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

    uint8_t* raw_malloc = malloc(sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*) + object_size + sizeof(ProtoObjectRequestHeader));
    ProtoObject* obj = (ProtoObject*)raw_malloc;
    raw_malloc += sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*) + sizeof(ProtoObjectRequestHeader);
    obj->object_size = object_size;
    ProtoObjectPropertyPtr* res_prop = obj->properties;

    property = last_property;
    while (property)
    {
        if (property->value_size)
        {
            ProtoObjectProperty *target_property = (ProtoObjectProperty *) raw_malloc;
            *res_prop++ = target_property;

#ifdef __SPECTRUM
            target_property->key = property->key;
            target_property->value_size = property->value_size;
            memcpy(target_property->value, property->value, property->value_size);
#else
            proto_property_write(raw_malloc, property->key, property->value, property->value_size);
#endif

            raw_malloc += property->value_size + sizeof(ProtoObjectProperty);
        }
        property = property->prev;
    }

    *res_prop = NULL;
    return obj;
}

ProtoObject* proto_object_copy(ProtoObject* obj) API_DECL
{
    uint8_t number_of_properties = 0;
    {
        ProtoObjectPropertyPtr* p = obj->properties;

        while (*p)
        {
            number_of_properties++;
            p++;
        }
    }

    uint8_t* raw_malloc = malloc(sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*) + obj->object_size + sizeof(ProtoObjectRequestHeader));
    ProtoObject* copy = (ProtoObject*)raw_malloc;
    uint8_t* dst_data = raw_malloc + sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*) + sizeof(ProtoObjectRequestHeader);

    copy->object_size = obj->object_size;


    {
        uint8_t* it = dst_data;

        ProtoObjectPropertyPtr* cp = copy->properties;
        ProtoObjectPropertyPtr* p = obj->properties;

        while (*p)
        {
            ProtoObjectProperty* prop = (ProtoObjectProperty*)it;
            uint16_t value_size = proto_property_value_size(*p);
            memcpy(it, *p, value_size + sizeof(ProtoObjectProperty));
            it += value_size + sizeof(ProtoObjectProperty);
            *cp++ = prop;
            p++;
        }

        *cp = NULL;
    }

    return copy;
}

char* copy_str_property(ProtoObjectProperty* property) API_DECL
{
    uint16_t value_size = proto_property_value_size(property);
    char* result = malloc(value_size + 1);
    result[value_size] = 0;
    memcpy(result, property->value, value_size);
    return result;
}
