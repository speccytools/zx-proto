#include <proto_objects.h>

#include <stdlib.h>
#include <string.h>

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

    uint8_t* raw_malloc = malloc(sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*) + object_size + 2);
    ProtoObject* obj = (ProtoObject*)raw_malloc;
    raw_malloc += sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*) + 2;
    obj->object_size = object_size;
    ProtoObjectProperty** res_prop = obj->properties;

    property = last_property;
    while (property)
    {
        if (property->value_size)
        {
            ProtoObjectProperty *target_property = (ProtoObjectProperty *) raw_malloc;
            *res_prop++ = target_property;

            target_property->key = property->key;
            target_property->value_size = property->value_size;
            memcpy(target_property->value, property->value, property->value_size);

            raw_malloc += target_property->value_size + sizeof(ProtoObjectProperty);
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
        ProtoObjectProperty** p = obj->properties;

        while (*p)
        {
            number_of_properties++;
            p++;
        }
    }

    uint8_t* raw_malloc = malloc(sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*) + obj->object_size + 2);
    ProtoObject* copy = (ProtoObject*)raw_malloc;
    uint8_t* dst_data = raw_malloc + sizeof(ProtoObject) + (number_of_properties + 1) * sizeof(ProtoObjectProperty*) + 2;

    copy->object_size = obj->object_size;


    {
        uint8_t* it = dst_data;

        ProtoObjectProperty** cp = copy->properties;
        ProtoObjectProperty** p = obj->properties;

        while (*p)
        {
            ProtoObjectProperty* prop = (ProtoObjectProperty*)it;
            memcpy(it, *p, (*p)->value_size + sizeof(ProtoObjectProperty));
            it += (*p)->value_size + sizeof(ProtoObjectProperty);
            *cp++ = prop;
            p++;
        }

        *cp = NULL;
    }

    return copy;
}

char* copy_str_property(ProtoObjectProperty* property) API_DECL
{
    char* result = malloc(property->value_size + 1);
    result[property->value_size] = 0;
    memcpy(result, property->value, property->value_size);
    return result;
}