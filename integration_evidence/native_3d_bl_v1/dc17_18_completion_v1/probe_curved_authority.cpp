#define main dc18_standalone_main
#include "integration_evidence/native_3d_bl_v1/dc18_prep/test_curved_reference.cpp"
#undef main
#undef CHECK
#include "Common/include/adaptation/CNativeReferenceIO3D.hpp"
// Frozen original M6 field diagnostic; no mesh construction or acceptance claim.
#include "SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include "SU2_CFD/include/adaptation/CNativeComplexity3D.hpp"
#include "Common/include/adaptation/CNativeField3D.hpp"
#include "Common/include/adaptation/CNativeImport3D.hpp"
#include <iomanip>
#include "Common/include/adaptation/CNativeSupport3D.hpp"
#include "Common/include/linear_algebra/blas_structure.hpp"
#include "Common/include/CConfig.hpp"
using BL=CBoundaryLayerMetric;
using namespace SU2Native3D;
static BL::Tensor Unpack(const Tensor& m) {
  BL::Tensor t;t.m[0][0]=m.xx;t.m[1][1]=m.yy;t.m[2][2]=m.zz;
  t.m[0][1]=t.m[1][0]=m.xy;t.m[0][2]=t.m[2][0]=m.xz;t.m[1][2]=t.m[2][1]=m.yz;return t;
}
static double Ratio(const BL::Tensor& a,const BL::Tensor& b,const std::array<double,3>& x,const std::array<double,3>& y,double growth) {
  su2double vec[3][3],val[3],work[3],inv[3][3],test[3][3]={},distance2=0;
  CBlasStructure::EigenDecomposition(a.m,vec,val,3,work);
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j) {
    inv[i][j]=vec[i][j]/sqrt(val[j]);distance2+=(x[i]-y[i])*b.m[i][j]*(x[j]-y[j]);
  }
  const auto factor=1/pow(1+growth*sqrt(fmax(0.,distance2)),2);
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)for(unsigned l=0;l<3;++l)
    test[i][j]+=inv[k][i]*factor*b.m[k][l]*inv[l][j];
  CBlasStructure::EigenDecomposition(test,vec,val,3,work);return *std::max_element(val,val+3);
}
int main(int argc,char** argv) {
  SU2_MPI::Init(&argc,&argv);int exitCode=0;
  try {
    if(argc!=5)throw std::runtime_error("usage: probe mesh config donors output.json");
    std::ifstream cf(argv[2]);std::stringstream input;std::string line;
    while(std::getline(cf,line))if(line.rfind("ADAP_LOOP=",0)!=0)input<<line<<"\n";
    input<<"\nADAP_BL_METHOD= FIRST_ROW\nADAP_BL_H0_RAMP= 5\nADAP_LOOP= YES\nADAP_SIZES= (500000,500000,500000,500000,500000,500000)\n";
    CConfig config(input,SU2_COMPONENT::SU2_CFD,false);config.SetAdap_BL_RampEvent(1);
    const auto mesh=ReadSu2(argv[1]);ReferenceState state;
    for(const auto& marker:mesh.markers) {
      const int m=state.marker_names.size();state.marker_names.push_back(marker.first);
      for(const auto& face:marker.second) {
        if(face.size()!=3)throw std::runtime_error("Probe requires tetrahedral surface triangles.");
        Facet f;f.id=state.facets.size();f.marker=m;
        for(unsigned k=0;k<3;++k){const auto& p=mesh.p.at(face[k]);f.v[k]={Id(face[k]),{p[0],p[1],p[2]}};}
        state.facets.push_back(f);
      }
    }
    std::vector<DonorCell> donors;double core=INFINITY;
    {std::ifstream in(argv[3],std::ios::binary);uint64_t count=0;in.read(reinterpret_cast<char*>(&count),8);
     if(!in || !count || count>20000000)throw std::runtime_error("Invalid donor count.");donors.resize(count);
     for(size_t r=0;r<count;++r){auto& d=donors[r];d.cell.id=r;
       for(unsigned k=0;k<4;++k){uint64_t id;double x[3],m[6];in.read(reinterpret_cast<char*>(&id),8);in.read(reinterpret_cast<char*>(x),24);in.read(reinterpret_cast<char*>(m),48);
         d.cell.v[k]={id,{x[0],x[1],x[2]}};d.sensor[k]={m[0],m[1],m[2],m[3],m[4],m[5]};
         const auto a=Unpack(d.sensor[k]);su2double v[3][3],l[3],w[3];CBlasStructure::EigenDecomposition(a.m,v,l,3,w);core=std::min(core,double(*std::min_element(l,l+3)));
       }
       if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0){std::swap(d.cell.v[0],d.cell.v[1]);std::swap(d.sensor[0],d.sensor[1]);}
     }
     if(!in || !(core>0))throw std::runtime_error("Incomplete or non-SPD donors.");}
    std::map<std::array<Id,3>,std::pair<size_t,unsigned>> surface;
    for(size_t f=0;f<state.facets.size();++f) {
      auto key=std::array<Id,3>{state.facets[f].v[0].id,state.facets[f].v[1].id,state.facets[f].v[2].id};std::sort(key.begin(),key.end());
      if(!surface.emplace(key,std::make_pair(f,0)).second)throw std::runtime_error("Duplicate boundary facet.");
    }
    for(const auto& d:donors)for(unsigned k=0;k<4;++k) {
      std::array<Id,3> key;unsigned next=0;for(unsigned j=0;j<4;++j)if(j!=k)key[next++]=d.cell.v[j].id;std::sort(key.begin(),key.end());
      const auto found=surface.find(key);if(found==surface.end())continue;
      auto& f=state.facets[found->second.first];++found->second.second;
      if(Orientation(f.v[0].p,f.v[1].p,f.v[2].p,d.cell.v[k].p)>0)std::swap(f.v[1],f.v[2]);
    }
    for(const auto& f:surface)if(f.second.second!=1)throw std::runtime_error("Boundary does not bind to exactly one original donor.");
    state.original=std::make_shared<PlanarReference>(state.facets);
    using SU2NativeGeometry::MappedFacetReference;
    std::vector<DonorCell>().swap(donors);
    std::vector<PNTriangleSurface::Triangle> triangles;std::vector<Vec<3>> boundary;std::map<Id,long> local;
    PNTriangleSurface::Options options;options.featureAngle=config.GetAdap_Angle();double scale=1;
    for(size_t marker=0;marker<state.marker_names.size();++marker) {
      bool wall=false;for(unsigned w=0;w<config.GetnAdap_BL();++w)wall=wall||config.GetAdap_BL(w).marker==state.marker_names[marker];
      if(config.GetMarker_CfgFile_KindBC(state.marker_names[marker])==SYMMETRY_PLANE)options.planarMarkers.insert(marker);
      else if(!wall)options.fixedMarkers.insert(marker);
    }
    for(const auto& f:state.facets) {
      PNTriangleSurface::Triangle t;t.marker=f.marker;
      for(unsigned k=0;k<3;++k){const auto n=f.v[k];auto found=local.find(n.id);
        if(found==local.end()){found=local.emplace(n.id,boundary.size()).first;boundary.push_back({n.p.x,n.p.y,n.p.z});}
        t.v[k]=found->second;scale=std::max({scale,std::abs(n.p.x),std::abs(n.p.y),std::abs(n.p.z)});}
      triangles.push_back(t);
    }
    PNTriangleSurface curvedSurface(boundary,triangles,options);
    MappedFacetReference::CurvedNets nets;std::map<Id,double> errors;
    for(size_t i=0;i<triangles.size();++i){const Id id=state.facets[i].id;errors[id]=1e-13*scale;
      for(unsigned k=0;k<10;++k){const auto p=curvedSurface.ControlNets()[i][k];nets[id][k]={p[0],p[1],p[2]};}}
    size_t fittedPlanes=0;state.original=FitPlanarMarkers(state.original,state.facets,options.planarMarkers,1e-13,fittedPlanes,size_t(2)<<30);
    auto geometry=std::make_shared<MappedFacetReference>(state.original,errors,18,size_t(2)<<30,false,nets);
    BoundaryPoints points;std::vector<SurfaceFace> skin;double envelope=0,mapping=0,deviation=0;
    for(const auto& f:state.facets) {
      skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});
      for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,geometry->Vertex(n.id)});
      const auto b=geometry->FacetPoint(f.id,.25,.25);const auto p=geometry->Realize(b);
      if(!geometry->Admissible(b,p))throw std::runtime_error("Curved point left its mapping contract.");
      mapping=std::max(mapping,geometry->ErrorBound(b,p));envelope=std::max(envelope,geometry->EnvelopeError(f.id));
      const std::array<SU2NativeGeometry::SurfaceBinding,3> binds{{geometry->Vertex(f.v[0].id),geometry->Vertex(f.v[1].id),geometry->Vertex(f.v[2].id)}};
      deviation=std::max(deviation,geometry->SurfaceError(f.id,binds,{f.v[0].p,f.v[1].p,f.v[2].p}));
    }
    auto parameters=std::make_shared<BoundaryParameterState>(geometry,points,skin,1,size_t(2)<<30);
    state.original=PlanarReference::WithParameters(state.original,parameters);
    const auto encoded=EncodeReference(state),restored=EncodeReference(DecodeReference(encoded,size_t(2)<<30));
    if(encoded!=restored)throw std::runtime_error("Curved reference changed across persistence.");
    std::ofstream sidecar(std::string(argv[4])+".native_ref");sidecar<<encoded;sidecar.close();
    if(!sidecar)throw std::runtime_error("Could not preserve curved reference sidecar.");
    std::ofstream out(argv[4]);out<<std::setprecision(17)<<"{\"authority_admitted\":true,\"mesh_constructed\":false,\"facets\":"<<triangles.size()
      <<",\"fallback_patches\":"<<curvedSurface.Statistics().fallbackFaces<<",\"mapping_error\":"<<mapping<<",\"envelope\":"<<envelope
      <<",\"child_error\":"<<deviation<<",\"resident_bytes\":"<<geometry->ResidentBytes()<<",\"sidecar_bytes\":"<<encoded.size()<<"}\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';exitCode=1;}
  SU2_MPI::Finalize();return exitCode;
}
