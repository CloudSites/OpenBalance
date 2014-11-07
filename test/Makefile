CC=gcc -g -Wall -I../src/
CC_OBJ=${CC} -c -o $@
OB_LIBS=-ljansson -luv -lzmq

all: suite_bc_ops

run_tests: all
	./suite_bc_ops

suite_bc_ops: test_bc_ops.c testing.h ../src/buffer_chain.c ../src/buffer_chain.h
	${CC} -o $@ test_bc_ops.c ../obj/buffer_chain.o
