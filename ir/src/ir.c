#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "infrared.h"

static mp_obj_t mp_ir_receive(void) {
    static uint32_t buf[2000];

    int len = infrared_receive_blocking(buf, 2000);

    mp_obj_t list = mp_obj_new_list(len, NULL);
    for (int i = 0; i < len; i++) {
        mp_obj_list_store(list, MP_OBJ_NEW_SMALL_INT(i), MP_OBJ_NEW_SMALL_INT(buf[i]));
    }

    return list;
}
static MP_DEFINE_CONST_FUN_OBJ_0(mp_ir_receive_obj, mp_ir_receive);

static mp_obj_t mp_ir_send(mp_obj_t list_obj) {
    size_t len;
    mp_obj_t *items;
    mp_obj_list_get(list_obj, &len, &items);

    static uint32_t buf[2000];
    for (size_t i = 0; i < len; i++) {
        buf[i] = mp_obj_get_int(items[i]);
    }

    infrared_send(buf, len, true);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_ir_send_obj, mp_ir_send);

static mp_obj_t mp_ir_init(void) {
    infrared_send_init();
    infrared_receive_init();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(mp_ir_init_obj, mp_ir_init);

static const mp_rom_map_elem_t ir_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&mp_ir_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_receive), MP_ROM_PTR(&mp_ir_receive_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&mp_ir_send_obj) },
};

static MP_DEFINE_CONST_DICT(ir_module_globals, ir_module_globals_table);

const mp_obj_module_t ir_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&ir_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_ir, ir_user_cmodule);
