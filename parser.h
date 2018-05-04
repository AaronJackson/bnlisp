#ifndef BNLISP_PARSER_H
#define BNLISP_PARSER_H

#define SYMBOL_MAX_LEN 32
#define STRING_MAX_LEN 128

int whitespace();
int peek();
int peek_skipping_whitespace();
int getchar_skipping_whitespace();
obj_t *reverse();
void skip_lines();
obj_t *read_sexp();
obj_t *read_list();
obj_t *read_string();
obj_t *read_quote();
obj_t *read_number();
obj_t *read_symbol();
obj_t *read_sex();

#endif
