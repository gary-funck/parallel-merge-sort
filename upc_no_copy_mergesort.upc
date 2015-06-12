/* UPC recursive merge sort
   Copyright (C) 2011  Atanas Radenski

   Derived from omp_mergesort.c by Gary Funck <gary@intrepid.com>
   Date: 2015-05-16

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
#include <upc.h>

// Arrays size <= SMALL switches to insertion sort
#define SMALL    32

extern double get_time (void);
void insertion_sort (int a[], int size);
void mergesort_serial (int a[], int size, int temp[]);
void merge (int a[], int size, int left_size, int temp[]);
void insertion_sort_upc (shared [] int a[], int size);
void mergesort_upc (shared [] int a[], int size, int temp[]);
void merge_upc (shared [] int a[], int size, int left_size, int temp[]);
void parallel_block_mergesort_upc (shared [] int a[], int size);
int main (int argc, char *argv[]);

int debug = 1;
shared [] int *shared a;
shared int size;

int
main (int argc, char *argv[])
{
  if (!MYTHREAD)
    {
      puts ("-UPC No Copy Recursive Mergesort-\t");
      // Check arguments
      if (argc != 2)		/* argc must be 2 for proper execution! */
	{
	  printf ("Usage: %s array-size\n", argv[0]);
	  upc_global_exit (1);
	}
      // Get arguments
      size = atoi (argv[1]);	// Array size 
      printf ("Array size = %d\nProcesses = %d\n\n", size, THREADS);
      // Array allocation (shared, on thread 0)
      a = upc_alloc (size * sizeof (int));
      if (a == NULL)
	{
	  printf ("Error: Could not allocate shred array of size %d\n", size);
	  upc_global_exit (1);
	}
      // Random array initialization
      srand (314159);
      for (int i = 0; i < size; i++)
	{
	  a[i] = rand () % size;
	}
    }
  upc_barrier;
  double start = get_time ();
  // All threads execute the parallel block merge procedure.
  parallel_block_mergesort_upc (a, size);
  double end = get_time ();
  if (!MYTHREAD)
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
	      upc_global_exit (1);
	    }
	}
      puts ("-Success-");
    }
  return 0;
}

// Each UPC thread sorts a block of data in a.
// The data in the shared array a is copied into a local array,
// sorted, and then copied back.
void
parallel_block_mergesort_upc (shared [] int a[], int size)
{
  int *temp = malloc (size * sizeof (int));
  if (temp == NULL)
    {
      printf ("Error: Could not allocate temporary array of size %d "
	      "on thread %d\n", size, MYTHREAD);
      upc_global_exit (1);
    }
  // Blocks are evenly distributed across threads.
  int block_size = (size + THREADS - 1) / THREADS;
  // For small problems, do everything on thread 0.
  if (block_size <= 1024)
    block_size = size;
  for (int blocks_per_chunk = 1, chunk_size = block_size;
       chunk_size <= size * 2; blocks_per_chunk *= 2, chunk_size *= 2)
    {
      int chunk_offset = MYTHREAD * block_size;
      // If this thread is a group leader this pass,
      //  execute the sort/merge step.
      if (((MYTHREAD % blocks_per_chunk) == 0) && (chunk_offset < size))
	{
	  int rem_size = size - chunk_offset;
	  int this_chunk_size = rem_size >= chunk_size
	    ? chunk_size : rem_size;
	  shared [] int *chunk = a + chunk_offset;
	  int *chunk_temp = temp + chunk_offset;
	  int half_chunk = chunk_size / 2;
	  if (!MYTHREAD)
	    {
	      // On thread 0, we can localize the array by casting it.
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
		mergesort_upc (chunk, this_chunk_size, chunk_temp);
	      else if (this_chunk_size > half_chunk)
		merge_upc (chunk, this_chunk_size, half_chunk, chunk_temp);
	    }
	}
      // Wait for this phase to complete.
      upc_barrier;
    }
  free (temp);
}

void
mergesort_upc (shared [] int a[], int size, int temp[])
{
  // Switch to insertion sort for small arrays
  if (size <= SMALL)
    {
      insertion_sort_upc (a, size);
      return;
    }
  mergesort_upc (a, size / 2, temp);
  mergesort_upc (a + size / 2, size - size / 2, temp);
  // Merge the two sorted sub-arrays
  merge_upc (a, size, size / 2, temp);
}

void
merge_upc (shared [] int a[], int size, int left_size, int temp[])
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
  upc_memput (a, temp, size * sizeof (int));
}

void
insertion_sort_upc (shared [] int a[], int size)
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
