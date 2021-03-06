# on Hopper, we will benchmark you against Cray LibSci, the default vendor-tuned BLAS. The Cray compiler wrappers handle all the linking. If you wish to compare with other BLAS implementations, check the NERSC documentation.
# This makefile is intended for the GNU C compiler. On Hopper, the Portland compilers are default, so you must instruct the Cray compiler wrappers to switch to GNU: type "module swap PrgEnv-pgi PrgEnv-gnu"

CC = cc
OPT = -O2 -march=native -funroll-loops -fsched-pressure -fsched-spec-load-dangerous
CFLAGS = -Wall -std=gnu99 $(OPT)
LDFLAGS = -Wall

# librt is needed for clock_gettime
LDLIBS = -lrt
UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
  LDLIBS = -lblas
endif

targets = benchmark-blocked
objects = benchmark.o dgemm-blocked.o

.PHONY : default
default : all

.PHONY : all
all : clean $(targets)

benchmark-blocked : benchmark.o dgemm-blocked.o
	$(CC) -o $@ $^ $(LDLIBS)

%.o : %.c
	$(CC) -c $(CFLAGS) $<

.PHONY : clean
clean:
	rm -f $(targets) $(objects)
