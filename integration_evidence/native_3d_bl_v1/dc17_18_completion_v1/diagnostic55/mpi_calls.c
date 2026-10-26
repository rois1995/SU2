/* Diagnostic LD_PRELOAD collector, enabled only by the frozen test's begin/end hooks.
 * No extra collective, barrier, payload, sampling thread or cross-rank clock comparison. */
#define _GNU_SOURCE
#include <mpi.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { ALLREDUCE, ALLGATHER, ALLGATHERV, ALLTOALL, ALLTOALLV, BCAST, REDUCE,
       EXSCAN, SCAN, BARRIER, GATHER, GATHERV, SEND, RECV, ISEND, IRECV, WAITALL, SPLIT };
static const char *names[] = {"Allreduce","Allgather","Allgatherv","Alltoall","Alltoallv","Bcast","Reduce",
    "Exscan","Scan","Barrier","Gather","Gatherv","Send","Recv","Isend","Irecv","Waitall","Comm_split"};
typedef struct {
  void *site; MPI_Datatype datatype; MPI_Op op; MPI_Fint communicator;
  int kind, count, bytes, ranks;
  uint64_t calls, over_100us; double seconds, maximum;
} Entry;
static Entry entries[4096];
static int enabled, rank;
static uint64_t overflow;
static double started;

void su2_m6_trace_begin(void) {
  memset(entries,0,sizeof(entries)); overflow=0;
  PMPI_Comm_rank(MPI_COMM_WORLD,&rank); started=PMPI_Wtime(); enabled=1;
}
static void record(void *site,int kind,int count,MPI_Datatype datatype,MPI_Op op,MPI_Comm comm,double seconds) {
  MPI_Fint key=comm==MPI_COMM_NULL?-1:PMPI_Comm_c2f(comm);
  size_t slot=(((uintptr_t)site>>3)+kind*17u+(unsigned)count*31u+(unsigned)key*7u)%4096;
  for(unsigned i=0;i<4096;++i,slot=(slot+1)%4096) {
    Entry *e=&entries[slot];
    if(!e->calls) {
      e->site=site;e->kind=kind;e->count=count;e->datatype=datatype;e->op=op;e->communicator=key;
      if(datatype!=MPI_DATATYPE_NULL)PMPI_Type_size(datatype,&e->bytes);
      if(comm!=MPI_COMM_NULL)PMPI_Comm_size(comm,&e->ranks);
    }
    if(e->site==site && e->kind==kind && e->count==count && e->datatype==datatype && e->op==op && e->communicator==key) {
      ++e->calls;e->seconds+=seconds;if(seconds>e->maximum)e->maximum=seconds;
      e->over_100us+=seconds>1e-4;return;
    }
  }
  ++overflow;
}
void su2_m6_trace_end(void) {
  double elapsed=PMPI_Wtime()-started;enabled=0;
  char path[100];snprintf(path,sizeof(path),"mpi_calls_rank%d.csv",rank);FILE *f=fopen(path,"w");
  if(!f){perror(path);return;}
  fprintf(f,"rank,call,caller_offset,module,count,type_bytes,op,comm,ranks,calls,seconds,max_seconds,over_100us\n");
  double total=0;uint64_t calls=0;
  for(size_t i=0;i<4096;++i)if(entries[i].calls) {
    Entry *e=&entries[i];Dl_info info={0};dladdr(e->site,&info);
    const char *op=e->op==MPI_SUM?"SUM":e->op==MPI_MIN?"MIN":e->op==MPI_MAX?"MAX":e->op==MPI_OP_NULL?"NONE":"OTHER";
    fprintf(f,"%d,%s,0x%lx,%s,%d,%d,%s,%d,%d,%llu,%.17g,%.17g,%llu\n",rank,names[e->kind],
      (unsigned long)((uintptr_t)e->site-(uintptr_t)info.dli_fbase),info.dli_fname?info.dli_fname:"unknown",
      e->count,e->bytes,op,(int)e->communicator,e->ranks,(unsigned long long)e->calls,e->seconds,e->maximum,
      (unsigned long long)e->over_100us);
    total+=e->seconds;calls+=e->calls;
  }
  fclose(f);snprintf(path,sizeof(path),"mpi_summary_rank%d.json",rank);f=fopen(path,"w");
  if(!f){perror(path);return;}
  fprintf(f,"{\"rank\":%d,\"elapsed\":%.17g,\"mpi_seconds\":%.17g,\"calls\":%llu,\"overflow\":%llu}\n",
      rank,elapsed,total,(unsigned long long)calls,(unsigned long long)overflow);fclose(f);
}

#define WRAP(NAME, KIND, ARGS, INVOKE, COUNT, TYPE, OP, COMM) \
 int MPI_##NAME ARGS { \
   if(!enabled)return PMPI_##NAME INVOKE; \
   void *site=__builtin_return_address(0);double begin=PMPI_Wtime(); \
   int result=PMPI_##NAME INVOKE;double elapsed=PMPI_Wtime()-begin; \
   record(site,KIND,COUNT,TYPE,OP,COMM,elapsed);return result; \
 }
WRAP(Allreduce,ALLREDUCE,(const void*s,void*r,int n,MPI_Datatype t,MPI_Op o,MPI_Comm c),(s,r,n,t,o,c),n,t,o,c)
WRAP(Allgather,ALLGATHER,(const void*s,int n,MPI_Datatype t,void*r,int m,MPI_Datatype u,MPI_Comm c),(s,n,t,r,m,u,c),n,t,MPI_OP_NULL,c)
WRAP(Allgatherv,ALLGATHERV,(const void*s,int n,MPI_Datatype t,void*r,const int*m,const int*d,MPI_Datatype u,MPI_Comm c),(s,n,t,r,m,d,u,c),n,t,MPI_OP_NULL,c)
WRAP(Alltoall,ALLTOALL,(const void*s,int n,MPI_Datatype t,void*r,int m,MPI_Datatype u,MPI_Comm c),(s,n,t,r,m,u,c),n,t,MPI_OP_NULL,c)
WRAP(Alltoallv,ALLTOALLV,(const void*s,const int*n,const int*d,MPI_Datatype t,void*r,const int*m,const int*e,MPI_Datatype u,MPI_Comm c),(s,n,d,t,r,m,e,u,c),-1,t,MPI_OP_NULL,c)
WRAP(Bcast,BCAST,(void*s,int n,MPI_Datatype t,int root,MPI_Comm c),(s,n,t,root,c),n,t,MPI_OP_NULL,c)
WRAP(Reduce,REDUCE,(const void*s,void*r,int n,MPI_Datatype t,MPI_Op o,int root,MPI_Comm c),(s,r,n,t,o,root,c),n,t,o,c)
WRAP(Exscan,EXSCAN,(const void*s,void*r,int n,MPI_Datatype t,MPI_Op o,MPI_Comm c),(s,r,n,t,o,c),n,t,o,c)
WRAP(Scan,SCAN,(const void*s,void*r,int n,MPI_Datatype t,MPI_Op o,MPI_Comm c),(s,r,n,t,o,c),n,t,o,c)
WRAP(Barrier,BARRIER,(MPI_Comm c),(c),0,MPI_DATATYPE_NULL,MPI_OP_NULL,c)
WRAP(Gather,GATHER,(const void*s,int n,MPI_Datatype t,void*r,int m,MPI_Datatype u,int root,MPI_Comm c),(s,n,t,r,m,u,root,c),n,t,MPI_OP_NULL,c)
WRAP(Gatherv,GATHERV,(const void*s,int n,MPI_Datatype t,void*r,const int*m,const int*d,MPI_Datatype u,int root,MPI_Comm c),(s,n,t,r,m,d,u,root,c),n,t,MPI_OP_NULL,c)
WRAP(Send,SEND,(const void*s,int n,MPI_Datatype t,int dest,int tag,MPI_Comm c),(s,n,t,dest,tag,c),n,t,MPI_OP_NULL,c)
WRAP(Recv,RECV,(void*s,int n,MPI_Datatype t,int src,int tag,MPI_Comm c,MPI_Status*status),(s,n,t,src,tag,c,status),n,t,MPI_OP_NULL,c)
WRAP(Isend,ISEND,(const void*s,int n,MPI_Datatype t,int dest,int tag,MPI_Comm c,MPI_Request*q),(s,n,t,dest,tag,c,q),n,t,MPI_OP_NULL,c)
WRAP(Irecv,IRECV,(void*s,int n,MPI_Datatype t,int src,int tag,MPI_Comm c,MPI_Request*q),(s,n,t,src,tag,c,q),n,t,MPI_OP_NULL,c)
WRAP(Waitall,WAITALL,(int n,MPI_Request*q,MPI_Status*s),(n,q,s),n,MPI_DATATYPE_NULL,MPI_OP_NULL,MPI_COMM_NULL)
WRAP(Comm_split,SPLIT,(MPI_Comm c,int color,int key,MPI_Comm*r),(c,color,key,r),0,MPI_DATATYPE_NULL,MPI_OP_NULL,c)
