
/* initialized by init_lisp */
obj_t *nil;
obj_t *tru;

#define NUM_OBJECTS 1024
#define STACK_SIZE 512

obj_t *eval();
obj_t *evlis();
obj_t *apply();
obj_t *eval();
void print();
