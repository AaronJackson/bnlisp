#include <stdlib.h>

#include "util.h"
#include "obj.h"
#include "machine.h"

vm_t *alloc_vm(size)
     size_t size;
{
  vm_t *vm = calloc(1, sizeof (vm_t));
  vm->semispace_size = size / OBJ_SIZE;
  vm->from_space = calloc(size, OBJ_SIZE);
  vm->to_space = calloc(size, OBJ_SIZE);
  vm->alloc_offset = 0;
  vm->to_space_offset = 0;
  vm->symbols = NULL;
  vm->global_bindings = NULL;
  return vm;
}

#if 0
/* forward o to the to_space, and return where it ended up */
obj_t *forward(o)
     obj_t *o;
{
  obj_t *ca, *cd;
  /* if it has already been moved, just return it */
  if (STATE_FORWARDED == o->type) return o;
  switch (o->type) {
  case TNIL:
  case TTRUE:
  case TINT:
  case TSTRING:
  case TSYMBOL:
  case TPRIMITIVE:
    memcpy((void *)(VM->to_space + VM->to_space_offset), o, OBJ_SIZE);
    o->type = STATE_FORWARDED;
    o->value.new_location = VM->to_space + VM->to_space_offset;
    VM->to_space_offset++;
    return o->value.new_location;

  case TCONS:
    ca = CAR(o);
    cd = CDR(o);
    memcpy((void *)(VM->to_space + VM->to_space_offset), o, OBJ_SIZE);
    o->type = STATE_FORWARDED;
    o->value.new_location = VM->to_space + VM->to_space_offset;
    VM->to_space_offset++;
    CAR(o->value.new_location) = forward(ca);
    CDR(o->value.new_location) = forward(cd);
    return o->value.new_location;

  default:
    fuck("unknown type to forward");
  }
  /* unreachable */
  return NULL;
}

#endif

void gc() {
  #if 0
  obj_t *tmp;
  size_t allocated = VM->alloc_offset;
  int i, j;
  obj_t **frame_pointer;

  fprintf(stderr, "\ngc start... ");

  /* traverse the constants */
  fprintf(stderr, "c");
  forward(nil);
  forward(tru);

  /* traverse stack */
  fprintf(stderr, "S");
  /* TODO */

  /* traverse the symbols */
  fprintf(stderr, "s");
  VM->symbols = forward(VM->symbols);

  /* traverse the global bindings */
  fprintf(stderr, "g");
  VM->global_bindings = forward(VM->global_bindings);

  /* expunge and swap */
  VM->alloc_offset = VM->to_space_offset;
  VM->to_space_offset = 0;
  memset(VM->from_space, 0, VM->semispace_size * OBJ_SIZE);
  tmp = VM->from_space;
  VM->from_space = VM->to_space;
  VM->to_space = VM->from_space;


  fprintf(stderr, " done: %zu -> %zu\n", allocated, VM->alloc_offset);
  #endif
  return;
}
