/* 
 Serial recursive merge sort
 Copyright (C) 2011  Atanas Radenski
 This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <omp.h> // Omp used for timing only

#define SMALL    32  // Arrays size <= SMALL switches to insertion sort

void merge(int a[], int size, int temp[]);
void insertion_sort(int a[], int size);
void mergesort_serial(int a[], int size, int temp[]);
int main(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    puts("-Serial Recursive Mergesort-\t");
    // Check arguments
    if ( argc != 2 ) /* argc must be 2 for proper execution! */
    {
        printf( "Usage: %s array-size\n", argv[0]);
        return 1;
    }
	// Get arguments
    int size = atoi(argv[1]); // Array size 
    printf("Array size = %d\n", size);
    // Array allocation
    int* a    = malloc(sizeof(int)*size);
    int* temp = malloc(sizeof(int)*size);
    if (a == NULL || temp == NULL) 
    {
		printf( "Error: Could not allocate array of size %d\n", size);
		return 1;
    }
    // Random array initialization
    int i;
    srand(time(NULL));
    for(i=0; i<size; i++) {
        a[i] = rand() % size;
    }
	// Sort
    double start = omp_get_wtime();
    mergesort_serial(a, size, temp);
    double end = omp_get_wtime( );
    printf("Start = %.2f\nEnd = %.2f\nElapsed = %.2f\n",
		start, end, end - start);
	// Result check
    for(i=1; i<size; i++) {
        if (!(a[i-1] <= a[i])) {
            printf("Implementtion error: a[%d]=%d > a[%d]=%d\n", i-1, a[i-1], i, a[i]);
            return 1;
        }
    }
    puts("-Success-");
    double wtick = omp_get_wtick( );
    printf("Wtick = %.8f\n1/Wtick = %.8f\n",
		wtick, 1.0 / wtick);
    return 0;
}

void mergesort_serial(int a[], int size, int temp[]) {
    // Switch to insertion sort for small arrays
     if (size < SMALL) {
       insertion_sort(a, size);
       return;
    }    
    mergesort_serial(a, size/2, temp);
    mergesort_serial(a + size/2, size - size/2, temp);
    // The above call will not work properly in an OpenMP program
    // Merge the two sorted subarrays into a temp array
    merge(a, size, temp);
}

void merge(int a[], int size, int temp[]) {
    int i1 = 0;
    int i2 = size/2;
    int tempi = 0;
    while (i1 < size/2 && i2 < size) {
        if (a[i1] < a[i2]) {
            temp[tempi] = a[i1];
            i1++;
        } else {
            temp[tempi] = a[i2];
            i2++;
        }
        tempi++;
    }
    while (i1 < size/2) {
        temp[tempi] = a[i1];
        i1++;
        tempi++;
    }
    while (i2 < size) {
        temp[tempi] = a[i2];
        i2++;
        tempi++;
    }
    // Copy sorted temp array into main array, a
    memcpy(a, temp, size*sizeof(int));
}

void insertion_sort(int a[], int size) {
    int i;
    for (i=0; i < size; i++) {
    int j, v = a[i];
        for (j = i - 1; j >= 0; j--) {
            if (a[j] <= v) break; 
            a[j + 1] = a[j];
        }
        a[j + 1] = v;
    }
}
