#include <mpi.h>
#include <assert.h>
extern void su2_m6_trace_begin(void);
extern void su2_m6_trace_end(void);
int main(int argc,char **argv) {
  MPI_Init(&argc,&argv);int rank,size;MPI_Comm_rank(MPI_COMM_WORLD,&rank);MPI_Comm_size(MPI_COMM_WORLD,&size);
  su2_m6_trace_begin();
  for(int i=0;i<10;++i){int x=rank+1,sum=0;MPI_Allreduce(&x,&sum,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);assert(sum==size*(size+1)/2);}
  int x=7;MPI_Bcast(&x,1,MPI_INT,0,MPI_COMM_WORLD);assert(x==7);
  su2_m6_trace_end();MPI_Finalize();return 0;
}
