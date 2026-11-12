// Existing projected-foot and Cone diagnostic, on the complete current coupled support.
std::vector<Id> ids;for(const auto& row:complete.cells)ids.push_back(row.cell.id);
const auto old=junction_wp0::PatchOf(current,ids);Patch split;std::string splitReason;
REQUIRE(directory.Reserve(complete,true,reason,ceiling,retained));
if(SplitSurfaceStrip(old,edge,point,{},nextCell,*current.reference,split,splitReason)) {
  auto privateGeometry=current.reference->PrivateParameters(split);const auto& geometry=privateGeometry?*privateGeometry:*current.reference;
  const Id targetFacet=entry.first=="recorded_edge_r5"?16436:entry.first=="smooth_control"?26617:
      entry.first=="leading_edge_control"?10692:47715;
  const auto found=std::find_if(split.surface.begin(),split.surface.end(),[&](const SurfaceFace& f){return f.facet==targetFacet;});
  REQUIRE(found!=split.surface.end());const auto face=*found;
  std::map<Id,Node> coordinates;for(const auto& cell:split.cells)for(auto n:cell.v)coordinates.emplace(n.id,n);
  std::array<Point,3> corners;for(unsigned k=0;k<3;++k)corners[k]=coordinates.at(face.v[k]).p;
  std::array<long double,3> u{(long double)corners[1].x-corners[0].x,(long double)corners[1].y-corners[0].y,(long double)corners[1].z-corners[0].z},
      v{(long double)corners[2].x-corners[0].x,(long double)corners[2].y-corners[0].y,(long double)corners[2].z-corners[0].z};
  std::array<long double,3> n{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
  const auto norm=std::hypot(n[0],n[1],n[2]);REQUIRE(norm>0);for(auto& x:n)x/=norm;
  const double h=heights.at(geometry.Get(face.facet).marker);
  const Point center{double(((long double)corners[0].x+corners[1].x+corners[2].x)/3),
      double(((long double)corners[0].y+corners[1].y+corners[2].y)/3),double(((long double)corners[0].z+corners[1].z+corners[2].z)/3)};
  const Point flat{double((long double)center.x-h*n[0]),double((long double)center.y-h*n[1]),double((long double)center.z-h*n[2])};
  const auto physical=PhysicalFirstHeightProposals(face.facet,corners,h,geometry);
  for(unsigned mode=0;mode<2;++mode) {
    const Point proposed=mode?physical[0]:flat;const char* operation=mode?"ProjectedFootCone":"FlatFootCone";
    const long double plane=-(((long double)proposed.x-corners[0].x)*n[0]+((long double)proposed.y-corners[0].y)*n[1]+((long double)proposed.z-corners[0].z)*n[2])/h;
    for(double ratio:{1e-4,1e-6,1e-8}) {
      size_t attempted=0;bool charged=false;
      try {
        const auto query=geometry.Parameters()->Geometry().ClosestCurved(face.facet,proposed,h*ratio,
            std::min(SU2NativeGeometry::MappedFacetReference::ClearanceWork,budget.maximum_work-budget.work),
            budget.maximum_bytes-budget.caller_bytes,{},&attempted,&budget);
        charged=true;REQUIRE(budget.ChargeWork(attempted));
        const long double witness=-(((long double)proposed.x-query.point.x)*query.normal.x+
            ((long double)proposed.y-query.point.y)*query.normal.y+((long double)proposed.z-query.point.z)*query.normal.z)/h;
        log<<"{\"stage\":\"apex_probe\",\"context\":"<<std::quoted(entry.first)<<",\"operator\":"<<std::quoted(operation)
           <<",\"facet\":"<<face.facet<<",\"point\":["<<proposed.x<<','<<proposed.y<<','<<proposed.z<<"],\"precision_over_h\":"<<ratio
           <<",\"plane_height_ratio\":"<<plane<<",\"distance_lower_over_h\":"<<query.lower/h<<",\"distance_upper_over_h\":"<<query.upper/h
           <<",\"clearance_lower_over_h\":"<<query.clearance_lower/h<<",\"signed_candidate_over_h\":"<<witness
           <<",\"candidate_is_sign_certificate\":false,\"work\":"<<attempted<<"}"<<std::endl;
      }catch(const std::exception& e) {
        if(!charged)budget.ChargeWork(attempted);
        log<<"{\"stage\":\"apex_probe_refused\",\"context\":"<<std::quoted(entry.first)<<",\"operator\":"<<std::quoted(operation)
           <<",\"precision_over_h\":"<<ratio<<",\"reason\":"<<junction_wp0::Json(e.what())<<"}"<<std::endl;break;
      }
    }
    const auto owners=Incidence(split.cells).FaceCells(face.v);REQUIRE(owners.size()==1);
    const auto cell=std::find_if(split.cells.begin(),split.cells.end(),[&](const Cell& c){return c.id==owners[0];});REQUIRE(cell!=split.cells.end());
    std::vector<Cell> children;std::string why;const bool positive=Cone({*cell},{nextNode+1,proposed},nextCell+MaximumReplacementCells,children,why);
    bool interface=false,parameters=false,contact=true,protocol=false,preserved=false;
    std::string interfaceReason,parameterReason,contactReason,preservationReason;
    Patch candidate=split;
    if(positive) {
      candidate.cells.erase(candidate.cells.begin()+(cell-split.cells.begin()));candidate.cells.insert(candidate.cells.end(),children.begin(),children.end());
      for(size_t i=0;i<candidate.cells.size();++i)candidate.cells[i].id=nextCell+MaximumReplacementCells+i;
      REQUIRE(candidate.cells.size()<=MaximumReplacementCells);
      interface=ValidateFixedInterface(split.cells,candidate.cells,interfaceReason);
      parameters=current.live_parameters->CheckReplacement(old,candidate,parameterReason,ceiling,budget.caller_bytes,&budget);
      protocol=directory.Clearance(current.mesh,old,candidate,contact,contactReason,ceiling,retained);
      preserved=PreservePassingFirstHeights(old,candidate,heights,*current.reference,preservationReason);
      observe(candidate,complete.cells,operation,"fresh");
    }
    log<<"{\"stage\":\"apex_candidate\",\"context\":"<<std::quoted(entry.first)<<",\"operator\":"<<std::quoted(operation)
       <<",\"positive\":"<<positive<<",\"fixed_split_interface\":"<<interface<<",\"parameter_preflight\":"<<parameters
       <<",\"contact_protocol\":"<<protocol<<",\"contact\":"<<contact<<",\"passing_heights_preserved\":"<<preserved
       <<",\"old_cells\":"<<old.cells.size()<<",\"new_cells\":"<<(positive?candidate.cells.size():0)
       <<",\"reason\":"<<junction_wp0::Json(why)<<",\"interface_reason\":"<<junction_wp0::Json(interfaceReason)
       <<",\"parameter_reason\":"<<junction_wp0::Json(parameterReason)<<",\"contact_reason\":"<<junction_wp0::Json(contactReason)
       <<",\"preservation_reason\":"<<junction_wp0::Json(preservationReason)<<",\"production_admitted\":false,\"mesh_accepted\":false}"<<std::endl;
    REQUIRE(current.live_parameters->Payload()==acceptedParameters);
    nextNode+=MaximumReplacementCells+2;point.id=nextNode;nextCell+=2*MaximumReplacementCells;
  }
} else log<<"{\"stage\":\"apex_split_refused\",\"context\":"<<std::quoted(entry.first)<<",\"reason\":"<<junction_wp0::Json(splitReason)<<"}"<<std::endl;
