#include "Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "Common/include/adaptation/CNativeAffineReference.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
int main(int argc,char** argv) {
  SU2_MPI::Init(&argc,&argv);int status=1;
  try {
    using namespace SU2Native3D;
    if(argc!=4)throw std::runtime_error("usage: probe sidecar sorted-face-index output.json");
    std::ifstream in(argv[1]);std::ostringstream text;text<<in.rdbuf();if(!in)throw std::runtime_error("Cannot read sidecar.");
    const auto state=DecodeReference(text.str(),size_t(2)<<30);const auto& g=state.original->Parameters()->Geometry();
    auto facets=state.facets;auto key=[](const Facet& f){Face k{{f.v[0].id,f.v[1].id,f.v[2].id}};std::sort(k.begin(),k.end());return k;};
    std::sort(facets.begin(),facets.end(),[&](const auto& a,const auto& b){return key(a)<key(b);});
    const auto& face=facets.at(std::stoull(argv[2]));auto nodes=face.v;
    std::sort(nodes.begin(),nodes.end(),[](const auto& a,const auto& b){return a.id<b.id;});
    double radius=0;for(const auto& f:facets)radius=std::max(radius,4*g.EnvelopeError(f.id));
    std::ofstream out(argv[3]);out<<std::setprecision(17);
    auto point=[&](Point p){out<<'['<<p.x<<','<<p.y<<','<<p.z<<']';};
    out<<"{\"facet\":"<<face.id<<",\"marker\":"<<face.marker<<",\"global_radius\":"<<radius<<",\"vertices\":[";
    std::array<Node,3> global,local;uint64_t work=0;
    for(unsigned k=0;k<3;++k) {
      std::vector<std::array<Point,3>> incident;double r=0;
      if(k)out<<',';out<<"{\"id\":"<<nodes[k].id<<",\"point\":";point(nodes[k].p);out<<",\"incident\":[";bool first=true;
      for(const auto& f:facets)if(std::any_of(f.v.begin(),f.v.end(),[&](Node n){return n.id==nodes[k].id;})) {
        incident.push_back({f.v[0].p,f.v[1].p,f.v[2].p});r=std::max(r,4*g.EnvelopeError(f.id));
        const auto a=f.v[0].p,b=f.v[1].p,c=f.v[2].p;
        const Point u{b.x-a.x,b.y-a.y,b.z-a.z},v{c.x-a.x,c.y-a.y,c.z-a.z};
        Point n{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};const double norm=std::hypot(n.x,n.y,n.z);n={n.x/norm,n.y/norm,n.z/norm};
        double lo=INFINITY,hi=-INFINITY;
        for(const auto& p:g.Curves().at(f.id)){const double d=(p.x-a.x)*n.x+(p.y-a.y)*n.y+(p.z-a.z)*n.z;lo=std::min(lo,d);hi=std::max(hi,d);}
        if(!first)out<<',';first=false;out<<"{\"facet\":"<<f.id<<",\"marker\":"<<f.marker<<",\"envelope\":"<<g.EnvelopeError(f.id)
          <<",\"signed_control_plane_min\":"<<lo<<",\"signed_control_plane_max\":"<<hi<<'}';
      }
      global[k]={nodes[k].id+1000000000,SU2NativeGeometry::SourceColumn(nodes[k].p,incident,radius,work,1000000)};
      local[k]={nodes[k].id+1000000000,SU2NativeGeometry::SourceColumn(nodes[k].p,incident,r,work,1000000)};
      out<<"],\"local_radius\":"<<r<<",\"global_column\":";point(global[k].p);out<<",\"local_column\":";point(local[k].p);out<<'}';
    }
    out<<"],\"product_orientations\":[";
    for(unsigned mode=0;mode<2;++mode) {
      if(mode)out<<',';out<<'[';const auto& column=mode?local:global;
      const std::array<Point,6> p{{nodes[0].p,nodes[1].p,nodes[2].p,column[0].p,column[1].p,column[2].p}};
      unsigned t=0;for(const auto& ids:std::array<std::array<unsigned,4>,3>{{{0,1,2,5},{0,1,4,5},{0,3,4,5}}}) {
        if(t++)out<<',';out<<Orientation(p[ids[0]],p[ids[1]],p[ids[2]],p[ids[3]]);
      }out<<']';
    }
    out<<"]}\n";if(!out)throw std::runtime_error("Cannot preserve junction witness.");status=0;
  }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;}
  SU2_MPI::Finalize();return status;
}
