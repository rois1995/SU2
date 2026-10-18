Read-only code review. Do not use tools, edit files, write a plan or request permission. Work only from this supplied source. Review the new binary-star compound reconstruction and its integration, prioritizing concrete correctness/atomic MPI/state/geometry defects. We have an offline complete355tet witness under unchanged field, height.1, q>=.2, J>=.05 and full edge<=1.8. Its old complete shared height apex star36cells cannot satisfy four corners with one apex. Native generic proposal uses wall-patch direction and local metric spacing; no fixture coordinates enter production. Intermediate private tets may have poor shape/height, only final complete patch is accepted. Six new node identities and128cell IDs are already reserved; original donors are immutable. Original edge is inside the full retired-vertex star. Existing engine place_patch imports/reserves union of edge and apices, remeasures current old field, imports original donors, calls operator, then revalidates full metric and height and publishes all cells atomically. `cavity_inserted` denotes a general replacement and skips simple bisection history. Complete fixed-interface ball/positive-volume validation preserves embedded source boundary. Report only concrete defects, distinguishing real issues from existing checks. Bounded heuristic candidate failure is allowed, not global infeasibility. Do not claim general geometry readiness. If you suggest a fix, give minimal code-level change and why existing final checks do not already handle it.

NEW OPERATOR
// A binary vertex-star split is a proposal: the cut's hard arcs encode visibility and final-cell shape.
bool SplitHeightStar(Patch& patch,Id vertex,const std::array<Node,2>& points,
                     const std::array<std::vector<Point>,2>& descendants,const std::set<Id>& future,
                     Id first,FrozenField& target,const PlanarReference& reference,
                     const std::map<int,double>& heights,std::string& reason) {
  std::vector<Cell> star;std::set<Id> removed;
  for(const auto& cell:patch.cells)for(auto n:cell.v)if(n.id==vertex){star.push_back(cell);removed.insert(cell.id);break;}
  if(star.empty() || star.size()>MaximumReplacementCells)throw std::invalid_argument("Native3D height partition lacks its bounded star.");
  const auto nodes=Nodes(star);const auto faces=Incidence(star).Boundary();
  if(faces.size()>MaximumReplacementCells || std::any_of(faces.begin(),faces.end(),[&](Face f){return Has(f,vertex);}))
    throw std::invalid_argument("Native3D height partition needs a complete interior vertex star.");
  std::map<Face,SurfaceFace> walls;
  for(const auto& f:patch.surface)if(heights.count(reference.Get(f.facet).marker))walls.emplace(Key(f.v),f);
  auto cell=[&](std::array<Node,4> v){
    if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);return Cell{0,v};
  };
  auto shape=[&](const Cell& c){
    return Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},target.Query(Centroid(c)).target);
  };
  auto cost=[](MetricMeasures m)->uint64_t {
    const auto q=m.mean_ratio,j=m.minimum_scaled_jacobian;
    const auto value=100*(std::pow(std::max(0.,1-q/(1.01*MinimumMeanRatio)),2)+
        std::pow(std::max(0.,1-j/(1.01*MinimumScaledJacobian)),2))+.01/std::max(q,.01)+.002/std::max(j,.01);
    if(!(std::isfinite(value) && value>=0 && value<1e6))throw std::invalid_argument("Native3D height partition has unrepresentable cost.");
    return uint64_t(std::ceil(1000*value));
  };
  auto admissible=[](MetricMeasures m){return m.mean_ratio>=MinimumMeanRatio && m.minimum_scaled_jacobian>=MinimumScaledJacobian;};
  struct Arc {size_t from,to;uint64_t cost;bool hard;};std::vector<Arc> arcs;
  const size_t source=faces.size(),sink=source+1,count=sink+1;
  auto add=[&](size_t from,size_t to,uint64_t value,bool hard=false){arcs.push_back({from,to,value,hard});};
  struct Use {size_t face;Id a,b;};std::map<Edge,std::vector<Use>> edges;
  for(size_t i=0;i<faces.size();++i) {
    const auto f=faces[i];const std::array<Node,3> v{nodes.at(f[0]),nodes.at(f[1]),nodes.at(f[2])};
    for(unsigned side=0;side<2;++side) {
      const auto c=cell({points[side],v[0],v[1],v[2]});
      bool allowed=Orientation(points[side].p,v[0].p,v[1].p,v[2].p)>0;
      uint64_t penalty=1000000;
      if(allowed) {
        const auto m=shape(c);penalty=cost(m);
        const bool intermediate=std::any_of(c.v.begin(),c.v.end(),[&](Node n){return future.count(n.id);});
        allowed=intermediate || admissible(m);
      }
      const auto wall=walls.find(Key(f));
      if(allowed && wall!=walls.end()) {
        allowed=false;
        for(auto p:descendants[side]) {
          Patch single;single.cells={cell({Node{points[side].id,p},v[0],v[1],v[2]})};single.surface={wall->second};
          FirstHeightReport height;std::string rejected;
          if(CheckFirstHeight(single,heights,reference,height,rejected)){allowed=true;break;}
        }
      }
      // Source side is label0; selecting label0 cuts i->sink and selecting label1 cuts source->i.
      add(side?source:i,side?i:sink,penalty);
      if(!allowed)add(side?source:i,side?i:sink,0,true);
    }
    for(unsigned k=0;k<3;++k){Id a=f[k],b=f[(k+1)%3];edges[{std::min(a,b),std::max(a,b)}].push_back({i,a,b});}
  }
  struct Bridge {Use a,b;long double sign;};std::vector<Bridge> bridges;
  for(const auto& edge:edges) {
    if(edge.second.size()!=2)throw std::invalid_argument("Native3D height partition has a nonmanifold link.");
    const auto a=edge.second[0],b=edge.second[1];const auto u=nodes.at(a.a),v=nodes.at(a.b);
    const auto sign=Orientation(points[0].p,points[1].p,u.p,v.p);
    const auto c=cell({points[0],points[1],u,v});bool quality=false;uint64_t penalty=1000000;
    if(sign) {
      const auto m=shape(c);penalty=cost(m);
      quality=admissible(m) || std::any_of(c.v.begin(),c.v.end(),[&](Node n){return future.count(n.id);});
    }
    // sign>0 permits a1/b0; sign<0 permits a0/b1. Zero/invalid final shape requires equal labels.
    if(sign>=0 || !quality)add(a.face,b.face,0,true);
    if(sign<=0 || !quality)add(b.face,a.face,0,true);
    if(sign>0)add(b.face,a.face,penalty);else if(sign<0)add(a.face,b.face,penalty);
    bridges.push_back({a,b,sign});
  }
  uint64_t total=0;
  for(const auto& arc:arcs)if(!arc.hard) {
    if(arc.cost>UINT64_MAX/4-total)throw std::invalid_argument("Native3D height partition cost exceeds range.");
    total+=arc.cost;
  }
  const auto hard=total+1;
  std::vector<uint64_t> capacity(count*count,0);
  for(const auto& arc:arcs) {
    auto& c=capacity[arc.from*count+arc.to];const auto value=arc.hard?hard:arc.cost;
    if(value>UINT64_MAX-c)throw std::invalid_argument("Native3D height partition capacity exceeds range.");c+=value;
  }
  size_t augmentations=0;std::vector<size_t> previous(count),queue;queue.reserve(count);
  while(true) {
    std::fill(previous.begin(),previous.end(),count);queue.clear();queue.push_back(source);previous[source]=source;
    for(size_t at=0;at<queue.size() && previous[sink]==count;++at)for(size_t j=0;j<count;++j)
      if(previous[j]==count && capacity[queue[at]*count+j]){previous[j]=queue[at];queue.push_back(j);}
    if(previous[sink]==count)break;
    if(++augmentations>4*count*count)throw std::invalid_argument("Native3D height partition exhausted bounded cut work.");
    auto flow=UINT64_MAX;
    for(size_t j=sink;j!=source;j=previous[j])flow=std::min(flow,capacity[previous[j]*count+j]);
    for(size_t j=sink;j!=source;j=previous[j]) {
      capacity[previous[j]*count+j]-=flow;auto& reverse=capacity[j*count+previous[j]];
      if(flow>UINT64_MAX-reverse)throw std::invalid_argument("Native3D height partition residual exceeds range.");reverse+=flow;
    }
  }
  std::vector<bool> label(faces.size());for(size_t i=0;i<faces.size();++i)label[i]=previous[i]==count;
  auto side=[&](size_t i){return i==source?false:i==sink?true:bool(label[i]);};
  for(const auto& arc:arcs)if(arc.hard && !side(arc.from) && side(arc.to))
    throw std::invalid_argument("Native3D height partition has no admitted positive/quality cut.");
  std::vector<Cell> replacement;
  for(size_t i=0;i<faces.size();++i) {
    const auto f=faces[i];Append(replacement,{points[label[i]],nodes.at(f[0]),nodes.at(f[1]),nodes.at(f[2])},first,nullptr);
  }
  for(const auto& bridge:bridges)if(label[bridge.a.face]!=label[bridge.b.face]) {
    const auto a=label[bridge.a.face]?points[1]:points[0],b=label[bridge.a.face]?points[0]:points[1];
    if(Orientation(a.p,b.p,nodes.at(bridge.a.b).p,nodes.at(bridge.a.a).p)<=0)
      throw std::invalid_argument("Native3D height partition cut produced an invalid bridge.");
    Append(replacement,{a,b,nodes.at(bridge.a.b),nodes.at(bridge.a.a)},first,nullptr);
  }
  if(!ValidateFixedInterface(star,replacement,reason,nullptr,MaximumReplacementCells))throw std::invalid_argument(reason);
  Patch next=patch;next.cells.erase(std::remove_if(next.cells.begin(),next.cells.end(),[&](const Cell& c){return removed.count(c.id);}),next.cells.end());
  next.cells.insert(next.cells.end(),replacement.begin(),replacement.end());
  if(next.cells.size()>MaximumReplacementCells || first>UINT64_MAX-(next.cells.size()-1))
    throw std::invalid_argument("Native3D compound height star exceeds replacement/ID budget.");
  for(size_t i=0;i<next.cells.size();++i)next.cells[i].id=first+i;
  patch=std::move(next);return true;
}
}  // namespace

bool ReconstructHeightStar(const Patch& old,Id vertex,Id first_node,Id first_cell,FrozenField& target,
                           const PlanarReference& reference,const std::map<int,double>& heights,
                           Patch& fresh,JointSplitReport& report,std::string& reason,size_t maximum_trials) {
  report={};
  try {
    Bounds(old);const auto nodes=Nodes(old.cells);const auto skin=SkinMap(Incidence(old.cells).Boundary());
    const auto physical=CheckSurface(old,skin,reference,nullptr);const auto origin=nodes.at(vertex).p;
    if(!maximum_trials || maximum_trials>256 || first_node>UINT64_MAX-5)
      throw std::invalid_argument("Native3D height reconstruction exceeds trial/ID admission.");
    for(unsigned i=0;i<6;++i)if(nodes.count(first_node+i))throw std::invalid_argument("Native3D height reconstruction requires reserved new identities.");
    for(const auto& face:skin)if(Has(face.second,vertex))
      throw std::invalid_argument("Native3D height reconstruction needs a complete interior vertex star.");
    FirstHeightReport height;
    if(!CheckFirstHeight(old,heights,reference,height,reason))throw std::invalid_argument(reason);
    std::map<Edge,unsigned> border;std::vector<SurfaceFace> walls;
    for(const auto& entry:physical)if(heights.count(reference.Get(entry.second.facet).marker)) {
      const auto& face=entry.second;
      if(std::any_of(old.cells.begin(),old.cells.end(),[&](const Cell& c){
        unsigned count=0;bool apex=false;for(auto n:c.v){count+=Has(face.v,n.id);apex=apex || n.id==vertex;}return count==3 && apex;
      })) {
        walls.push_back(face);
        for(unsigned k=0;k<3;++k){auto a=face.v[k],b=face.v[(k+1)%3];++border[{std::min(a,b),std::max(a,b)}];}
      }
    }
    if(walls.size()<2)throw std::invalid_argument("Native3D height reconstruction has no shared wall ownership.");
    const auto face=walls.front().v;
    auto normal=Cross(Delta(nodes.at(face[1]).p,nodes.at(face[0]).p),Delta(nodes.at(face[2]).p,nodes.at(face[0]).p));
    const auto norm=Norm(normal);if(!(norm>0))throw std::invalid_argument("Native3D height reconstruction has degenerate wall geometry.");
    for(auto& x:normal)x/=norm;
    Vector u{};long double length=0;
    for(const auto& edge:border)if(edge.second==1) {
      auto d=Delta(nodes.at(edge.first[1]).p,nodes.at(edge.first[0]).p);const auto h=Dot(d,normal);
      for(unsigned j=0;j<3;++j)d[j]-=h*normal[j];const auto l=Norm(d);
      if(l>length){u=d;length=l;}
    }
    if(!(length>0))throw std::invalid_argument("Native3D height reconstruction needs a finite wall-patch direction.");
    for(auto& x:u)x/=length;const auto v=Cross(normal,u);
    const auto metric=target.Query(origin).target;
    auto spacing=[&](Vector d){return 1/std::sqrt(d[0]*(metric.xx*d[0]+metric.xy*d[1]+metric.xz*d[2])+
        d[1]*(metric.xy*d[0]+metric.yy*d[1]+metric.yz*d[2])+d[2]*(metric.xz*d[0]+metric.yz*d[1]+metric.zz*d[2]));};
    const auto scale=std::min({spacing(u),spacing(v),length})/8;
    if(!(scale>0 && std::isfinite(scale)))throw std::invalid_argument("Native3D height reconstruction has unrepresentable spacing.");
    const auto before=MeasureMetricCells(old.cells,target);
    for(double factor:{1.,1.25,1.5,1.75}) {
      if(maximum_trials-report.trials<3){reason="Native3D multi-apex reconstruction exhausted its admitted trial budget.";break;}
      auto p=[&](long double a,long double b){return Point{double(origin.x+scale*factor*(a*u[0]+b*v[0])),
          double(origin.y+scale*factor*(a*u[1]+b*v[1])),double(origin.z+scale*factor*(a*u[2]+b*v[2]))};};
      const std::array<Point,4> leaves{p(-1,-1),p(-1,1),p(1,-1),p(1,1)};
      Patch proposal=old;
      try {
        ++report.trials;++report.connectivity_trials;SplitHeightStar(proposal,vertex,{{{first_node,p(-1,0)},{first_node+1,p(1,0)}}},
            {{{leaves[0],leaves[1]},{leaves[2],leaves[3]}}},{first_node,first_node+1},first_cell,target,reference,heights,reason);
        ++report.trials;++report.connectivity_trials;SplitHeightStar(proposal,first_node,{{{first_node+2,leaves[0]},{first_node+3,leaves[1]}}},
            {{{leaves[0]},{leaves[1]}}},{first_node+1},first_cell,target,reference,heights,reason);
        ++report.trials;++report.connectivity_trials;SplitHeightStar(proposal,first_node+1,{{{first_node+4,leaves[2]},{first_node+5,leaves[3]}}},
            {{{leaves[2]},{leaves[3]}}},{},first_cell,target,reference,heights,reason);
        proposal.surface.clear();Finish(old,proposal,skin,physical,reference,nullptr);
        if(!CheckFirstHeight(proposal,heights,reference,height,reason)){++report.height_rejected;continue;}
        report.best=proposal;PatchMetric measured;
        if(!ValidateMetricChange(old.cells,proposal.cells,MetricChange::Refine,target,measured,reason,nullptr,&before.edges)) {
          ++report.metric_rejected;continue;
        }
        fresh=std::move(proposal);reason.clear();return true;
      }catch(const std::exception& error){++report.geometry_rejected;reason=error.what();}
    }
    if(reason.empty())reason="Native3D bounded multi-apex reconstruction found no admitted proposal.";
    return false;
  }catch(const std::exception& error){reason=error.what();return false;}
}


ENGINE FULL place_patch CONTEXT
auto place_patch=[&](bool search,const std::vector<Id>& apices,Id height_apex=UINT64_MAX) {
        if(!world.maximum(search))return true;
        if(stats.global_work>=stats.work_limit){stats.stop=EngineStop::WorkBudget;result.reason="Native3D joint placement exhausted the global work budget.";return false;}
        if(options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
          stats.stop=EngineStop::TimeBudget;result.reason="Native3D joint placement exhausted the elapsed budget.";return false;
        }
        std::vector<Feature> features;
        if(search) {
          features.push_back({{choice.edge[0],choice.edge[1],0},2});
          for(Id id:apices)features.push_back({{id,0,0},1});
        }
        Dependencies joined;
        {Timer timer(stats.dependency_seconds,options.timings);
          joined=directory.Lookup(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if(!joined.valid) {
          if(joined.reason=="Native 3D dependency star exceeds budget." || joined.reason=="Native 3D dependency union exceeds budget.")search=false;
          else failure.Set(1,0,joined.reason);
        }
        if(!(ready=vote(failure)))return false;
        if(!search){joined.valid=true;joined.cells.clear();}
        Imported more;
        {Timer timer(stats.dependency_seconds,options.timings);
          more=directory.Fetch(joined,working,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if(!more.valid){result.reason=more.reason;return false;}
        if(search)try {
          std::set<Id> required;
          for(const auto& c:old.cells)if(std::count_if(c.v.begin(),c.v.end(),[&](Node n){return n.id==choice.edge[0] || n.id==choice.edge[1];})==2)required.insert(c.id);
          // Discard unused support from abandoned proposals; the actual edge and moved stars remain complete.
          for(const auto& dep:dependencies.cells)if(required.count(dep.cell)) {
            const auto found=std::find_if(joined.cells.begin(),joined.cells.end(),[&](const auto& d){return d.cell==dep.cell;});
            if(found==joined.cells.end() || found->version!=dep.version || found->owner!=dep.owner)
              throw std::runtime_error("Native3D joint placement lost a complete/versioned dependency.");
          }
          dependencies=std::move(joined);imported=std::move(more);old=GeometryPatch(imported);reuse.clear();
          for(const auto& c:old.cells)selection.Cached(c,reuse);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return false;
        bool reserved=false;
        {Timer timer(stats.reservation_seconds,options.timings);
          reserved=directory.Reserve(imported,search || approved,result.reason,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if((search || approved) && !reserved) {
          if(result.reason=="Native 3D conflicting dependency reservation.") {
            ++stats.conflicts;
            if(!concurrent){search=false;approved=false;}
          } else failure.Set(1,0,"Native3D joint placement lost its complete-star reservation: "+result.reason);
        }
        if(!(ready=vote(failure)))return false;
        PatchMetric expanded_before;
        const bool measured=measure_private(search?old.cells:empty,expanded_before,false,reuse);
        if(search && measured)before=std::move(expanded_before);
        if(!ready)return false;
        if(search && !measured)failure.Set(1,0,result.reason);
        if(!(ready=vote(failure)))return false;
        DonorPatch support;
        {Timer timer(stats.donor_seconds,options.timings);++stats.donor_batches;
          support=source.ImportCells(search?old.cells:empty,options.memory_ceiling,transfer_memory::Add(bytes(),directory.ResidentBytes(),private_scratch));}
        if(!support.valid) {
          if(DonorCap(support.reason))search=false;
          else {result.reason=support.reason;return false;}
        }
        const auto participants=concurrent ? CPassiveComm::Allreduce(uint64_t(search),CPassiveComm::Op::SUM,world.comm) : uint64_t(1);
        const auto allowance=std::min<uint64_t>(256,(stats.work_limit-stats.global_work)/std::max<uint64_t>(1,participants));
        if(!allowance){stats.stop=EngineStop::WorkBudget;result.reason="Native3D joint placement cannot admit simultaneous work within the global budget.";return false;}
        bool candidate=false;JointSplitReport report;
        if(search)try {
          Timer timer(stats.reconstruction_seconds,options.timings);CpuTimer cpu(stats.reconstruction_cpu_seconds,options.timings);
          const auto components=AssociatedExtensions(support.cells,geometry);
          FrozenField target(std::move(support.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
          FieldAccounting accounting{stats,target};std::vector<std::pair<Id,Id>> reserved_apices;
          for(size_t i=0;i<apices.size();++i)reserved_apices.emplace_back(apices[i],maximum_node+2+i+node_offset);
          if(height_apex!=UINT64_MAX) {
            candidate=ReconstructHeightStar(old,height_apex,maximum_node+1+node_offset,maximum_cell+1+offset,target,
                geometry,options.wall_heights,fresh,report,result.reason,allowance);
            stats.wall_reconstruction_trials+=report.trials;stats.wall_topology_trials+=report.trials;
          } else candidate=PlaceSplitPatch(old,choice.edge,point,reserved_apices,maximum_cell+1+offset,target,geometry,options.wall_heights,
              fresh,report,result.reason,allowance);
          stats.repair_proposals+=report.trials;stats.placement_trials+=report.trials;
          stats.placement_connectivity_trials+=report.connectivity_trials;stats.placement_flips+=report.flips;stats.placement_moves+=report.moves;
          stats.metric_linearizations+=report.linearizations;stats.metric_probes+=report.metric_probes;
          stats.metric_linear_iterations+=report.linear_iterations;stats.metric_solve_trials+=report.solve_trials;
          placement_obstruction=report.blocked_interior_vertex;
          if(!candidate && !report.best.cells.empty()){fresh=std::move(report.best);candidate=true;}
          candidate=guarded_geometry(candidate);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return false;
        stats.global_work+=concurrent ? CPassiveComm::Allreduce(uint64_t(report.trials),CPassiveComm::Op::SUM,world.comm) : world.maximum(report.trials);
        const auto retained_metric=certified;
        const bool passed=validate_private(candidate,MetricChange::Refine);
        if(search && !approved){approved=passed;stats.placement_validated+=passed;}
        else if(!search)certified=retained_metric;
        if(!ready)return false;
        if(search && approved){cavity_inserted=true;joint=false;}
        return true;
      };
      Id height_apex=UINT64_MAX;
      if(joint && composition && !boundary_edge && !choice.geometric && !options.wall_heights.empty())
        for(Id vertex:choice.edge) {
          size_t walls=0;
          for(const auto& f:old.surface)if(options.wall_heights.count(geometry.Get(f.facet).marker) &&
              std::find(f.v.begin(),f.v.end(),vertex)==f.v.end())
            for(const auto& c:old.cells) {
              unsigned matches=0;bool apex=false;
              for(auto n:c.v){matches+=std::find(f.v.begin(),f.v.end(),n.id)!=f.v.end();apex=apex || n.id==vertex;}
              walls+=matches==3 && apex;
            }
          if(walls>=2){height_apex=vertex;break;}
        }
      if(!place_patch(height_apex!=UINT64_MAX,{height_apex},height_apex))return finish();
      if(!place_patch(joint && composition && (!boundary_edge || (flat_edge && wallApices.empty())) && !choice.geometric,{}))return finish();
      
EXISTING FIRST-HEIGHT CHECK
bool CheckFirstHeight(const Patch& patch,const std::map<int,double>& heights,const PlanarReference& reference,
                      FirstHeightReport& report,std::string& reason,KernelStats* stats) {
  report={};
  try {
    for(const auto& height:heights)if(!(height.second>0) || !std::isfinite(height.second))
      throw std::invalid_argument("Native3D wall height must be finite and positive.");
    const auto skin=SkinMap(Incidence(patch.cells,stats).Boundary());
    const auto physical=CheckSurface(patch,skin,reference,stats);
    const auto nodes=Nodes(patch.cells);
    for(const auto& entry:physical) {
      const auto& face=entry.second;
      const auto height=heights.find(reference.Get(face.facet).marker);
      if(height==heights.end())continue;
      ++report.wall_faces;
      const auto a=nodes.at(face.v[0]).p,b=nodes.at(face.v[1]).p,c=nodes.at(face.v[2]).p;
      auto normal=Cross(Delta(b,a),Delta(c,a));const auto area=Norm(normal);
      if(!(area>0))throw std::invalid_argument("Native3D first-height face is degenerate.");
      for(auto& x:normal)x/=area;
      Point center{double(((long double)a.x+b.x+c.x)/3),double(((long double)a.y+b.y+c.y)/3),
                   double(((long double)a.z+b.z+c.z)/3)};
      size_t found=0;Node apex;Id cell=0;
      for(const auto& candidate:patch.cells) {
        unsigned count=0;Node opposite;
        for(auto node:candidate.v) {
          if(std::find(face.v.begin(),face.v.end(),node.id)!=face.v.end())++count;
          else opposite=node;
        }
        if(count==3){++found;apex=opposite;cell=candidate.id;}
      }
      if(found!=1){++report.missing_cells;throw std::invalid_argument("Native3D wall face has no unique first child.");}
      if(reference.CompleteAnalytic(face.facet)) {
        const auto closest=reference.ClosestAnalytic(face.facet,apex.p);
        const auto n=closest.normal,d=closest.direction;
        const long double clearance=-closest.distance*((long double)d.x*n.x+(long double)d.y*n.y+(long double)d.z*n.z);
        if(clearance/height->second<report.minimum_reference_clearance_ratio) {
          report.minimum_reference_clearance_ratio=double(clearance/height->second);
          report.reference_clearance_facet=face.facet;report.reference_clearance_cell=cell;report.reference_clearance_apex=apex.p;
        }
        if(!(clearance>0))++report.reference_clearance_failures;
      }
      const auto altitude=std::abs(Orientation(a,b,c,apex.p,stats))/area;
      const double error=double(std::abs(altitude/height->second-1));
      auto tangent=Delta(apex.p,center);const auto projection=Dot(tangent,normal);
      for(unsigned j=0;j<3;++j)tangent[j]-=projection*normal[j];
      report.maximum_tilt=std::max(report.maximum_tilt,double(Norm(tangent)/height->second));
      // Normal-foot barycentrics use tangential areas, without rounding a projected point.
      const std::array<Vector,3> relative{{Delta(a,apex.p),Delta(b,apex.p),Delta(c,apex.p)}};
      long double minimum=1./3;bool outside=false;
      for(unsigned j=0;j<3;++j) {
        const auto& u=relative[(j+1)%3];const auto& v=relative[(j+2)%3];
        const auto weight=Dot(Cross(u,v),normal)/area;
        const auto tolerance=256*std::numeric_limits<long double>::epsilon()*std::max(1.L,Norm(u)*Norm(v)/area);
        if(!std::isfinite(weight) || tolerance>1e-8L)
          throw std::invalid_argument("Native3D first-row footprint cannot be resolved at cell "+std::to_string(cell));
        minimum=std::min(minimum,weight);outside=outside || weight < -tolerance;
      }
      report.footprint_failures+=outside;
      if(minimum<report.minimum_barycentric) {
        report.minimum_barycentric=double(minimum);report.footprint_facet=face.facet;report.footprint_cell=cell;
      }
      report.height_failures+=error>1e-8;
      if(error>report.maximum_relative_error) {
        report.maximum_relative_error=error;report.worst_facet=face.facet;report.worst_cell=cell;report.worst_apex=apex.p;
      }
    }
    if(report.reference_clearance_failures)throw std::invalid_argument("Native3D geometry-height incompatibility of the normal-row proposal at facet "+
        std::to_string(report.reference_clearance_facet)+" cell "+std::to_string(report.reference_clearance_cell)+
        " clearance/h0="+std::to_string(report.minimum_reference_clearance_ratio)+
        ": apex crosses smooth reference; resolve curvature before constructing the row.");
    if(report.height_failures)throw std::invalid_argument("Native3D prescribed height fails at facet "+std::to_string(report.worst_facet)+
        " cell "+std::to_string(report.worst_cell)+" relative error "+std::to_string(report.maximum_relative_error));
    if(report.footprint_failures)throw std::invalid_argument("Native3D first-row normal foot leaves wall triangle at facet "+
        std::to_string(report.footprint_facet)+" cell "+std::to_string(report.footprint_cell)+
        " minimum barycentric "+std::to_string(report.minimum_barycentric));
    reason.clear();return true;
  }catch(const std::exception& error){reason=error.what();return false;}
}

