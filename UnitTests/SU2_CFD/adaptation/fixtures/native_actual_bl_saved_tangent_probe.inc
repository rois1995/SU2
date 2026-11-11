// Saved prepared geometry only: this reports target diagnostics, never repairs or certifies a mesh.
{
  const char* rowPath=std::getenv("SU2_NATIVE_BL_SAVED_PATCH");
  const char* neighborPath=std::getenv("SU2_NATIVE_BL_SAVED_NEIGHBORS");
  if(rowPath||neighborPath) {
    std::ofstream saved("actual_bl_saved_tangent.jsonl");REQUIRE(bool(saved));saved<<std::setprecision(17)<<std::boolalpha;
    auto pointEqual=[](Point a,Point b){return a.x==b.x&&a.y==b.y&&a.z==b.z;};
    auto patchBytes=[](const Patch& p) {
      return transfer_memory::Add(sizeof(Patch),transfer_memory::Bytes(p.cells),transfer_memory::Bytes(p.surface),
          transfer_memory::Mul(p.parameters.size(),sizeof(BoundaryPoints::value_type)+4*sizeof(void*)),
          BoundaryCoverageBytes(p.coverage));
    };
    size_t savedInputBytes=0;
    auto merge=[&](Patch& into,const Patch& from,std::string& why) {
      std::map<Id,Cell> cells;for(const auto& c:into.cells)if(!cells.emplace(c.id,c).second){why="duplicate cell id in first patch";return false;}
      for(const auto& c:from.cells) {
        const auto at=cells.find(c.id);
        if(at!=cells.end()) {
          for(unsigned k=0;k<4;++k)if(at->second.v[k].id!=c.v[k].id||!pointEqual(at->second.v[k].p,c.v[k].p)) {
            why="duplicate cell identity has different ordered nodes";return false;
          }
        } else cells.emplace(c.id,c);
      }
      if(cells.size()>MaximumReplacementCells){why="union exceeds MaximumReplacementCells";return false;}
      into.cells.clear();into.cells.reserve(cells.size());for(const auto& c:cells)into.cells.push_back(c.second);
      std::map<Face,SurfaceFace> surface;
      auto addSurface=[&](const SurfaceFace& f) {
        auto key=f.v;std::sort(key.begin(),key.end());const auto at=surface.find(key);
        if(at==surface.end()){surface.emplace(key,f);return true;}
        if(at->second.facet!=f.facet||at->second.v!=f.v){why="duplicate physical face has different authority/orientation";return false;}
        return true;
      };
      for(const auto& f:into.surface)if(!addSurface(f))return false;
      for(const auto& f:from.surface)if(!addSurface(f))return false;
      into.surface.clear();for(const auto& f:surface)into.surface.push_back(f.second);
      for(const auto& p:from.parameters) {
        const auto at=into.parameters.find(p.first);
        if(at==into.parameters.end()){into.parameters.emplace(p);continue;}
        const auto& a=at->second;const auto& b=p.second;
        const auto& x=a.binding;const auto& y=b.binding;
        if(a.node.id!=b.node.id||!pointEqual(a.node.p,b.node.p)||x.feature!=y.feature||x.version!=y.version||
           x.dimension!=y.dimension||x.u!=y.u||x.v!=y.v) {
          why="duplicate boundary parameter has different coordinate/binding";return false;
        }
      }
      for(const auto& p:from.coverage.faces) {
        const auto at=into.coverage.faces.find(p.first);
        if(at==into.coverage.faces.end()){into.coverage.faces.emplace(p);continue;}
        const auto& a=at->second.pieces;const auto& b=p.second.pieces;
        if(a.size()!=b.size()){why="duplicate face coverage differs";return false;}
        auto ratioSame=[](const SU2NativeGeometry::ExactRatio& x,const SU2NativeGeometry::ExactRatio& y) {
          return x.negative==y.negative&&x.numerator==y.numerator&&x.denominator==y.denominator;
        };
        auto barySame=[&](const SU2NativeGeometry::ExactBary& x,const SU2NativeGeometry::ExactBary& y) {
          return ratioSame(x.u,y.u)&&ratioSame(x.v,y.v);
        };
        for(size_t i=0;i<a.size();++i) {
          if(a[i].facet!=b[i].facet){why="duplicate face coverage authority differs";return false;}
          for(unsigned k=0;k<3;++k)if(!barySame(a[i].reference[k],b[i].reference[k])||!barySame(a[i].active[k],b[i].active[k])) {
            why="duplicate face coverage coordinates differ";return false;
          }
        }
      }
      for(const auto& p:from.coverage.traces) {
        const auto at=into.coverage.traces.find(p.first);
        if(at==into.coverage.traces.end()){into.coverage.traces.emplace(p);continue;}
        const auto& a=at->second.pieces;const auto& b=p.second.pieces;
        if(a.size()!=b.size()){why="duplicate edge trace differs";return false;}
        auto ratioSame=[](const SU2NativeGeometry::ExactRatio& x,const SU2NativeGeometry::ExactRatio& y) {
          return x.negative==y.negative&&x.numerator==y.numerator&&x.denominator==y.denominator;
        };
        auto barySame=[&](const SU2NativeGeometry::ExactBary& x,const SU2NativeGeometry::ExactBary& y) {
          return ratioSame(x.u,y.u)&&ratioSame(x.v,y.v);
        };
        for(size_t i=0;i<a.size();++i)if(a[i].facet!=b[i].facet||!barySame(a[i].first,b[i].first)||
            !barySame(a[i].last,b[i].last)||!ratioSame(a[i].end,b[i].end)) {
          why="duplicate edge trace coordinates differ";return false;
        }
      }
      return true;
    };
    struct PointSamples {Point p;std::vector<CBoundaryLayerMetric::PointSample> full,base;long double weight=0;uint64_t fullVersion=0,baseVersion=0;};
    auto assess=[&](const char* label,const Patch& patch) {
      const size_t savedCaller=budget.caller_bytes;
      const size_t pointCount=transfer_memory::Mul(patch.cells.size(),5);
      const size_t walls=std::max<size_t>(1,policy->full->GetnWall());
      const size_t sampleCount=transfer_memory::Mul(2,transfer_memory::Mul(pointCount,walls));
      const size_t queryScratch=transfer_memory::Add(transfer_memory::Mul(pointCount,sizeof(PointSamples)+sizeof(std::pair<const std::array<double,3>,PointSamples>)+4*sizeof(void*)),
          transfer_memory::GrowthBound(sampleCount,sizeof(CBoundaryLayerMetric::PointSample)),
          transfer_memory::Mul(pointCount,sizeof(std::pair<const std::array<double,3>,Tensor>)+4*sizeof(void*)));
      const size_t extra=transfer_memory::Add(savedInputBytes,patchBytes(patch),queryScratch);
      if(patch.cells.empty()||patch.cells.size()>MaximumReplacementCells||transfer_memory::Add(savedCaller,extra)>ceiling) {
        saved<<"{\"context\":"<<std::quoted(label)<<",\"status\":\"limited\",\"reason\":\"patch size/storage admission\",\"cells\":"<<patch.cells.size()<<"}"<<std::endl;return FirstHeightReport{};
      }
      budget.caller_bytes=transfer_memory::Add(savedCaller,extra);
      struct RestoreCaller {SU2NativeGeometry::AtlasBudget& budget;size_t bytes;~RestoreCaller(){budget.caller_bytes=bytes;}} restoreCaller{budget,savedCaller};
      std::string geometryReason,heightReason;bool geometryPass=true;
      std::unique_ptr<PlanarReference> privateGeometry;
      try {privateGeometry=scope->PrivateParameters(patch,&budget);} catch(const std::exception& e){geometryPass=false;geometryReason=e.what();}
      const auto& ref=privateGeometry?*privateGeometry:*scope;
      std::map<Id,Point> nodes;for(const auto& c:patch.cells)for(const auto& n:c.v)nodes.emplace(n.id,n.p);
      size_t geometryBad=0;double maximumGeometryError=0;
      if(geometryPass)for(const auto& f:patch.surface) {
        std::array<Point,3> tri;for(unsigned k=0;k<3;++k)tri[k]=nodes.at(f.v[k]);
        try {
          if(!ref.ContainsFace(f.facet,tri)){geometryPass=false;++geometryBad;geometryReason="physical face leaves retained support";continue;}
          const auto error=ref.SurfaceError(f.facet,tri);maximumGeometryError=std::max(maximumGeometryError,error);
          if(error>SU2_TYPE::GetValue(config.GetAdap_Hausd())){geometryPass=false;++geometryBad;geometryReason="surface error exceeds configured tolerance";}
        } catch(const std::exception& e){geometryPass=false;++geometryBad;geometryReason=e.what();}
      }
      FirstHeightReport height;bool heightPass=false;
      try {heightPass=CheckFirstHeight(patch,heights,ref,height,heightReason);}catch(const std::exception& e){heightReason=e.what();}

      std::map<std::array<double,3>,PointSamples> samples;
      auto ensurePoint=[&](Point p) {
        const std::array<double,3> key{{p.x,p.y,p.z}};
        if(samples.count(key))return;
        PointSamples q;q.p=p;q.full=policy->full->SamplePoints({p.x,p.y,p.z});q.base=policy->base->SamplePoints({p.x,p.y,p.z});
        q.fullVersion=policy->full->GetGeometrySampleVersion();q.baseVersion=policy->base->GetGeometrySampleVersion();
        q.weight=policy->A->Weight(junction_wp0::Of(p));samples.emplace(key,std::move(q));
      };
      const double originalScale=policy->full->GetTangentialScale();
      try {
       policy->full->SelectTangentialScale(1);policy->Update(9.9999999999999734e-5,SU2_TYPE::GetValue(config.GetAdap_Hgrad()));
       for(const auto& c:patch.cells){ensurePoint(Centroid(c));for(const auto& n:c.v)ensurePoint(n.p);}
       for(double factor:{1.,.75,.5}) {
        policy->full->SelectTangentialScale(factor);policy->Update(9.9999999999999734e-5,SU2_TYPE::GetValue(config.GetAdap_Hgrad()));
        std::map<std::array<double,3>,Tensor> targets;
        for(auto& row:samples) {
          auto& q=row.second;
          if(q.fullVersion!=policy->full->GetGeometrySampleVersion()) {
            q.full=policy->full->SamplePoints({q.p.x,q.p.y,q.p.z});q.fullVersion=policy->full->GetGeometrySampleVersion();
          }
          if(q.baseVersion!=policy->base->GetGeometrySampleVersion()) {
            q.base=policy->base->SamplePoints({q.p.x,q.p.y,q.p.z});q.baseVersion=policy->base->GetGeometrySampleVersion();
          }
          for(auto& s:q.full)policy->full->RecomposeTangentialSample(s.wall,s.sample,policy->core);
          for(auto& s:q.base)policy->base->RecomposeTangentialSample(s.wall,s.sample,policy->core);
          q.weight=policy->A->Weight(junction_wp0::Of(q.p));
          CBoundaryLayerMetric::Tensor m;for(unsigned k=0;k<3;++k)m.m[k][k]=policy->core;
          policy->ApplySamples(q.full,q.base,q.weight,m);
          targets.emplace(row.first,SU2NativeBLPipeline3D::TargetPolicy::Pack(m));
        }
        size_t positive=0,nonpositive=0,wallCells=0,otherCells=0,qBadWall=0,jBadWall=0,qBadOther=0,jBadOther=0;
        double minQWall=INFINITY,minJWall=INFINITY,minQOther=INFINITY,minJOther=INFINITY;
        long double endpointLengthMin=INFINITY,endpointLengthMax=0,centroidLengthMin=INFINITY,centroidLengthMax=0;
        size_t edgeSamples=0;
        for(const auto& c:patch.cells) {
          const Tetrahedron tet{{c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p}};
          if(!(Orientation(tet[0],tet[1],tet[2],tet[3])>0)){++nonpositive;continue;}++positive;
          const auto cp=Centroid(c);CBoundaryLayerMetric::Tensor applied;
          for(unsigned k=0;k<3;++k)applied.m[k][k]=policy->core;
          policy->ApplyPoint(cp,applied);
          const Tensor center=SU2NativeBLPipeline3D::TargetPolicy::Pack(applied);
          const auto measured=Measure(tet,center);bool wallAdjacent=false;
          for(const auto& f:patch.surface) {
            const auto shared=std::count_if(f.v.begin(),f.v.end(),[&](Id id){return std::any_of(c.v.begin(),c.v.end(),[&](Node n){return n.id==id;});});
            if(shared!=3)continue;
            if(heights.count(ref.Get(f.facet).marker)){wallAdjacent=true;break;}
          }
          if(wallAdjacent){++wallCells;minQWall=std::min(minQWall,measured.mean_ratio);minJWall=std::min(minJWall,measured.minimum_scaled_jacobian);
            qBadWall+=measured.mean_ratio<MinimumMeanRatio;jBadWall+=measured.minimum_scaled_jacobian<MinimumScaledJacobian;}
          else {++otherCells;minQOther=std::min(minQOther,measured.mean_ratio);minJOther=std::min(minJOther,measured.minimum_scaled_jacobian);
            qBadOther+=measured.mean_ratio<MinimumMeanRatio;jBadOther+=measured.minimum_scaled_jacobian<MinimumScaledJacobian;}
          for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
            const auto d=junction_wp0::Of(c.v[j].p)-junction_wp0::Of(c.v[i].p);
            const auto norm=[&](Tensor m){return std::sqrt(std::max(0.L,(long double)d.x*(m.xx*d.x+m.xy*d.y+m.xz*d.z)+(long double)d.y*(m.xy*d.x+m.yy*d.y+m.yz*d.z)+(long double)d.z*(m.xz*d.x+m.yz*d.y+m.zz*d.z)));};
            const auto centerLength=norm(center);centroidLengthMin=std::min(centroidLengthMin,(long double)centerLength);centroidLengthMax=std::max(centroidLengthMax,(long double)centerLength);
            const auto& ma=targets.at({c.v[i].p.x,c.v[i].p.y,c.v[i].p.z});const auto& mb=targets.at({c.v[j].p.x,c.v[j].p.y,c.v[j].p.z});
            const auto ea=norm(ma),eb=norm(mb),endLength=std::sqrt((ea*ea+eb*eb)/2);
            endpointLengthMin=std::min(endpointLengthMin,(long double)endLength);endpointLengthMax=std::max(endpointLengthMax,(long double)endLength);++edgeSamples;
          }
        }
        saved<<"{\"context\":"<<std::quoted(label)<<",\"status\":\"measured\",\"tangent_metric_factor\":"<<factor
             <<",\"cells\":"<<patch.cells.size()<<",\"positive_cells\":"<<positive<<",\"nonpositive_cells\":"<<nonpositive
             <<",\"strict_geometry_pass\":"<<geometryPass<<",\"geometry_bad_faces\":"<<geometryBad<<",\"maximum_surface_error\":"<<maximumGeometryError
             <<",\"geometry_reason\":"<<std::quoted(geometryReason)<<",\"strict_first_height_pass\":"<<heightPass
             <<",\"first_height_max_relative_error\":"<<height.maximum_relative_error<<",\"first_height_reason\":"<<std::quoted(heightReason)
             <<",\"wall_adjacent_cells\":"<<wallCells<<",\"wall_q_bad\":"<<qBadWall<<",\"wall_J_bad\":"<<jBadWall
             <<",\"wall_min_q\":"<<(std::isfinite(minQWall)?minQWall:0)<<",\"wall_min_J\":"<<(std::isfinite(minJWall)?minJWall:0)
             <<",\"other_cells\":"<<otherCells<<",\"other_q_bad\":"<<qBadOther<<",\"other_J_bad\":"<<jBadOther
             <<",\"other_min_q\":"<<(std::isfinite(minQOther)?minQOther:0)<<",\"other_min_J\":"<<(std::isfinite(minJOther)?minJOther:0)
             <<",\"edge_occurrences\":"<<edgeSamples<<",\"metric_edge_centroid_constant_min\":"<<(std::isfinite((double)centroidLengthMin)?(double)centroidLengthMin:0)
             <<",\"metric_edge_centroid_constant_max\":"<<(double)centroidLengthMax<<",\"metric_edge_endpoint_tensor_average_min\":"<<(std::isfinite((double)endpointLengthMin)?(double)endpointLengthMin:0)
             <<",\"metric_edge_endpoint_tensor_average_max\":"<<(double)endpointLengthMax
             <<",\"metric_edge_approximation\":\"cell centroid tensor constant and endpoint tensor average\",\"exact_edge_quadrature\":false,\"production_normalized_or_accepted\":false}"<<std::endl;
        CHECK(nonpositive==0);
       }
      } catch(const std::exception& e) {
        saved<<"{\"context\":"<<std::quoted(label)<<",\"status\":\"limited\",\"reason\":"<<std::quoted(e.what())<<",\"factor_measurement_complete\":false}"<<std::endl;
      }
      policy->full->SelectTangentialScale(originalScale);policy->Update(9.9999999999999734e-5,SU2_TYPE::GetValue(config.GetAdap_Hgrad()));
      REQUIRE(geometryPass);
      if(std::string(label)=="saved_prepared_repair_before")CHECK(heightPass);
      return height;
    };
    std::unique_ptr<Patch> row,neighbors;
    if(rowPath)row=std::make_unique<Patch>(TransactionPatch(rowPath));
    if(neighborPath)neighbors=std::make_unique<Patch>(TransactionPatch(neighborPath));
    savedInputBytes=transfer_memory::Add(row?patchBytes(*row):0,neighbors?patchBytes(*neighbors):0);
    FirstHeightReport rowHeight;
    if(row)rowHeight=assess("saved_prepared_repair_before",*row);
    else saved<<"{\"context\":\"saved_prepared_repair_before\",\"status\":\"limited\",\"reason\":\"SU2_NATIVE_BL_SAVED_PATCH absent\"}"<<std::endl;
    if(row&&neighbors) {
      std::string why;
      const size_t unionScratch=transfer_memory::GrowthBound(row->cells.size()+neighbors->cells.size(),sizeof(std::pair<const Id,Cell>)+4*sizeof(void*));
      if(transfer_memory::Add(budget.caller_bytes,transfer_memory::Mul(2,patchBytes(*row)),
          transfer_memory::Mul(2,patchBytes(*neighbors)),unionScratch)>ceiling)why="union inputs exceed storage admission";
      Patch joined;
      if(why.empty())joined=*row;
      if(why.empty() && merge(joined,*neighbors,why)) {
        std::map<Id,bool> rowIds;for(const auto& c:row->cells)rowIds[c.id]=true;
        size_t neighborWall=0,neighborOther=0;
        for(const auto& c:joined.cells)if(!rowIds.count(c.id)) {
          bool wall=false;for(const auto& f:joined.surface) {
            const auto shared=std::count_if(f.v.begin(),f.v.end(),[&](Id id){return std::any_of(c.v.begin(),c.v.end(),[&](Node n){return n.id==id;});});
            if(shared==3 && heights.count(scope->Get(f.facet).marker)){wall=true;break;}
          }
          (wall?neighborWall:neighborOther)++;
        }
        const auto joinedHeight=assess("saved_prepared_repair_before_plus_neighbor_star",joined);
        saved<<"{\"context\":\"saved_neighbor_height_delta\",\"neighbor_only_wall_adjacent_cells\":"<<neighborWall
             <<",\"neighbor_only_other_cells\":"<<neighborOther<<",\"row_wall_faces\":"<<rowHeight.wall_faces
             <<",\"union_wall_faces\":"<<joinedHeight.wall_faces<<",\"row_height_failures\":"<<rowHeight.height_failures
             <<",\"union_height_failures\":"<<joinedHeight.height_failures<<",\"row_missing_cells\":"<<rowHeight.missing_cells
             <<",\"union_missing_cells\":"<<joinedHeight.missing_cells<<",\"union_worst_cell\":"<<joinedHeight.worst_cell
             <<",\"height_delta_available\":"<<(rowHeight.measured&&joinedHeight.measured)
             <<",\"worst_cell_is_neighbor_only\":"<<(joinedHeight.height_failures+joinedHeight.missing_cells>0&&!rowIds.count(joinedHeight.worst_cell))<<"}"<<std::endl;
      } else saved<<"{\"context\":\"saved_prepared_repair_before_plus_neighbor_star\",\"status\":\"limited\",\"reason\":"<<std::quoted(why)<<"}"<<std::endl;
    } else saved<<"{\"context\":\"saved_prepared_repair_before_plus_neighbor_star\",\"status\":\"limited\",\"reason\":\"both saved patch paths are required\"}"<<std::endl;
    saved.close();REQUIRE(bool(saved));
  }
}
