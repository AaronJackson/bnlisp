#include <stdlib.h>
#include <stdio.h>

#include "util.h"

void fuck(msg)
     char * msg;
{
  printf("fuck: %s\n", msg);
  exit(1);
}
