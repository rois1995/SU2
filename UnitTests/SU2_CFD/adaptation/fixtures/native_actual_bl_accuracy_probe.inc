// Test-only physical-chord integrand, measured by the unchanged production G7/K15 engine.
// No result from this diagnostic supplies a construction decision or final acceptance value.
auto gauss10=[&](auto&& value) {
  constexpr long double node[5]={.148874338981631210884826001129720L,.433395394129247190799265943165784L,
      .679409568299024406234327365114874L,.865063366688984510732096688423493L,.973906528517171720077964012084452L};
  constexpr long double weight[5]={.295524224714752870173892994651338L,.269266719309996355091226921569469L,
      .219086362515982043995534934228163L,.149451349150580593145776339657697L,.066671344308688137593568809893332L};
  long double total=0;for(unsigned j=0;j<5;++j)total+=weight[j]*(value((1-node[j])/2)+value((1+node[j])/2));
  return total/2;
};
CHECK(std::abs(gauss10([](long double){return 1.L;})-1)<1e-17L);
CHECK(std::abs(gauss10([](long double t){return std::pow(t,18);})-1.L/19)<1e-17L);
// Along an original mesh edge, every incident original P1 donor has the same endpoint interpolation.
bool originalEdge=false;for(const auto& row:imported.cells) {
  bool first=false,last=false;for(const auto& n:row.second.cell.v){first=first||n.id==edge[0];last=last||n.id==edge[1];}
  originalEdge=originalEdge||(first&&last);
}
REQUIRE(originalEdge);
const auto sensorA=coreAt({edge[0],a}),sensorB=coreAt({edge[1],b});
const auto prepStart=std::chrono::steady_clock::now();const auto prepWork=budget.work;
const auto resolved=policy->Edges()(a,b);
frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"accuracy_preparation\",\"cuts\":"<<resolved.cuts.size()
        <<",\"geometry_work\":"<<budget.work-prepWork<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-prepStart).count()<<"}"<<std::endl;
auto speed=[&](long double t) {
  const Point p{double((1-t)*a.x+t*b.x),double((1-t)*a.y+t*b.y),double((1-t)*a.z+t*b.z)};
  const auto blend=[&](double x,double y){return double((1-t)*x+t*y);};
  const Tensor sensor{blend(sensorA.xx,sensorB.xx),blend(sensorA.xy,sensorB.xy),blend(sensorA.xz,sensorB.xz),
      blend(sensorA.yy,sensorB.yy),blend(sensorA.yz,sensorB.yz),blend(sensorA.zz,sensorB.zz)};
  const auto target=resolved.pinned?resolved.pinned(t,p,sensor):policy->Composition()(p,sensor);
  ValidateTensor(target);return MetricNorm(a,b,target);
};
size_t widest=0;for(size_t j=1;j+1<resolved.cuts.size();++j)
  if(resolved.cuts[j+1]-resolved.cuts[j]>resolved.cuts[widest+1]-resolved.cuts[widest])widest=j;
const auto wholeStart=std::chrono::steady_clock::now();const auto wholeWork=budget.work;
const auto whole10=gauss10(speed);
frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"accuracy_fixed10_whole\",\"length\":"<<whole10
        <<",\"split_estimate\":"<<(whole10>MaximumMetricEdge)<<",\"coarsen_estimate\":"<<(whole10<.9)
        <<",\"geometry_work\":"<<budget.work-wholeWork<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-wholeStart).count()
        <<",\"crosses_cuts\":true,\"certified\":false}"<<std::endl;
for(bool whole:{false,true}) {
  const long double low=whole?0:resolved.cuts[widest],high=whole?1:resolved.cuts[widest+1];
  size_t evaluations=0;const auto workStart=budget.work;const auto clockStart=std::chrono::steady_clock::now();
  auto localSpeed=[&](long double t){++evaluations;return (high-low)*speed(low+t*(high-low));};
  const auto small=whole?whole10:gauss10(localSpeed);evaluations=0;
  // Reparameterize the actual speed onto a unit chord to reuse the existing integration engine and tolerances.
  DonorCell unit;unit.cell.id=0;unit.cell.v={Node{0,{0,0,0}},Node{1,{1,0,0}},Node{2,{0,1,0}},Node{3,{0,0,1}}};
  for(auto& sensor:unit.sensor)sensor={1,0,0,1,0,1};
  auto composition=[&](Point p,Tensor){const auto n=localSpeed(p.x);return Tensor{double(n*n),0,0,1,0,1};};
  auto geometry=[&](Point,Point) {
    std::vector<double> cuts{0};for(double t:resolved.cuts)if(t>low&&t<high)cuts.push_back(double((t-low)/(high-low)));
    cuts.push_back(1);std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());return EdgeGeometry{std::move(cuts)};
  };
  FrozenField numerical({unit},composition,true,{},geometry);
  try {
    const auto referenceLength=numerical.TargetEdgeLength({0,0,0},{1,0,0});
    const auto relativeError=std::abs(small-referenceLength)/referenceLength;
    frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"accuracy_comparison\",\"whole_chord\":"<<whole
            <<",\"parameter_interval\":["<<low<<','<<high<<"],\"fixed10\":"<<small<<",\"existing_engine\":"<<referenceLength
            <<",\"relative_difference\":"<<relativeError<<",\"reference_evaluations\":"<<evaluations
            <<",\"split_decision_differs\":"<<((small>MaximumMetricEdge)!=(referenceLength>MaximumMetricEdge))
            <<",\"coarsen_decision_differs\":"<<((small<.9)!=(referenceLength<.9))
            <<",\"geometry_work\":"<<budget.work-workStart<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-clockStart).count()
            <<",\"scalar_reparameterization\":true,\"numerical_reference_not_enclosure\":true,\"used_for_acceptance\":false}"<<std::endl;
  } catch(const std::exception& e) {
    frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"accuracy_reference_refusal\",\"whole_chord\":"<<whole
            <<",\"reason\":"<<std::quoted(e.what())<<",\"reference_evaluations\":"<<evaluations
            <<",\"geometry_work\":"<<budget.work-workStart<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-clockStart).count()
            <<",\"fixed10_error_known\":false}"<<std::endl;
    throw;
  }
}
