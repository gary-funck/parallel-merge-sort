
CC=gcc
MPICC=mpicc
OFLAGS=-O3
LDFLAGS=-lm
CFLAGS=$(OFLAGS) $(LDFLAGS)
MPIFLAGS=
OMPFLAGS=-fopenmp

SRC :=	hybrid_mergesort.c \
	mpi_mergesort.c \
	omp_mergesort.c \
	serial_mergesort.c

ALL :=  $(foreach src,$(SRC),$(subst .c,,$(src)))

default: $(ALL)

tags: $(SRC)
	ctags $?

hybrid_mergesort: hybrid_mergesort.c
	$(MPICC) $(CFLAGS) $(MPIFLAGS) $(OMPFLAGS) $? -o $@

mpi_mergesort: mpi_mergesort.c
	$(MPICC) $(CFLAGS) $(MPIFLAGS) $? -o $@

omp_mergesort: omp_mergesort.c
	$(CC) $(CFLAGS) $(OMPFLAGS) $? -o $@

# Serial merge sort uses OMP for checking.
serial_mergesort: serial_mergesort.c
	$(CC) $(CFLAGS) $(OMPFLAGS) $? -o $@

clean:
	@- rm -f $(ALL) tags
