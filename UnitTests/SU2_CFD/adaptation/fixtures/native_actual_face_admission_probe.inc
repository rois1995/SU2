// Observations follow all constructors; they never change their native admission or publish a candidate.
struct AdmissionFace {
  size_t patch=0;unsigned side=0;Face face{};Id facet=0;
  std::array<Point,3> points{};std::array<SU2NativeGeometry::SurfaceBinding,3> binding{};
  double canonical=0,affine=0;
};
std::vector<AdmissionFace> admissionFaces;
admissionFaces.reserve(3*4*(MaximumCavityCells+MaximumReplacementCells));
std::map<BoundaryParameterState::EntityKey,std::vector<Id>> entityMembers;
const double physical=SU2_TYPE::GetValue(config.GetAdap_Hausd());
auto up=[](double a,double b){return std::nextafter(a+b,INFINITY);};
auto logFace=[&](const Face& ids){log<<'['<<ids[0]<<','<<ids[1]<<','<<ids[2]<<']';};
for(unsigned pass=0;pass<3;++pass) {
  unsigned contextsDone=0,facesDone=0;bool complete=true;std::string refusal;
  try {
    if(admissionPatches.size()!=3)throw std::runtime_error("Face attribution lacks a constructed candidate for a planned context.");
    for(size_t index=0;index<admissionPatches.size();++index) {
      const auto& probe=admissionPatches[index];
      for(unsigned side=0;side<2;++side) {
        const auto& patch=side?probe.fresh:probe.old;const char* label=side?"fresh":"old";
        budget.caller_bytes=transfer_memory::Add(caller,directory.ResidentBytes());
        auto context=trial.reference->PrivateParameters(patch);REQUIRE(context);
        const auto& state=*context->Parameters();const auto& g=state.Geometry();
        REQUIRE(physical==state.GeometryTolerance());
        if(pass==0) {
          std::map<Id,Node> nodes;for(const auto& cell:patch.cells)for(auto node:cell.v)nodes.emplace(node.id,node);
          for(const auto& face:patch.surface) {
            budget.CheckTime();AdmissionFace item;item.patch=index;item.side=side;item.face=face.v;item.facet=face.facet;
            std::array<Point,3> points;for(unsigned k=0;k<3;++k)points[k]=nodes.at(face.v[k]).p;
            std::array<double,3> upper{};
            for(unsigned rotation=0;rotation<3;++rotation) {
              std::array<Point,3> rotated;for(unsigned k=0;k<3;++k)rotated[k]=points[(k+rotation)%3];
              upper[rotation]=context->SurfaceDeviation(face.facet,rotated,true);REQUIRE(std::isfinite(upper[rotation]));
            }
            const unsigned rotation=std::distance(face.v.begin(),std::min_element(face.v.begin(),face.v.end()));
            std::rotate(item.face.begin(),item.face.begin()+rotation,item.face.end());item.canonical=upper[rotation];
            bool direct=true;
            for(unsigned k=0;k<3;++k) {
              item.points[k]=nodes.at(item.face[k]).p;const auto* record=context->Parameter(item.points[k]);REQUIRE(record);
              item.binding[k]=record->binding;direct=direct&&g.Supports(face.facet,item.binding[k]);
              const auto plane=context->Affine().find(face.facet);
              if(plane!=context->Affine().end())item.affine=std::max(item.affine,SU2NativeGeometry::AffineChart(plane->second).ErrorBound(item.points[k]));
            }
            log<<"{\"stage\":\"face_admission_identity\",\"context\":"<<std::quoted(probe.label)<<",\"side\":"<<std::quoted(label)<<",\"face\":";logFace(item.face);
            log<<",\"facet\":"<<item.facet<<",\"direct\":"<<direct<<",\"physical_tolerance\":"<<physical<<",\"canonical_upper\":"<<item.canonical
               <<",\"affine_floor\":"<<item.affine<<",\"rotation_upper\":["<<upper[0]<<','<<upper[1]<<','<<upper[2]<<"],\"vertices\":[";
            for(unsigned k=0;k<3;++k){if(k)log<<',';const auto p=item.points[k];const auto b=item.binding[k];
              log<<'['<<item.face[k]<<','<<p.x<<','<<p.y<<','<<p.z<<','<<b.dimension<<','<<b.feature<<','<<b.version<<','<<b.u<<','<<b.v<<']';}
            log<<"]}"<<std::endl;admissionFaces.push_back(item);++facesDone;
          }
        } else for(const auto& item:admissionFaces)if(item.patch==index&&item.side==side) {
          const auto started=std::chrono::steady_clock::now();const auto work=budget.work;budget.CheckTime();
          const double precision=physical/(pass==1?4:16);double query=item.canonical,lower=0,combined=item.canonical;
          bool scopeComplete=true;size_t missing=0;std::vector<Id> support;
          if(item.canonical>physical) {
            for(unsigned k=0;k<3;++k) {
              const auto* record=context->Parameter(item.points[k]);REQUIRE(record);const auto& a=record->binding;const auto& b=item.binding[k];
              REQUIRE((a.feature==b.feature&&a.version==b.version&&a.dimension==b.dimension&&a.u==b.u&&a.v==b.v));
            }
            const auto entity=state.Entity(item.facet);bool anchorIncident=false;
            g.ForFacets(item.binding[0],[&](Id id){anchorIncident=anchorIncident||state.Entity(id)==entity;});
            const auto anchor=g.Realize(item.binding[0]);
            const double uncertainty=g.ErrorBound(item.binding[0],anchor);
            double radius=0;
            Point lo=item.points[0],hi=lo;
            for(auto p:item.points) {
              radius=std::max(radius,SU2NativeGeometry::DisplacementBound(p,anchor));lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};
            }
            radius=up(up(radius,uncertainty),physical);if(!std::isfinite(radius))throw std::length_error("Nearest-support envelope is unrepresentable.");
            lo={std::nextafter(lo.x-radius,-INFINITY),std::nextafter(lo.y-radius,-INFINITY),std::nextafter(lo.z-radius,-INFINITY)};
            hi={std::nextafter(hi.x+radius,INFINITY),std::nextafter(hi.y+radius,INFINITY),std::nextafter(hi.z+radius,INFINITY)};
            support=g.NearbySurfaceFacets(item.facet,{lo,hi},&budget);
            support.erase(std::remove_if(support.begin(),support.end(),[&](Id id){return state.Entity(id)!=entity;}),support.end());
            REQUIRE(std::is_sorted(support.begin(),support.end()));
            auto found=entityMembers.find(entity);
            if(found==entityMembers.end()) {
              std::vector<Id> members;for(const auto& row:g.Reference().Facets()){if(!budget.ChargeWork())throw std::length_error("Entity support audit exhausts work.");if(state.Entity(row.first)==entity)members.push_back(row.first);}
              found=entityMembers.emplace(entity,std::move(members)).first;
            }
            const auto& members=found->second;
            for(Id id:members) {
              if(!budget.ChargeWork(16))throw std::length_error("Entity control-hull audit exhausts work.");
              const auto curve=g.Curves().find(id);if(curve==g.Curves().end()){++missing;continue;}
              Point low=curve->second[0],high=low;for(auto p:curve->second){low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};}
              const bool intersects=low.x<=hi.x&&high.x>=lo.x&&low.y<=hi.y&&high.y>=lo.y&&low.z<=hi.z&&high.z>=lo.z;
              if(intersects&&!std::binary_search(support.begin(),support.end(),id))++missing;
            }
            scopeComplete=anchorIncident&&missing==0;
            if(support.empty())throw std::runtime_error("Nearest-support envelope has no model facets.");
            query=g.BoundPhysicalSurface(support,{item.points},precision,budget,SU2NativeGeometry::MappedFacetReference::SurfaceBoundPurpose::ErrorMagnitude);
            REQUIRE(std::isfinite(query));lower=query>precision?std::max(0.,std::nextafter(query-precision,-INFINITY)):0.;combined=std::min(item.canonical,query);
            REQUIRE((!scopeComplete||lower<=combined));
            if(!scopeComplete)lower=0;
          }
          log<<"{\"stage\":\"face_admission_bound\",\"context\":"<<std::quoted(probe.label)<<",\"side\":"<<std::quoted(label)<<",\"face\":";logFace(item.face);
          log<<",\"facet\":"<<item.facet<<",\"physical_tolerance\":"<<physical<<",\"precision\":"<<precision<<",\"query_upper\":"<<query<<",\"derived_lower\":"<<lower
             <<",\"combined_upper\":"<<combined<<",\"affine_floor\":"<<item.affine<<",\"query_evaluated\":"<<(item.canonical>physical)<<",\"scope_complete\":"<<scopeComplete<<",\"missing_control_hulls\":"<<missing<<",\"support\":[";
          for(size_t k=0;k<support.size();++k){if(k)log<<',';log<<support[k];}
          log<<"],\"geometry_work\":"<<budget.work-work<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<"}"<<std::endl;++facesDone;
        }
      }
      ++contextsDone;
    }
  }catch(const std::exception& error){complete=false;refusal=error.what();}
  log<<"{\"stage\":\"face_admission_pass\",\"pass\":"<<pass<<",\"complete\":"<<complete<<",\"contexts_completed\":"<<contextsDone
     <<",\"faces_completed\":"<<facesDone<<",\"reason\":"<<junction_wp0::Json(refusal)<<"}"<<std::endl;
  if(!complete)break;
}
