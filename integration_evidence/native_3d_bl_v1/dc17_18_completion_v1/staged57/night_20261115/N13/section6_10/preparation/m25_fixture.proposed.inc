// UNAPPLIED: hidden N13/M25 test-only fixture; add beside the hidden D9 case.
namespace {
struct M25Input { std::array<std::string, 9> raw; SU2Native3D::Edge edge{}; std::string verdict; unsigned group=0; };
struct M25Row {
  M25Input input; std::string status="not_attempted_time_limit";
  SU2Native3D::GeometrySplitPrediction prediction; double seconds=0,length=0,distance=0;
  size_t work=0; SU2Native3D::Point midpoint{},bound{}; bool selfcheck=false;
  std::array<const SU2Native3D::SurfaceFace*,2> faces{};
  std::array<double,2> parent{}; std::array<std::array<double,2>,2> child{};
  bool point_valid=false,midpoint_valid=false,distance_valid=false,length_valid=false;
  std::array<bool,2> parent_valid{}; std::array<std::array<bool,2>,2> child_valid{};
  bool law_rows_valid=false;
  std::array<std::array<SU2Native3D::WallEdgeEstimate,3>,2> law{};
  std::array<std::array<SU2Native3D::Edge,3>,2> law_edges{};
};
std::vector<M25Input> M25Read(const char* path) {
  std::ifstream in(path); if(!in) throw std::runtime_error("cannot open SU2_M25_EDGES");
  std::string line; std::getline(in,line);
  if(line!="edge_a,edge_b,law_pick,n11_first_verdict,failing_faces,limiting_faces,limiting_failing_faces,need,estimate")
    throw std::runtime_error("M25 input header mismatch");
  std::vector<M25Input> rows;
  while(std::getline(in,line)) {
    std::stringstream s(line); M25Input r;
    for(auto& x:r.raw) if(!std::getline(s,x,',')) throw std::runtime_error("short M25 row");
    std::string extra; if(std::getline(s,extra,',')) throw std::runtime_error("long M25 row");
    r.edge={std::stoull(r.raw[0]),std::stoull(r.raw[1])};
    if(r.edge[0]>=r.edge[1]) throw std::runtime_error("noncanonical M25 edge");
    r.verdict=r.raw[3];
    r.group=unsigned(std::stoul(r.raw[6])); rows.push_back(std::move(r));
  }
  if(rows.size()!=2408) throw std::runtime_error("M25 input row count mismatch");
  return rows;
}
AtlasBudget M25Budget(size_t held,const std::function<bool()>& within) {
  auto b=GeometryAtlasBudget(); b.maximum_work=D9WorkLimit; b.maximum_bytes=D9WorkspaceLimit;
  b.maximum_integer_bits=D9IntegerLimit; b.caller_bytes=held; b.within_time=within; return b;
}
#pragma GCC push_options
#pragma GCC optimize ("no-fast-math", "no-unsafe-math-optimizations", "ffp-contract=off")
bool M25Score(const PlanarReference& c,Id facet,const std::array<Point,3>& tri,double tau,
              long double& sum,size_t& count,double& bound) {
  if(!c.ContainsFace(facet,tri)) return false;
  bound=c.SurfaceDeviation(facet,tri,SurfaceQueryNeed::Magnitude(),true);
  if(!std::isfinite(bound)) return false;
  const long double excess=std::max(0.L,(long double)bound-tau);
  sum+=excess*excess; ++count; return std::isfinite(sum);
}
#pragma GCC pop_options
void M25Rotate(std::array<Point,3>& tri,std::array<Id,3> ids) {
  const auto n=std::min_element(ids.begin(),ids.end())-ids.begin();
  std::rotate(tri.begin(),tri.begin()+n,tri.end());
}
void M25Prediction(M25Row& r,const D9Loaded& loaded,double tau,const std::function<bool()>& within) {
  auto budget=M25Budget(loaded.held,within); auto geometry=loaded.base->WithAtlasReservation(budget);
  const auto* a=loaded.parameters->Find(r.input.edge[0]); const auto* b=loaded.parameters->Find(r.input.edge[1]);
  std::array<Point,2> endpoints{}; if(a) endpoints[0]=a->node.p; if(b) endpoints[1]=b->node.p;
  const auto start=D9Clock::now(); const size_t work=budget.work;
  try { r.prediction=PredictGeometrySplit(r.input.edge,endpoints,*geometry,tau); }
  catch(const std::length_error&) {
    r.seconds=std::chrono::duration<double>(D9Clock::now()-start).count(); r.work=budget.work-work;
    if(!within()) { r.status="incomplete_time_limit"; return; }
    throw;
  }
  r.seconds=std::chrono::duration<double>(D9Clock::now()-start).count(); r.work=budget.work-work;
  r.status=r.prediction.complete?"complete":"incomplete";
}
#pragma GCC push_options
#pragma GCC optimize ("no-fast-math", "no-unsafe-math-optimizations", "ffp-contract=off")
bool M25Reconstruct(M25Row& r,const D9Loaded& loaded,double tau,const std::function<bool()>& within) {
  auto& pred=r.prediction; long double before=0,after=0; size_t count=0;
  auto budget=M25Budget(loaded.held,within); const auto* state=loaded.parameters.get();
  if(r.input.edge[0]>=r.input.edge[1]||!std::isfinite(tau)||tau<0) return false;
  if(!state||!loaded.base->EntityGeometry()) goto verify;
  {
    const auto* a=state->Find(r.input.edge[0]); const auto* b=state->Find(r.input.edge[1]);
    if(!a||!b||state->Geometry().MobilityDimension(a->binding,state->Mobility())!=2||
       state->Geometry().MobilityDimension(b->binding,state->Mobility())!=2) goto verify;
    if(state->Coverage().traces.count(r.input.edge)) goto verify;
    const auto incident=state->IncidentFaces(r.input.edge);
    if(!incident[0]||!incident[1]||state->Entity(incident[0]->facet)!=state->Entity(incident[1]->facet)) goto verify;
    r.faces={incident[0],incident[1]};
    try {
    auto context=loaded.base->PrivateParameters(Patch{},&budget);
    if(!context) goto verify;
    const std::set<Id> facets{incident[0]->facet,incident[1]->facet};
    r.midpoint={double(((long double)a->node.p.x+b->node.p.x)/2),
                double(((long double)a->node.p.y+b->node.p.y)/2),
                double(((long double)a->node.p.z+b->node.p.z)/2)}; r.midpoint_valid=true;
    r.length=std::hypot(b->node.p.x-a->node.p.x,b->node.p.y-a->node.p.y,b->node.p.z-a->node.p.z); r.length_valid=true;
    const auto bound=context->BindNewPoint(Node{UINT64_MAX,r.midpoint},facets,&r.input.edge);
    r.bound=bound.p; r.point_valid=true; r.distance=std::hypot(r.bound.x-r.midpoint.x,r.bound.y-r.midpoint.y,r.bound.z-r.midpoint.z); r.distance_valid=true;
    if(PointKey(bound.p)==PointKey(a->node.p)||PointKey(bound.p)==PointKey(b->node.p)) goto verify;
    for(size_t f=0;f<2;++f) {
      const auto& face=*r.faces[f]; std::array<Point,3> parent; unsigned ia=3,ib=3;
      for(unsigned k=0;k<3;++k) { const auto* p=state->Find(face.v[k]); if(!p) goto verify; parent[k]=p->node.p;
        if(face.v[k]==r.input.edge[0]) ia=k; if(face.v[k]==r.input.edge[1]) ib=k; }
      if(ia==3||ib==3||!M25Score(*context,face.facet,parent,tau,before,count,r.parent[f])) goto verify;
      r.parent_valid[f]=true;
      auto left=parent,right=parent; left[ia]=bound.p; right[ib]=bound.p;
      auto li=face.v,ri=face.v; li[ia]=bound.id; ri[ib]=bound.id;
      M25Rotate(left,li); M25Rotate(right,ri);
      if(!M25Score(*context,face.facet,left,tau,after,count,r.child[f][0])) goto verify;
      r.child_valid[f][0]=true;
      if(!M25Score(*context,face.facet,right,tau,after,count,r.child[f][1])) goto verify;
      r.child_valid[f][1]=true;
    }
    } catch(const std::invalid_argument&) {
      if(budget.work_exhausted||!budget.geometry_failure.empty()) throw;
    }
  }
verify:
  r.selfcheck=count==pred.evaluated_faces&&before==pred.before&&after==pred.after;
  return r.selfcheck;
}
#pragma GCC pop_options
std::array<Edge,3> M25EdgesForFace(const M25Row& r,size_t face_index) {
  const auto& face=*r.faces[face_index]; std::array<Edge,3> out{}; size_t n=1;
  bool found=false;
  for(unsigned k=0;k<3;++k) {
    Edge edge{face.v[k],face.v[(k+1)%3]}; std::sort(edge.begin(),edge.end());
    if(edge==r.input.edge) { out[0]=edge; found=true; }
    else out[n++]=edge;
  }
  if(!found||n!=3) throw std::runtime_error("incident face does not contain M25 edge");
  return out;
}
void M25Laws(M25Row& r,const D9Loaded& loaded) {
  r.law_rows_valid=true;
  for(size_t f=0;f<2;++f) {
    r.law_edges[f]=M25EdgesForFace(r,f);
    for(size_t e=0;e<3;++e) {
      const auto& edge=r.law_edges[f][e];
      const auto* a=loaded.parameters->Find(edge[0]); const auto* b=loaded.parameters->Find(edge[1]);
      if(a&&b) r.law[f][e]=EstimateWallEdge(*loaded.parameters,a->node,b->node,r.faces[f]->facet);
    }
  }
}
void M25Write(const std::vector<M25Row>& rows) {
  std::ofstream out("m25_split_outcomes.csv"); if(!out) throw std::runtime_error("cannot create M25 CSV");
  out << "edge_a,edge_b,law_pick,n11_first_verdict,failing_faces,limiting_faces,limiting_failing_faces,need,estimate"
         ",attempt_status,complete,before,after,evaluated_faces,seconds,atlas_work,selfcheck,point_valid,bound_x,bound_y,bound_z"
         ",midpoint_valid,midpoint_x,midpoint_y,midpoint_z,distance_valid,midpoint_distance,length_valid,edge_length,parent0_valid,parent0,child00_valid,child00,child01_valid,child01"
         ",parent1_valid,parent1,child10_valid,child10,child11_valid,child11,law_rows_valid";
  for(size_t f=0;f<2;++f) for(size_t v=0;v<3;++v) out << ",parent"<<f<<"_v"<<v;
  for(size_t f=0;f<2;++f) for(size_t e=0;e<3;++e) out << ",law_f"<<f<<"_e"<<e<<"_edge,law_f"<<f<<"_e"<<e<<"_supported,law_f"<<f<<"_e"<<e<<"_value";
  out << '\n' << std::setprecision(std::numeric_limits<long double>::max_digits10);
  for(const auto& r:rows) {
    for(size_t i=0;i<9;++i) out << (i?",":"") << r.input.raw[i];
    if(r.status=="not_attempted_time_limit") { for(size_t i=0;i<54;++i) out << ","; out << '\n'; continue; }
    if(r.status=="incomplete_time_limit") { out << "," << r.status; for(size_t i=0;i<53;++i) out << ","; out << '\n'; continue; }
    out << ',' << r.status << ',' << r.prediction.complete << ',' << r.prediction.before << ',' << r.prediction.after
        << ',' << r.prediction.evaluated_faces << ',' << r.seconds << ',' << r.work << ',' << r.selfcheck
        << ',' << r.point_valid << ',' << r.bound.x << ',' << r.bound.y << ',' << r.bound.z << ',' << r.midpoint.x << ',' << r.midpoint.y
        << ',' << r.midpoint.z << ',' << r.distance << ',' << r.length;
    for(size_t f=0;f<2;++f) { out << ',' << r.parent_valid[f] << ',' << r.parent[f];
      for(size_t c=0;c<2;++c) out << ',' << r.child_valid[f][c] << ',' << r.child[f][c]; }
    out << ',' << r.law_rows_valid;
    for(size_t f=0;f<2;++f) for(size_t v=0;v<3;++v) out << ',' << (r.faces[f]?r.faces[f]->v[v]:0);
    for(size_t f=0;f<2;++f) for(size_t e=0;e<3;++e) {
      out << ',' << r.law_edges[f][e][0] << ':' << r.law_edges[f][e][1] << ',' << r.law[f][e].supported << ',' << r.law[f][e].value;
    }
    out << '\n';
  }
  if(!out) throw std::runtime_error("M25 CSV write failed");
}
}
TEST_CASE("M25 saved492 geometry split outcomes", "[.][NativeM25Split]") {
  using namespace SU2Native3D;
  D9CheckNeedScaling();
  const double tau=1e-4;
  World world; REQUIRE(world.size==1);
  const char* ref_path=std::getenv("SU2_D9_REFERENCE"); REQUIRE(ref_path);
  const char* param_path=std::getenv("SU2_D9_PARAMETERS");
  const char* input_path=std::getenv("SU2_M25_EDGES"); REQUIRE(input_path);
  const auto started=D9Clock::now();
  const auto within=[started] { return D9Clock::now()-started<std::chrono::minutes(60); };
  D9Loaded loaded; loaded.bytes=D9ReadBytes(ref_path);
  loaded.reference=DecodeReference(std::string(loaded.bytes.begin(),loaded.bytes.end()),D9HeldMemoryLimit);
  REQUIRE(loaded.reference.original);
  if(param_path) {
    const auto* initial=loaded.reference.original->Parameters();
    const auto authority=initial?initial->Authority():std::shared_ptr<const MappedFacetReference>{}; REQUIRE(authority);
    const auto bytes=D9ReadBytes(param_path);
    loaded.parameters=BoundaryParameterState::FromPayload(authority,bytes,D9HeldMemoryLimit,loaded.reference.original->ResidentBytes());
  } else if(loaded.reference.original->Parameters()) {
    loaded.parameters=std::make_shared<BoundaryParameterState>(*loaded.reference.original->Parameters());
  }
  REQUIRE(loaded.parameters);
  loaded.authority=loaded.parameters->Authority(); REQUIRE(loaded.authority);
  loaded.entity_geometry=loaded.parameters->EntityGeometry();
  loaded.query_tolerance=loaded.parameters->GeometryTolerance(); REQUIRE(loaded.query_tolerance==tau);
  loaded.base=loaded.reference.original->WithParameterState(loaded.parameters);
  loaded.held=D9HeldReferenceBytes(loaded.reference,loaded.parameters,loaded.bytes,*loaded.base);
  REQUIRE(loaded.held<=D9HeldMemoryLimit);
  const auto inputs=M25Read(input_path); std::vector<M25Row> rows(inputs.size());
  for(size_t i=0;i<inputs.size();++i) rows[i].input=inputs[i];
  std::vector<size_t> order(inputs.size()); for(size_t i=0;i<order.size();++i) order[i]=i;
  size_t done=0; bool reprioritized=false;
  for(size_t at=0;at<order.size()&&within();++at) {
    const auto i=order[at];
    try {
      M25Prediction(rows[i],loaded,tau,within);
      if(rows[i].status!="incomplete_time_limit") {
        if(!M25Reconstruct(rows[i],loaded,tau,within))
          throw std::runtime_error("M25 partial/complete sum self-check failed at edge "+inputs[i].raw[0]+":"+inputs[i].raw[1]);
        if(rows[i].faces[0]&&rows[i].faces[1]) M25Laws(rows[i],loaded);
        if(rows[i].prediction.complete) rows[i].status="complete";
      }
    } catch(const std::length_error&) {
      if(!within()) rows[i].status="incomplete_time_limit"; else throw;
    }
    ++done;
    if(done==100&&within()) {
      const double elapsed=std::chrono::duration<double>(D9Clock::now()-started).count();
      if(elapsed*double(inputs.size())/100.0>3600.0) {
        std::stable_sort(order.begin()+100,order.end(),[&](size_t a,size_t b) {
          return (inputs[a].verdict!="none")>(inputs[b].verdict!="none");
        });
        reprioritized=true;
      }
    }
  }
  for(auto& r:rows) if(r.status=="not_attempted_time_limit") r.status="not_attempted_time_limit";
  M25Write(rows);
  std::ofstream meta("m25_fixture_summary.json"); REQUIRE(bool(meta));
  const auto complete=std::count_if(rows.begin(),rows.end(),[](const M25Row& r){return r.status=="complete";});
  meta << "{\"schema\":\"m25_fixture_v1\",\"rows\":"<<rows.size()<<",\"attempted\":"<<done
       <<",\"complete\":"<<complete<<",\"interrupted_time_limit\":"<<std::count_if(rows.begin(),rows.end(),[](const M25Row& r){return r.status=="incomplete_time_limit";})<<",\"reprioritized_after_100\":"<<(reprioritized?"true":"false")
       <<",\"time_limit_seconds\":3600,\"production_workflow_launched\":false}\n";
  REQUIRE(bool(meta)); REQUIRE(done>0);
}
