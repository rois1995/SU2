// Representation/API probe only: no PN geometry, volume quality, BL, MPI, transfer or joint network.
#include "geometrycentral/surface/integer_coordinates_intrinsic_triangulation.h"
#include "geometrycentral/surface/surface_mesh_factories.h"
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace geometrycentral;
using namespace geometrycentral::surface;

static void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static uint64_t fingerprint(ManifoldSurfaceMesh& mesh,VertexPositionGeometry& geom) {
  uint64_t h=1469598103934665603ULL;auto add=[&](uint64_t x){h^=x;h*=1099511628211ULL;};
  for(auto v:mesh.vertices()){add(v.getIndex());auto p=geom.vertexPositions[v];for(double x:{p.x,p.y,p.z}){uint64_t bits;std::memcpy(&bits,&x,8);add(bits);}}
  for(auto f:mesh.faces()){add(f.getIndex());for(auto v:f.adjacentVertices())add(v.getIndex());}return h;
}
static SurfacePoint locate(IntegerCoordinatesIntrinsicTriangulation& tri,Face input,Vector3 target) {
  auto& cs=tri.getCommonSubdivision();cs.constructMesh();
  for(auto face:cs.mesh->faces()) {
    if(cs.sourceFaceA[face]!=input)continue;
    std::array<Vector3,3> a,b;unsigned i=0;
    for(auto v:face.adjacentVertices()){require(i<3,"nontriangle subdivision");auto* p=cs.sourcePoints[v];require(p,"missing subdivision point");
      a[i]=p->posA.inFace(input).faceCoords;b[i]=p->posB.inFace(cs.sourceFaceB[face]).faceCoords;++i;}
    require(i==3,"nontriangle subdivision");
    const double det=(a[1].y-a[0].y)*(a[2].z-a[0].z)-(a[2].y-a[0].y)*(a[1].z-a[0].z);
    if(std::abs(det)<1e-18)continue;
    const double u=((target.y-a[0].y)*(a[2].z-a[0].z)-(a[2].y-a[0].y)*(target.z-a[0].z))/det;
    const double v=((a[1].y-a[0].y)*(target.z-a[0].z)-(target.y-a[0].y)*(a[1].z-a[0].z))/det;
    const double w=1-u-v;if(w< -1e-10||u< -1e-10||v< -1e-10)continue;
    const Vector3 check=a[0]*w+a[1]*u+a[2]*v, mapped=b[0]*w+b[1]*u+b[2]*v;
    require(std::isfinite(mapped.x)&&std::isfinite(mapped.y)&&std::isfinite(mapped.z),"nonfinite mapped barycentric");
    require(std::abs(check.x-target.x)<1e-10&&std::abs(check.y-target.y)<1e-10&&std::abs(check.z-target.z)<1e-10,"association mismatch");
    return SurfacePoint(cs.sourceFaceB[face],mapped);
  }
  throw std::runtime_error("unsupported: no common subdivision triangle contains requested association");
}
int main(){try {
  const std::vector<std::vector<size_t>> faces{{1,0,2},{2,0,3},{3,0,4},{4,0,1}};
  const std::vector<Vector3> positions{{.5,.5,0},{0,0,0},{1,0,0},{1,1,0},{0,1,0}};
  auto input=makeManifoldSurfaceMeshAndGeometry(faces,positions);auto& mesh=*std::get<0>(input);auto& geom=*std::get<1>(input);
  const auto original=fingerprint(mesh,geom);std::array<Face,4> face;for(auto f:mesh.faces())face[f.getIndex()]=f;
  IntegerCoordinatesIntrinsicTriangulation tri(mesh,geom,0.0);
  VertexData<int> labels(*tri.intrinsicMesh,-1);labels[tri.intrinsicMesh->vertex(0)]=0;
  auto find=[&](int slot){Vertex found;for(auto v:tri.intrinsicMesh->vertices())if(labels[v]==slot){require(found==Vertex(),"duplicate logical slot");found=v;}require(found!=Vertex(),"missing logical slot");return found;};
  auto first=locate(tri,face[0],Vector3{0,.5,.5});labels[tri.insertVertex(first)]=1;
  bool inserted[2]={false,true};
  const std::array<unsigned,4> which{{1,0,3,1}};
  const std::array<Vector3,4> targets{{{.125,.75,.125},{.125,.5,.375},{.125,.75,.125},{.375,.5,.125}}};
  for(unsigned event=0;event<16;++event){auto begin=std::chrono::steady_clock::now();const unsigned slot=event%2,phase=event%4;
    const auto location=locate(tri,face[which[phase]],targets[phase]);
    const size_t before=tri.intrinsicMesh->nVertices();Vertex prior=find(slot);
    Vertex fresh=tri.insertVertex(location);require(fresh!=Vertex(),"insert returned invalid vertex");
    labels[prior]=-1;labels[fresh]=slot;
    const size_t added=tri.intrinsicMesh->nVertices();require(added==before+1,"insert did not add one vertex");
    if(inserted[slot]){tri.removeInsertedVertex(prior);require(tri.intrinsicMesh->nVertices()+1==added,"old inserted vertex was not retired");}
    inserted[slot]=true;require(tri.intrinsicMesh->nVertices()<=7,"current vertex count grew");
    require(fingerprint(mesh,geom)==original,"immutable original changed");auto& cs=tri.getCommonSubdivision();cs.constructMesh();
    const auto mapped=tri.equivalentPointOnInput(SurfacePoint(find(slot))).inFace(face[which[phase]]).faceCoords;
    require(std::isfinite(mapped.x)&&std::isfinite(mapped.y)&&std::isfinite(mapped.z)&&
      std::abs(mapped.x-targets[phase].x)<1e-10&&std::abs(mapped.y-targets[phase].y)<1e-10&&
      std::abs(mapped.z-targets[phase].z)<1e-10,"post-insertion association mismatch");
    size_t intersections=0,maxnormal=0,sumnormal=0,pieces=0;
    for(const auto& p:cs.subdivisionPoints)if(p.intersectionType==CSIntersectionType::EDGE_TRANSVERSE)++intersections;
    for(auto e:tri.intrinsicMesh->edges()){const size_t n=std::abs(tri.normalCoordinates[e]);maxnormal=std::max(maxnormal,n);sumnormal+=n;}
    auto paths=tri.traceAllIntrinsicEdgesAlongInput();for(auto e:tri.intrinsicMesh->edges())if(paths[e].size()>1)pieces+=paths[e].size()-1;
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    std::cout<<"{\"event\":"<<event<<",\"input_vertices\":"<<mesh.nVertices()<<",\"input_faces\":"<<mesh.nFaces()
      <<",\"current_vertices\":"<<tri.intrinsicMesh->nVertices()<<",\"current_faces\":"<<tri.intrinsicMesh->nFaces()
      <<",\"vertex_capacity\":"<<tri.intrinsicMesh->nVerticesCapacity()<<",\"edge_capacity\":"<<tri.intrinsicMesh->nEdgesCapacity()
      <<",\"face_capacity\":"<<tri.intrinsicMesh->nFacesCapacity()<<",\"cs_vertices\":"<<cs.nVertices()<<",\"intersections\":"<<intersections<<",\"max_normal\":"<<maxnormal
      <<",\"sum_normal\":"<<sumnormal<<",\"traced_pieces\":"<<pieces<<",\"seconds\":"<<seconds<<"}"<<std::endl;
  }
  return 0;
}catch(const std::exception& e){std::cerr<<"probe failure or unsupported API: "<<e.what()<<std::endl;return 1;}}
