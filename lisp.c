/* bnlisp - lisp for the PDP-11 under 2.11BSD
 * dedicated to Marc Cleave
 *
 * by Robert Smith and Aaron Jackson
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/***** UTILITIES *****/

void fuck(msg)
     char * msg;
{
  printf("fuck: %s\n", msg);
  exit(1);
}


/***** OBJECT REPRESENTATION *****/

typedef enum {
  /* states */
  /* STATE_FORWARDED, */
  /* types */
  TNIL,
  TTRUE,
  TINT,
  TCONS,
  TSTRING,
  TSYMBOL,
  TPRIMITIVE
} obj_type_t;

/* function type: (obj **env, obj *args) -> obj *return */
/* args are expected to have been evaluated */
typedef struct obj * (*primitive_t)();

typedef struct obj {
  int type;
  union {
    /* when the object moves during GC, it gets a new location */
    /* struct obj *new_location; */
    int i;
    char *str;
    struct {
      char *name;
    } sym;
    struct {
      struct obj *car;
      struct obj *cdr;
    } c;
    struct {
      primitive_t code;
    } prim;
  } value;
} obj_t;

#define OBJ_SIZE (sizeof (obj_t))

#define CAR(x) ((x)->value.c.car)
#define CDR(x) ((x)->value.c.cdr)
#define FIRST(x) CAR(x)
#define SECOND(x) CAR(CDR(x))
#define THIRD(x) CAR(CDR(CDR(x)))
#define REST(x) CDR(x)

/***** MACHINE REPRESENTATION *****/

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

/* initialized by init_lisp */
obj_t *nil = NULL;
obj_t *tru = NULL;
vm_t  *VM = NULL;

vm_t *alloc_vm(size, stack_size)
     size_t size;
{
  vm_t *vm = calloc(1, sizeof (vm_t));
  vm->stack_size = stack_size;
  vm->sp = stack_size - 1;
  vm->stack = calloc(size, sizeof (void*));
  vm->semispace_size = size / OBJ_SIZE;
  vm->from_space = calloc(size, OBJ_SIZE);
  vm->to_space = calloc(size, OBJ_SIZE);
  vm->alloc_offset = 0;
  vm->to_space_offset = 0;
  vm->symbols = NULL;
  vm->global_bindings = NULL;
  return vm;
}

void push_stack(p)
     void *p;
{
  if (0 == VM->sp) {
    fuck("stack overflow");
  }
  VM->stack[--VM->sp] = p;
}

void pop_stack(n)
     int n;
{
  if (VM->sp + n >= VM->stack_size) {
    VM->sp = VM->stack_size - 1;
  } else {
    VM->sp += n;
  }
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

#define FRAME_END ((obj_t *) (-1))
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
  for (i = VM->sp; i < VM->stack_size - 1; i++) {
    frame_pointer = (obj_t **)VM->stack[i];
    for (j = 0; FRAME_END != frame_pointer[j]; j++) {
      if (frame_pointer[j])
        frame_pointer[j] = forward(frame_pointer[j]);
    }
  }

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

obj_t * alloc_obj(type)
     obj_type_t type;
{
  obj_t *o;

  if (VM->alloc_offset == VM->semispace_size) gc();
  if (VM->alloc_offset == VM->semispace_size) fuck("game over");

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

obj_t * alloc_string(s)
     char * s;
{
  obj_t * x = alloc_obj(TSTRING);
  x->value.str = s;
  return x;
}

obj_t * alloc_cons(ca, cd)
  obj_t * ca, * cd;
{
  obj_t * x = alloc_obj(TCONS);
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

/***** ENVIRONMENTS *****/

obj_t *lookup_env(env, sym)
  obj_t *env, *sym;
{
  obj_t *entry;
  assert(TSYMBOL == sym->type);
  for (; nil != env; env = CDR(env)) {
    entry = CAR(env);
    assert(TCONS == entry->type);
    if (sym == CAR(entry))
      return CDR(entry);
  }
  return NULL;
}

obj_t *push_env(env, sym, val)
  obj_t *env, *sym, *val;
{
  obj_t *entry = alloc_cons(sym, val);
  return alloc_cons(entry, env);
}

obj_t *pop_env(env)
     obj_t *env;
{
  /* memory leak */
  if (nil == env) return nil;
  return CDR(env);
}

/***** EVALUATION *****/

obj_t *eval();

/* eval each element of a list */
obj_t *evlis(args, env)
  obj_t *args, **env;
{
  obj_t * head = NULL;
  obj_t * current = NULL;
  obj_t * node;

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

obj_t * eval_progn(body, env)
  obj_t *body, **env;
{
  obj_t *ret;

  for ( ; nil != body; body = CDR(body)) {
    ret = eval(CAR(body), env);
  }

  return ret;
}

obj_t *primitive_add(env, args)
     obj_t **env, *args;
{
  int sum = 0;
  obj_t *node, *node_val;

  for (node = args; node != nil; node = node->value.c.cdr) {
    node_val = node->value.c.car;
    if (TINT != node_val->type) fuck("can only add ints");

    sum += node_val->value.i;
  }

  return alloc_int(sum);
}

obj_t *primitive_eval(env, args)
     obj_t **env, *args;
{
  return eval(FIRST(args), env);
}

obj_t *primitive_cons(env, args)
     obj_t **env, *args;
{
  return alloc_cons(FIRST(args), SECOND(args));
}

obj_t *primitive_car(env, args)
     obj_t **env, *args;
{
  return CAR(FIRST(args));
}

obj_t *primitive_cdr(env, args)
     obj_t **env, *args;
{
  return CDR(FIRST(args));
}

obj_t *primitive_rplaca(env, args)
     obj_t **env, *args;
{
  CAR(FIRST(args)) = SECOND(args);
  return FIRST(args);
}

obj_t *primitive_rplacd(env, args)
     obj_t **env, *args;
{
  CDR(FIRST(args)) = SECOND(args);
  return FIRST(args);
}


void print();

obj_t *primitive_print(env, args)
     obj_t **env, *args;
{
  obj_t *arg = FIRST(args);
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
  a = FIRST(args);
  b = SECOND(args);

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
  a = FIRST(args);
  b = SECOND(args);
  if (TINT != a->type || TINT != b->type) fuck("can't do = on non-numbers");

  return (a->value.i == b->value.i) ? tru : nil;
}

obj_t *primitive_not(env, args)
     obj_t **env, *args;
{
  return (nil == FIRST(args)) ? tru : nil;
}


obj_t *eval(form, env)
  obj_t *form, **env;
{
  obj_t *op, *arg, *args, *cond, *defn, *var, *val;
  char * op_name;

  switch (form->type) {
    /* self evaluating forms */
  case TNIL:
  case TTRUE:
  case TINT:
  case TSTRING:
  case TPRIMITIVE:
    return form;

  case TSYMBOL:
    return lookup_env(*env, form);

    /* a form to evaluate: (f x1 x2 ...) */
  case TCONS:
    if (!proper_list_p(form)) fuck("no bueno thing being eval'd");

    op = form->value.c.car;
    args = form->value.c.cdr;

    if (TSYMBOL != op->type && TPRIMITIVE != op->type){
      fuck("bad operator type");
    }

    if (TSYMBOL == op->type) {
      defn = lookup_env(*env, op);
    } else if (TPRIMITIVE == op->type) {
      defn = op;
    }

    if (NULL != defn) {
      assert(TPRIMITIVE == defn->type);
      args = evlis(args, env);
      return defn->value.prim.code(env, args);
    } else {
      op_name = op->value.sym.name;
      if (0 == strcmp("QUOTE", op_name)) {
        if (1 != list_length(args))
          fuck("bad no. of args to QUOTE");
        return args->value.c.car;
      } else if (0 == strcmp("SETQ", op_name)) {
        var = FIRST(args);
        val = eval(SECOND(args), env);
        *env = push_env(*env, var, val);
        return val;
      } else if (0 == strcmp("IF", op_name)) {
        cond = eval(args->value.c.car, env);
        if (nil != cond)
          return eval(args->value.c.cdr->value.c.car, env);
        else
          return eval(args->value.c.cdr->value.c.cdr->value.c.car, env);
      } else if (0 == strcmp("PROGN", op_name)) {
        return eval_progn(args, env);
      } else if (0 == strcmp("WHILE", op_name)) {
        cond = FIRST(args);
        args = REST(args);

        while (nil != eval(cond, env)) {
          for (arg = args; nil != arg; arg = CDR(arg)) {
            eval(CAR(arg), env);
          }
        }
        return nil;
      } else {
        fuck("bad list to eval");
      }
    }
  default:
    fuck("i don't know how to eval this object");
  }
  fuck("shouldn't get here");
  return NULL;
}

/* parser */

#define SYMBOL_MAX_LEN 32

char *symbol_chars = "~!@#$%^&*-_=+:/?<>";

obj_t *read_sexp();

int whitespace(c)
     int c;
{
  return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
}

int peek() {
  int c = getchar();
  ungetc(c, stdin);
  return c;
}

int peek_skipping_whitespace() {
  int peeked;
  for (;;) {
    peeked = peek();
    if (whitespace(peeked)) {
      assert(whitespace(getchar()));
    } else {
      return peeked;
    }
  }
}

int getchar_skipping_whitespace() {
  for (;;) {
    if (whitespace(peek())) {
      assert(whitespace(getchar()));
    } else {
      return getchar();
    }
  }
}

/* reverse a list */
obj_t *reverse(p)
     obj_t *p;
{
  obj_t *ret = nil;
  obj_t *head;
  while (nil != p) {
    head = p;
    p = CDR(p);
    CDR(head) = ret;
    ret = head;
  }
  return ret;
}

/* skip the rest of a line */
void skip_line() {
  int c;
  for (;;) {
    c = getchar();
    if (c == EOF || c == '\n')
      return;
  }
}

/* read a list, starting after a '(' has been read */
obj_t *read_list() {
  int peeked;
  obj_t *obj, *head, *last, *ret;
  head = nil;
  for (;;) {
    peeked = peek_skipping_whitespace();
    if (EOF == peeked) fuck("unclosed parenthesis");
    if (')' == peeked) {
      /* skip the paren */
      (void)getchar();
      return reverse(head);
    }
    if ('.' == peeked) {
      /* skip the dot */
      (void)getchar();
      last = read_sexp();
      if (')' != getchar_skipping_whitespace())
        fuck("closed parenthesis expected after dot");
      ret = reverse(head);
      CDR(head) = last;
      return ret;
    }
    obj = read_sexp();
    head = alloc_cons(obj, head);
  }
}

/* translate 'x into (QUOTE x) */
obj_t *read_quote() {
  obj_t *sym, *tmp;
  sym = intern("QUOTE");
  tmp = read_sexp();
  tmp = alloc_cons(tmp, nil);
  tmp = alloc_cons(sym, tmp);
  return tmp;
}

/* read in a number, whose first digit is val */
obj_t *read_number(val)
     int val;
{
  while (isdigit(peek()))
    val = 10 * val + (getchar() - '0');
  return alloc_int(val);
}

/* read in a symbol, whose first char is c */
obj_t *read_symbol(c)
     char c;
{
  char buf[SYMBOL_MAX_LEN + 1];
  int len = 1;
  buf[0] = c;
  while (isalnum(peek()) || strchr(symbol_chars, peek())) {
    if (SYMBOL_MAX_LEN <= len)
      fuck("symbol name too damn long");
    buf[len++] = getchar();
  }
  buf[len] = '\0';
  if (0 == strcmp(buf, "NIL")) {
    return nil;
  } else if (0 == strcmp(buf, "T")) {
    return tru;
  } else {
    return intern(strdup(buf));
  }
}

obj_t *read_sexp() {
  int c;
  obj_t *o;
  for (;;) {
    c = getchar_skipping_whitespace();
    if (c == EOF) {
      return NULL;
    } else if (c == ';') {
      skip_line();
    } else if (c == '(') {
      return read_list();
    } else if (c == ')' || c == '.') {
      fuck("unexpected dot or close paren");
    } else if (c == '\'') {
      return read_quote();
    } else if (isdigit(c)) {
      return read_number(c - '0');
    } else if (c == '-' && isdigit(peek())) {
      o = read_number(0);
      o->value.i = -o->value.i;
      return o;
    } else if (isalpha(c) || strchr(symbol_chars, c)) {
      return read_symbol(c);
    } else {
      fuck("don't know how to handle character");
    }
  }
}

/* print an object */
void print(o)
     obj_t *o;
{
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
    printf("%s", o->value.str);
    return;

  case TSYMBOL:
    printf("%s", o->value.sym.name);
    return;

  case TINT:
    printf("%d", o->value.i);
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

  default:
    fuck("print: unknown type");
  }
}

#define NUM_OBJECTS 1024
#define STACK_SIZE 512

void init_lisp ()
{
  obj_t **env;
  VM = alloc_vm(NUM_OBJECTS * OBJ_SIZE, STACK_SIZE);
  nil = alloc_obj(TNIL);
  tru = alloc_obj(TTRUE);
  VM->symbols = nil;
  VM->global_bindings = nil;
  env = &VM->global_bindings;
  *env = push_env(*env,
                  intern("+"),
                  alloc_primitive(primitive_add));
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
                  intern("ALL-SYMBOLS"),
                  alloc_primitive(primitive_all_symbols));
  *env = push_env(*env,
                  intern("EQ"),
                  alloc_primitive(primitive_eq));
  *env = push_env(*env,
                  intern("="),
                  alloc_primitive(primitive_number_equals));
  *env = push_env(*env,
                  intern("NOT"),
                  alloc_primitive(primitive_not));

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

int main () {
  obj_t *form;
  init_lisp();

  fprintf(stderr, "welcome to bnlisp\n");

  for (;;) {
    form = read_sexp();
    if (!form) break;
    eval_form(form, &VM->global_bindings);
  }
  exit(1);
}
