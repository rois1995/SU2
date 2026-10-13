TEST_CASE("Curved partition-interface publication preserves frozen source on MPI veto", "[NativeCurvedTransaction3D]") {
  World parent;const int m=GENERATE(1,2,4);if(m>parent.size)return;
  MPI_Comm subset=MPI_COMM_NULL;MPI_Comm_split(parent.comm,parent.rank<m?0:MPI_UNDEFINED,parent.rank,&subset);
  bool valid=true;
  if(subset!=MPI_COMM_NULL) {
    World world(subset);const std::array<Node,5> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{.5,.5,-1}},{4,{1,1,0}}}};
    Patch old;
    for(auto v:{std::array<Node,4>{nodes[0],nodes[1],nodes[2],nodes[3]},std::array<Node,4>{nodes[1],nodes[4],nodes[2],nodes[3]}}) {
      if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);old.cells.push_back({Id(old.cells.size()),v});
    }
    std::vector<Facet> facets;std::map<Id,AnalyticSurface> curves;
    AnalyticSurface g;g.kind=AnalyticSurface::Kind::Bump;g.extent={1,1,.1};
    for(auto f:Incidence(old.cells).Boundary()) {
      const auto id=Id(facets.size());const bool roof=std::find(f.begin(),f.end(),3)==f.end();
      facets.push_back({id,roof?0:1,{nodes[f[0]],nodes[f[1]],nodes[f[2]]}});old.surface.push_back({f,id});if(roof)curves.emplace(id,g);
    }
    const PlanarReference geometry(facets,SIZE_MAX,curves);Owned owned=Partition(Rows(old),world);
    const Tensor sensor{.5,0,0,.5,0,.5};std::vector<DonorCell> complete;
    for(const auto& c:old.cells){DonorCell d;d.cell=c;d.sensor.fill(sensor);complete.push_back(d);}
    // Declare the finite graph extension before constructing or freezing any private proposal.
    for(const auto& f:old.surface)if(curves.count(f.facet)) {
      std::array<Node,3> a{{nodes[f.v[0]],nodes[f.v[1]],nodes[f.v[2]]}};std::sort(a.begin(),a.end(),[](Node x,Node y){return x.id<y.id;});auto b=a;
      for(auto& n:b){n.id+=100;n.p.z=std::nextafter(.1/16+2*g.tolerance,INFINITY);}
      for(auto v:std::array<std::array<Node,4>,3>{{{a[0],a[1],a[2],b[2]},{a[0],a[1],b[1],b[2]},{a[0],b[0],b[1],b[2]}}}) {
        if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);DonorCell d;d.cell={Id(1000+complete.size()),v};d.sensor.fill(sensor);d.extension_component=geometry.Canonical(f.facet);complete.push_back(d);
      }
    }
    std::vector<DonorCell> local;for(auto d:complete)if(d.cell.id%world.size==unsigned(world.rank))local.push_back(d);
    auto source_bytes=[&](){std::vector<char> bytes;RecordStream s(bytes);for(auto d:local)s(d);return bytes;};const auto frozen=source_bytes();
    Directory directory(world);std::string reason;REQUIRE(directory.Build(owned,reason));const auto mesh_before=Bytes(owned),directory_before=directory.Snapshot();
    const auto deps=directory.Lookup(Query(world,{{1,2,0},2},true));REQUIRE(deps.valid);REQUIRE(deps.cells.size()==2);
    if(world.size>1){std::set<int> owners;for(auto d:deps.cells)owners.insert(d.owner);CHECK(owners.size()==2);}
    auto stale=deps;if(!world.rank)++stale.cells.front().version;CHECK_FALSE(directory.Fetch(stale,owned).valid);
    CHECK(Bytes(owned)==mesh_before);CHECK(directory.Snapshot()==directory_before);CHECK(source_bytes()==frozen);
    const auto imported=directory.Fetch(deps,owned);REQUIRE(imported.valid);const bool chosen=directory.Reserve(imported,true,reason);CHECK(chosen==(!world.rank));
    DonorImport service(world);REQUIRE(service.Build(local,reason));const auto payload=service.Import({DonorRegion{0,0,-1,1,1,.00626}});REQUIRE(payload.valid);
    FrozenField field(payload.cells,{},false,AssociatedExtensions(payload.cells,geometry));Patch fresh;bool approved=!chosen;
    if(chosen) {
      approved=SplitEdge(PatchOf(imported.cells),{1,2},{5,{.5,.5,0}},10000,geometry,fresh,reason);
      PatchMetric measured;if(approved)approved=ValidateMetricChange(old.cells,fresh.cells,MetricChange::GeometryRefine,field,measured,reason);
      auto error=[&](const Patch& p){long double sum=0;for(auto f:p.surface){std::array<Point,3> q{};for(size_t k=0;k<3;++k)for(auto c:p.cells)for(auto n:c.v)if(n.id==f.v[k])q[k]=n.p;const auto e=std::max(0.,geometry.SurfaceError(f.facet,q)-.003);const auto a=q[0],b=q[1],c=q[2];const auto area=std::hypot((b.y-a.y)*(c.z-a.z)-(b.z-a.z)*(c.y-a.y),(b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z),(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x));sum+=e*e*area;}return sum;};
      if(approved)approved=error(fresh)<error(old);INFO(reason);CHECK(approved);
    }
    valid=!world.maximum(!approved);REQUIRE(valid);
    std::vector<std::vector<Dependency>> remove_to(world.size);std::vector<std::vector<OwnedCell>> add_to(world.size);
    if(chosen){for(auto d:deps.cells)remove_to[d.owner].push_back(d);for(auto c:Rows(fresh))add_to[c.cell.id%world.size].push_back(c);}
    const auto removed=world.exchange(remove_to);const auto added=world.exchange(add_to);
    CHECK_FALSE(directory.Publish(owned,removed,added,world.rank!=0,reason));CHECK(Bytes(owned)==mesh_before);CHECK(directory.Snapshot()==directory_before);CHECK(source_bytes()==frozen);
    CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,world.rank?SIZE_MAX:1));CHECK(Bytes(owned)==mesh_before);CHECK(directory.Snapshot()==directory_before);CHECK(source_bytes()==frozen);
    REQUIRE(directory.Publish(owned,removed,added,true,reason));CHECK(source_bytes()==frozen);
    const auto final=directory.Lookup(Query(world,{{5,0,0},1},true));REQUIRE(final.valid);CHECK(final.cells.size()==4);
    const auto returned=directory.Fetch(final,owned);REQUIRE(returned.valid);CHECK(returned.cells.size()==4);
    for(const auto& c:returned.cells)for(auto n:c.cell.v)if(n.id==5)CHECK(n.p.z==Approx(.00625).epsilon(1e-12));
    CHECK(field.Query({.5,.5,.00625}).sensor.xx==.5);CHECK(source_bytes()==frozen);
  }
  CHECK_FALSE(parent.maximum(!valid));if(subset!=MPI_COMM_NULL)MPI_Comm_free(&subset);
}
