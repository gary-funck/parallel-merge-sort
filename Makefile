CC=gcc
MPICC=mpicc
UPC=gupc
OFLAGS=-O3 -g
WFLAGS=-Wall -Werror
LDFLAGS=-lm
CFLAGS=$(OFLAGS) $(WFLAGS) $(LDFLAGS)
MPIFLAGS=
OMPFLAGS=-fopenmp
UPCFLAGS=

SRC :=	hybrid_mergesort.c \
	mpi_mergesort.c \
	mpi_rma_mergesort.c \
	mpi_rma_nc_mergesort.c \
	omp_mergesort.c \
	serial_mergesort.c \
	upc_hybrid_mergesort.upc \
	upc_no_copy_mergesort.upc \
	upc_mergesort.upc

ALL :=  $(foreach src,$(SRC),$(subst .upc,,$(subst .c,,$(src))))

default: $(ALL)

tags: $(SRC)
	ctags $^

get_time.o: get_time.c
	$(CC) $(CFLAGS) -c $^ -o $@

hybrid_mergesort: hybrid_mergesort.c get_time.o
	$(MPICC) -cc=$(CC) $(CFLAGS) $(MPIFLAGS) $(OMPFLAGS) $^ -o $@

mpi_mergesort: mpi_mergesort.c get_time.o
	$(MPICC) -cc=$(CC) $(CFLAGS) $(MPIFLAGS) $^ -o $@

mpi_rma_mergesort: mpi_rma_mergesort.c get_time.o
	$(MPICC) -cc=$(CC) $(CFLAGS) $(MPIFLAGS) $^ -o $@

mpi_rma_nc_mergesort: mpi_rma_nc_mergesort.c get_time.o
	$(MPICC) -cc=$(CC) $(CFLAGS) $(MPIFLAGS) $^ -o $@

omp_mergesort: omp_mergesort.c get_time.o
	$(CC) $(CFLAGS) $(OMPFLAGS) $^ -o $@

serial_mergesort: serial_mergesort.c get_time.o
	$(CC) $(CFLAGS) $^ -o $@

upc_hybrid_mergesort: upc_hybrid_mergesort.upc get_time.o
	$(UPC) $(CFLAGS) $(OMPFLAGS) $(UPCFLAGS) $^ -o $@

upc_mergesort: upc_mergesort.upc get_time.o
	$(UPC) $(CFLAGS) $(UPCFLAGS) $^ -o $@

upc_no_copy_mergesort: upc_no_copy_mergesort.upc get_time.o
	$(UPC) $(CFLAGS) $(UPCFLAGS) $^ -o $@

clean:
	@- rm -f get_time.o
	@- rm -f $(ALL) tags
