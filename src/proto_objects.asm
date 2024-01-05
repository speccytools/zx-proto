public _get_uint16_property
public _get_uint8_property
public _get_property_ptr
public _find_property

; uint16_t get_uint16_property(ProtoObject* o, uint8_t key, uint16_t def) API_DECL

_get_uint16_property:
    pop iy                          ; store ret

    pop ix                          ; get default value into ix
    pop bc                          ; get key into c
    pop hl                          ; ProtoObject*

    inc hl                          ; access ProtoObjectProperty* properties[];
    inc hl                          ; by skipping object_size

    ld de, hl                       ; get it into de

get_16_next_property:
    ld a, (de)                      ; get ProtoObjectProperty
    ld l, a
    inc de                          ; into hl
    ld a, (de)
    ld h, a
    inc de

    ld a, h                         ; NULL?
    or l
    jr z, get_16_not_found          ; end of object, return

    ; each ProtoObjectProperty has the following syntax
    ; uint16_t value_size;
    ; uint8_t key;
    ; char value[];

    ld a, 2
    cp (hl)                         ; compaere value_size.low
    jr nz, get_16_next_property     ; not 0x02
    inc hl
    ld a, 0
    cp (hl)                         ; compaere value_size.high
    jr nz, get_16_next_property     ; not 0x0002
    inc hl
    ld a, c                         ; get key into a
    cp (hl)                         ; compaere key
    jr nz, get_16_next_property     ; key does not match
    inc hl
    ex de, hl
    ld l, (de)                      ; get uint16 value into hl (ret)
    inc de
    ld h, (de)
    jr done_16

get_16_not_found:
    ld hl, ix                       ; return default value

done_16:
    push iy
    ret


; uint8_t get_uint8_property(ProtoObject* o, uint8_t key, uint8_t def) API_DECL

_get_uint8_property:
    pop iy                          ; store ret

    pop ix                          ; get default value into ixl
    pop bc                          ; get key into c
    pop hl                          ; ProtoObject*

    inc hl                          ; access ProtoObjectProperty* properties[];
    inc hl                          ; by skipping object_size

    ld de, hl                       ; get it into de

get_8_next_property:
    ld a, (de)                      ; get ProtoObjectProperty
    ld l, a
    inc de                          ; into hl
    ld a, (de)
    ld h, a
    inc de

    ld a, h                         ; NULL?
    or l
    jr z, get_8_not_found           ; end of object, return

    ; each ProtoObjectProperty has the following syntax
    ; uint16_t value_size;
    ; uint8_t key;
    ; char value[];

    ld a, 1
    cp (hl)                         ; compaere value_size.low
    jr nz, get_8_next_property      ; not 0x01
    inc hl
    ld a, 0
    cp (hl)                         ; compaere value_size.high
    jr nz, get_8_next_property      ; not 0x0001
    inc hl
    ld a, c                         ; get key into a
    cp (hl)                         ; compaere key
    jr nz, get_8_next_property      ; key does not match
    inc hl
    ex de, hl
    ld l, (de)                      ; get uint8 value into hl (ret)
    ld h, 0
    jr done_8

get_8_not_found:
    ld h, 0
    ld a, ixl                       ; return default value
    ld l, a

done_8:
    push iy
    ret


; ProtoObjectProperty* find_property(ProtoObject* o, uint8_t key) API_DECL

_find_property:
    pop iy                          ; store ret

    pop bc                          ; get key into c
    pop hl                          ; ProtoObject*

    inc hl                          ; access ProtoObjectProperty* properties[];
    inc hl                          ; by skipping object_size

    ld de, hl                       ; get it into de

get_next_property:
    ld a, (de)                      ; get ProtoObjectProperty
    ld ixl, a
    inc de                          ; into ix
    ld a, (de)
    ld ixh, a
    inc de

    ld a, ixh                       ; NULL?
    or ixl
    jr z, get_not_found             ; end of object, return

    ; each ProtoObjectProperty has the following syntax
    ; uint16_t value_size;
    ; uint8_t key;
    ; char value[];

    ld a, c                         ; get key into a
    cp (ix+2)                       ; compaere key
    jr nz, get_next_property        ; key does not match

    ld a, ixh                       ; return default value
    ld h, a
    ld a, ixl
    ld l, a

    jr done

get_not_found:
    ld hl, 0                        ; return NULL

done:
    push iy
    ret


; void* get_property_ptr(ProtoObject* o, uint8_t key) API_DECL

_get_property_ptr:
    pop iy                          ; store ret

    pop bc                          ; get key into c
    pop hl                          ; ProtoObject*

    inc hl                          ; access ProtoObjectProperty* properties[];
    inc hl                          ; by skipping object_size

    ld de, hl                       ; get it into de

get_ptr_next_property:
    ld a, (de)                      ; get ProtoObjectProperty
    ld ixl, a
    inc de                          ; into ix
    ld a, (de)
    ld ixh, a
    inc de

    ld a, ixh                       ; NULL?
    or ixl
    jr z, get_ptr_not_found         ; end of object, return

    ; each ProtoObjectProperty has the following syntax
    ; uint16_t value_size;
    ; uint8_t key;
    ; char value[];

    ld a, c                         ; get key into a
    cp (ix+2)                       ; compaere key
    jr nz, get_ptr_next_property    ; key does not match

    ld a, ixh                       ; value
    ld h, a
    ld a, ixl
    ld l, a

    inc hl                          ; skip key and size
    inc hl
    inc hl

    jr get_ptr_done

get_ptr_not_found:
    ld hl, 0                        ; return NULL

get_ptr_done:
    push iy
    ret