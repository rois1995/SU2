#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  int r; MPI_Comm_rank(MPI_COMM_WORLD, &r);
  MPI_Barrier(MPI_COMM_WORLD);
  sleep(3);
  MPI_Barrier(MPI_COMM_WORLD);
  if (r == 0) printf("hello done\n");
  MPI_Finalize();
  return 0;
}
