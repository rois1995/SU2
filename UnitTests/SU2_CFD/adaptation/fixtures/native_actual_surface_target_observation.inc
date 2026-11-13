// Observational fixed-authority support coverage; never changes production admission or target.
const auto* nosePath=std::getenv("SU2_NATIVE_NOSE_FIXED_FACETS");
std::map<Id,unsigned> noseFacets;
constexpr size_t noseBytes=size_t(8)<<20;
if(nosePath) {
  std::ifstream input(nosePath);size_t count=0;input>>count;REQUIRE((count>0&&count<=8192));
  for(size_t k=0;k<count;++k){Id facet=0;unsigned mask=0;input>>facet>>mask;REQUIRE(bool(input));REQUIRE((mask>0&&mask<8));REQUIRE(noseFacets.emplace(facet,mask).second);}
  input>>std::ws;REQUIRE(input.eof());
}
std::ofstream noseLog;
if(nosePath){noseLog.open("nose_target_observation.jsonl");REQUIRE(bool(noseLog));noseLog<<std::setprecision(17)<<std::boolalpha;}
auto noseObserve=[&](const Owned& mesh,const PlanarReference& context,const char* side) {
  if(!nosePath)return;
  const auto started=std::chrono::steady_clock::now();const auto initialWork=budget.work;
  auto observation=budget;
  auto observationLease=policy.ReserveGeometry(observation);observation.caller_bytes=options.caller_bytes;
  const auto savedWithin=observation.within_time;
  observation.within_time=[&](){return (!savedWithin||savedWithin()) && std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<30.;};
  bool complete=true;size_t done=0,total=0,missingFacets=0;std::string refusal;
  struct Row {Face face;Id facet;unsigned mask;std::array<Point,3> points;};
  std::vector<Row> rows;rows.reserve(8192);
  try {
    const auto& parameters=*context.Parameters();const auto& geometry=parameters.Geometry();
    for(const auto& item:mesh) {
      const auto& cell=item.second;const auto faces=junction_wp0::CellFaces(cell.cell);
      for(unsigned k=0;k<4;++k)if(cell.physical[k].present) {
        const auto found=noseFacets.find(cell.physical[k].facet);if(found==noseFacets.end())continue;
        if(rows.size()==8192)throw std::length_error("Fixed nose observation face storage exhausted.");
        Row row{faces[k],found->first,found->second,{}};
        for(unsigned j=0;j<3;++j)for(auto node:cell.cell.v)if(node.id==row.face[j])row.points[j]=node.p;
        rows.push_back(row);
      }
    }
    std::set<Id> covered;for(const auto& row:rows)covered.insert(row.facet);
    for(const auto& item:noseFacets)missingFacets+=!covered.count(item.first);
    if(missingFacets){complete=false;refusal="Fixed authority supports are missing; descendant coverage unresolved.";}
    total=rows.size();std::stable_sort(rows.begin(),rows.end(),[](const Row& a,const Row& b){return bool(a.mask&3)>bool(b.mask&3);});
    for(const auto& row:rows) {
      if(std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()>=30.)
        throw std::length_error("Fixed nose observation reaches30s side observation.");
      observation.CheckTime();const auto* record=context.Parameter(row.points[0]);
      if(!record)throw std::logic_error("Fixed nose face lacks geometry association.");
      const auto entity=parameters.Entity(row.facet);const auto anchor=geometry.Realize(record->binding);
      bool incident=false;geometry.ForFacets(record->binding,[&](Id f){incident=incident||parameters.Entity(f)==entity;});
      auto up=[](double a,double b){return std::nextafter(a+b,INFINITY);};
      double radius=0;Point lo=row.points[0],hi=lo;
      for(auto p:row.points){radius=std::max(radius,SU2NativeGeometry::DisplacementBound(p,anchor));
        lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
      const double tolerance=options.surface_tolerance,precision=tolerance/16;
      radius=up(up(radius,geometry.ErrorBound(record->binding,anchor)),tolerance);
      lo={std::nextafter(lo.x-radius,-INFINITY),std::nextafter(lo.y-radius,-INFINITY),std::nextafter(lo.z-radius,-INFINITY)};
      hi={std::nextafter(hi.x+radius,INFINITY),std::nextafter(hi.y+radius,INFINITY),std::nextafter(hi.z+radius,INFINITY)};
      auto support=geometry.NearbySurfaceFacets(row.facet,{lo,hi},&observation);
      support.erase(std::remove_if(support.begin(),support.end(),[&](Id f){return parameters.Entity(f)!=entity;}),support.end());
      bool scope=incident;
      for(const auto& item:geometry.Reference().Facets())if(parameters.Entity(item.first)==entity) {
        if(!observation.ChargeWork())throw std::length_error("Fixed nose support audit exhausts work.");
        Point low=item.second.v[0].p,high=low;
        auto extend=[&](Point p){low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};};
        const auto curve=geometry.Curves().find(item.first);
        if(curve!=geometry.Curves().end())for(auto p:curve->second)extend(p);
        else for(auto n:item.second.v)extend(n.p);
        const bool intersects=low.x<=hi.x&&high.x>=lo.x&&low.y<=hi.y&&high.y>=lo.y&&low.z<=hi.z&&high.z>=lo.z;
        if(intersects&&!std::binary_search(support.begin(),support.end(),item.first))scope=false;
      }
      if(support.empty())throw std::logic_error("Fixed nose support is empty.");
      const auto upper=geometry.BoundPhysicalSurface(support,{row.points},precision,observation,
          SU2NativeGeometry::MappedFacetReference::SurfaceBoundPurpose::ErrorMagnitude);
      if(!scope){complete=false;refusal="Full entity nearest-support coverage unresolved.";}
      const auto lower=scope&&upper>precision?std::max(0.,std::nextafter(upper-precision,-INFINITY)):0.;
      noseLog<<"{\"stage\":\"fixed_face\",\"side\":"<<std::quoted(side)<<",\"facet\":"<<row.facet<<",\"mask\":"<<row.mask
          <<",\"face\":["<<row.face[0]<<','<<row.face[1]<<','<<row.face[2]<<"],\"scope_complete\":"<<scope
          <<",\"precision\":"<<precision<<",\"lower\":"<<lower<<",\"upper\":"<<upper<<"}"<<std::endl;
      if(row.mask&3) {
        const auto binding=geometry.FacetPoint(row.facet,1./3,1./3);
        for(unsigned short wall=0;wall<policy.base->GetnWall();++wall) {
          if(policy.base->GetLayer(wall).marker!=reference->marker_names.at(context.Get(row.facet).marker))continue;
          {
            const auto sample=policy.base->EvaluateAtSurfaceAssociation(wall,row.facet,binding,policy.core);
            const auto differential=geometry.EvaluateSurface(row.facet,binding);
            const std::array<Point,2> directions{{differential.direction1,differential.direction2}};
            const double curvature[2]={differential.curvature1,differential.curvature2};double demand[2]={};
            for(unsigned principal=0;principal<2;++principal) {
              const auto d=directions[principal];const double v[3]={d.x,d.y,d.z};
              for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)demand[principal]+=v[i]*SU2_TYPE::GetValue(sample.full.m[i][j])*v[j];
              if(curvature[principal]!=0) {
                const long double radius=1/std::abs((long double)curvature[principal]),e=std::min((long double)tolerance,radius);
                const auto cap=std::pow(MaximumMetricEdge/(2*std::sqrt(e*(2*radius-e))),2);
                CHECK(demand[principal]>=cap*(1-1e-8L));
              }
            }
            noseLog<<"{\"stage\":\"principal_probe\",\"side\":"<<std::quoted(side)<<",\"facet\":"<<row.facet<<",\"mask\":"<<row.mask
              <<",\"tangent_scale\":"<<policy.base->GetTangentialScale()<<",\"curvatures\":["<<differential.curvature1<<','<<differential.curvature2
              <<"],\"directions\":[["<<differential.direction1.x<<','<<differential.direction1.y<<','<<differential.direction1.z
              <<"],["<<differential.direction2.x<<','<<differential.direction2.y<<','<<differential.direction2.z
              <<"]],\"directional_demands\":["<<demand[0]<<','<<demand[1]
              <<"],\"normal\":["<<sample.normal[0]<<','<<sample.normal[1]<<','<<sample.normal[2]
              <<"],\"caps_max_abs_first\":["<<sample.curvature<<','<<sample.secondary_curvature<<"],\"direction\":["<<sample.curvature_direction[0]<<','<<sample.curvature_direction[1]<<','<<sample.curvature_direction[2]<<"],\"tensor\":[";
            for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){if(i||j)noseLog<<',';noseLog<<sample.full.m[i][j];}
            noseLog<<"]}"<<std::endl;
          }
        }
      }
      ++done;
    }
  }catch(const std::exception& e){complete=false;refusal=e.what();}
  noseLog<<"{\"stage\":\"fixed_summary\",\"side\":"<<std::quoted(side)<<",\"complete\":"<<complete<<",\"missing_authority_facets\":"<<missingFacets<<",\"faces\":"<<total<<",\"measured\":"<<done
    <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<",\"geometry_work\":"<<observation.work-initialWork<<",\"reason\":"<<std::quoted(refusal)<<"}"<<std::endl;
  budget.work=observation.work;budget.peak_bytes=std::max(budget.peak_bytes,observation.peak_bytes);
  budget.work_exhausted=budget.work_exhausted||observation.work_exhausted;
};
