/* bnlisp - lisp for the PDP-11 under 2.11BSD
 * dedicated to Marc Cleave
 *
 * by Robert Smith and Aaron Jackson
 */

#include <stdlib.h>

enum {
        TNIL,
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

struct obj * nil = NULL;

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
        return proper_list_p(o->value.c.cdr);
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

struct obj * eval(form)
struct obj * form;
{
        struct obj * op, * args;
        char * op_name;

        struct obj * a1, * a2;

        switch (form->type) {
        /* self evaluating forms */
        case TNIL:
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
                } else if {0 == strcmp("EVAL", op_name)) {
                        args = evlis(args);
                        return eval(args->value.c.car);
                } else if (0 == strcmp("CONS", op_name)) {
                        args = evlis(args);
                        a1 = args->value.c.car;
                        a2 = args->value.c.cdr->value.c.car;
                        return alloc_cons(a1, a2);
                } else {
                        fuck("bad list to eval");
                }
        default:
                fuck("i don't know how to eval this object");
        }
        fuck("shouldn't get here");
        return NULL;
}

/* printing */

void print_proper_list(o)
struct obj * o;
{
        struct obj * node;

        putchar('(');
        for (node = o; node != nil; node = node->value.c.cdr) {
                print_obj(node->value.c.car);
                if (nil != node->value.c.cdr) putchar(' ');
        }
        putchar(')');
}

void print_obj(o)
struct obj * o;
{
        if (!o) fuck("bad obj to print");

        switch (o->type) {
        case TNIL:
                printf("NIL");
                break;
        case TINT:
                printf("%d", o->value.i);
                break;
        case TSTRING:
                printf("%s", o->value.str);
                break;
        case TCONS:
                if (proper_list_p(o)) {
                        print_proper_list(o);
                } else {
                        putchar('(');
                        print_obj(o->value.c.car);
                        printf(" . ");
                        print_obj(o->value.c.cdr);
                        putchar(')');
                }
                break;
        default:
                fuck("unknown object type in print_obj");
        }
}

void init_lisp () {
        nil = alloc_obj();
        nil->type = TNIL;
}

void main () {
        struct obj * form;
        struct obj * ev_form;

        init_lisp();

        form = alloc_cons(alloc_string("+"),
                                alloc_cons(alloc_int(1),
                                alloc_cons(alloc_int(2), nil)));

        printf("input: ");
        print_obj(form);
        printf("\n\noutput: ");
        ev_form = eval(form);
        print_obj(ev_form);

}
