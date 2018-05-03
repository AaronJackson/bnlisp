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

typedef enum {
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

struct obj {
  int type;
  union {
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
};

#define CAR(x) ((x)->value.c.car)
#define CDR(x) ((x)->value.c.cdr)
#define FIRST(x) CAR(x)
#define SECOND(x) CAR(CDR(x))
#define THIRD(x) CAR(CDR(CDR(x)))

/* initialized by init_lisp */
struct obj *nil = NULL;
struct obj *tru = NULL;

struct obj * alloc_obj(type)
     obj_type_t type;
{
  struct obj *o = (struct obj *)calloc(1, sizeof (struct obj));
  o->type = type;
  return o;
}

struct obj * alloc_int(val)
     int val;
{
  struct obj * x = alloc_obj(TINT);
  x->value.i = val;
  return x;
}

struct obj * alloc_string(s)
     char * s;
{
  struct obj * x = alloc_obj(TSTRING);
  x->value.str = s;
  return x;
}

struct obj * alloc_cons(ca, cd)
  struct obj * ca, * cd;
{
  struct obj * x = alloc_obj(TCONS);
  x->value.c.car = ca;
  x->value.c.cdr = cd;
  return x;
}

struct obj *alloc_primitive(code)
     primitive_t code;
{
  struct obj *x = alloc_obj(TPRIMITIVE);
  x->value.prim.code = code;
  return x;
}

struct obj *symbols;

struct obj *find_symbol(name)
     char *name;
{
  struct obj *entry, *table, *val;
  for (table = symbols; nil != table; table = CDR(table)) {
    entry = CAR(table);
    assert(TSYMBOL == entry->type);
    if (0 == strcmp(entry->value.sym.name, name)) {
      return entry;
    }
  }
  return NULL;
}

struct obj *alloc_symbol(name, intern)
     char *name;
     int intern;
{
  struct obj *x;
  if (intern) {
    x = find_symbol(name);
    if (!x) {
      x = alloc_obj(TSYMBOL);
      x->value.sym.name = name;
      symbols = alloc_cons(x, symbols);
    }
  } else {
    /* make a fresh symbol */
    x = alloc_obj(TSYMBOL);
    x->value.sym.name = name;
  }
  return x;
}

struct obj *intern(name)
     char *name;
{
  return alloc_symbol(name, 1);
}

void fuck(msg)
     char * msg;
{
  printf("fuck: %s\n", msg);
  exit(1);
}

int proper_list_p(o)
     struct obj * o;
{
  if (nil == o) return 1;
  if (TCONS != o->type) return 0;
  return proper_list_p(CDR(o));
}

int list_length(o)
     struct obj * o;
{
  /* assumes that o is a proper list */
  struct obj * node;
  int len = 0;
  for (node = o; node != nil; node = node->value.c.cdr) len++;
  return len;
}

struct obj *lookup_env(env, sym)
  struct obj *env, *sym;
{
  struct obj *entry;
  assert(TSYMBOL == sym->type);
  for (; nil != env; env = CDR(env)) {
    entry = CAR(env);
    assert(TCONS == entry->type);
    if (sym == CAR(entry))
      return CDR(entry);
  }
  return NULL;
}

struct obj *push_env(env, sym, val)
  struct obj *env, *sym, *val;
{
  struct obj *entry = alloc_cons(sym, val);
  return alloc_cons(entry, env);
}

struct obj *pop_env(env)
     struct obj *env;
{
  /* memory leak */
  if (nil == env) return nil;
  return CDR(env);
}

struct obj * eval();

/* eval each element of a list */
struct obj * evlis(args, env)
  struct obj *args, **env;
{
  struct obj * head = NULL;
  struct obj * current = NULL;
  struct obj * node;

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

struct obj * eval_progn(body, env)
  struct obj *body, **env;
{
  struct obj *ret;

  for ( ; nil != body; body = CDR(body)) {
    ret = eval(CAR(body), env);
  }

  return ret;
}

struct obj *primitive_add(env, args)
     struct obj **env, *args;
{
  int sum = 0;
  struct obj *node, *node_val;

  for (node = args; node != nil; node = node->value.c.cdr) {
    node_val = node->value.c.car;
    if (TINT != node_val->type) fuck("can only add ints");

    sum += node_val->value.i;
  }

  return alloc_int(sum);
}

struct obj *primitive_eval(env, args)
     struct obj **env, *args;
{
  return eval(FIRST(args), env);
}

struct obj *primitive_cons(env, args)
     struct obj **env, *args;
{
  return alloc_cons(FIRST(args), SECOND(args));
}

void print();

struct obj *primitive_princ(env, args)
     struct obj **env, *args;
{
  struct obj *arg = FIRST(args);
  print(arg);
  putchar('\n');
  return arg;
}

struct obj *primitive_all_symbols(env, args)
     struct obj **env, *args;
{
  return symbols;
}

struct obj *eval(form, env)
  struct obj *form, **env;
{
  struct obj *op, *args, *cond, *defn;
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

    printf("TCONS form: \n");
    print(form);
    printf("\n");
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
      } else if (0 == strcmp("IF", op_name)) {
        cond = eval(args->value.c.car, env);
        if (nil != cond)
          return eval(args->value.c.cdr->value.c.car, env);
        else
          return eval(args->value.c.cdr->value.c.cdr->value.c.car, env);
      } else if (0 == strcmp("PROGN", op_name)) {
        return eval_progn(args, env);
      }else {
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

struct obj *read_sexp();

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
struct obj *reverse(p)
     struct obj *p;
{
  struct obj *ret = nil;
  struct obj *head;
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
struct obj *read_list() {
  int peeked;
  struct obj *obj, *head, *last, *ret;
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
struct obj *read_quote() {
  struct obj *sym, *tmp;
  sym = intern("QUOTE");
  tmp = read_sexp();
  tmp = alloc_cons(tmp, nil);
  tmp = alloc_cons(sym, tmp);
  return tmp;
}

/* read in a number, whose first digit is val */
struct obj *read_number(val)
     int val;
{
  while (isdigit(peek()))
    val = 10 * val + (getchar() - '0');
  return alloc_int(val);
}

/* read in a symbol, whose first char is c */
struct obj *read_symbol(c)
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

struct obj *read_sexp() {
  int c;
  struct obj *o;
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
     struct obj *o;
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

void init_lisp (env)
     struct obj **env;
{
  nil = alloc_obj(TNIL);
  tru = alloc_obj(TTRUE);
  symbols = nil;
  *env = nil;
  *env = push_env(*env,
                  intern("+"),
                  alloc_primitive(primitive_add));
  *env = push_env(*env,
                  intern("CONS"),
                  alloc_primitive(primitive_cons));
  *env = push_env(*env,
                  intern("EVAL"),
                  alloc_primitive(primitive_eval));
  *env = push_env(*env,
                  intern("PRINC"),
                  alloc_primitive(primitive_princ));
  *env = push_env(*env,
                  intern("ALL-SYMBOLS"),
                  alloc_primitive(primitive_all_symbols));  
}

void eval_form(f, env)
  struct obj *f, **env;
{
  printf("\n INPUT: ");
  print(f);
  putchar('\n');
  f = eval(f, env);
  printf("OUTPUT: ");
  print(f);
  printf("\n");
}

void main () {
  struct obj *form, *root_env;
  init_lisp(&root_env);
  
  fprintf(stderr, "welcome to bnlisp\n");

  for (;;) {
    form = read_sexp();
    if (!form) break;
    eval_form(form, &root_env);
  }
}
