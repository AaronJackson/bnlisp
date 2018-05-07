#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

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

obj_t *primitive_princ(env, args)
     obj_t **env, *args;
{
  /* being lazy for now... wasn't working for printing string.. hm */
  obj_t *e = eval(FIRST(args), env);
  if (TSTRING == e->type)
    printf("%s", e->value.str);
  return e;
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

obj_t *primitive_load(env, args)
  obj_t **env, *args;
{
  char *path = CAR(args)->value.str;
  char c;
  FILE *fid = fopen(path, "r");
  obj_t *form;

  if (NULL == fid)
    return nil;

  stream_i = fid;
  while (EOF != (c = fgetc(fid))) {
    ungetc(c, fid);
    /* putchar(c); */
    form = read_sexp();
    if (NULL != form)
      eval_form(form, &VM->global_bindings);
  }
  stream_i = stdin;

  fclose(fid);

  return tru;
}

obj_t *primitive_concatenate(env, args)
  obj_t **env, *args;
{
  obj_t *a, *b;
  char *s, *type;
  obj_t *eargs = evlis(args,env);

  if (CAR(CAR(args))->type != TSYMBOL ||
      0 != strcmp("QUOTE", CAR(CAR(args))->value.str))
    fuck("concatenate must specify type");

  a = SECOND(eargs);
  b = THIRD(eargs);

  type = CAR(eargs)->value.str;
  if (0 == strcmp(type, "STRING")) { /* CONCAT STRING */
    s = (char*)malloc((strlen(a->value.str)+
		       strlen(b->value.str)+
		       1)*sizeof(char));
    strcat(s, a->value.str);
    strcat(s, b->value.str);
    return alloc_string(s);
  } else if (0 == strcmp(type, "LIST")) { /* CONCAT LIST */
    fuck("not yet implemented");
  }

  return tru;
}


/********************************************************************/
/*                    STREAMS and SOCKETS                           */
/********************************************************************/

/* (STREAM-OPEN 'FILE path)
   (STREAM-OPEN 'TCP host port)
   (STREAM-OPEN 'UDP host port)
*/
obj_t *primitive_stream_open(env, args)
  obj_t **env, *args;
{
  char *type;
  obj_t *eargs;

  FILE *stream;
  int sock;
  struct sockaddr_in sock_addr;
  struct hostent *sock_server;

  eargs = evlis(args, env);

  if (CAR(CAR(args))->type != TSYMBOL ||
      0 != strcmp("QUOTE", CAR(CAR(args))->value.str)) {
    fuck("must specify file, tcp or udp");
  }

  type = CAR(eargs)->value.str;
  if (0 == strcmp(type, "FILE")) {
    stream = fopen(SECOND(eargs)->value.str, "ab+");
    if (NULL == stream) {
      return nil;
    }
    return alloc_stream(stream);
  } else if (0 == strcmp(type, "TCP")) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    sock_server = gethostbyname(SECOND(eargs)->value.str);
    if (NULL == sock_server)
      fuck("No such host");

    bzero((char*)&sock_addr, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    bcopy((char*)sock_server->h_addr,
	  (char*)&sock_addr.sin_addr.s_addr,
	  sock_server->h_length);
    sock_addr.sin_port = htons(THIRD(eargs)->value.i);

    if (connect(sock, (struct sockaddr *)&sock_addr,
		sizeof(struct sockaddr)) < 0)
      fuck("could not connect to host");

    return alloc_socket(sock);

  } else if (0 == strcmp(type, "UDP")) {

  } else {
    fuck("Only supported streams are FILE, TCP, UDP");
  }

  return nil;
}

/* (STREAM-CLOSE stream) */
obj_t *primitive_stream_close(env, args)
  obj_t **env, *args;
{
  obj_t *s = evlis(args, env);
  if (TSTREAM == CAR(s)->type) {
    fclose(CAR(s)->value.stream);
    return tru;
  } else if (TSOCKET == CAR(s)->type) {
    close(CAR(s)->value.i);
    return tru;
  } else {
    fuck("That's not a stream or a socket!");
  }
}

/* (STREAM-READ stream) */
obj_t *primitive_stream_read(env, args)
  obj_t **env, *args;
{
  char c[2];
  obj_t *s = evlis(args, env);

  if (TSTREAM == CAR(s)->type) {
    c[0] = fgetc(CAR(s)->value.stream);
    c[1] = 0;
    return alloc_string(c);
  } else if (TSOCKET == CAR(s)->type) {
    if (read(CAR(s)->value.i, c, 1) <= 0) {
      return nil;
    }
    c[1] = 0;

    return alloc_string(c);
  } else {
    fuck("That is not a stream or a socket!");
  }
}

/* (STREAM-WRITE stream string) */
obj_t *primitive_stream_write(env, args)
  obj_t **env, *args;
{
  obj_t *s = evlis(args, env);

  if (TSTREAM == CAR(s)->type) {
    fprintf(CAR(s)->value.stream,
	    CAR(CDR(s))->value.str);
    return tru;
  } else if (TSOCKET == CAR(s)->type) {
    write(CAR(s)->value.i,
	   CAR(CDR(s))->value.str,
	   strlen(CAR(CDR(s))->value.str));
    return tru;
  } else {
    fuck("That is not a stream or a socket!");
  }
}


/* (STREAM-EOF? stream) */
obj_t *primitive_stream_iseof(env, args)
  obj_t **env, *args;
{
  char c;
  obj_t *s = evlis(args, env);
  if (TSTREAM == CAR(s)->type) {
    c = getc(CAR(s)->value.stream);
    ungetc(c, CAR(s)->value.stream);
    return c == EOF ? tru : nil;
  } else if (TSOCKET == CAR(s)->type) {
    return tru; /* always true, no eof */
  } else {
    fuck("That is not a stream or a socket!");
  }
}
