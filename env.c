#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "obj.h"
#include "alloc.h"
#include "lisp.h"
#include "symbol.h"
#include "env.h"

obj_t *lookup_env(env, sym)
  obj_t *env, *sym;
{
  obj_t *entry;
  assert(TSYMBOL == sym->type);
  for (; nil != env; env = CDR(env)) {
    entry = CAR(env);
    assert(TCONS == entry->type);
    if (sym == CAR(entry))
      return entry;
  }
  return NULL;
}

obj_t *push_env(env, sym, val)
  obj_t *env, *sym, *val;
{
  obj_t *entry = alloc_cons(sym, val);
  return alloc_cons(entry, env);
}

/* syms: list of symbols
   vals: list of evaluated values */
obj_t *augment_env(env, syms, vals)
  obj_t *env, *syms, *vals;
{
  obj_t *entry;
  obj_t *aug_env = env;
  if (list_length(syms) != list_length(vals)) fuck("fun/arg mismatch");
  while (nil != syms) {
    entry = alloc_cons(CAR(syms), CAR(vals));
    aug_env = alloc_cons(entry, aug_env);
    syms = CDR(syms);
    vals = CDR(vals);
  }
  assert(nil == syms);
  assert(nil == vals);
  return aug_env;
}

obj_t *pop_env(env)
     obj_t *env;
{
  /* memory leak */
  if (nil == env) return nil;
  return CDR(env);
}
