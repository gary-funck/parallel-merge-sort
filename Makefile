
CC=gcc
MPICC=mpicc
UPC=gupc
OFLAGS=-O3 -g
LDFLAGS=-lm
CFLAGS=$(OFLAGS) $(LDFLAGS)
MPIFLAGS=
OMPFLAGS=-fopenmp
UPCFLAGS=

SRC :=	hybrid_mergesort.c \
	mpi_mergesort.c \
	omp_mergesort.c \
	serial_mergesort.c \
	upc_hybrid_mergesort.upc \
	upc_no_copy_mergesort.upc \
	upc_mergesort.upc

ALL :=  $(foreach src,$(SRC),$(subst .upc,,$(subst .c,,$(src))))

default: $(ALL)

tags: $(SRC)
	ctags $?

hybrid_mergesort: hybrid_mergesort.c
	$(MPICC) -cc=$(CC) $(CFLAGS) $(MPIFLAGS) $(OMPFLAGS) $? -o $@

mpi_mergesort: mpi_mergesort.c
	$(MPICC) -cc=$(CC) $(CFLAGS) $(MPIFLAGS) $? -o $@

omp_mergesort: omp_mergesort.c
	$(CC) $(CFLAGS) $(OMPFLAGS) $? -o $@

serial_mergesort: serial_mergesort.c
	$(CC) $(CFLAGS) $? -o $@

upc_hybrid_mergesort: upc_hybrid_mergesort.upc
	$(UPC) $(CFLAGS) $(OMPFLAGS) $(UPCFLAGS) $? -o $@

upc_mergesort: upc_mergesort.upc
	$(UPC) $(CFLAGS) $(UPCFLAGS) $? -o $@

upc_no_copy_mergesort: upc_no_copy_mergesort.upc
	$(UPC) $(CFLAGS) $(UPCFLAGS) $? -o $@

clean:
	@- rm -f $(ALL) tags
