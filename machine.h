
#ifndef _INC_MACHINE_H
#define _INC_MACHINE_H

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
  /* stack - a fixed size C array of void* */
  int sp;
  int stack_size;
  void **stack;
} vm_t;

#define FRAME_END ((obj_t *) (-1))

vm_t *alloc_vm();
void push_stack();
void pop_stack();
void *forward();
void gc();

vm_t  *VM;


#endif /* _INC_MACHINE_H */
