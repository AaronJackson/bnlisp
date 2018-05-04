
#ifndef BNLISP_MACHINE_H
#define BNLISP_MACHINE_H

typedef struct vm {
  /* memory */
  size_t semispace_size;
  obj_t *from_space;
  obj_t *to_space;
  size_t alloc_offset;
  size_t to_space_offset;
  /* symbol intern table - lisp list of symbols */
  obj_t *symbols;
  /* global environment - lisp list of cons cells */
  obj_t *global_bindings;
} vm_t;

#define FRAME_END ((obj_t *) (-1))

vm_t *alloc_vm();
void *forward();
void gc();

vm_t  *VM;


#endif /* BNLISP_MACHINE_H */
