// Same prepared production authority and composed target; only the requested certificate accuracy changes.
const auto* pointPath=std::getenv("SU2_NATIVE_BL_POINT_REPLAY");REQUIRE(pointPath);
std::ifstream points(pointPath);REQUIRE(bool(points));
double retainedCore=0,trialCore=0;size_t samples=0;
points>>retainedCore>>trialCore>>samples;REQUIRE(bool(points));
REQUIRE(retainedCore==selectedCore);REQUIRE(trialCore==selectedCore);REQUIRE(samples>0);REQUIRE(samples<=12);
const auto& geometry=policy->authority->Parameters()->Geometry();
const auto* mobility=policy->authority->Mobility();REQUIRE(mobility);
DonorCell retainedDonor;const DonorCell* donor=nullptr;
for(const auto& row:imported.cells) {
  const auto& candidate=row.second;
  bool hasA=false,hasB=false;for(const auto& node:candidate.cell.v){hasA|=node.id==edge[0];hasB|=node.id==edge[1];}
  if(hasA&&hasB){retainedDonor.cell=candidate.cell;for(unsigned j=0;j<4;++j)retainedDonor.sensor[j]=coreAt(candidate.cell.v[j]);donor=&retainedDonor;break;}
}
REQUIRE(donor);
auto tensorJSON=[&](Tensor m){frontLog<<'['<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz<<']';};
for(size_t sampleIndex=0;sampleIndex<samples;++sampleIndex) {
  Id facet=0;size_t wall=0;Point q{};double error=0;
  points>>facet>>wall>>q.x>>q.y>>q.z>>error;REQUIRE(bool(points));
  REQUIRE(std::isfinite(q.x));REQUIRE(std::isfinite(q.y));REQUIRE(std::isfinite(q.z));
  REQUIRE(std::isfinite(error));REQUIRE(error>0);REQUIRE(wall<policy->full->GetnWall());
  const double delta[3]={b.x-a.x,b.y-a.y,b.z-a.z},offset[3]={q.x-a.x,q.y-a.y,q.z-a.z};
  unsigned axis=0;for(unsigned j=1;j<3;++j)if(std::abs(delta[j])>std::abs(delta[axis]))axis=j;
  REQUIRE(delta[axis]!=0);const long double t=static_cast<long double>(offset[axis])/delta[axis];
  REQUIRE(t>=0);REQUIRE(t<=1);
  const double av[3]={a.x,a.y,a.z},qv[3]={q.x,q.y,q.z};
  for(unsigned j=0;j<3;++j)REQUIRE(std::abs((av[j]+t*delta[j])-qv[j])<=64*std::numeric_limits<double>::epsilon()*std::max({1.,std::abs(av[j]),std::abs(qv[j])}));
  std::array<long double,4> weights{};
  for(unsigned j=0;j<4;++j){if(donor->cell.v[j].id==edge[0])weights[j]=1-t;if(donor->cell.v[j].id==edge[1])weights[j]=t;}
  const auto sensor=InterpolateSensor(*donor,weights);ValidateTensor(sensor);
  const auto startWork=budget.work;const auto started=std::chrono::steady_clock::now();
  auto evaluation=[&](const char* phase) {
    const auto combined=policy->Composition()(q,sensor);ValidateTensor(combined);
    const su2double xyz[3]={q.x,q.y,q.z};
    const auto full=policy->full->Evaluate(static_cast<unsigned short>(wall),xyz,selectedCore);
    const auto base=policy->base->Evaluate(static_cast<unsigned short>(wall),xyz,selectedCore);
    frontLog<<"{\"stage\":\"prepared_target_stability\",\"sample\":"<<sampleIndex<<",\"phase\":"<<std::quoted(phase)
            <<",\"point\":";pointJSON(q);frontLog<<",\"sensor\":";tensorJSON(sensor);
    frontLog<<",\"combined\":";tensorJSON(combined);
    frontLog<<",\"chord_speed\":"<<MetricNorm(a,b,combined)<<",\"full_normal\":["<<full.normal[0]<<','<<full.normal[1]<<','<<full.normal[2]
            <<"],\"base_normal\":["<<base.normal[0]<<','<<base.normal[1]<<','<<base.normal[2]
            <<"],\"hn\":"<<full.hn<<",\"distance\":"<<full.distance<<",\"weight\":"<<full.weight
            <<",\"full\":";tensorJSON(policy->Pack(full.full));frontLog<<",\"base\":";tensorJSON(policy->Pack(base.full));
    frontLog<<",\"mobility_identity\":"<<mobility->identity<<",\"mobility_version\":"<<mobility->version
            <<",\"mobility_angle\":"<<mobility->angle<<",\"parameter_reconstructed\":true}"<<std::endl;
  };
  evaluation("before");evaluation("repeat");
  using SU2NativeGeometry::MappedFacetReference;
  budget.CheckTime();REQUIRE(budget.work<budget.maximum_work);REQUIRE(budget.caller_bytes<=budget.maximum_bytes);
  const auto work=std::min(MappedFacetReference::ClearanceWork,budget.maximum_work-budget.work),available=budget.maximum_bytes-budget.caller_bytes;
  const auto scratch=geometry.CurvedQueryWorkspaceBytes(work);REQUIRE(scratch<=available);
  budget.peak_bytes=std::max(budget.peak_bytes,transfer_memory::Add(budget.caller_bytes,scratch));
  size_t attempted=0;MappedFacetReference::CurvedClosest closest;
  auto charge=[&]{budget.work=transfer_memory::Add(budget.work,attempted);budget.work_exhausted|=budget.work>budget.maximum_work;};
  try{closest=geometry.ClosestCurved(facet,q,error,work,available,{},&attempted,&budget);}
  catch(...){charge();throw;}
  charge();REQUIRE_FALSE(budget.work_exhausted);
  REQUIRE(geometry.CurvedComponent(closest.facet)==geometry.CurvedComponent(facet));
  REQUIRE(closest.lower>=0);REQUIRE(closest.upper>=closest.lower);REQUIRE(closest.upper-closest.lower<=error);
  const auto& binding=closest.binding;size_t supportCount=0;
  frontLog<<"{\"stage\":\"prepared_target_certificate\",\"sample\":"<<sampleIndex<<",\"requested_error\":"<<error
          <<",\"facet\":"<<closest.facet<<",\"binding\":["<<binding.dimension<<','<<binding.feature<<','<<binding.version<<','<<binding.u<<','<<binding.v
          <<"],\"mobility_dimension\":"<<geometry.MobilityDimension(binding,*mobility)<<",\"physical_support_without_policy\":"<<geometry.PhysicalSupport(binding)
          <<",\"point\":";pointJSON(closest.point);frontLog<<",\"normal\":";pointJSON(closest.normal);
  frontLog<<",\"lower\":"<<closest.lower<<",\"upper\":"<<closest.upper<<",\"query_work\":"<<attempted<<",\"supports\":[";
  geometry.ForFacets(binding,[&](Id side){if(supportCount<64){if(supportCount)frontLog<<',';frontLog<<side;}++supportCount;});
  frontLog<<"],\"support_count\":"<<supportCount<<",\"support_truncated\":"<<(supportCount>64)<<"}"<<std::endl;
  evaluation("after");
  frontLog<<"{\"stage\":\"prepared_target_point_complete\",\"sample\":"<<sampleIndex<<",\"geometry_work\":"<<budget.work-startWork
          <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<"}"<<std::endl;
}
points>>std::ws;REQUIRE(points.eof());
