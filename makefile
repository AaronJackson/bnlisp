OBJS = machine.o util.o alloc.o parser.o symbol.o env.o prim.o lisp.o
CC = cc
CFLAGS =

lisp: ${OBJS}
	${CC} -o lisp ${CFLAGS} ${OBJS}

lisp.o:
	${CC} ${CFLAGS} -c lisp.c

machine.o:
	${CC} ${CFLAGS} -c machine.c

util.o:
	${CC} ${CFLAGS} -c util.c

parser.o:
	${CC} ${CFLAGS} -c parser.c

alloc.o:
	${CC} ${CFLAGS} -c alloc.c

symbol.o:
	${CC} ${CFLAGS} -c symbol.c

env.o:
	${CC} ${CFLAGS} -c env.c

prim.o:
	${CC} ${CFLAGS} -c prim.c

clean:
	rm -f lisp ${OBJS}
	@echo "Nice and clean."
