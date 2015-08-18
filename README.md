# parallel-merge-sort
Shared Memory, Message Passing, and Hybrid  Merge Sort: UPC, OpenMP, MPI and Hybrid Implementations


This project demonstrates the implementation of a parallel merge sort algorithm implemented using sevaeral approaches.
The programming languages and frameworks used, include:
  - UPC (Unified Parallel C)
  - OpenMP
  - MPI
  - MPI-3 RMA
  - Hybrid MPI and OpenMP
  - Hybrid UPC and OpenMP
  
The MPI and OpenMP implementations were developed by Prof. [Atanas Radenski](http://www1.chapman.edu/~radenski/)
of Chapman University.  His work is described in [_Shared Memory, Message Passing, and Hybrid Merge Sorts
for Standalone and Clustered SMPs_](http://www1.chapman.edu/~radenski/research/papers/mergesort-pdpta11.pdf)
(Proc. PDPTA'11, the 2011 International Conference on Parallel and Distributed Processing
Techniques and Applications, CSREA Press, 2011, pp. 367-373).

Gary Funck (<gary@intrepid.com>) re-implemented the OpenMP and hybrid programs using the
[UPC](https://upc-lang.org/assets/Uploads/spec/upc-lang-spec-1.3.pdf) programming language.
The hybrid implementation combines UPC and OpenMP.  The UPC programs were compiled with
the [GNU UPC](http://gccupc.org/) compiler.
Gary also re-implemented the UPC benchmarks into MPI programs that make use of
MPI-3 one-sided (RMA) operations.

This project is open source, per the [GNU GPL](http://www.gnu.org/licenses/gpl.html) license.
