/*!
 * \file CNativeTraceCoverage3D.cpp
 * \brief Fresh constrained reference regions and sparse Dirichlet face correspondence.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeTraceCoverage3D.hpp"
#include "../../include/adaptation/CNativeReferenceTriangulation3D.hpp"
#include "../../include/adaptation/CNativeCurvedAtlas3D.hpp"
#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>

namespace SU2NativeGeometry {
namespace {
using namespace SU2Native3D;
using Key=std::vector<uint64_t>;
using MeshEdge=std::array<size_t,2>;
constexpr size_t Missing=SIZE_MAX;
size_t Add(size_t a,size_t b){if(b>SIZE_MAX-a)throw std::length_error("Trace coverage size overflow.");return a+b;}
size_t Mul(size_t a,size_t b){if(a&&b>SIZE_MAX/a)throw std::length_error("Trace coverage size overflow.");return a*b;}
size_t Bytes(const ExactRatio& r){return Mul(Add(r.numerator.capacity(),r.denominator.capacity()),sizeof(uint32_t));}
size_t Bytes(const ExactBary& p){return Add(Bytes(p.u),Bytes(p.v));}
size_t Bytes(const CoveragePiece& p){size_t bytes=0;for(unsigned k=0;k<3;++k)bytes=Add(bytes,Add(Bytes(p.reference[k]),Bytes(p.active[k])));return bytes;}
size_t Bytes(const std::vector<CoveragePiece>& p){size_t bytes=Mul(p.capacity(),sizeof(CoveragePiece));for(const auto& x:p)bytes=Add(bytes,Bytes(x));return bytes;}
size_t Bytes(const EdgeTrace& t){size_t bytes=Mul(t.pieces.capacity(),sizeof(TracePiece));for(const auto& p:t.pieces)bytes=Add(bytes,Add(Add(Bytes(p.first),Bytes(p.last)),Bytes(p.end)));return bytes;}
size_t Bytes(const std::map<Face,FaceCoverage>& covers){size_t bytes=Mul(covers.size(),sizeof(std::map<Face,FaceCoverage>::value_type)+4*sizeof(void*));
  for(const auto& c:covers)bytes=Add(bytes,Bytes(c.second.pieces));return bytes;}
MeshEdge Sorted(size_t a,size_t b){return a<b?MeshEdge{a,b}:MeshEdge{b,a};}
Edge CurrentEdge(Id a,Id b){return a<b?Edge{a,b}:Edge{b,a};}
Face Ordered(Face face){std::rotate(face.begin(),std::min_element(face.begin(),face.end()),face.end());return face;}
struct Ops {
  AtlasBudget& budget;size_t saved,held,extra;std::string reason;
  Ops(AtlasBudget& b,size_t bytes):budget(b),saved(b.caller_bytes),held(bytes),extra(CurvedAtlasWorkspaceBytes(b.maximum_integer_bits)){Sync();}
  ~Ops(){budget.caller_bytes=saved;}
  void Sync(size_t temporary=0){const auto bytes=Add(saved,Add(held,Add(extra,temporary)));if(bytes>budget.maximum_bytes)throw std::length_error("Trace coverage storage exhausted.");
    budget.peak_bytes=std::max(budget.peak_bytes,bytes);budget.caller_bytes=bytes;}
  void Charge(size_t bytes){Sync(bytes);held=Add(held,bytes);Sync();}
  void Work(size_t n=1){if(budget.work>budget.maximum_work||n>budget.maximum_work-budget.work)throw std::length_error("Trace coverage work exhausted.");budget.work+=n;}
  void Check(bool ok){if(!ok)throw std::invalid_argument(reason);}
  ExactRatio Integer(int64_t x){Sync();ExactRatio r;Check(ExactInteger(x,r,budget,reason));return r;}
  ExactRatio Binary(double x){Sync();ExactRatio r;Check(ExactBinary64(x,r,budget,reason));return r;}
  ExactRatio Plus(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactAdd(a,b,r,budget,reason));return r;}
  ExactRatio Sub(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactSubtract(a,b,r,budget,reason));return r;}
  ExactRatio Product(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactMultiply(a,b,r,budget,reason));return r;}
  ExactRatio Divide(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactDivide(a,b,r,budget,reason));return r;}
  int Compare(const ExactRatio& a,const ExactRatio& b){Sync();int result;Check(ExactCompare(a,b,result,budget,reason));return result;}
  bool Same(const ExactBary& a,const ExactBary& b){return !Compare(a.u,b.u)&&!Compare(a.v,b.v);}
  ExactRatio Orient(const ExactBary& a,const ExactBary& b,const ExactBary& c){return Sub(Product(Sub(b.u,a.u),Sub(c.v,a.v)),Product(Sub(b.v,a.v),Sub(c.u,a.u)));}
  bool On(const ExactBary& a,const ExactBary& b,const ExactBary& p){if(Compare(Orient(a,b,p),Integer(0)))return false;
    const bool u=Compare(a.u,b.u)!=0;const auto& first=u?a.u:a.v;const auto& last=u?b.u:b.v;const auto& x=u?p.u:p.v;
    return Compare(x,first)*Compare(x,last)<=0;}
  ExactRatio Fraction(const ExactBary& a,const ExactBary& b,const ExactBary& p){const bool u=Compare(a.u,b.u)!=0;return Divide(Sub(u?p.u:p.v,u?a.u:a.v),Sub(u?b.u:b.v,u?a.u:a.v));}
  Key ReferenceKey(const MappedFacetReference& g,Id f,const ExactBary& p){Sync();Key key;Check(g.ReferencePointKey(f,p,key,budget,reason));return key;}
  Key BindingKey(const MappedFacetReference& g,const SurfaceBinding& b){Key key;g.ForFacets(b,[&](Id f){if(!key.empty())return;ExactBary p;Sync();Check(g.BindingParameters(f,b,p,budget,reason));key=ReferenceKey(g,f,p);});
    if(key.empty())throw std::invalid_argument("Trace coverage binding has no reference support.");return key;}
  bool Parameters(const MappedFacetReference& g,Id f,const Key& key,ExactBary& p){Sync();reason.clear();const bool ok=g.ReferenceParameters(f,key,p,budget,reason);if(!ok&&!reason.empty())throw std::invalid_argument(reason);return ok;}
  double Round(const ExactRatio& r){Sync();std::array<double,2> interval;Check(ExactBounds(r,interval,budget,reason));
    const double x=interval[0]/2+interval[1]/2;if(!std::isfinite(x))throw std::invalid_argument("Unrepresentable trace boundary parameter.");return x;}
};
struct Scratch {Ops& op;size_t bytes;Scratch(Ops& o,size_t n):op(o),bytes(n){op.Sync(n);op.extra=Add(op.extra,n);}~Scratch(){op.extra-=bytes;}};
template<class T>void Reserve(std::vector<T>& v,size_t count,Ops& op){if(count<=v.capacity())return;const auto old=v.capacity(),next=std::max(count,old?Add(old,old):size_t(1));op.Sync(Mul(next,sizeof(T)));v.reserve(next);op.held=Add(op.held,Mul(v.capacity()-old,sizeof(T)));}
struct Use {size_t triangle=0,slot=0;};
struct MeshTriangle {std::array<size_t,3> vertex{};};
struct MeshEdgeRow {MeshEdge vertex{};std::vector<Use> uses;size_t owner=Missing;};
struct ProjectedEdge {size_t edge=0;ExactBary first,last;};
struct BoundarySide {size_t face=0,side=0;bool forward=true;};
ExactBary ActiveSide(size_t side,const ExactRatio& t,Ops& op){const auto zero=op.Integer(0),one=op.Integer(1);
  return side==0?ExactBary{t,zero}:side==1?ExactBary{op.Sub(one,t),t}:ExactBary{zero,op.Sub(one,t)};}
EdgeTrace Reverse(const EdgeTrace& trace,Ops& op){EdgeTrace out;out.pieces.reserve(trace.pieces.size());const auto one=op.Integer(1);
  for(size_t i=trace.pieces.size();i;--i){auto p=trace.pieces[i-1];std::swap(p.first,p.last);p.end=op.Sub(one,i==1?op.Integer(0):trace.pieces[i-2].end);out.pieces.push_back(std::move(p));}return out;}
void Dirichlet(const std::vector<std::vector<size_t>>& graph,const std::vector<size_t>& free,
    const std::vector<std::array<double,2>>& boundary,std::vector<std::array<double,2>>& value,Ops& op) {
  const size_t n=free.size();if(!n)return;
  if(n>size_t(std::numeric_limits<int>::max()))throw std::length_error("Face Dirichlet system exceeds Eigen index range.");
  size_t adjacency=0;for(size_t v:free)adjacency=Add(adjacency,graph[v].size());
  const size_t entries=Add(n,adjacency),iterations=Add(Mul(2,n),64);
  if(entries>size_t(std::numeric_limits<int>::max())||iterations>size_t(std::numeric_limits<int>::max()))
    throw std::length_error("Face Dirichlet system exceeds Eigen index range.");
  Scratch memory(op,Add(Mul(graph.size(),sizeof(size_t)),Add(Mul(entries,64),Mul(n,256))));
  std::vector<size_t> index(graph.size(),Missing);for(size_t i=0;i<n;++i)index[free[i]]=i;
  using Matrix=Eigen::SparseMatrix<double>;std::vector<Eigen::Triplet<double>> triplets;triplets.reserve(entries);
  op.Work(entries);for(size_t i=0;i<n;++i){const auto v=free[i];if(graph[v].empty())throw std::invalid_argument("Isolated face interior vertex.");
    triplets.emplace_back(int(i),int(i),double(graph[v].size()));for(size_t w:graph[v])if(index[w]!=Missing)triplets.emplace_back(int(i),int(index[w]),-1.);}
  Matrix matrix{int(n),int(n)};matrix.setFromTriplets(triplets.begin(),triplets.end());matrix.makeCompressed();
  Eigen::ConjugateGradient<Matrix,Eigen::Lower|Eigen::Upper,Eigen::DiagonalPreconditioner<double>> solver;
  solver.compute(matrix);if(solver.info()!=Eigen::Success)throw std::invalid_argument("Singular face Dirichlet proposal.");
  const size_t per_step=Add(entries,Mul(n,8));const double tolerance=64*std::numeric_limits<double>::epsilon();
  for(unsigned axis=0;axis<2;++axis){Eigen::VectorXd b=Eigen::VectorXd::Zero(n);op.Work(adjacency);
    for(size_t i=0;i<n;++i)for(size_t w:graph[free[i]])if(index[w]==Missing)b[Eigen::Index(i)]+=boundary[w][axis];
    if(!b.allFinite())throw std::invalid_argument("Nonfinite face Dirichlet proposal.");
    const size_t remaining=op.budget.maximum_work-op.budget.work;
    const size_t fixed_work=Add(Mul(2,entries),Mul(4,n));
    if(remaining<fixed_work)throw std::length_error("Face Dirichlet proposal reached its bounded work limit.");
    const size_t allowed=std::min(iterations,(remaining-fixed_work)/per_step);
    solver.setMaxIterations(int(allowed));solver.setTolerance(tolerance);
    const size_t reserved=Mul(allowed,per_step);op.Work(Add(reserved,fixed_work));
    Eigen::VectorXd x=solver.solveWithGuess(b,Eigen::VectorXd::Zero(n));
    if(solver.iterations()<0||size_t(solver.iterations())>allowed)throw std::logic_error("Eigen exceeded the admitted Dirichlet iteration limit.");
    // Eigen reports the zero-based last iteration when convergence breaks its loop.
    const size_t used=b.squaredNorm()==0?0:std::min(allowed,Add(size_t(solver.iterations()),1));
    op.budget.work-=reserved-Mul(used,per_step);
    const Eigen::VectorXd residual=matrix*x-b;
    if(solver.info()!=Eigen::Success||!x.allFinite()||!residual.allFinite()||residual.norm()>tolerance*b.norm())
      throw std::length_error("Face Dirichlet proposal failed its bounded residual check.");
    for(size_t i=0;i<n;++i)value[free[i]][axis]=x[Eigen::Index(i)];
  }
}

}  // namespace
bool RebuildTraceCoverage(const MappedFacetReference& geometry,const std::vector<CoveragePiece>& accepted_region,
    const std::vector<SurfaceFace>& candidate,const std::map<Id,SurfaceBinding>& bindings,
    const std::map<Edge,EdgeTrace>& traces,std::map<Face,FaceCoverage>& out,AtlasBudget& budget,std::string& reason) {
  std::vector<CoveragePiece> fresh;
  try {
    if(accepted_region.empty()||candidate.empty())throw std::invalid_argument("Empty prescribed-trace reference region/faces.");
    size_t borrowed=Add(Bytes(accepted_region),Add(Bytes(out),Mul(candidate.capacity(),sizeof(SurfaceFace))));
    borrowed=Add(borrowed,Mul(bindings.size(),sizeof(std::map<Id,SurfaceBinding>::value_type)+4*sizeof(void*)));
    size_t retained_bytes=Mul(traces.size(),sizeof(EdgeTrace));
    for(const auto& row:traces){borrowed=Add(borrowed,Add(sizeof(std::map<Edge,EdgeTrace>::value_type)+4*sizeof(void*),Bytes(row.second)));retained_bytes=Add(retained_bytes,Bytes(row.second));}
    const auto initial=Add(budget.caller_bytes,Add(borrowed,retained_bytes));
    if(initial>budget.maximum_bytes)throw std::length_error("Prescribed-trace input copies exceed storage admission.");budget.peak_bytes=std::max(budget.peak_bytes,initial);
    {
      struct Restore {AtlasBudget& budget;size_t caller;~Restore(){budget.caller_bytes=caller;}} restore{budget,budget.caller_bytes};
      // The nested reference builder accounts its region/binding/trace-vector inputs; retain the other live inputs here.
      const size_t external=borrowed-Bytes(accepted_region)-Mul(bindings.size(),sizeof(std::map<Id,SurfaceBinding>::value_type)+4*sizeof(void*));
      budget.caller_bytes=Add(budget.caller_bytes,external);
      std::vector<EdgeTrace> retained;retained.reserve(traces.size());for(const auto& row:traces)retained.push_back(row.second);
      if(!RebuildReferenceTriangulation(geometry,accepted_region,bindings,retained,fresh,budget,reason))return false;
    }
    Ops op(budget,Add(borrowed,Bytes(fresh)));
    std::map<Face,FaceCoverage> output;
    std::map<Key,size_t> vertex_ids;std::vector<Key> keys;
    std::vector<MeshTriangle> triangles;std::vector<MeshEdgeRow> edges;std::map<MeshEdge,size_t> edge_ids;
    std::vector<size_t> label;std::vector<std::map<size_t,ExactBary>> boundary;
    Reserve(boundary,candidate.size(),op);boundary.resize(candidate.size());
    std::map<Edge,std::vector<BoundarySide>> sides;std::map<Id,Key> current;
    std::set<Face> face_ids;
    for(size_t f=0;f<candidate.size();++f){op.Work();const auto& face=candidate[f];geometry.Reference().Get(face.facet);
      const auto ordered=Ordered(face.v);
      if(face.v[0]==face.v[1]||face.v[1]==face.v[2]||face.v[2]==face.v[0]||face_ids.count(ordered))throw std::invalid_argument("Repeated/degenerate candidate face.");
      op.Charge(sizeof(Face)+4*sizeof(void*));face_ids.insert(ordered);
      for(unsigned k=0;k<3;++k){const Id id=face.v[k];const auto found=bindings.find(id);if(found==bindings.end())throw std::invalid_argument("Candidate face lacks its current binding.");
        if(!current.count(id)){auto key=op.BindingKey(geometry,found->second);op.Charge(sizeof(decltype(current)::value_type)+4*sizeof(void*)+Mul(key.capacity(),sizeof(uint64_t)));current.emplace(id,std::move(key));}
        const Edge edge=CurrentEdge(id,face.v[(k+1)%3]);auto side=sides.find(edge);
        if(side==sides.end()){op.Charge(sizeof(decltype(sides)::value_type)+4*sizeof(void*));side=sides.emplace(edge,std::vector<BoundarySide>{}).first;}
        Reserve(side->second,Add(side->second.size(),1),op);side->second.push_back({f,k,id<face.v[(k+1)%3]});if(side->second.size()>2)throw std::invalid_argument("Nonmanifold candidate edge.");
      }
    }
    if(current.size()!=bindings.size()||sides.size()!=traces.size())throw std::invalid_argument("Prescribed traces/bindings do not match the current candidate mesh.");
    std::vector<Edge> trace_ids;Reserve(trace_ids,traces.size(),op);
    for(const auto& row:traces){op.Work();if(row.first[0]>=row.first[1]||!sides.count(row.first)||row.second.pieces.empty())throw std::invalid_argument("Unrequested or malformed prescribed trace.");
      if(op.ReferenceKey(geometry,row.second.pieces.front().facet,row.second.pieces.front().first)!=current.at(row.first[0])||
         op.ReferenceKey(geometry,row.second.pieces.back().facet,row.second.pieces.back().last)!=current.at(row.first[1]))throw std::invalid_argument("Prescribed trace endpoints differ from their current bindings.");
      trace_ids.push_back(row.first);
    }
    auto vertex=[&](Key key){const auto found=vertex_ids.find(key);if(found!=vertex_ids.end())return found->second;
      Reserve(keys,Add(keys.size(),1),op);op.Charge(sizeof(decltype(vertex_ids)::value_type)+4*sizeof(void*)+Mul(2*key.capacity(),sizeof(uint64_t)));
      const size_t id=keys.size();vertex_ids.emplace(key,id);keys.push_back(std::move(key));return id;};
    Reserve(triangles,fresh.size(),op);Reserve(label,fresh.size(),op);
    for(const auto& piece:fresh){MeshTriangle triangle;for(unsigned k=0;k<3;++k)triangle.vertex[k]=vertex(op.ReferenceKey(geometry,piece.facet,piece.reference[k]));triangles.push_back(triangle);label.push_back(Missing);}
    auto build_edges=[&]() {
      size_t retired=Add(Mul(edges.capacity(),sizeof(MeshEdgeRow)),Mul(edge_ids.size(),sizeof(decltype(edge_ids)::value_type)+4*sizeof(void*)));
      for(const auto& edge:edges)retired=Add(retired,Mul(edge.uses.capacity(),sizeof(Use)));
      std::vector<MeshEdgeRow>().swap(edges);edge_ids.clear();op.held-=retired;
      for(size_t t=0;t<triangles.size();++t)for(unsigned k=0;k<3;++k){op.Work();const auto& v=triangles[t].vertex;const auto key=Sorted(v[k],v[(k+1)%3]);auto found=edge_ids.find(key);size_t id;
        if(found==edge_ids.end()){Reserve(edges,Add(edges.size(),1),op);id=edges.size();edges.push_back({key,{},Missing});op.Charge(sizeof(decltype(edge_ids)::value_type)+4*sizeof(void*));edge_ids.emplace(key,id);}else id=found->second;
        auto& uses=edges[id].uses;Reserve(uses,Add(uses.size(),1),op);uses.push_back({t,k});if(uses.size()>2)throw std::invalid_argument("Nonmanifold fresh reference edge.");
      }
      for(const auto& edge:edges)if(edge.uses.size()==2){const auto a=edge.uses[0],b=edge.uses[1];
        if(triangles[a.triangle].vertex[a.slot]==triangles[b.triangle].vertex[b.slot])throw std::invalid_argument("Reference charts disagree on edge orientation.");}
    };
    build_edges();
    std::map<Id,std::vector<ProjectedEdge>> projected;
    for(const auto& row:traces)for(const auto& piece:row.second.pieces)if(!projected.count(piece.facet)) {
      op.Charge(sizeof(decltype(projected)::value_type)+4*sizeof(void*));auto& support=projected[piece.facet];
      for(size_t e=0;e<edges.size();++e){op.Work();const auto v=edges[e].vertex;ExactBary a,b;
        if(!op.Parameters(geometry,piece.facet,keys[v[0]],a)||!op.Parameters(geometry,piece.facet,keys[v[1]],b))continue;
        Reserve(support,Add(support.size(),1),op);op.Charge(Add(Bytes(a),Bytes(b)));support.push_back({e,std::move(a),std::move(b)});
      }
    }
    auto seed=[&](size_t triangle,size_t face){if(label[triangle]!=Missing&&label[triangle]!=face)throw std::invalid_argument("Prescribed face boundaries seed conflicting reference regions.");label[triangle]=face;};
    auto prescribe=[&](size_t face,size_t v,ExactBary position){auto& values=boundary[face];const auto old=values.find(v);
      if(old!=values.end()){if(!op.Same(old->second,position))throw std::invalid_argument("Prescribed edge parameters disagree at a face-boundary vertex.");return;}
      op.Charge(sizeof(std::map<size_t,ExactBary>::value_type)+4*sizeof(void*)+Bytes(position));values.emplace(v,std::move(position));};
    for(size_t tr=0;tr<trace_ids.size();++tr){const auto& edge=trace_ids[tr];const auto& trace=traces.at(edge);auto begin=op.Integer(0);
      for(const auto& piece:trace.pieces){bool represented=false,inside=false;
        for(const auto& segment:projected.at(piece.facet)){op.Work();if(!op.On(piece.first,piece.last,segment.first)||!op.On(piece.first,piece.last,segment.last))continue;
          auto& row=edges[segment.edge];if(row.owner!=Missing&&row.owner!=tr)throw std::invalid_argument("Distinct prescribed current edges overlap on a reference path.");row.owner=tr;represented=true;
          const auto a=op.Fraction(piece.first,piece.last,segment.first),b=op.Fraction(piece.first,piece.last,segment.last);
          if(!op.Compare(a,b))throw std::invalid_argument("Collapsed prescribed reference subedge.");
          const auto ta=op.Plus(begin,op.Product(op.Sub(piece.end,begin),a)),tb=op.Plus(begin,op.Product(op.Sub(piece.end,begin),b));
          for(const auto& side:sides.at(edge)) {
            const auto first=side.forward?ta:op.Sub(op.Integer(1),ta),last=side.forward?tb:op.Sub(op.Integer(1),tb);
            prescribe(side.face,row.vertex[0],ActiveSide(side.side,first,op));prescribe(side.face,row.vertex[1],ActiveSide(side.side,last,op));
            for(const auto& use:row.uses){const bool halfedge_sorted=triangles[use.triangle].vertex[use.slot]==row.vertex[0];
              if((op.Compare(a,b)<0)==(halfedge_sorted==side.forward)){seed(use.triangle,side.face);inside=true;}}
          }
        }
        if(!represented||!inside)throw std::invalid_argument("Prescribed trace has no correctly oriented incident reference region.");begin=piece.end;
      }
    }
    {
      Scratch queue_memory(op,Mul(triangles.size(),sizeof(size_t)));std::vector<size_t> queue;queue.reserve(triangles.size());for(size_t t=0;t<label.size();++t)if(label[t]!=Missing)queue.push_back(t);
      for(size_t at=0;at<queue.size();++at){const size_t t=queue[at];for(unsigned k=0;k<3;++k){op.Work();const auto& v=triangles[t].vertex;const auto& edge=edges[edge_ids.at(Sorted(v[k],v[(k+1)%3]))];if(edge.owner!=Missing)continue;
          if(edge.uses.size()!=2)throw std::invalid_argument("Reference region perimeter lacks a prescribed candidate edge.");
          const auto other=edge.uses[0].triangle==t?edge.uses[1].triangle:edge.uses[0].triangle;
          if(label[other]==Missing){label[other]=label[t];queue.push_back(other);}else if(label[other]!=label[t])throw std::invalid_argument("Candidate reference regions overlap without a prescribed separating path.");
        }}
      if(std::find(label.begin(),label.end(),Missing)!=label.end())throw std::invalid_argument("Unlabelled accepted reference region.");
    }
    // Weakly convex simplex boundaries need free chord vertices; no CFD node or inherited interior cut is added.
    {
      Scratch chord_memory(op,Mul(edges.size(),sizeof(MeshEdge)));std::vector<MeshEdge> chords;chords.reserve(edges.size());
      for(const auto& edge:edges)if(edge.owner==Missing&&edge.uses.size()==2){op.Work();const auto face=label[edge.uses[0].triangle];if(label[edge.uses[1].triangle]!=face)throw std::invalid_argument("Unconstrained reference edge separates two candidate faces.");
        const auto a=boundary[face].find(edge.vertex[0]),b=boundary[face].find(edge.vertex[1]);if(a==boundary[face].end()||b==boundary[face].end())continue;
        bool common=false;for(unsigned k=0;k<3&&!common;++k){auto weight=[&](const ExactBary& p){return k==1?p.u:k==2?p.v:op.Sub(op.Sub(op.Integer(1),p.u),p.v);};common=!op.Compare(weight(a->second),op.Integer(0))&&!op.Compare(weight(b->second),op.Integer(0));}
        if(common)chords.push_back(edge.vertex);
      }
      for(const auto& chord:chords){size_t midpoint=Missing,uses=0;const size_t count=triangles.size();
        for(size_t t=0;t<count;++t){op.Work();const auto old=triangles[t];for(unsigned k=0;k<3;++k)if(Sorted(old.vertex[k],old.vertex[(k+1)%3])==chord){
            const unsigned next=(k+1)%3,last=(k+2)%3;const auto before=fresh[t];ExactBary middle{op.Divide(op.Plus(before.reference[k].u,before.reference[next].u),op.Integer(2)),op.Divide(op.Plus(before.reference[k].v,before.reference[next].v),op.Integer(2))};
            auto key=op.ReferenceKey(geometry,before.facet,middle);if(midpoint==Missing){if(vertex_ids.count(key))throw std::invalid_argument("Nonconforming reference chord midpoint.");midpoint=vertex(std::move(key));}
            else if(keys[midpoint]!=key)throw std::invalid_argument("Original charts disagree on a chord midpoint.");
            CoveragePiece first=before,second=before;first.reference={before.reference[k],middle,before.reference[last]};second.reference={middle,before.reference[next],before.reference[last]};
            if(op.Compare(op.Orient(first.reference[0],first.reference[1],first.reference[2]),op.Integer(0))<=0||op.Compare(op.Orient(second.reference[0],second.reference[1],second.reference[2]),op.Integer(0))<=0)throw std::invalid_argument("Fresh chord preparation reverses a reference triangle.");
            Reserve(fresh,Add(fresh.size(),1),op);Reserve(triangles,Add(triangles.size(),1),op);Reserve(label,Add(label.size(),1),op);op.Charge(Add(Bytes(first),Bytes(second)));op.held-=Bytes(fresh[t]);
            fresh[t]=std::move(first);fresh.push_back(std::move(second));triangles[t]={{old.vertex[k],midpoint,old.vertex[last]}};triangles.push_back({{midpoint,old.vertex[next],old.vertex[last]}});label.push_back(label[t]);++uses;break;
          }}
        if(uses!=2)throw std::invalid_argument("Fresh reference chord lacks both incident triangles.");
      }
      if(!chords.empty()) {
        std::map<MeshEdge,size_t> owners;for(const auto& edge:edges)if(edge.owner!=Missing){op.Charge(sizeof(decltype(owners)::value_type)+4*sizeof(void*));owners.emplace(edge.vertex,edge.owner);}
        build_edges();for(auto& edge:edges){const auto old=owners.find(edge.vertex);if(old!=owners.end())edge.owner=old->second;}
        const auto owner_bytes=Mul(owners.size(),sizeof(decltype(owners)::value_type)+4*sizeof(void*));owners.clear();op.held-=owner_bytes;
      }
    }
    // Projection tables are no longer needed once the fixed-boundary reference topology is prepared.
    for(const auto& chart:projected){op.held-=sizeof(decltype(projected)::value_type)+4*sizeof(void*)+Mul(chart.second.capacity(),sizeof(ProjectedEdge));for(const auto& edge:chart.second)op.held-=Add(Bytes(edge.first),Bytes(edge.last));}projected.clear();
    std::vector<std::vector<size_t>> face_triangles;Reserve(face_triangles,candidate.size(),op);face_triangles.resize(candidate.size());
    for(size_t t=0;t<label.size();++t){auto& face=face_triangles.at(label[t]);Reserve(face,Add(face.size(),1),op);face.push_back(t);}
    for(size_t f=0;f<candidate.size();++f){const auto& selected=face_triangles[f];if(selected.empty())throw std::invalid_argument("Candidate face has no reference region.");
      Scratch vertex_memory(op,Mul(Mul(3,selected.size()),sizeof(size_t)));std::vector<size_t> vertices;vertices.reserve(3*selected.size());
      for(size_t t:selected)for(size_t v:triangles[t].vertex)vertices.push_back(v);std::sort(vertices.begin(),vertices.end());vertices.erase(std::unique(vertices.begin(),vertices.end()),vertices.end());
      size_t boundary_bytes=0;for(const auto& p:boundary[f])boundary_bytes=Add(boundary_bytes,Bytes(p.second));
      Scratch solve_memory(op,Add(Add(Mul(vertices.size(),1024),Mul(selected.size(),256)),Mul(boundary_bytes,2)));
      auto local=[&](size_t v){const auto found=std::lower_bound(vertices.begin(),vertices.end(),v);if(found==vertices.end()||*found!=v)throw std::logic_error("Face graph omits its own reference vertex.");return size_t(found-vertices.begin());};
      std::vector<std::vector<size_t>> graph(vertices.size()),border(vertices.size());size_t edge_count=0;
      for(const auto& edge:edges){op.Work();size_t uses=0;for(const auto& use:edge.uses)uses+=label[use.triangle]==f;if(!uses)continue;++edge_count;const auto a=local(edge.vertex[0]),b=local(edge.vertex[1]);graph[a].push_back(b);graph[b].push_back(a);
        if(uses==1){if(edge.owner==Missing)throw std::invalid_argument("Face disk boundary lacks a prescribed trace.");border[a].push_back(b);border[b].push_back(a);}
        else if(edge.owner!=Missing)throw std::invalid_argument("Prescribed face boundary runs through its own interior.");
      }
      if(Add(vertices.size(),selected.size())!=Add(edge_count,1))throw std::invalid_argument("Candidate reference region is not a disk.");
      std::vector<unsigned char> visited(vertices.size());std::vector<size_t> queue{0};visited[0]=1;
      for(size_t at=0;at<queue.size();++at)for(size_t w:graph[queue[at]]){op.Work();if(!visited[w]){visited[w]=1;queue.push_back(w);}}
      if(queue.size()!=vertices.size())throw std::invalid_argument("Candidate reference region is disconnected.");
      size_t boundary_count=0,start=Missing;for(size_t v=0;v<vertices.size();++v){if(border[v].empty()){if(boundary[f].count(vertices[v]))throw std::invalid_argument("Prescribed face-boundary vertex became interior.");continue;}
        if(border[v].size()!=2||!boundary[f].count(vertices[v]))throw std::invalid_argument("Face boundary is not one prescribed simple cycle.");++boundary_count;start=v;}
      if(start==Missing)throw std::invalid_argument("Candidate reference disk has no boundary.");std::fill(visited.begin(),visited.end(),0);queue={start};visited[start]=1;
      for(size_t at=0;at<queue.size();++at)for(size_t w:border[queue[at]]){op.Work();if(!visited[w]){visited[w]=1;queue.push_back(w);}}
      if(queue.size()!=boundary_count)throw std::invalid_argument("Candidate reference region has holes/multiple boundaries.");
      std::vector<ExactBary> active(vertices.size());std::vector<std::array<double,2>> positions(vertices.size()),fixed(vertices.size());std::vector<size_t> free;
      for(size_t v=0;v<vertices.size();++v){const auto prescribed=boundary[f].find(vertices[v]);if(prescribed==boundary[f].end())free.push_back(v);
        else{active[v]=prescribed->second;fixed[v]={op.Round(active[v].u),op.Round(active[v].v)};positions[v]=fixed[v];}}
      Dirichlet(graph,free,fixed,positions,op);
      for(size_t v:free)active[v]={op.Binary(positions[v][0]),op.Binary(positions[v][1])};
      FaceCoverage cover;op.Charge(Mul(selected.size(),sizeof(CoveragePiece)));cover.pieces.reserve(selected.size());
      for(size_t t:selected){CoveragePiece piece;piece.facet=fresh[t].facet;piece.reference=fresh[t].reference;for(unsigned k=0;k<3;++k)piece.active[k]=active[local(triangles[t].vertex[k])];
        if(op.Compare(op.Orient(piece.active[0],piece.active[1],piece.active[2]),op.Integer(0))<=0)throw std::invalid_argument("Dyadic face Dirichlet map is not strictly positive.");op.Charge(Bytes(piece));cover.pieces.push_back(std::move(piece));}
      const auto& face=candidate[f];size_t trace_copy_bytes=0;
      for(unsigned k=0;k<3;++k)trace_copy_bytes=Add(trace_copy_bytes,Bytes(traces.at(CurrentEdge(face.v[k],face.v[(k+1)%3]))));
      Scratch trace_memory(op,Mul(trace_copy_bytes,2));
      std::array<SurfaceBinding,3> binding;std::array<Point,3> realized;std::array<EdgeTrace,3> prescribed;
      for(unsigned k=0;k<3;++k){binding[k]=bindings.at(face.v[k]);realized[k]=geometry.Realize(binding[k]);const auto& trace=traces.at(CurrentEdge(face.v[k],face.v[(k+1)%3]));prescribed[k]=face.v[k]<face.v[(k+1)%3]?trace:Reverse(trace,op);}
      op.Sync();op.Check(geometry.ValidateCoverage(face.facet,binding,realized,cover,prescribed,budget,op.reason));
      const size_t rotation=std::min_element(face.v.begin(),face.v.end())-face.v.begin();
      if(rotation)for(auto& piece:cover.pieces)for(auto& p:piece.active){const std::array<ExactRatio,3> weights{op.Sub(op.Sub(op.Integer(1),p.u),p.v),p.u,p.v};
        const size_t next=Add(Bytes(weights[(rotation+1)%3]),Bytes(weights[(rotation+2)%3])),old=Bytes(p);op.Charge(next);p.u=weights[(rotation+1)%3];p.v=weights[(rotation+2)%3];op.held-=old;}
      op.Charge(sizeof(decltype(output)::value_type)+4*sizeof(void*));output.emplace(Ordered(face.v),std::move(cover));
    }
    {
      size_t copies=Add(Bytes(accepted_region),Bytes(output));Scratch proof_memory(op,Add(copies,Mul(Add(output.size(),1),sizeof(FaceCoverage))));
      std::vector<FaceCoverage> old(1),next;old.front().pieces=accepted_region;next.reserve(output.size());for(const auto& cover:output)next.push_back(cover.second);
      op.Sync();op.Check(geometry.ValidateCoverageReplacement(old,next,budget,op.reason));
    }
    out=std::move(output);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
}  // namespace SU2NativeGeometry
