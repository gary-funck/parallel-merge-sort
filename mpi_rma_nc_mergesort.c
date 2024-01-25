/* MPI recursive merge sort using MPI-3 RMA operations

   Derived from upc_no_copy_mergesort.upc by Gary Funck <gary@intrepidtechnologyinc.com>
   Date: 2015-08-17

   Derived from omp_mergesort.c by Gary Funck <gary@intrepidtechnologyinc.com>
   Date: 2015-05-16

   Copyright (C) 2011  Atanas Radenski

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation; either version 2 of
 the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public
 License along with this program; if not, write to the Free
 Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

// Arrays size <= SMALL switches to insertion sort
#define SMALL    32

extern double get_time (void);
void insertion_sort (int a[], int size);
void mergesort_serial (int a[], int size, int temp[]);
void merge (int a[], int size, int left_size, int temp[]);
void insertion_sort_rma (int a_offset, int size);
void mergesort_rma (int a_offset, int size, int temp[]);
void merge_rma (int a_offset, int size, int left_size, int temp[]);
void parallel_block_mergesort_rma (int a[], int size);
int main (int argc, char *argv[]);

int debug = 1;

int comm_size, my_rank, max_rank;
MPI_Win win;

int
main (int argc, char *argv[])
{
  int size;
  int *a;
  // All processes
  MPI_Init (&argc, &argv);
  // Check processes and their ranks.
  // number of processes == communicator size
  MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank (MPI_COMM_WORLD, &my_rank);
  max_rank = comm_size - 1;
  if (!my_rank)
    {
      // Rank 0.
      puts ("-MPI RMA Recursive Mergesort-\t");
      // Check arguments
      if (argc != 2)
	{
	  printf ("Usage: %s array-size\n", argv[0]);
	  MPI_Abort (MPI_COMM_WORLD, 1);
	}
      // Get arguments
      size = atoi (argv[1]);	// Array size
      if (size <= 0)
	{
	  printf ("ERROR: invalid array-size: %s\n", argv[1]);
	  MPI_Abort (MPI_COMM_WORLD, 1);
	}
      printf ("Array size = %d\nProcesses = %d\n\n", size, comm_size);
      MPI_Bcast (&size, 1, MPI_INT, 0, MPI_COMM_WORLD);
      // All shared storage is on rank 0.
      MPI_Win_allocate (size * sizeof (int), sizeof (int), MPI_INFO_NULL,
			MPI_COMM_WORLD, &a, &win);
      // Random array initialization
      srand (314159);
      for (int i = 0; i < size; i++)
	{
	  a[i] = rand () % size;
	}
    }
  else
    {
      // Not rank 0.
      MPI_Bcast (&size, 1, MPI_INT, 0, MPI_COMM_WORLD);
      //  Allocation size is 0. This rank doesn't contribute to 'a'.
      MPI_Win_allocate (0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &a, &win);
    }
  MPI_Win_lock_all (MPI_MODE_NOCHECK, win);
  MPI_Barrier (MPI_COMM_WORLD);
  double start = get_time ();
  // All ranks execute the parallel block merge procedure.
  parallel_block_mergesort_rma (a, size);
  double end = get_time ();
  if (!my_rank)
    {
      printf ("Start = %.2f\nEnd = %.2f\nElapsed = %.2f\n",
	      start, end, end - start);
      // Result check
      for (int i = 1; i < size; i++)
	{
	  if (!(a[i - 1] <= a[i]))
	    {
	      printf ("Implementation error: a[%d]=%d > a[%d]=%d\n", i - 1,
		      a[i - 1], i, a[i]);
	      MPI_Abort (MPI_COMM_WORLD, 1);
	    }
	}
      puts ("-Success-");
    }
  fflush (stdout);
  MPI_Win_unlock_all (win);
  MPI_Win_free (&win);
  MPI_Finalize ();
  return 0;
}

// Each rank sorts a block of data in a.
// The data in the shared array a is copied into a local array,
// sorted, and then copied back.
void
parallel_block_mergesort_rma (int a[], int size)
{
  int *temp = malloc (size * sizeof (int));
  if (temp == NULL)
    {
      printf ("Error: Could not allocate temporary array of size %d "
	      "on rank %d\n", size, my_rank);
      MPI_Abort (MPI_COMM_WORLD, 1);
    }
  // Blocks are evenly distributed across ranks.
  int block_size = (size + comm_size - 1) / comm_size;
  // For small problems, do everything on rank 0.
  if (block_size <= 1024)
    block_size = size;
  for (int blocks_per_chunk = 1, chunk_size = block_size;
       chunk_size <= size * 2; blocks_per_chunk *= 2, chunk_size *= 2)
    {
      int chunk_offset = my_rank * block_size;
      // If this rank is a group leader this pass,
      // execute the sort/merge step.
      if (((my_rank % blocks_per_chunk) == 0) && (chunk_offset < size))
	{
	  int rem_size = size - chunk_offset;
	  int this_chunk_size = rem_size >= chunk_size
	    ? chunk_size : rem_size;
	  int *chunk_temp = temp + chunk_offset;
	  int half_chunk = chunk_size / 2;
	  if (!my_rank)
	    {
	      // On rank 0, we can localize the array by casting it.
	      int *a_local = (int *) a;
	      int *chunk_local = a_local + chunk_offset;
	      if (blocks_per_chunk == 1)
		mergesort_serial (chunk_local, this_chunk_size, chunk_temp);
	      else if (this_chunk_size > half_chunk)
		merge (chunk_local, this_chunk_size, half_chunk, chunk_temp);
	    }
	  else
	    {
	      if (blocks_per_chunk == 1)
		mergesort_rma (chunk_offset, this_chunk_size,
		               chunk_temp);
	      else if (this_chunk_size > half_chunk)
		merge_rma (chunk_offset, this_chunk_size,
		           half_chunk, chunk_temp);
	    }
	}
      // Wait for this phase to complete.
      MPI_Barrier (MPI_COMM_WORLD);
    }
  free (temp);
}

void
mergesort_rma (int a_offset, int size, int temp[])
{
  // Switch to insertion sort for small arrays
  if (size <= SMALL)
    {
      insertion_sort_rma (a_offset, size);
      return;
    }
  mergesort_rma (a_offset, size / 2, temp);
  mergesort_rma (a_offset + size / 2, size - size / 2, temp);
  // Merge the two sorted sub-arrays
  merge_rma (a_offset, size, size / 2, temp);
}

void
merge_rma (int a_offset, int size, int left_size, int temp[])
{
  int i1 = 0;
  int i2 = left_size;
  int tempi = 0;
  int a_i1, a_i2;
  while (i1 < left_size && i2 < size)
    {
      MPI_Get_accumulate (NULL, 0, MPI_DATATYPE_NULL,
                          &a_i1, 1, MPI_INT,
                          0, a_offset + i1, 1, MPI_INT,
                          MPI_NO_OP, win);
      MPI_Get_accumulate (NULL, 0, MPI_DATATYPE_NULL,
                          &a_i2, 1, MPI_INT,
                          0, a_offset + i2, 1, MPI_INT,
                          MPI_NO_OP, win);
      MPI_Win_flush_local (0, win);
      if (a_i1 < a_i2)
	{
	  temp[tempi] = a_i1;
	  i1++;
	}
      else
	{
	  temp[tempi] = a_i2;
	  i2++;
	}
      tempi++;
    }
  while (i1 < left_size)
    {
      MPI_Get_accumulate (NULL, 0, MPI_DATATYPE_NULL,
                          &a_i1, 1, MPI_INT,
                          0, a_offset + i1, 1, MPI_INT,
                          MPI_NO_OP, win);
      MPI_Win_flush_local (0, win);
      temp[tempi] = a_i1;
      i1++;
      tempi++;
    }
  while (i2 < size)
    {
      MPI_Get_accumulate (NULL, 0, MPI_DATATYPE_NULL,
                          &a_i2, 1, MPI_INT,
                          0, a_offset + i2, 1, MPI_INT,
                          MPI_NO_OP, win);
      MPI_Win_flush_local (0, win);
      temp[tempi] = a_i2;
      i2++;
      tempi++;
    }
  // Copy sorted temp array into main array, a
  MPI_Put (temp, size, MPI_INT, 0, a_offset, size, MPI_INT, win);
  MPI_Win_flush (0, win);
}

void
insertion_sort_rma (int a_offset, int size)
{
  int i;
  for (i = 0; i < size; i++)
    {
      int j, v, a_i;
      MPI_Get_accumulate (NULL, 0, MPI_DATATYPE_NULL,
                          &a_i, 1, MPI_INT,
                          0, a_offset + i, 1, MPI_INT,
                          MPI_NO_OP, win);
      MPI_Win_flush_local (0, win);
      v = a_i;
      for (j = i - 1; j >= 0; j--)
	{
          int a_j;
	  MPI_Get_accumulate (NULL, 0, MPI_DATATYPE_NULL,
			      &a_j, 1, MPI_INT,
			      0, a_offset + j, 1, MPI_INT,
			      MPI_NO_OP, win);
	  MPI_Win_flush_local (0, win);
	  if (a_j <= v)
	    break;
	  // a[j + 1] = a_j;
          MPI_Accumulate (&a_j, 1, MPI_INT,
                          0, a_offset + j + 1, 1, MPI_INT,
                          MPI_REPLACE, win);
          // We don't need a flush here because j is descendinj is descending
          // Therefore, we don't need to worry about re-using a[j].
	}
      // a[j + 1] = v;
      MPI_Accumulate (&v, 1, MPI_INT,
		      0, a_offset + j + 1, 1, MPI_INT,
                      MPI_REPLACE, win);
      // We need a local flush here, to be able to re-use 'v'.
      // If we bounce buffered it, we could avoid this local flush.
      MPI_Win_flush_local (0, win);
    }
    // Let the put's quiesce.
    MPI_Win_flush (0, win);
}

void
mergesort_serial (int a[], int size, int temp[])
{
  // Switch to insertion sort for small arrays
  if (size <= SMALL)
    {
      insertion_sort (a, size);
      return;
    }
  mergesort_serial (a, size / 2, temp);
  mergesort_serial (a + size / 2, size - size / 2, temp);
  // Merge the two sorted sub-arrays
  merge (a, size, size / 2, temp);
}

void
merge (int a[], int size, int left_size, int temp[])
{
  int i1 = 0;
  int i2 = left_size;
  int tempi = 0;
  while (i1 < left_size && i2 < size)
    {
      if (a[i1] < a[i2])
	{
	  temp[tempi] = a[i1];
	  i1++;
	}
      else
	{
	  temp[tempi] = a[i2];
	  i2++;
	}
      tempi++;
    }
  while (i1 < left_size)
    {
      temp[tempi] = a[i1];
      i1++;
      tempi++;
    }
  while (i2 < size)
    {
      temp[tempi] = a[i2];
      i2++;
      tempi++;
    }
  // Copy sorted temp array into main array, a
  memcpy (a, temp, size * sizeof (int));
}

void
insertion_sort (int a[], int size)
{
  int i;
  for (i = 0; i < size; i++)
    {
      int j, v = a[i];
      for (j = i - 1; j >= 0; j--)
	{
	  if (a[j] <= v)
	    break;
	  a[j + 1] = a[j];
	}
      a[j + 1] = v;
    }
}
