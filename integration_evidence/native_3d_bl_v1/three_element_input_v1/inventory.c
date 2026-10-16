#include "cgnslib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define CHECK(call) do { if (call) { fprintf(stderr,"%s: %s\n",#call,cg_get_error()); return 1; } } while (0)
int main(int argc,char** argv) {
  if(argc!=2 && argc!=3)return 2;
  int f,nb; CHECK(cg_open(argv[1],CG_MODE_READ,&f)); CHECK(cg_nbases(f,&nb));
  for(int b=1;b<=nb;++b) {
    char name[33];int dim,physical,nz;CHECK(cg_base_read(f,b,name,&dim,&physical));
    printf("base\t%s\t%d\t%d\n",name,dim,physical);CHECK(cg_nzones(f,b,&nz));
    for(int z=1;z<=nz;++z) {
      cgsize_t size[9]={0};int ns,nbc;ZoneType_t zoneType;
      CHECK(cg_zone_read(f,b,z,name,size));CHECK(cg_zone_type(f,b,z,&zoneType));
      printf("zone\t%s\t%d",name,(int)zoneType);
      for(int i=0;i<9;++i)printf("\t%lld",(long long)size[i]);puts("");
      CHECK(cg_nsections(f,b,z,&ns));
      if(argc==3) {
        if(nb!=1 || nz!=1 || zoneType!=Unstructured || size[0]>2000000)return 2;
        FILE* out=fopen(argv[2],"wb");if(!out)return 1;
        uint64_t n=(uint64_t)size[0],sections=(uint64_t)ns;
        fwrite("NATCGNS1",1,8,out);fwrite(&n,8,1,out);fwrite(&sections,8,1,out);
        double* coords=malloc(n*sizeof(double));if(!coords)return 1;
        const char* names[3]={"CoordinateX","CoordinateY","CoordinateZ"};
        cgsize_t lo=1,hi=size[0];
        for(int d=0;d<3;++d) {
          CHECK(cg_coord_read(f,b,z,names[d],RealDouble,&lo,&hi,coords));
          if(fwrite(coords,sizeof(double),n,out)!=n)return 1;
        }
        free(coords);
        for(int i=1;i<=ns;++i) {
          ElementType_t type;cgsize_t first,last;int boundary,parent;
          CHECK(cg_section_read(f,b,z,i,name,&type,&first,&last,&boundary,&parent));
          uint64_t header[3]={(uint64_t)type,(uint64_t)first,(uint64_t)(last-first+1)};
          if(type!=QUAD_4)header[2]=0;
          fwrite(header,8,3,out);
          if(header[2]) {
            cgsize_t* conn=malloc(4*header[2]*sizeof(cgsize_t));if(!conn)return 1;
            CHECK(cg_elements_read(f,b,z,i,conn,NULL));
            for(uint64_t k=0;k<4*header[2];++k){uint64_t id=(uint64_t)conn[k];if(fwrite(&id,8,1,out)!=1)return 1;}
            free(conn);
          }
        }
        if(fclose(out))return 1;
      }
      for(int i=1;i<=ns;++i) {
        ElementType_t type;cgsize_t first,last;int boundary,parent;
        CHECK(cg_section_read(f,b,z,i,name,&type,&first,&last,&boundary,&parent));
        printf("section\t%s\t%s\t%lld\t%lld\n",name,cg_ElementTypeName(type),(long long)first,(long long)last);
      }
      CHECK(cg_nbocos(f,b,z,&nbc));
      for(int i=1;i<=nbc;++i) {
        BCType_t type;PointSetType_t pointset;DataType_t normalType;
        cgsize_t points,normals;int index[3],datasets;
        CHECK(cg_boco_info(f,b,z,i,name,&type,&pointset,&points,index,&normals,&normalType,&datasets));
        printf("boundary\t%s\t%s\t%lld\n",name,cg_BCTypeName(type),(long long)points);
      }
    }
  }
  CHECK(cg_close(f));return 0;
}
