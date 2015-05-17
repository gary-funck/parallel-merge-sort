
CC=gcc
MPICC=mpicc
UPC=/eng/upc/dev/gary/gupc-dev/rls/packed-dbg/bin/gupc
OFLAGS=-O3 -g
LDFLAGS=-lm
CFLAGS=$(OFLAGS) $(LDFLAGS)
MPIFLAGS=
OMPFLAGS=-fopenmp
UPCFLAGS=
# UPCFLAGS=-fupc-pthreads-model-tls

SRC :=	hybrid_mergesort.c \
	mpi_mergesort.c \
	omp_mergesort.c \
	serial_mergesort.c \
	upc_hybrid_mergesort.upc \
	upc_mergesort.upc

ALL :=  $(foreach src,$(SRC),$(subst .upc,,$(subst .c,,$(src))))

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

upc_hybrid_mergesort: upc_hybrid_mergesort.upc
	$(UPC) $(CFLAGS) $(OMPFLAGS) $(UPCFLAGS) $? -o $@

upc_mergesort: upc_mergesort.upc
	$(UPC) $(CFLAGS) $(UPCFLAGS) $? -o $@

clean:
	@- rm -f $(ALL) tags
