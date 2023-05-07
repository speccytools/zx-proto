public _proto_object_read

; Stack:
; const uint8_t* buffer_from
; uint16_t object_size
; uint16_t buffer_available
; ProtoObject* objs

_proto_object_read:

    pop iy                      ; ret

    pop hl                      ; buffer_from
    pop de                      ; object_size
    pop bc                      ; buffer_available
    pop ix                      ; obj

    push iy                     ; ret

    ld (ix), e                  ; obj->object_size = object_size, switch to properties
    inc ix
    ld (ix), d
    inc ix

    push hl
    add hl, de
    ex de, hl                   ; end = it + object_size;
    pop hl

proto_object_read_property:
    ld (ix), l                  ; *properties++ = (ProtoObjectProperty*)it;
    inc ix
    ld (ix), h
    inc ix

    ld c, (hl)                  ; get ((ProtoObjectProperty*)it)->value_size
    inc hl
    ld b, (hl)
    inc hl

    inc hl                      ; it += 1 (key)

    add hl, bc                  ; it += ((ProtoObjectProperty*)it)->value_size

    or a
    sbc hl, de
    add hl, de
    jr c, proto_object_read_property

    ld (ix), 0                  ; *properties = NULL;
    inc ix
    ld (ix), 0

    ld hl, 0                    ; return 0

    ret
