/* bnlisp - lisp for the PDP-11 under 2.11BSD
 * dedicated to Marc Cleave
 *
 * by Robert Smith and Aaron Jackson
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "util.h"
#include "obj.h"
#include "machine.h"
#include "alloc.h"
#include "parser.h"
#include "symbol.h"
#include "env.h"
#include "prim.h"
#include "lisp.h"

/* eval each element of a list */
obj_t *evlis(args, env)
  obj_t *args, **env;
{
  obj_t * head = NULL;
  obj_t * current = NULL;
  obj_t * node;

  if (nil == args) return nil;

  for (node = args; node != nil; node = CDR(node)) {
    if (!current) {
      current = alloc_cons(nil, nil);
      head = current;
    } else {
      current->value.c.cdr = alloc_cons(nil, nil);
      current = current->value.c.cdr;
    }
    current->value.c.car = eval(node->value.c.car, env);
  }
  return head;
}

obj_t *apply(fn, args, env)
     obj_t *fn, *args, **env;
{
  obj_t *aug_env, *eargs;

  if (TPRIMITIVE == fn->type) {
    return fn->value.prim.code(env, args);
  } else if (TFUNCTION == fn->type) {
    /* env is ignored, because of lexical scope */
    eargs = evlis(args, env);
    aug_env = augment_env(fn->value.fun.env, fn->value.fun.params, eargs);
    return primitive_progn(&aug_env, fn->value.fun.body);
  } else {
    fuck("can't apply this thing");
  }
  fuck("cant reach here");
  return NULL;
}

obj_t *eval(form, env)
  obj_t *form, **env;
{
  obj_t *op, *args, *val;

  switch (form->type) {
    /* self evaluating forms */
  case TNIL:
  case TTRUE:
  case TINT:
  case TFLOAT:
  case TSTRING:
  case TPRIMITIVE:
  case TFUNCTION:
  case TSOCKET:
  case TSTREAM:
    return form;

  case TSYMBOL:
    val = lookup_env(*env, form);
    if (!val) {
      printf("undefined: ");
      print(form);
      putchar('\n');
      fuck("undefined variable");
    }
    return CDR(val);

    /* a form to evaluate: (f x1 x2 ...) */
  case TCONS:
    if (!proper_list_p(form)) fuck("no bueno thing being eval'd");

    op = eval(CAR(form), env);
    args = CDR(form);

    if (TPRIMITIVE != op->type && TFUNCTION != op->type){
      printf("got type %d\n", op->type);
      fuck("bad operator type");
    }

    return apply(op, args, env);
    fuck("bug: shouldn't get here in eval");
  default:
    fuck("i don't know how to eval this object");
  }
  fuck("shouldn't get here");
  return NULL;
}


void print(o)
     obj_t *o;
{
  int i;
  char c;
  switch (o->type) {
  case TCONS:
    putchar('(');
    for (;;) {
      print(CAR(o));
      if (nil == CDR(o))
        break;
      if (TCONS != CDR(o)->type) {
        printf(" . ");
        print(CDR(o));
        break;
      }
      putchar(' ');
      o = CDR(o);
    }
    putchar(')');
    return;

  case TSTRING:
    putchar('"');
    for (i = 0; o->value.str[i]; i++) {
      c = o->value.str[i];
      if ('\t' == c)
        printf("\\t");
      else if ('"' == c)
        printf("\\\"");
      else if ('\\' == c)
        printf("\\\\");
      else if ('\n' == c)
	printf("\\n");
      else if ('\r' == c)
	printf("\\r");
      else
        putchar(c);
    }
    putchar('"');
    return;

  case TSYMBOL:
    printf("%s", o->value.sym.name);
    return;

  case TINT:
    printf("%d", o->value.i);
    return;

  case TFLOAT:
    printf("%f", o->value.f);
    return;

  case TTRUE:
    printf("T");
    return;

  case TNIL:
    printf("NIL");
    return;

  case TPRIMITIVE:
    printf("<primitive>");
    return;

  case TFUNCTION:
    printf("<fn of ");
    print(o->value.fun.params);
    printf(">");
    return;

  case TSTREAM:
    printf("<stream file>");
    return;

  case TSOCKET:
    printf("<stream network>");
    return;

  default:
    fuck("print: unknown type");
  }
}

void init_lisp ()
{
  obj_t **env;
  VM = alloc_vm(NUM_OBJECTS * OBJ_SIZE);
  nil = alloc_obj(TNIL);
  tru = alloc_obj(TTRUE);
  VM->symbols = nil;
  VM->global_bindings = nil;
  env = &VM->global_bindings;
  *env = push_env(*env,
                  intern("PROGN"),
                  alloc_primitive(primitive_progn));
  *env = push_env(*env,
                  intern("QUOTE"),
                  alloc_primitive(primitive_quote));
  *env = push_env(*env,
                  intern("WHILE"),
                  alloc_primitive(primitive_while));
  *env = push_env(*env,
                  intern("IF"),
                  alloc_primitive(primitive_if));
  *env = push_env(*env,
                  intern("SETQ"),
                  alloc_primitive(primitive_setq));
  *env = push_env(*env,
                  intern("LAMBDA"),
                  alloc_primitive(primitive_lambda));
  *env = push_env(*env,
                  intern("+"),
                  alloc_primitive(primitive_add));
  *env = push_env(*env,
                  intern("-"),
                  alloc_primitive(primitive_subtract));
  *env = push_env(*env,
                  intern("*"),
                  alloc_primitive(primitive_multiply)); 
  *env = push_env(*env,
                  intern("CONS"),
                  alloc_primitive(primitive_cons));
  *env = push_env(*env,
                  intern("CAR"),
                  alloc_primitive(primitive_car));
  *env = push_env(*env,
                  intern("CDR"),
                  alloc_primitive(primitive_cdr));
  *env = push_env(*env,
                  intern("RPLACA"),
                  alloc_primitive(primitive_rplaca));
  *env = push_env(*env,
                  intern("RPLACD"),
                  alloc_primitive(primitive_rplacd));
  *env = push_env(*env,
                  intern("EVAL"),
                  alloc_primitive(primitive_eval));
  *env = push_env(*env,
                  intern("PRINT"),
                  alloc_primitive(primitive_print));
  *env = push_env(*env,
                  intern("PRINC"),
                  alloc_primitive(primitive_princ));
  *env = push_env(*env,
                  intern("ALL-SYMBOLS"),
                  alloc_primitive(primitive_all_symbols));
  *env = push_env(*env,
                  intern("EQ"),
                  alloc_primitive(primitive_eq));
  *env = push_env(*env,
                  intern("="),
                  alloc_primitive(primitive_number_equals));
  *env = push_env(*env,
		  intern(">"),
		  alloc_primitive(primitive_number_gt));
  *env = push_env(*env,
		  intern("STRING="),
		  alloc_primitive(primitive_string_equals));
  *env = push_env(*env,
                  intern("NOT"),
                  alloc_primitive(primitive_not));
  *env = push_env(*env,
		  intern("READ-CHAR"),
		  alloc_primitive(primitive_readchar));
  *env = push_env(*env,
		  intern("READ"),
		  alloc_primitive(primitive_read));
  *env = push_env(*env,
		  intern("LOAD"),
		  alloc_primitive(primitive_load));
  *env = push_env(*env,
		  intern("CONCATENATE"),
		  alloc_primitive(primitive_concatenate));

  /* SOCKETS and STREAMS */
  *env = push_env(*env,
		  intern("STREAM-OPEN"),
		  alloc_primitive(primitive_stream_open));
  *env = push_env(*env,
		  intern("STREAM-CLOSE"),
		  alloc_primitive(primitive_stream_close));
  *env = push_env(*env,
		  intern("STREAM-READ"),
		  alloc_primitive(primitive_stream_read));
  *env = push_env(*env,
  		  intern("STREAM-WRITE"),
  		  alloc_primitive(primitive_stream_write));
  *env = push_env(*env,
		  intern("STREAM-EOF?"),
		  alloc_primitive(primitive_stream_iseof));

}

void eval_form(f, env)
  obj_t *f, **env;
{
  printf("\n INPUT: ");
  print(f);
  putchar('\n');
  f = eval(f, env);
  printf("OUTPUT: ");
  print(f);
  printf("\n");
}

int main (argc, argv)
     int argc;
     char *argv[];
{
  obj_t *form;
  int silent = 0;

  silent = (argc > 1 && 0 == strcmp(argv[1], "-s"));

  init_lisp();
  stream_i = stdin;
  stream_o = stdout;

  fprintf(stderr, "welcome to bnlisp\n");

  for (;;) {
    form = read_sexp();
    if (!form) break;
    if (silent) {
      eval(form, &VM->global_bindings);
    } else {
      eval_form(form, &VM->global_bindings);
    }
  }

  exit(1);
}
