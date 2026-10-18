Continue a focused read-only construction design review. Use exactly Opus5.5 at medium effort. Tools are disabled; do not edit/run anything. We followed up your earlier review: native coupled surface/interior placement now passes the exact old50-cell frozen patch in12trials/4linearizations, including full native query/integral gates and an independent exact geometry/shape audit. The whole annular cold run still rejects under UNCHANGED constraints, but now after96edits with only4 oversized edges, no bad-shape cells. No need for another point-placement fallback.

The4 remaining edges join the floor corners0,1,8,9 to ONE first-height apex50328=(.5,.5,.1), shared by8 wall triangles. Tangential sensor is constant theta=5.047122..., h0=.1, growth1.2, thickness.3/fadeunchanged. Full lengths1.865929>1.8. An exact-rational analytic-profile lower bound1.834077 proves that a shared height apex cannot satisfy all four corners; this is a retained-topology conflict, not whole-target infeasibility. Independent per-face height-centroid seeds on the EXISTING8wall triangles have q>=.887185/J>=.714051/fullL<=1.588572. They are not a complete volume replacement; transition closure is missing.

A complete36-cell star of apex50328 fits the64-cell old-cavity bound. Its maximum vertexZ=.18827. Floating geometry LP: any common background seeing its retained skin has Z<=.154541; the existing centroid first-layer constructor needs Z>=.300000 to put all8 normal points inside its background-wall cones. The joint LP is infeasible. This is a floating diagnostic, not a formal full-target infeasibility proof.

Native currently routes ReconstructSplitPatch only for PHYSICAL split edges; these four offending spokes are INTERIOR. ConstructFirstLayer makes one background cone and carves individual normal points. ReconstructWallSplit performs sequential protected-child normal cavity insertions after a surface split. Repair protects existing height apices. We must escape the shared fan, preserving original boundary geometry, exact first height, full field/quality, positive/conforming volume, original sensor-only P1 and frozen target, full-star MPI reservation and atomic rejection. No volume gather or testcase/solver mode.

Recommend the smallest COMPLETE connected construction strategy to test next on the saved state: can existing constructors be reused with proper complete-star retirement/growth, or is a multi-apex/front reconstruction actually required? Explain why, particularly transition closure without long floor-corner/background spokes. Do not propose a fallback ladder or larger caps. Give one prioritized plan with a falsifiable exact-replay test; distinguish what is proved, plausible and unknown. At most800words. Do not assume centroid seeds or a larger cavity alone will pass metric/shape gates. The full general symmetry/2D/exterior goal remains open, not reduced to this annulus.

Relevant exact star data:
{"nodes": [[0, 0, 0, 0], [1, 1, 0, 0], [8, 0, 1, 0], [9, 1, 1, 0], [4215, 0.3429100556180847, 0.22342031127978496, 0.18826990204511956], [13647, 0.5, 0.5, 0], [18887, 0.8301526035208452, 1, 0.16327566036184843], [50327, 0.5, 0, 0], [50328, 0.5, 0.5, 0.1], [52423, 1, 0.5, 0], [54519, 0, 0.5, 0], [54520, 0.3373390230862565, 0.6117290249485455, 0.14835462013005468], [61855, 0.833042212118521, 0.6718032322808899, 0.06399321375806502], [62903, 0.32609637944743974, 0.16487928571266056, 0.06398452750825406], [66047, 0.5865530995492172, 0.6117448531928303, 0.1757716644616524], [77575, 0.1626082953456108, 0.1059461376753968, 0.08927777804958444], [113207, 0.6764757928740303, 0.35982114755807326, 0.11160683285860468], [124735, 0.653483035974493, 0.8307766258938449, 0.07405947819649218], [134167, 0.18968694326507912, 0.332271576182, 0.07758320437954253], [158271, 0.24507015062124393, 0.7907992082896645, 0.0686087472865957], [165607, 0.5310129744105221, 1, 0]], "cells": [{"id": 53415, "nodes": [50328, 0, 54519, 13647]}, {"id": 53416, "nodes": [50328, 54519, 8, 13647]}, {"id": 53419, "nodes": [50328, 0, 13647, 50327]}, {"id": 53421, "nodes": [50328, 1, 50327, 13647]}, {"id": 53422, "nodes": [50328, 1, 13647, 52423]}, {"id": 53429, "nodes": [50328, 9, 52423, 13647]}, {"id": 60577, "nodes": [61855, 9, 50328, 18887]}, {"id": 60578, "nodes": [61855, 9, 52423, 50328]}, {"id": 61602, "nodes": [62903, 0, 50328, 50327]}, {"id": 64672, "nodes": [66047, 4215, 54520, 50328]}, {"id": 64677, "nodes": [66047, 18887, 50328, 54520]}, {"id": 75941, "nodes": [77575, 0, 50328, 62903]}, {"id": 75949, "nodes": [77575, 4215, 62903, 50328]}, {"id": 110753, "nodes": [50328, 1, 113207, 50327]}, {"id": 110754, "nodes": [50328, 1, 52423, 113207]}, {"id": 110759, "nodes": [61855, 18887, 50328, 113207]}, {"id": 110760, "nodes": [61855, 50328, 52423, 113207]}, {"id": 110762, "nodes": [62903, 4215, 113207, 50328]}, {"id": 110764, "nodes": [62903, 50327, 50328, 113207]}, {"id": 110765, "nodes": [66047, 4215, 50328, 113207]}, {"id": 110767, "nodes": [66047, 18887, 113207, 50328]}, {"id": 122016, "nodes": [124735, 9, 18887, 50328]}, {"id": 122020, "nodes": [124735, 18887, 54520, 50328]}, {"id": 131231, "nodes": [134167, 0, 54519, 50328]}, {"id": 131233, "nodes": [134167, 4215, 50328, 54520]}, {"id": 131235, "nodes": [134167, 50328, 54519, 54520]}, {"id": 131237, "nodes": [77575, 0, 134167, 50328]}, {"id": 131239, "nodes": [77575, 4215, 50328, 134167]}, {"id": 154787, "nodes": [158271, 8, 50328, 54519]}, {"id": 154793, "nodes": [158271, 50328, 54520, 54519]}, {"id": 161951, "nodes": [50328, 8, 165607, 13647]}, {"id": 161952, "nodes": [50328, 9, 13647, 165607]}, {"id": 161954, "nodes": [124735, 9, 50328, 165607]}, {"id": 161956, "nodes": [124735, 50328, 54520, 165607]}, {"id": 161958, "nodes": [158271, 8, 165607, 50328]}, {"id": 161960, "nodes": [158271, 50328, 165607, 54520]}], "surface": [{"facet": 1, "nodes": [0, 54519, 13647]}, {"facet": 1, "nodes": [54519, 8, 13647]}, {"facet": 1, "nodes": [0, 13647, 50327]}, {"facet": 1, "nodes": [1, 50327, 13647]}, {"facet": 1, "nodes": [1, 13647, 52423]}, {"facet": 1, "nodes": [9, 52423, 13647]}, {"facet": 1, "nodes": [8, 165607, 13647]}, {"facet": 1, "nodes": [9, 13647, 165607]}]}

Existing implementation:
bool ConstructFirstLayer(const Patch& old,Node background,Id first_wall_node,Id first_cell,
                         const std::map<int,double>& heights,const PlanarReference& reference,
                         Patch& fresh,FirstHeightReport& report,std::string& reason,KernelStats* stats) {
  report={};
  try {
    Bounds(old);
    if(heights.empty())throw std::invalid_argument("Native3D first-layer construction needs wall heights.");
    for(const auto& height:heights)if(!(height.second>0) || !std::isfinite(height.second))
      throw std::invalid_argument("Native3D wall height must be finite and positive.");
    const auto skin=SkinMap(Incidence(old.cells,stats).Boundary());
    const auto physical=CheckSurface(old,skin,reference,stats);const auto nodes=Nodes(old.cells);
    if(nodes.count(background.id))throw std::invalid_argument("Native3D background identity is not newly reserved.");
    size_t count=0;for(const auto& face:physical)count+=heights.count(reference.Get(face.second.facet).marker);
    if(!count || count>MaximumReplacementCells || first_wall_node>UINT64_MAX-(count-1))
      throw std::invalid_argument("Native3D first-layer node range/coverage exceeds budget.");
    for(size_t k=0;k<count;++k)if(nodes.count(first_wall_node+k) || background.id==first_wall_node+k)
      throw std::invalid_argument("Native3D wall apex identity is not newly reserved.");
    Patch proposal;
    if(!Cone(old.cells,background,first_cell,proposal.cells,reason,stats))throw std::invalid_argument(reason);
    Id next=first_wall_node;
    for(const auto& entry:physical) {
      const auto& face=entry.second;const auto height=heights.find(reference.Get(face.facet).marker);
      if(height==heights.end())continue;
      const auto a=nodes.at(face.v[0]).p,b=nodes.at(face.v[1]).p,c=nodes.at(face.v[2]).p;
      auto normal=Cross(Delta(b,a),Delta(c,a));const auto length=Norm(normal);
      if(!(length>0))throw std::invalid_argument("Native3D first-layer face is degenerate.");
      for(auto& value:normal)value/=length;
      Point center{double(((long double)a.x+b.x+c.x)/3),double(((long double)a.y+b.y+c.y)/3),
                   double(((long double)a.z+b.z+c.z)/3)};
      // The oriented physical skin points out of the cavity; use its inward normal.
      Node apex{next++,{double((long double)center.x-height->second*normal[0]),
                       double((long double)center.y-height->second*normal[1]),
                       double((long double)center.z-height->second*normal[2])}};
      auto found=proposal.cells.end();
      for(auto cell=proposal.cells.begin();cell!=proposal.cells.end();++cell) {
        unsigned matches=0;for(auto node:cell->v)matches+=std::find(face.v.begin(),face.v.end(),node.id)!=face.v.end();
        if(matches==3){found=cell;break;}
      }
      if(found==proposal.cells.end())throw std::invalid_argument("Native3D background fan misses wall coverage.");
      const auto opposite=std::find_if(found->v.begin(),found->v.end(),[&](Node node){return std::find(face.v.begin(),face.v.end(),node.id)==face.v.end();});
      long double admittedClearance=0;
      for(const auto& node:nodes)admittedClearance=std::max(admittedClearance,-Dot(Delta(node.second.p,center),normal));
      if(!(admittedClearance>height->second))
        throw std::invalid_argument("Native3D geometry-height incompatibility within admitted volume at facet "+std::to_string(face.facet));
      const auto clearance=-Dot(Delta(opposite->p,center),normal);
      if(!(clearance>height->second))throw std::invalid_argument("Native3D background obstruction at facet "+std::to_string(face.facet));
      std::vector<Cell> children;
      if(!Cone({*found},apex,first_cell,children,reason,stats))
        throw std::invalid_argument("Native3D normal wall child obstructed at facet "+std::to_string(face.facet)+": "+reason);
      const auto index=found-proposal.cells.begin();proposal.cells.erase(proposal.cells.begin()+index);
      proposal.cells.insert(proposal.cells.end(),children.begin(),children.end());
      if(proposal.cells.size()>MaximumReplacementCells)throw std::invalid_argument("Native3D first layer exceeds replacement budget.");
      if(first_cell>UINT64_MAX-(proposal.cells.size()-1))throw std::invalid_argument("Native3D first-layer cell range overflows.");
      for(size_t k=0;k<proposal.cells.size();++k)proposal.cells[k].id=first_cell+k;
    }
    Finish(old,proposal,skin,physical,reference,stats);
    if(!CheckFirstHeight(proposal,heights,reference,report,reason,stats))throw std::invalid_argument(reason);
    fresh=std::move(proposal);reason.clear();return true;
  }catch(const std::exception& error){reason=error.what();return false;}
}


Existing implementation:
void RestorePatchPoints(Patch& proposal,const std::map<Id,Node>& wanted,Id first,std::string& reason,KernelStats* stats) {
  for(const auto& row:wanted) {
    const auto current=Nodes(proposal.cells);if(current.count(row.first))continue;
    const auto coincident=std::find_if(current.begin(),current.end(),[&](const auto& n) {
      return n.second.p.x==row.second.p.x && n.second.p.y==row.second.p.y && n.second.p.z==row.second.p.z;
    });
    if(coincident!=current.end()) {
      for(auto& c:proposal.cells)for(auto& n:c.v)if(n.id==coincident->first)n=row.second;
      for(auto& f:proposal.surface)for(auto& id:f.v)if(id==coincident->first)id=row.first;
      continue;
    }
    std::vector<Cell> region;
    for(const auto& c:proposal.cells) {
      const auto volume=Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p,stats);bool contains=true;
      for(unsigned k=0;k<4;++k) {
        auto v=c.v;v[k]=row.second;
        contains=contains && Orientation(v[0].p,v[1].p,v[2].p,v[3].p,stats)>=-64*std::numeric_limits<double>::epsilon()*volume;
      }
      if(contains)region.push_back(c);
    }
    if(region.empty())throw std::invalid_argument("Native3D wall reconstruction lost interior point "+std::to_string(row.first));
    std::vector<Cell> children;if(!Cone(region,row.second,first,children,reason,stats))throw std::invalid_argument(reason);
    proposal.cells.erase(std::remove_if(proposal.cells.begin(),proposal.cells.end(),[&](const auto& c) {
      return std::any_of(region.begin(),region.end(),[&](const auto& r){return r.id==c.id;});
    }),proposal.cells.end());
    proposal.cells.insert(proposal.cells.end(),children.begin(),children.end());
    if(proposal.cells.size()>MaximumReplacementCells || first>UINT64_MAX-(proposal.cells.size()-1))
      throw std::invalid_argument("Native3D wall point restoration exceeds replacement/identity budget.");
    for(size_t k=0;k<proposal.cells.size();++k)proposal.cells[k].id=first+k;
  }
}

Existing implementation:
bool ReconstructWallSplit(const Patch& old,Edge edge,Node point,Id first_wall_node,Id first,
                          const std::map<int,double>& heights,const PlanarReference& reference,
                          Patch& fresh,Face& obstruction,std::string& reason,KernelStats* stats,double surface_tolerance) {
  obstruction={};
  try {
    Bounds(old);Endpoints(edge);
    if(!std::isfinite(surface_tolerance) || surface_tolerance<0)
      throw std::invalid_argument("Native3D coupled wall split has an invalid surface tolerance.");
    FirstHeightReport height;
    if(heights.empty() || !CheckFirstHeight(old,heights,reference,height,reason,stats))
      throw std::invalid_argument("Native3D retained wall split needs an admissible prescribed-height input: "+reason);
    Patch proposal;
    if(!SplitSurfaceStrip(old,edge,point,{},first,reference,proposal,reason,stats))throw std::invalid_argument(reason);
    size_t perimeter_points=0;
    if(surface_tolerance>0) {
      const auto boundary=SkinMap(Incidence(old.cells,stats).Boundary());
      const auto marked=CheckSurface(old,boundary,reference,stats);const auto original_nodes=Nodes(old.cells);
      std::map<Edge,double> perimeter;
      for(const auto& f:old.surface)if(Has(f.v,edge[0]) && Has(f.v,edge[1]) &&
          heights.count(reference.Get(f.facet).marker) && reference.Analytic().count(f.facet)) {
        const auto tolerance=std::min(surface_tolerance,.5*heights.at(reference.Get(f.facet).marker));
        for(unsigned i=0;i<3;++i) {
          Edge other{f.v[i],f.v[(i+1)%3]};std::sort(other.begin(),other.end());
          auto selected=edge;std::sort(selected.begin(),selected.end());if(other==selected)continue;
          const auto found=perimeter.emplace(other,tolerance);if(!found.second)found.first->second=std::min(found.first->second,tolerance);
        }
      }
      for(const auto& row:perimeter) {
        if(first_wall_node>UINT64_MAX-perimeter_points)throw std::invalid_argument("Native3D perimeter split identity overflows.");
        const auto a=original_nodes.at(row.first[0]).p,b=original_nodes.at(row.first[1]).p;
        Node mid{first_wall_node+perimeter_points,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
        const auto projected=ProjectSplitPoint(row.first,mid,original_nodes,marked,reference,stats);
        if(Norm(Delta(projected.p,mid.p))<=row.second)continue;
        for(const auto& f:boundary)if(Has(f.second,row.first[0]) && Has(f.second,row.first[1]) && !marked.count(f.first)) {
          obstruction=f.second;throw std::invalid_argument("Native3D coupled perimeter split needs its complete physical edge star.");
        }
        Patch next;
        if(!SplitSurfaceStrip(proposal,row.first,mid,{},first,reference,next,reason,stats))throw std::invalid_argument(reason);
        proposal=std::move(next);++perimeter_points;
      }
    }
    if(first_wall_node>UINT64_MAX-perimeter_points)throw std::invalid_argument("Native3D wall identities overflow after perimeter splits.");
    first_wall_node+=perimeter_points;
    const auto skin=SkinMap(Incidence(proposal.cells,stats).Boundary());
    const auto physical=CheckSurface(proposal,skin,reference,stats);
    if(std::none_of(old.surface.begin(),old.surface.end(),[&](const auto& f) {
      return heights.count(reference.Get(f.facet).marker) && Has(f.v,edge[0]) && Has(f.v,edge[1]);
    }))throw std::invalid_argument("Native3D retained wall split needs a requested physical wall edge.");
    std::vector<SurfaceFace> walls;
    for(const auto& row:physical)if(heights.count(reference.Get(row.second.facet).marker))walls.push_back(row.second);
    if(walls.empty() || walls.size()+perimeter_points>MaximumReplacementCells || first_wall_node>UINT64_MAX-(walls.size()-1))
      throw std::invalid_argument("Native3D retained wall split exceeds reserved wall identities.");
    auto nodes=Nodes(proposal.cells);
    for(size_t i=0;i<walls.size();++i)if(nodes.count(first_wall_node+i))
      throw std::invalid_argument("Native3D retained wall split reuses a node identity.");
    std::set<Face> protected_faces;
    for(size_t at=0;at<walls.size();++at) {
      const auto face=walls[at];nodes=Nodes(proposal.cells);
      const auto a=nodes.at(face.v[0]).p,b=nodes.at(face.v[1]).p,c=nodes.at(face.v[2]).p;
      auto normal=Cross(Delta(b,a),Delta(c,a));const auto area=Norm(normal);
      if(!(area>0))throw std::invalid_argument("Native3D retained wall split has a degenerate face.");
      for(auto& value:normal)value/=area;
      const auto h=heights.at(reference.Get(face.facet).marker);
      const Point center{double(((long double)a.x+b.x+c.x)/3),double(((long double)a.y+b.y+c.y)/3),double(((long double)a.z+b.z+c.z)/3)};
      Node apex{first_wall_node+at,{double((long double)center.x-h*normal[0]),double((long double)center.y-h*normal[1]),double((long double)center.z-h*normal[2])}};
      const auto coincident=std::find_if(nodes.begin(),nodes.end(),[&](const auto& row) {
        const auto p=row.second.p;return p.x==apex.p.x && p.y==apex.p.y && p.z==apex.p.z;
      });
      if(coincident!=nodes.end())apex=coincident->second;
      const Incidence global(proposal.cells,stats);
      const auto owners=global.FaceCells(face.v);
      if(owners.size()!=1)throw std::invalid_argument("Native3D retained wall split loses a physical face owner.");
      auto cell=[&](Id id)->const Cell& {
        const auto found=std::find_if(proposal.cells.begin(),proposal.cells.end(),[&](const auto& c){return c.id==id;});
        if(found==proposal.cells.end())throw std::invalid_argument("Native3D retained wall split loses a cavity dependency.");
        return *found;
      };
      bool already=false;
      for(auto n:cell(owners.front()).v)already=already || n.id==apex.id;
      if(already){protected_faces.insert(Key(face.v));continue;}
      const auto seed_faces=Incidence({cell(owners.front())},stats).Boundary();
      if(std::any_of(seed_faces.begin(),seed_faces.end(),[&](Face f){return protected_faces.count(Key(f));}))
        throw std::invalid_argument("Native3D normal wall seed also owns a completed wall child.");
      std::set<Id> region_ids{owners.front()};std::vector<Cell> region;
      for(;;) {
        region.clear();for(Id id:region_ids)region.push_back(cell(id));
        const auto boundary=Incidence(region,stats).Boundary();Id neighbor=UINT64_MAX;
        for(const auto f:boundary) {
          if(Has(f,apex.id))continue;
          if(Orientation(apex.p,nodes.at(f[0]).p,nodes.at(f[1]).p,nodes.at(f[2]).p,stats)>0)continue;
          const auto adjacent=global.FaceCells(f);
          const auto outside=std::find_if(adjacent.begin(),adjacent.end(),[&](Id id){return !region_ids.count(id);});
          if(outside==adjacent.end()) {
            if(!physical.count(Key(f)))obstruction=f;
            throw std::invalid_argument("Native3D normal wall point is obstructed by retained patch interface.");
          }
          const auto& candidate=cell(*outside);const auto faces=Incidence({candidate},stats).Boundary();
          if(std::any_of(faces.begin(),faces.end(),[&](Face f){return protected_faces.count(Key(f));}))
            throw std::invalid_argument("Native3D normal wall insertion is blocked by a completed wall child.");
          neighbor=*outside;break;
        }
        if(neighbor==UINT64_MAX)break;
        if(region_ids.size()==MaximumCavityCells)
          throw std::invalid_argument("Native3D normal wall visibility cavity exceeds its admitted cell budget.");
        region_ids.insert(neighbor);
      }
      std::vector<Cell> children;
      if(!Cone(region,apex,first,children,reason,stats))throw std::invalid_argument(reason);
      proposal.cells.erase(std::remove_if(proposal.cells.begin(),proposal.cells.end(),[&](const auto& c) {
        return region_ids.count(c.id);
      }),proposal.cells.end());
      proposal.cells.insert(proposal.cells.end(),children.begin(),children.end());
      if(proposal.cells.size()>MaximumReplacementCells || first>UINT64_MAX-(proposal.cells.size()-1))
        throw std::invalid_argument("Native3D retained wall split exceeds replacement cells/identities.");
      for(size_t i=0;i<proposal.cells.size();++i)proposal.cells[i].id=first+i;
      protected_faces.insert(Key(face.v));
    }
    proposal.surface.clear();Finish(old,proposal,skin,physical,reference,stats);
    if(!CheckFirstHeight(proposal,heights,reference,height,reason,stats))throw std::invalid_argument(reason);
    fresh=std::move(proposal);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}

bool ReconstructSplitPatch(const Patch& old,Edge edge,Node point,Node interior,const Tensor& placement,
                           Id first,const PlanarReference& reference,Patch& fresh,Face& obstruction,
                           std::string& reason,KernelStats* stats,const std::map<int,double>& heights) {
  obstruction={};Face wallObstruction{};
  try {
    Bounds(old);Endpoints(edge);ValidateTensor(placement,stats);
    const auto index=Incidence(old.cells,stats);
    auto skin=SkinMap(index.Boundary());auto physical=CheckSurface(old,skin,reference,stats);
    auto nodes=Nodes(old.cells);
    if(std::none_of(physical.begin(),physical.end(),[&](const auto& f){return Has(f.second.v,edge[0]) && Has(f.second.v,edge[1]);}))
      throw std::invalid_argument("Native3D joint boundary split requires a physical edge.");
    if(nodes.count(interior.id) || interior.id==point.id)throw std::invalid_argument("Native3D joint insertion reuses a node identity.");
    point=ProjectSplitPoint(edge,point,nodes,physical,reference,stats,true);
    SplitSkin(edge,point,skin,physical);nodes.emplace(point.id,point);
    long double wallViolation=-1;std::map<Face,Point> wallPoints;
    for(const auto& entry:physical) {
      const auto height=heights.find(reference.Get(entry.second.facet).marker);if(height==heights.end())continue;
      const auto f=entry.second.v;const auto a=nodes.at(f[0]).p,b=nodes.at(f[1]).p,c=nodes.at(f[2]).p;
      auto normal=Cross(Delta(b,a),Delta(c,a));const auto area=Norm(normal);for(auto& n:normal)n/=area;
      const Point center{double(((long double)a.x+b.x+c.x)/3),double(((long double)a.y+b.y+c.y)/3),double(((long double)a.z+b.z+c.z)/3)};
      const Point apex{double((long double)center.x-height->second*normal[0]),
                       double((long double)center.y-height->second*normal[1]),double((long double)center.z-height->second*normal[2])};
      wallPoints.emplace(entry.first,apex);
      for(const auto& row:skin)if(!physical.count(row.first)) {
        const auto face=row.second;const auto x=nodes.at(face[0]).p,y=nodes.at(face[1]).p,z=nodes.at(face[2]).p;
        const auto orient=Orientation(apex,x,y,z,stats);
        if(orient>0)continue;
        const auto violation=-orient/Norm(Cross(Delta(y,x),Delta(z,x)));
        if(violation>wallViolation){wallViolation=violation;wallObstruction=row.first;}
      }
    }
    if(skin.size()>MaximumReplacementCells)throw std::invalid_argument("Native3D joint skin exceeds replacement budget.");
    const auto origin=old.cells.front().v.front().p;
    Vector moment{};long double volume=0,rms=0;
    for(const auto& c:old.cells) {
      const auto det=Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p,stats);
      if(!(det>0))throw std::invalid_argument("Native3D joint patch contains an inverted donor.");
      for(const auto n:c.v){const auto d=Delta(n.p,origin);for(size_t k=0;k<3;++k)moment[k]+=det*d[k]/4;}
      volume+=det;rms+=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},placement,stats).rms_edge;
    }
    interior.p={double(origin.x+moment[0]/volume),double(origin.y+moment[1]/volume),double(origin.z+moment[2]/volume)};
    rms/=old.cells.size();
    const long double scale=std::max({std::abs(placement.xx),std::abs(placement.xy),std::abs(placement.xz),std::abs(placement.yy),std::abs(placement.yz),std::abs(placement.zz)});
    const long double m[3][3]={{placement.xx/scale,placement.xy/scale,placement.xz/scale},{placement.xy/scale,placement.yy/scale,placement.yz/scale},{placement.xz/scale,placement.yz/scale,placement.zz/scale}};
    long double l[3][3]{};
    for(size_t i=0;i<3;++i)for(size_t j=0;j<=i;++j) {
      auto value=m[i][j];for(size_t k=0;k<j;++k)value-=l[i][k]*l[j][k];
      if(i==j){if(!(value>0))throw std::invalid_argument("Unrepresentable native3D placement frame.");l[i][j]=std::sqrt(value);}
      else l[i][j]=value/l[j][j];
    }
    std::array<Vector,3> basis{};
    for(size_t column=0;column<3;++column)for(int i=2;i>=0;--i) {
      long double value=size_t(i)==column ? 1/std::sqrt(scale) : 0;
      for(size_t j=i+1;j<3;++j)value-=l[j][i]*basis[column][j];
      basis[column][i]=value/l[i][i];
    }
    auto cost=[&](Point apex,Face* blocked) {
      long double score=0,worst=INFINITY;
      for(const auto& row:skin) {
        const auto f=row.second;
        const Tetrahedron tet{apex,nodes.at(f[0]).p,nodes.at(f[1]).p,nodes.at(f[2]).p};
        const auto wall=wallPoints.find(row.first);
        long double ratio=INFINITY;
        // Score the actual four wall children, so placement respects their height and visibility.
        for(unsigned child=0;child<(wall==wallPoints.end()?1u:4u);++child) {
          auto candidate=tet;if(wall!=wallPoints.end())candidate[child]=wall->second;
          long double deficit=10,qualityRatio=-INFINITY;
          if(Orientation(candidate[0],candidate[1],candidate[2],candidate[3],stats)>0) {
            const auto quality=Measure(candidate,placement,stats);
            const auto q=std::max(0.,.22-quality.mean_ratio),j=std::max(0.,.06-quality.minimum_scaled_jacobian);
            deficit=q*q+j*j;
            qualityRatio=std::min(quality.mean_ratio/MinimumMeanRatio,quality.minimum_scaled_jacobian/MinimumScaledJacobian);
          }
          score+=deficit;ratio=std::min(ratio,qualityRatio);
        }
        if(blocked && !physical.count(row.first) && ratio<worst){worst=ratio;*blocked=row.first;}
      }
      return score;
    };
    // Placement uses one frozen proxy; the engine certifies the complete actual-query field afterwards.
    auto best=cost(interior.p,nullptr);
    for(unsigned level=0;level<9 && best>1e-12L;++level)for(unsigned sweep=0;sweep<3 && best>1e-12L;++sweep) {
      const auto center=interior.p;const auto previous=best;
      for(int a=-1;a<=1;++a)for(int b=-1;b<=1;++b)for(int c=-1;c<=1;++c)if(a || b || c) {
        const auto step=std::ldexp(rms,-int(level)-1)/std::sqrt(static_cast<long double>(a*a+b*b+c*c));
        const Point p{double(center.x+step*(a*basis[0][0]+b*basis[1][0]+c*basis[2][0])),
                      double(center.y+step*(a*basis[0][1]+b*basis[1][1]+c*basis[2][1])),
                      double(center.z+step*(a*basis[0][2]+b*basis[1][2]+c*basis[2][2]))};
        const auto score=cost(p,nullptr);if(score<best){best=score;interior.p=p;}
      }
      if(best>=previous)break;
    }
    cost(interior.p,&obstruction);
    Patch proposal;
    for(const auto& row:skin) {
      const auto f=row.second;const std::array<Node,4> v{interior,nodes.at(f[0]),nodes.at(f[1]),nodes.at(f[2])};
      if(!(Orientation(v[0].p,v[1].p,v[2].p,v[3].p,stats)>0))throw std::invalid_argument("Native3D joint point cannot see its retained skin.");
      Append(proposal.cells,v,first,stats);
    }
    Finish(old,proposal,skin,physical,reference,stats);
    const bool wall=std::any_of(physical.begin(),physical.end(),[&](const auto& row) {
      return heights.count(reference.Get(row.second.facet).marker);
    });
    if(wall) {
      if(interior.id>UINT64_MAX-2)throw std::invalid_argument("Native3D joint wall identities overflow.");
      FirstHeightReport report;Patch layer;
      // Reuse the certified constructor; only the complete carved replacement reaches metric validation.
      if(!ConstructFirstLayer(proposal,{interior.id+1,interior.p},interior.id+2,first,heights,reference,layer,report,reason,stats))
        throw std::invalid_argument(reason);
      proposal=std::move(layer);
      RestorePatchPoints(proposal,nodes,first,reason,stats);
      proposal.surface.clear();
      Finish(old,proposal,skin,physical,reference,stats);
      if(!CheckFirstHeight(proposal,heights,reference,report,reason,stats))throw std::invalid_argument(reason);
    }
    fresh.cells.swap(proposal.cells);fresh.surface.swap(proposal.surface);reason.clear();return true;
  }catch(const std::exception& e){if(wallObstruction!=Face{})obstruction=wallObstruction;reason=e.what();return false;}
}
