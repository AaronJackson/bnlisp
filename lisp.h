#ifndef BNLISP_LISP_H
#define BNLISP_LISP_H

/* initialized by init_lisp */
obj_t *nil;
obj_t *tru;

#define NUM_OBJECTS 1024

obj_t *eval();
obj_t *evlis();
obj_t *apply();
void print();

#endif
