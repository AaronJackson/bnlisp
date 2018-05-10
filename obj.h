#include <stdio.h>

/* Both TSOCKET and TSTREAM are handled with the same set of
 *  primitives STREAM-*, but the type needs to be preserved
 *  separately.
 */

typedef enum {
  /* states */
  /* STATE_FORWARDED, */
  /* types */
  TNIL,
  TTRUE,
  TINT,
  TFLOAT,
  TCONS,
  TSTRING,
  TSYMBOL,
  TPRIMITIVE,
  TFUNCTION,
  TSOCKET, /* INET Socket Stream */
  TSTREAM /* FILE stream */
} obj_type_t;

/* function type: (obj **env, obj *args) -> obj *return */
/* args have not been evaluated */
typedef struct obj * (*primitive_t)();

typedef struct obj {
  int type;
  union {
    /* when the object moves during GC, it gets a new location */
    /* struct obj *new_location; */

    /* TINT and TSOCKET */
    int i;

    /* TFLOAT */
    float f;

    /* TSTRING */
    char *str;

    /* TSTREAM pointer  */
    FILE *stream;

    /* symbols */
    struct {
      char *name;
    } sym;

    /* cons cells */
    struct {
      struct obj *car;
      struct obj *cdr;
    } c;

    /* primitive (C-implemented) functions */
    struct {
      primitive_t code;
    } prim;

    /* functions*/
    struct {
      struct obj *params;
      struct obj *body;
      struct obj *env;
    } fun;
  } value;
} obj_t;




#define OBJ_SIZE (sizeof (obj_t))

#define CAR(x) ((x)->value.c.car)
#define CDR(x) ((x)->value.c.cdr)
#define FIRST(x) CAR(x)
#define SECOND(x) CAR(CDR(x))
#define THIRD(x) CAR(CDR(CDR(x)))
#define REST(x) CDR(x)
