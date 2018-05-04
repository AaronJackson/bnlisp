#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "obj.h"
#include "machine.h"
#include "alloc.h"
#include "lisp.h"
#include "env.h"
#include "parser.h"
#include "prim.h"

obj_t *primitive_progn(env, body)
  obj_t **env, *body;
{
  obj_t *ret;

  for ( ; nil != body; body = CDR(body)) {
    ret = eval(CAR(body), env);
  }

  return ret;
}

obj_t *primitive_quote(env, args)
  obj_t **env, *args;
{
  return FIRST(args);
}

obj_t *primitive_lambda(env, args)
  obj_t **env, *args;
{
  obj_t *params, *body;
  params = FIRST(args);
  body   = REST(args);
  return alloc_function(params, body, *env);
}

obj_t *primitive_setq(env, args)
  obj_t **env, *args;
{
  obj_t *entry, *var, *val;
  var = FIRST(args);
  assert(TSYMBOL == var->type);
  entry = lookup_env(*env, var);
  /* we do this before EVAL so recursion works */
  if (!entry) {
    *env = push_env(*env, var, nil);
    entry = lookup_env(*env, var);
  }
  val = eval(SECOND(args), env);
  CDR(entry) = val;
  return val;
}

obj_t *primitive_if(env, args)
  obj_t **env, *args;
{
  obj_t *cond = eval(args->value.c.car, env);
  if (nil != cond)
    return eval(SECOND(args), env);
  else
    return eval(THIRD(args), env);
}

obj_t *primitive_while(env, args)
  obj_t **env, *args;
{
  obj_t *forms, *cond = FIRST(args);

  while (nil != eval(cond, env)) {
    for (forms = REST(args); nil != forms; forms = CDR(forms)) {
      eval(CAR(forms), env);
    }
  }
  return nil;
}

obj_t *primitive_add(env, args)
     obj_t **env, *args;
{
  int sum = 0;
  obj_t *node, *node_val;

  for (node = args; node != nil; node = CDR(node)) {
    node_val = eval(CAR(node), env);
    if (TINT != node_val->type) fuck("can only add ints");

    sum += node_val->value.i;
  }

  return alloc_int(sum);
}

obj_t *primitive_eval(env, args)
     obj_t **env, *args;
{
  obj_t *eargs = evlis(args, env);
  return eval(FIRST(eargs), env);
}

obj_t *primitive_cons(env, args)
     obj_t **env, *args;
{
  obj_t *eargs = evlis(args, env);
  return alloc_cons(FIRST(eargs), SECOND(eargs));
}

obj_t *primitive_car(env, args)
     obj_t **env, *args;
{
  obj_t *eargs = evlis(args, env);
  return CAR(FIRST(eargs));
}

obj_t *primitive_cdr(env, args)
     obj_t **env, *args;
{
  obj_t *eargs = evlis(args, env);
  return CDR(FIRST(eargs));
}

obj_t *primitive_rplaca(env, args)
     obj_t **env, *args;
{
  obj_t *eargs = evlis(args, env);

  CAR(FIRST(eargs)) = SECOND(eargs);
  return FIRST(eargs);
}

obj_t *primitive_rplacd(env, args)
     obj_t **env, *args;
{
  obj_t *eargs = evlis(args, env);
  CDR(FIRST(eargs)) = SECOND(eargs);
  return FIRST(eargs);
}

obj_t *primitive_print(env, args)
     obj_t **env, *args;
{
  obj_t *arg = eval(FIRST(args), env);
  print(arg);
  putchar('\n');
  return arg;
}

obj_t *primitive_all_symbols(env, args)
     obj_t **env, *args;
{
  return VM->symbols;
}

obj_t *primitive_eq(env, args)
     obj_t **env, *args;
{
  obj_t *a, *b;
  obj_t *eargs = evlis(args, env);
  a = FIRST(eargs);
  b = SECOND(eargs);

  /* type mismatch */
  if (a->type != b->type) return nil;

  /* constants */
  if (nil == a || tru == a) return tru;

  /* numbers */
  if (TINT == a->type) return (a->value.i == b->value.i) ? tru : nil;

  /* everything else */
  return (a == b) ? tru : nil;
}

obj_t *primitive_number_equals(env, args)
     obj_t **env, *args;
{
  obj_t *a, *b;
  obj_t *eargs = evlis(args, env);
  a = FIRST(eargs);
  b = SECOND(eargs);
  if (TINT != a->type || TINT != b->type) fuck("can't do = on non-numbers");

  return (a->value.i == b->value.i) ? tru : nil;
}

obj_t *primitive_string_equals(env, args)
  obj_t **env, *args;
{
  obj_t *a, *b;
  obj_t *eargs = evlis(args, env);

  a = FIRST(eargs);
  b = SECOND(eargs);

  if (TSTRING != a->type || TSTRING != b->type)
    fuck("can't do STRING= on non-strings");

  return (0 == strcmp(a->value.str, b->value.str)) ? tru : nil;
}

obj_t *primitive_not(env, args)
     obj_t **env, *args;
{
  obj_t *eargs = evlis(args, env);
  return (nil == FIRST(eargs)) ? tru : nil;
}

obj_t *primitive_readchar(env, args)
  obj_t **env, *args;
{
  char c = getchar();
  char s[2];
  s[0] = c;
  s[1] = '\0';
  return alloc_string(s);
}

obj_t *primitive_read(env, args)
  obj_t **env, *args;
{
  return read_sexp();
}
