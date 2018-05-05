#ifndef BNLISP_PRIM_H
#define BNLISP_PRIM_H

obj_t *primitive_progn();
obj_t *primitive_quote();
obj_t *primitive_lambda();
obj_t *primitive_setq();
obj_t *primitive_if();
obj_t *primitive_while();
obj_t *primitive_add();
obj_t *primitive_eval();
obj_t *primitive_cons();
obj_t *primitive_car();
obj_t *primitive_cdr();
obj_t *primitive_rplaca();
obj_t *primitive_rplacd();
obj_t *primitive_print();
obj_t *primitive_all_symbols();
obj_t *primitive_eq();
obj_t *primitive_number_equals();
obj_t *primitive_string_equals();
obj_t *primitive_not();
obj_t *primitive_readchar();
obj_t *primitive_read();
obj_t *primitive_load();
#endif

