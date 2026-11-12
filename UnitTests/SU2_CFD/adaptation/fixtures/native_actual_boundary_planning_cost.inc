// Independent nested component calls, not additive phase timers or certificates.
const auto acceptedParameters=current.live_parameters->Payload();
for(const auto& entry:contexts) {
  const auto patch=junction_wp0::PatchOf(current,entry.second);
  std::vector<OwnedCell> rows;for(Id id:entry.second)rows.push_back(current.mesh.at(id));
  auto imported=source.ImportCellsLocal(patch.cells,rows,ceiling,retained);REQUIRE(imported.valid);
  const auto components=AssociatedExtensions(imported.cells,*current.reference);
  FrozenField field(std::move(imported.cells),{},true,components,{},identity,true,MaximumDonors,true);
  auto privateGeometry=current.reference->PrivateParameters(patch);
  const auto& geometry=privateGeometry?*privateGeometry:*current.reference;
  std::map<Id,Node> nodes;for(const auto& cell:patch.cells)for(auto node:cell.v)nodes.emplace(node.id,node);
  std::map<Id,std::vector<BoundaryMetricSample>> witnesses;
  for(unsigned repeat=0;repeat<2;++repeat) {
    struct Cost {size_t calls=0,work=0;double wall=0,cpu=0;};
    std::map<std::string,Cost> costs;
    auto timed=[&](const char* name,auto query) {
      auto& cost=costs[name];const auto work=budget.work;const auto cpu=std::clock();
      const auto t=std::chrono::steady_clock::now();query();
      cost.wall+=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();
      cost.cpu+=double(std::clock()-cpu)/CLOCKS_PER_SEC;cost.work+=budget.work-work;++cost.calls;
    };
    std::vector<Node> order;for(const auto& row:nodes)order.push_back(row.second);
    if(repeat)std::reverse(order.begin(),order.end());
    const auto storageBefore=policy.base->StorageBytes();size_t curvedSides=0,sampleCount=0;
    for(auto node:order) {
      const auto* point=geometry.Parameter(node.p);if(!point)continue;
      const auto sensor=field.Sensor(node.p);
      auto sides=[&]() {
        geometry.Parameters()->Geometry().ForFacets(point->binding,[&](Id facet) {
          const auto& face=geometry.Get(facet);if(!heights.count(face.marker))return;
          if(!geometry.Parameters()->Geometry().Curves().count(facet))return;
          for(unsigned short wall=0;wall<policy.base->GetnWall();++wall) {
            if(!policy.base->OwnsNativeFacet(wall,facet,reference->marker_names.at(face.marker)))continue;
            ++curvedSides;
            timed("storage_accounting",[&](){REQUIRE(policy.base->StorageBytes()>0);});
            timed("mapped_surface_differential",[&](){
              const auto value=geometry.Parameters()->Geometry().EvaluateSurface(facet,point->binding);
              REQUIRE(std::isfinite(value.point.x));REQUIRE(std::isfinite(value.normal.x));
            });
            timed("associated_wall_sample",[&](){
              const auto sample=policy.base->EvaluateAtSurfaceAssociation(wall,facet,point->binding,policy.core);
              REQUIRE(std::isfinite(SU2_TYPE::GetValue(sample.normal[0])));
            });
            timed("intrinsic_composed_guidance",[&](){
              const auto value=policy.IntrinsicSurfaceGuidance(wall,facet,node,geometry,sensor);ValidateTensor(value.tensor);
            });
          }
        });
      };
      auto full=[&]() {
        std::vector<BoundaryMetricSample> legacy;
        timed("boundary_guidance_before_strip_reuse",[&](){
          legacy=native_boundary_guidance_test::BeforeStripReuse(policy,node,geometry,sensor,reference->marker_names);
        });
        timed("complete_boundary_guidance",[&](){
          const auto result=policy.BoundaryGuidance(node,geometry,sensor,reference->marker_names);
          REQUIRE(result.size()==legacy.size());
          for(size_t i=0;i<result.size();++i) {
            REQUIRE(sameTensor(result[i].tensor,legacy[i].tensor));
            REQUIRE(samePoint(result[i].normal,legacy[i].normal));
            REQUIRE(samePoint(result[i].strip_rule,legacy[i].strip_rule));
          }
          sampleCount+=result.size();const auto inserted=witnesses.emplace(node.id,result);
          if(!inserted.second) {
            REQUIRE(inserted.first->second.size()==result.size());
            for(size_t i=0;i<result.size();++i) {
              REQUIRE(sameTensor(inserted.first->second[i].tensor,result[i].tensor));
              REQUIRE(samePoint(inserted.first->second[i].normal,result[i].normal));
              REQUIRE(samePoint(inserted.first->second[i].strip_rule,result[i].strip_rule));
            }
          }
        });
      };
      if(repeat){full();sides();}else{sides();full();}
    }
    REQUIRE(curvedSides>0);REQUIRE(sampleCount>0);
    for(const auto& row:costs) {
      const auto& cost=row.second;
      log<<"{\"context\":"<<std::quoted(entry.first)<<",\"repeat\":"<<repeat<<",\"component\":"<<std::quoted(row.first)
         <<",\"cells\":"<<patch.cells.size()<<",\"nodes\":"<<nodes.size()<<",\"curved_sides\":"<<curvedSides
         <<",\"strip_pairs\":"<<policy.A->strips.size()<<",\"samples\":"<<sampleCount<<",\"calls\":"<<cost.calls<<",\"wall_seconds\":"<<cost.wall<<",\"cpu_seconds\":"<<cost.cpu
         <<",\"geometry_work\":"<<cost.work<<",\"storage_before\":"<<storageBefore<<",\"storage_after\":"<<policy.base->StorageBytes()
         <<",\"separate_nested_invocations\":true,\"mesh_accepted\":false}"<<std::endl;
      REQUIRE(bool(log));
    }
  }
}
REQUIRE(current.live_parameters->Payload()==acceptedParameters);
NativeStageCapture::WriteOwned("correction_unchanged_owned.bin",current.mesh);
