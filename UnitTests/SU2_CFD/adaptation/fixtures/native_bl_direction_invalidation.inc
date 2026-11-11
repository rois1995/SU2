TEST_CASE("BL wall direction revisions follow their actual boundary dependencies", "[NativeBLDirectionInvalidation3D]") {
  using namespace junction_wp0;
  auto make=[](bool carrier) {
    auto s=std::make_unique<State>();
    OwnedCell row;row.cell={0,{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}}};
    const auto faces=CellFaces(row.cell);std::vector<Facet> facets;
    for(unsigned k=0;k<4;++k) {
      const bool wall=std::find(faces[k].begin(),faces[k].end(),Id(3))==faces[k].end();
      const bool partner=std::find(faces[k].begin(),faces[k].end(),Id(2))==faces[k].end();
      if(!wall&&!(carrier&&partner))continue;
      const Id facet=wall?0:1;std::array<Node,3> nodes;
      for(unsigned j=0;j<3;++j)for(auto node:row.cell.v)if(node.id==faces[k][j])nodes[j]=node;
      facets.push_back({facet,wall?7:8,nodes});row.physical[k]={true,facet};
    }
    s->walls.insert(7);s->facets=facets;s->reference=std::make_shared<PlanarReference>(facets);
    s->mesh.emplace(0,row);Index(*s);return s;
  };
  SECTION("Interior and apex-only changes retain the revision") {
    auto s=make(false);const auto revision=s->boundary_revision;
    auto fresh=PatchOf(*s,{0});fresh.cells[0].v[3]={13,{.1,.1,2}};
    Apply(*s,{0},fresh);CHECK(s->boundary_revision==revision);
    OwnedCell interior;interior.cell={20,{{{20,{3,0,0}},{21,{4,0,0}},{22,{3,1,0}},{23,{3,0,1}}}}};
    s->mesh.emplace(20,interior);Index(*s);
    const auto indexed=s->boundary_revision;
    Patch moved;moved.cells.push_back(interior.cell);moved.cells[0].v[3]={24,{3,0,2}};
    Apply(*s,{20},moved);CHECK(s->boundary_revision==indexed);
  }
  SECTION("Wall movement changes the revision") {
    auto s=make(false);const auto revision=s->boundary_revision;
    auto fresh=PatchOf(*s,{0});fresh.cells[0].v[2].p.y=2;
    Apply(*s,{0},fresh);CHECK(s->boundary_revision==revision+1);
  }
  SECTION("A failed mutation poisons the unpublished state") {
    auto s=make(false);auto fresh=PatchOf(*s,{0});fresh.cells.push_back(fresh.cells.front());
    CHECK_THROWS(Apply(*s,{0},fresh));CHECK(s->publication_failed);
    CHECK_THROWS(Apply(*s,{0},Patch{}));
  }
  SECTION("Wall topology changes the revision") {
    auto s=make(false);const auto revision=s->boundary_revision;
    auto fresh=PatchOf(*s,{0});fresh.cells[0].v[2].id=12;
    for(auto& face:fresh.surface)for(auto& id:face.v)if(id==2)id=12;
    Apply(*s,{0},fresh);CHECK(s->boundary_revision==revision+1);
  }
  SECTION("The same axis carrier normal remains reusable") {
    auto s=make(true);const auto revision=s->boundary_revision;
    auto fresh=PatchOf(*s,{0});fresh.cells[0].v[3].p.z=2;
    Apply(*s,{0},fresh);CHECK(s->boundary_revision==revision);
    fresh=PatchOf(*s,{0});fresh.cells[0].v[3].p.y=.1;
    Apply(*s,{0},fresh);CHECK(s->boundary_revision==revision+1);
  }
  SECTION("DirectionAt invokes its synchronous refresh") {
    auto s=make(false);size_t calls=0;
    struct Restore {
      std::function<void(State&)> callback=dc9::ensureDirectionField;
      std::map<Id,V>* field=dc9::directionField;
      ~Restore(){dc9::ensureDirectionField=std::move(callback);dc9::directionField=field;}
    } restore;
    dc9::directionField=nullptr;
    dc9::ensureDirectionField=[&](State& current) {
      CHECK(&current==s.get());++calls;current.direction_revision=current.boundary_revision;
    };
    CHECK(dc9::DirectionAt(*s,0).ok);CHECK(calls==1);
    CHECK(s->direction_revision==s->boundary_revision);
    dc9::ensureDirectionField=[](State& current){dc9::DirectionAt(current,0);};
    CHECK_THROWS(dc9::DirectionAt(*s,0));
  }
}

TEST_CASE("BL private publication retains parameter authority and nodal core", "[NativeBLDirectionInvalidation3D]") {
  using namespace junction_wp0;
  State s;OwnedCell original;original.cell={0,{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}}};
  original.sensor_present=true;for(auto& sensor:original.sensor)sensor={2,0,0,3,0,4};
  std::vector<SurfaceFace> surface;
  const auto faces=CellFaces(original.cell);
  for(unsigned k=0;k<4;++k) {
    std::array<Node,3> nodes;
    for(unsigned j=0;j<3;++j)for(auto node:original.cell.v)if(node.id==faces[k][j])nodes[j]=node;
    s.facets.push_back({k,k==3?7:8,nodes});surface.push_back({faces[k],k});original.physical[k]={true,k};
  }
  s.reference=std::make_shared<PlanarReference>(s.facets);
  std::map<Id,double> errors;for(const auto& facet:s.facets)errors.emplace(facet.id,1e-13);
  auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(s.reference,errors,7,SIZE_MAX,true);
  BoundaryPoints points;for(auto node:original.cell.v)points.emplace(node.id,BoundaryPoint{node,authority->Vertex(node.id)});
  s.live_parameters=std::make_shared<BoundaryParameterState>(authority,points,surface);
  s.reference=PlanarReference::WithParameters(s.reference,s.live_parameters);s.walls.insert(7);s.mesh.emplace(0,original);Index(s);
  s.memory_ceiling=size_t(1)<<28;s.retained_bytes=[&]{return s.live_parameters->ResidentBytes();};
  size_t admitted=0;s.admit_scratch=[&](size_t bytes){admitted=std::max(admitted,bytes);};
  auto fresh=PatchOf(s,{0});fresh.cells.clear();const Node center{4,{.25,.25,.25}};
  for(const auto& face:surface) {
    Cell cell;cell.id=10+fresh.cells.size();cell.v[0]=center;
    for(unsigned j=0;j<3;++j)for(auto node:original.cell.v)if(node.id==face.v[j])cell.v[j+1]=node;
    REQUIRE(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);fresh.cells.push_back(cell);
  }
  const auto revision=s.live_parameters->Revision();
  SECTION("Private receipts cannot commit into another state or a later epoch") {
    std::vector<BoundaryFaceChange> changes;
    for(const auto& face:surface){changes.push_back({face.v,face.facet,false});changes.push_back({face.v,face.facet,true});}
    std::string why;auto receipt=s.live_parameters->Prepare({},changes,why);
    REQUIRE(receipt.valid);
    BoundaryParameterState foreign(*s.live_parameters);
    CHECK_THROWS(foreign.CommitPrivate(std::move(receipt)));CHECK(foreign.Revision()==revision);
    Apply(s,{0},fresh);CHECK_THROWS(s.live_parameters->CommitPrivate(std::move(receipt)));
  }
  SECTION("Admission refusal leaves the private state unchanged") {
    s.admit_scratch=[](size_t){throw std::length_error("test scratch refusal");};
    CHECK_THROWS(Apply(s,{0},fresh));CHECK(s.mesh.size()==1);CHECK(s.mesh.count(0)==1);
    CHECK_FALSE(s.publication_failed);
    CHECK(s.live_parameters->Revision()==revision);
  }
  SECTION("Prepared publication carries interpolated core fields") {
    Apply(s,{0},fresh);CHECK(admitted>0);CHECK(s.mesh.size()==4);
    CHECK(s.live_parameters->Revision()>revision);CHECK(s.reference->Parameters()==s.live_parameters.get());
    CHECK(s.live_parameters->Authority()==authority);CHECK(s.live_parameters->Surface().size()==4);CHECK(s.overlay.empty());
    for(const auto& row:s.mesh) {
      REQUIRE(row.second.sensor_present);
      for(const auto& sensor:row.second.sensor)CHECK(SameMetric(sensor,original.sensor[0]));
    }
  }
}

TEST_CASE("A curved first-child cone tries a physically admissible existing LP candidate", "[NativeCurvedFirstChild3D]") {
  using namespace junction_wp0;using namespace SU2NativeGeometry;
  const std::array<Node,4> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{.25,.25,.2}}}};
  const Cell cell{0,nodes};std::vector<Facet> facets;std::vector<SurfaceFace> surface;
  std::vector<Face> skin;Id wall=UINT64_MAX;
  for(const auto& face:Incidence({cell}).Boundary()) {
    std::array<Node,3> triangle;bool bottom=true;
    for(unsigned k=0;k<3;++k){triangle[k]=nodes.at(face[k]);bottom=bottom&&triangle[k].p.z==0;}
    const Id id=facets.size();facets.push_back({id,bottom?7:8,triangle});surface.push_back({face,id});
    if(bottom){wall=id;skin.insert(skin.begin(),face);}else skin.push_back(face);
  }
  REQUIRE(wall!=UINT64_MAX);auto flat=std::make_shared<PlanarReference>(facets);
  MappedFacetReference::CurvedNets nets;std::map<Id,double> errors;
  const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
  for(const auto& facet:facets) {
    errors[facet.id]=1e-12;auto& net=nets[facet.id];
    for(unsigned q=0;q<10;++q){V p{};for(unsigned k=0;k<3;++k)p=p+(powers[q][k]/3.L)*Of(facet.v[k].p);net[q]=Pt(p);}
  }
  nets.at(wall)[9].z+=.006;
  auto authority=std::make_shared<MappedFacetReference>(flat,errors,7,SIZE_MAX,false,nets);
  BoundaryPoints points;for(auto node:nodes)points.emplace(node.id,BoundaryPoint{node,authority->Vertex(node.id)});
  auto parameters=std::make_shared<BoundaryParameterState>(authority,points,surface);
  auto reference=PlanarReference::WithParameters(flat,parameters);
  State state;state.reference=reference;state.heights[7]=.02;state.h0=.02;
  std::map<Id,Node> byId;for(auto node:nodes)byId.emplace(node.id,node);
  const auto cone=dc9::ConeColumn(state,skin,byId,10,false,*reference,wall,.02);
  INFO(cone.witness);REQUIRE(cone.ok);REQUIRE_FALSE(cone.cells.empty());
  Patch first;first.cells={cone.cells.front()};first.surface={{skin.front(),wall}};
  FirstHeightReport measured;std::string reason;
  INFO(reason);CHECK(CheckFirstHeight(first,state.heights,*reference,measured,reason));
  CHECK(measured.measured);CHECK(measured.maximum_relative_error<=.1);CHECK(measured.reference_clearance_failures==0);
  SECTION("A split wall child uses its new binding in the current private context") {
    Patch old;old.cells={cell};old.surface=surface;auto current=reference->PrivateParameters(old);REQUIRE(current);
    std::set<Id> incident;
    for(const auto& facet:facets) {
      bool a=false,b=false;for(auto node:facet.v){a=a||node.id==0;b=b||node.id==1;}
      if(a&&b)incident.insert(facet.id);
    }
    const Edge edge{0,1};const auto midpoint=current->BindNewPoint({4,{.5,0,0}},incident,&edge);
    auto split=cell;split.v[1]=midpoint;
    std::vector<Face> childSkin;Face wallFace{};
    for(auto face:Incidence({split}).Boundary()) {
      if(std::find(face.begin(),face.end(),Id(3))==face.end()){wallFace=face;childSkin.insert(childSkin.begin(),face);}
      else childSkin.push_back(face);
    }
    auto splitNodes=byId;splitNodes.emplace(midpoint.id,midpoint);Id childFacet=UINT64_MAX;
    REQUIRE(current->FindFace(7,{splitNodes.at(wallFace[0]).p,splitNodes.at(wallFace[1]).p,splitNodes.at(wallFace[2]).p},childFacet));
    CHECK_THROWS(dc9::ConeColumn(state,childSkin,splitNodes,11,false,*reference,childFacet,.02));
    const auto child=dc9::ConeColumn(state,childSkin,splitNodes,11,false,*current,childFacet,.02);
    INFO(child.witness);REQUIRE(child.ok);
    Patch firstChild;firstChild.cells={child.cells.front()};firstChild.surface={{wallFace,childFacet}};
    current->ExportParameters(firstChild);
    CHECK(CheckFirstHeight(firstChild,state.heights,*current,measured,reason));CHECK(measured.measured);
  }
}

TEST_CASE("BL private publication uses the parameter state's canonical component anchors", "[NativeBLDirectionInvalidation3D]") {
  using namespace junction_wp0;
  State s;
  const std::array<Node,5> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{1,1,0}},{4,{.5,.5,1}}}};
  std::vector<Cell> cells{{0,{nodes[0],nodes[1],nodes[2],nodes[4]}},{1,{nodes[1],nodes[3],nodes[2],nodes[4]}}};
  std::vector<SurfaceFace> surface;std::map<Face,Id> physical;
  for(const auto& face:Incidence(cells).Boundary()) {
    std::array<Node,3> triangle;bool wall=true;
    for(unsigned k=0;k<3;++k){triangle[k]=nodes.at(face[k]);wall=wall&&triangle[k].p.z==0;}
    const Id facet=s.facets.size();s.facets.push_back({facet,wall?7:8,triangle});surface.push_back({face,facet});physical.emplace(Sorted(face),facet);
  }
  s.reference=std::make_shared<PlanarReference>(s.facets);
  std::map<Id,double> errors;for(const auto& facet:s.facets)errors.emplace(facet.id,1e-13);
  auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(s.reference,errors,7,SIZE_MAX,true);
  BoundaryPoints points;for(auto node:nodes)points.emplace(node.id,BoundaryPoint{node,authority->Vertex(node.id)});
  s.live_parameters=std::make_shared<BoundaryParameterState>(authority,points,surface);
  s.reference=PlanarReference::WithParameters(s.reference,s.live_parameters);s.walls.insert(7);
  bool noncanonical=false;
  for(const auto& cell:cells) {
    OwnedCell row;row.cell=cell;const auto faces=CellFaces(cell);
    for(unsigned k=0;k<4;++k) {
      const auto found=physical.find(Sorted(faces[k]));if(found==physical.end())continue;
      row.physical[k]={true,found->second};noncanonical|=found->second!=s.reference->Canonical(found->second);
    }
    row.sensor_present=true;row.sensor.fill(Tensor{1,0,0,1,0,1});s.mesh.emplace(cell.id,row);
  }
  REQUIRE(noncanonical);Index(s);const auto boundary_revision=s.boundary_revision;
  auto fresh=PatchOf(s,{0,1});for(auto& cell:fresh.cells)cell.id+=100;
  const auto revision=s.live_parameters->Revision();
  SECTION("Equivalent raw anchors publish through exact canonical topology") {
    Apply(s,{0,1},fresh);CHECK(s.live_parameters->Revision()>revision);CHECK_FALSE(s.publication_failed);
    CHECK(s.boundary_revision==boundary_revision);
    for(const auto& row:s.mesh)for(const auto& face:row.second.physical)if(face.present)
      CHECK(face.facet==s.reference->Canonical(face.facet));
    CHECK(s.live_parameters->Surface().size()==surface.size());
  }
  SECTION("Reversing a physical face still refuses before mutation") {
    std::swap(fresh.surface.front().v[0],fresh.surface.front().v[1]);
    CHECK_THROWS(Apply(s,{0,1},fresh));CHECK(s.live_parameters->Revision()==revision);
    CHECK(s.mesh.count(0)==1);CHECK(s.mesh.count(1)==1);CHECK_FALSE(s.publication_failed);
  }
}
