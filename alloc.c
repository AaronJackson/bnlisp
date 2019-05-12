#include <stdlib.h>

#include "util.h"
#include "obj.h"
#include "machine.h"
#include "alloc.h"


obj_t * alloc_obj(type)
     obj_type_t type;
{
  obj_t *o;

  if (VM->alloc_offset == VM->semispace_size) gc();
  /*  if (VM->alloc_offset == VM->semispace_size) fuck("game over"); */

  o = &VM->from_space[++VM->alloc_offset];
  o->type = type;
  return o;
}

obj_t *alloc_int(val)
     int val;
{
  obj_t * x = alloc_obj(TINT);
  x->value.i = val;
  return x;
}

obj_t *alloc_float(val)
     float val;
{
  obj_t * x = alloc_obj(TFLOAT);
  x->value.f = val;
  return x;
}

obj_t * alloc_string(s)
     char * s;
{
  obj_t *x = alloc_obj(TSTRING);
  x->value.str = s;
  return x;
}

obj_t * alloc_cons(ca, cd)
  obj_t *ca, *cd;
{
  obj_t *x = alloc_obj(TCONS);
  CAR(x) = ca;
  CDR(x) = cd;
  return x;
}

obj_t *alloc_primitive(code)
     primitive_t code;
{
  obj_t *x = alloc_obj(TPRIMITIVE);
  x->value.prim.code = code;
  return x;
}

obj_t *alloc_function(params, body, env)
     obj_t *params, *body, *env;
{
  obj_t *x = alloc_obj(TFUNCTION);
  x->value.fun.params = params;
  x->value.fun.body = body;
  x->value.fun.env = env;
  return x;
}

obj_t *alloc_socket(socket)
     int socket;
{
  obj_t * x = alloc_obj(TSOCKET);
  x->value.i = socket;
  return x;
}


obj_t *alloc_stream(stream)
     FILE *stream;
{
  obj_t *x = alloc_obj(TSTREAM);
  x->value.stream = stream;
  return x;
}
