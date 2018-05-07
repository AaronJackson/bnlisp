#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "util.h"
#include "obj.h"
#include "alloc.h"
#include "machine.h"
#include "lisp.h"
#include "symbol.h"
#include "env.h"
#include "parser.h"

char *symbol_chars = "~!@#$%^&*-_=+:/?<>";

int whitespace(c)
     int c;
{
  return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
}

int peek() {
  int c = getc(stream_i);
  ungetc(c, stream_i);
  return c;
}

int peek_skipping_whitespace() {
  int peeked;
  for (;;) {
    peeked = peek();
    if (whitespace(peeked)) {
      assert(whitespace(getc(stream_i)));
    } else {
      return peeked;
    }
  }
}

int getchar_skipping_whitespace() {
  for (;;) {
    if (whitespace(peek())) {
      assert(whitespace(getc(stream_i)));
    } else {
      return getc(stream_i);
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
    c = getc(stream_i);
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
      (void)getc(stream_i);
      return reverse(head);
    }
    if ('.' == peeked) {
      /* skip the dot */
      (void)getc(stream_i);
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

obj_t *read_string() {
  int escaped = 0;
  int c, i;
  char *s = (char*)malloc(STRING_MAX_LEN*sizeof(char));

  for (i=0 ;; i++) {
    c = getc(stream_i);

    if ('\\' == c && !escaped) { /* ESCAPE CHARACTER */
      escaped = 1;
      i--;
      continue;
    } else if ('"' == c && !escaped) { /* END OF STRING */
      s[i] = '\0';
      break;
    } else if ('"' == c && escaped) { /* ESCAPED DOUBLE QUOTE */
      s[i] = c;
    } else if ('t' == c && escaped) { /* ESCAPED TAB */
      s[i] = '\t';
    } else if ('\\' == c && escaped) { /* ESCAPED BACKWARDS SLASH */
      s[i] = '\\';
    } else if ('n' == c && escaped) { /* Line feed */
      s[i] = '\n';
    } else if ('r' == c && escaped) { /* carriage return */
      s[i] = '\r';
    } else {
      s[i] = c;
    }
    escaped = 0;
    if (STRING_MAX_LEN == i) fuck("your string is too fucking long");
  }

  return alloc_string(s);

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
    val = 10 * val + (getc(stream_i) - '0');
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
    buf[len++] = getc(stream_i);
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

obj_t *read_sexp()
{
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
    } else if (c == '"') {
      o = read_string();
      return o;
    } else if (isalpha(c) || strchr(symbol_chars, c)) {
      return read_symbol(c);
    } else {
      fuck("don't know how to handle character");
    }
  }
}
