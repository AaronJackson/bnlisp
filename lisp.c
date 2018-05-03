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

enum {
        TNIL,
        TTRUE,
        TINT,
        TCONS,
        TSTRING,
        TSYMBOL,
        TFUNCTION
};

struct obj {
        int type;
        union {
                int i;
                char * str;
                struct {
                        struct obj * car;
                        struct obj * cdr;
                } c;
        } value;
};

#define CAR(x) ((x)->value.c.car)
#define CDR(x) ((x)->value.c.cdr)
#define FIRST(x) CAR(x)
#define SECOND(x) CAR(CDR(x))
#define THIRD(x) CAR(CDR(CDR(x)))

struct obj *nil = NULL;
struct obj *tru = NULL;

struct obj * alloc_obj() {
        return (struct obj *)calloc(1, sizeof (struct obj));
}

struct obj * alloc_int(val)
int val;
{
        struct obj * x = alloc_obj();
        x->type = TINT;
        x->value.i = val;
        return x;
}

/*
char *strdup(src)
     char *src;
{
  char *str;
  char *p;
  int len = 0;

  while (src[len])
    len++;
  str = malloc(len + 1);
  p = str;
  while (*src)
    *p++ = *src++;
  *p = '\0';
  return str;
}
*/

struct obj * alloc_string(s)
char * s;
{
        struct obj * x = alloc_obj();
        x->type = TSTRING;
        x->value.str = s;
        return x;
}

struct obj * alloc_cons(ca, cd)
struct obj * ca, * cd;
{
        struct obj * x = alloc_obj();
        x->type = TCONS;
        x->value.c.car = ca;
        x->value.c.cdr = cd;
        return x;
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

struct obj * prim_add(args)
struct obj * args;
{
        int sum = 0;
        struct obj * node, * node_val;

        for (node = args; node != nil; node = node->value.c.cdr) {
                node_val = node->value.c.car;
                if (TINT != node_val->type) fuck("can only add ints");

                sum += node_val->value.i;
        }

        return alloc_int(sum);
}

struct obj * eval();

/* eval each element of a list */
struct obj * evlis(args)
struct obj * args;
{
        struct obj * head = NULL;
        struct obj * current = NULL;
        struct obj * node;

        for (node = args; node != nil; node = node->value.c.cdr) {
                if (!current) {
                        current = alloc_cons(nil, nil);
                        head = current;
                } else {
                        current->value.c.cdr = alloc_cons(nil, nil);
                        current = current->value.c.cdr;
                }
                current->value.c.car = eval(node->value.c.car);
        }
        return head;
}

struct obj * eval_progn(body)
struct obj * body;
{
	struct obj * ret;

	for ( ; nil != body; body = body->value.c.cdr) {
		ret = eval(body->value.c.car);
	}

	return ret;
}

struct obj * eval(form)
     struct obj * form;
{
        struct obj * op, * args, * cond;
        char * op_name;

        struct obj * a1, * a2;

        switch (form->type) {
        /* self evaluating forms */
        case TNIL:
        case TTRUE:
        case TINT:
        case TSTRING:
                return form;

        /* a form to evaluate: (f x1 x2 ...) */
        case TCONS:
                if (!proper_list_p(form)) fuck("no bueno");

                op = form->value.c.car;
                args = form->value.c.cdr;

                if (TSTRING != op->type) fuck("bad operator");

                op_name = op->value.str;

                if (0 == strcmp("QUOTE", op_name)) {
                        if (1 != list_length(args))
                                fuck("bad no. of args to QUOTE");
                        return args->value.c.car;
                } else if (0 == strcmp("+", op_name)) {
                        args = evlis(args);
                        return prim_add(args);
                } else if (0 == strcmp("EVAL", op_name)) {
                        args = evlis(args);
                        return eval(args->value.c.car);
                } else if (0 == strcmp("CONS", op_name)) {
                        args = evlis(args);
                        a1 = args->value.c.car;
                        a2 = args->value.c.cdr->value.c.car;
                        return alloc_cons(a1, a2);
		} else if (0 == strcmp("IF", op_name)) {
			cond = eval(args->value.c.car);
			if (nil != cond)
				return eval(args->value.c.cdr->value.c.car);
			else
				return eval(args->value.c.cdr->value.c.cdr->value.c.car);
		} else if (0 == strcmp("PROGN", op_name)) {
			return eval_progn(args);
		} else if (0 == strcmp("PRINC", op_name)) {
			printf("%s\n", args->value.c.car->value.str);
			return args->value.c.car;
                } else {
                        fuck("bad list to eval");
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
    // obj = read_sexp();
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
  sym = alloc_string("QUOTE"); /* FIXME */
  tmp = read_sexp();
  tmp = alloc_cons(tmp, nil);
  tmp = alloc_cons(sym, tmp);
  return tmp;
}

/* read in a number, whose first digit is val */
struct obj *read_number(int val) {
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
    return alloc_string(strdup(buf)); /* FIXME */
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

  case TINT:
    printf("%d", o->value.i);
    return;

  case TTRUE:
    printf("T");
    return;

  case TNIL:
    printf("NIL");
    return;

  default:
    fuck("print: unknown type");
  }
}

void init_lisp () {
        nil = alloc_obj();
        nil->type = TNIL;
        tru = alloc_obj();
        tru->type = TTRUE;
}

void eval_form(f)
     struct obj * f;
{
  printf(" INPUT: ");
  print(f);
  printf("\nOUTPUT: ");
  print(eval(f));
  printf("\n");
}

void main () {
        struct obj * form, * tmp1, * tmp2;
        struct obj * ev_form;

        init_lisp();
        fprintf(stderr, "welcome to bnlisp\n");

        for (;;) {
          form = read_sexp();
          if (!form) break;
          eval_form(form);
        }

        return;

	/* (+ 1 2) */
        form = alloc_cons(alloc_string("+"),
		alloc_cons(alloc_int(1),
		alloc_cons(alloc_int(2), nil)));

	eval_form(form);

	/* (QUOTE (+ 1 2)) */
	form = alloc_cons(alloc_string("QUOTE"),
		alloc_cons(form, nil));
	eval_form(form);

	/* (CONS 2 1) */
	form = alloc_cons(alloc_string("CONS"),
		alloc_cons(alloc_int(2),
			alloc_cons(alloc_int(3), nil)));
	eval_form(form);

	/* (EVAL (CONS + (CONS 1 (CONS 2 NIL)))) */
	form = alloc_cons(alloc_string("CONS"),
		alloc_cons(alloc_int(2),
		alloc_cons(nil, nil)));
	form = alloc_cons(alloc_string("CONS"),
		alloc_cons(alloc_int(1),
		alloc_cons(form, nil)));
	form = alloc_cons(alloc_string("CONS"),
		alloc_cons(alloc_string("+"),
		alloc_cons(form, nil)));
	form = alloc_cons(alloc_string("EVAL"),
		alloc_cons(form, nil));
	eval_form(form);

	/* (EVAL (QUOTE (+ 1 2)) */
	form = alloc_cons(alloc_string("+"),
		alloc_cons(alloc_int(1),
		alloc_cons(alloc_int(2), nil)));
	form = alloc_cons(alloc_string("QUOTE"),
		alloc_cons(form, nil));
	form = alloc_cons(alloc_string("EVAL"),
		alloc_cons(form, nil));
	eval_form(form);

	tmp1 = alloc_cons(alloc_string("PRINC"),
		alloc_cons(alloc_string("something"), nil));
	tmp2 = alloc_cons(alloc_string("PRINC"),
		alloc_cons(alloc_string("something-else"), nil));

        /* (IF 1 (PRINC something) (PRINC something-else)) */
	form = alloc_cons(alloc_string("IF"),
		alloc_cons(alloc_int(1),
		alloc_cons(tmp1,
		alloc_cons(tmp2, nil))));
	eval_form(form);

        /* (IF nil (PRINC something) (PRINC something-else)) */
	form = alloc_cons(alloc_string("IF"),
		alloc_cons(nil,
		alloc_cons(tmp1,
		alloc_cons(tmp2, nil))));
	eval_form(form);

	/* (PROGN (PRINC something) (PRINC something-else)) */
	form = alloc_cons(alloc_string("PROGN"),
		alloc_cons(tmp1, alloc_cons(tmp2, nil)));

	eval_form(form);
}
