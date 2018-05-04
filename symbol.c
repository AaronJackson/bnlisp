#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "obj.h"
#include "alloc.h"
#include "machine.h"
#include "lisp.h"
#include "symbol.h"

obj_t *find_symbol(name)
     char *name;
{
  obj_t *entry, *table, *val;
  for (table = VM->symbols; nil != table; table = CDR(table)) {
    entry = CAR(table);
    assert(TSYMBOL == entry->type);
    if (0 == strcmp(entry->value.sym.name, name)) {
      return entry;
    }
  }
  return NULL;
}

obj_t *alloc_symbol(name)
     char *name;
{
  obj_t *x;
  x = alloc_obj(TSYMBOL);
  x->value.sym.name = name;
  return x;
}

obj_t *intern(name)
     char *name;
{
  obj_t *x;
  x = find_symbol(name);
  if (!x) {
    x = alloc_symbol(name);
    VM->symbols = alloc_cons(x, VM->symbols);
  }
  return x;
}

int proper_list_p(o)
     obj_t * o;
{
  if (nil == o) return 1;
  if (TCONS != o->type) return 0;
  return proper_list_p(CDR(o));
}

int list_length(o)
     obj_t * o;
{
  /* assumes that o is a proper list */
  obj_t * node;
  int len = 0;
  for (node = o; node != nil; node = node->value.c.cdr) len++;
  return len;
}
