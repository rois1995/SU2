  if(const auto* path=std::getenv("SU2_NATIVE_FIRST_ROW_PROBE_TETS")) {
    // Diagnostic only: known ORIGINAL parent support avoids tracing a field on candidate nodes.
    std::ifstream input(path,std::ios::binary|std::ios::ate);REQUIRE(bool(input));
    const auto bytes=static_cast<std::streamoff>(input.tellg());input.seekg(0);uint64_t magic=0,count=0;
    input.read(reinterpret_cast<char*>(&magic),8);input.read(reinterpret_cast<char*>(&count),8);
    REQUIRE(magic==0x4e41544c50524f42ULL);REQUIRE(count>0);REQUIRE(count<=262144);
    REQUIRE(bytes==std::streamoff(16+count*(8+12*sizeof(double))));
    std::ofstream output("layer_probe_rank"+std::to_string(world.rank)+".csv");REQUIRE(bool(output));
    output<<std::setprecision(17)<<"row,parent,x,y,z,mxx,mxy,mxz,myy,myz,mzz,q,J,b0,b1,b2,b3\n";
    CLocalFailure failure;uint64_t evaluated=0;const auto begin=world.seconds();
    try {
      for(uint64_t row=0;row<count;++row) {
        Id parent;double xyz[12];input.read(reinterpret_cast<char*>(&parent),8);input.read(reinterpret_cast<char*>(xyz),sizeof(xyz));
        if(!input || parent>=elements)throw std::runtime_error("Invalid layer-probe original parent.");
        for(double x:xyz)if(!std::isfinite(x))throw std::runtime_error("Nonfinite layer-probe coordinate.");
        if((parent+1)%world.size!=unsigned(world.rank))continue;
        const auto it=std::lower_bound(original.begin(),original.end(),parent,[](const DonorCell& d,Id id){return d.cell.id<id;});
        if(it==original.end() || it->cell.id!=parent)throw std::runtime_error("Layer-probe lost its original publisher.");
        const auto& v=it->cell.v;const Tetrahedron support{v[0].p,v[1].p,v[2].p,v[3].p};Cell cell;cell.id=row;
        for(unsigned k=0;k<4;++k) {
          cell.v[k]={k,{xyz[3*k],xyz[3*k+1],xyz[3*k+2]}};std::array<long double,4> weights;
          if(!Barycentric(support,cell.v[k].p,weights))throw std::runtime_error("Layer-probe vertex leaves its original parent.");
        }
        if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);
        const auto p=Centroid(cell);std::array<long double,4> weights;
        if(!Barycentric(support,p,weights))throw std::runtime_error("Layer-probe centroid leaves its original parent.");
        const auto m=composition(p,InterpolateSensor(*it,weights));
        const auto measured=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},m);
        output<<row<<','<<parent<<','<<p.x<<','<<p.y<<','<<p.z<<','<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz
            <<','<<measured.mean_ratio<<','<<measured.minimum_scaled_jacobian;
        for(auto w:weights)output<<','<<double(w);output<<'\n';++evaluated;
      }
      output.close();if(!output)throw std::runtime_error("Layer-probe output failed.");
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    const auto rejected=world.elect(failure);INFO(rejected.message);REQUIRE_FALSE(rejected.any);
    REQUIRE(world.sum(evaluated)==count);
    if(const auto* edgePath=std::getenv("SU2_NATIVE_FIRST_ROW_PROBE_EDGES")) {
      std::ifstream edges(edgePath,std::ios::binary|std::ios::ate);REQUIRE(bool(edges));
      const auto edgeBytes=static_cast<std::streamoff>(edges.tellg());edges.seekg(0);
      uint64_t edgeMagic=0,edgeCount=0;edges.read(reinterpret_cast<char*>(&edgeMagic),8);edges.read(reinterpret_cast<char*>(&edgeCount),8);
      REQUIRE(edgeMagic==0x4e41544550524f42ULL);REQUIRE(edgeCount>0);REQUIRE(edgeCount<=262144);
      REQUIRE(edgeBytes==std::streamoff(16+24*edgeCount));
      std::ofstream lengths("wall_edges_rank"+std::to_string(world.rank)+".csv");REQUIRE(bool(lengths));
      lengths<<std::setprecision(20)<<"row,parent,u,v,length\n";uint64_t measuredEdges=0;
      try {
        for(uint64_t row=0;row<edgeCount;++row) {
          uint64_t ids[3];edges.read(reinterpret_cast<char*>(ids),sizeof(ids));
          if(!edges || ids[0]>=elements || ids[1]>=ids[2])throw std::runtime_error("Invalid unique original wall edge.");
          if((ids[0]+1)%world.size!=unsigned(world.rank))continue;
          const auto it=std::lower_bound(original.begin(),original.end(),ids[0],[](const DonorCell& d,Id id){return d.cell.id<id;});
          if(it==original.end() || it->cell.id!=ids[0])throw std::runtime_error("Wall-edge probe lost original parent.");
          for(unsigned k=1;k<3;++k)if(std::none_of(it->cell.v.begin(),it->cell.v.end(),[&](Node n){return n.id==ids[k];}))
            throw std::runtime_error("Wall-edge probe endpoint outside original parent.");
          FrozenField field({*it},composition,false,{},[&](Point a,Point b){return layers->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});});
          const auto length=field.TargetEdgeLength(nodes.at(ids[1]).point,nodes.at(ids[2]).point);
          lengths<<row<<','<<ids[0]<<','<<ids[1]<<','<<ids[2]<<','<<length<<'\n';++measuredEdges;
        }
        lengths.close();if(!lengths)throw std::runtime_error("Wall-edge probe output failed.");
      }catch(const std::exception& e){failure.Set(2,0,e.what());}
      const auto edgeFailure=world.elect(failure);INFO(edgeFailure.message);REQUIRE_FALSE(edgeFailure.any);
      REQUIRE(world.sum(measuredEdges)==edgeCount);
    }
    // This geometry scalar only guides cuts. It never replaces the actual-query BL composition.
    std::set<Id> wallNodes;
    for(const auto& facet:reference.facets)for(unsigned w=0;w<layers->GetnWall();++w)
      if(reference.marker_names.at(facet.marker)==layers->GetLayer(w).marker)for(auto v:facet.v)wallNodes.insert(v.id);
    std::ofstream guide("layer_guide_rank"+std::to_string(world.rank)+".csv");REQUIRE(bool(guide));
    guide<<std::setprecision(17)<<"node,normalized_distance\n";
    try {
      for(const auto& row:nodes)if(row.first%world.size==unsigned(world.rank)) {
        double phi=0;
        if(!wallNodes.count(row.first)) {
          phi=INFINITY;const auto p=row.second.point;const su2double x[3]={p.x,p.y,p.z};
          for(unsigned w=0;w<layers->GetnWall();++w)phi=std::min(phi,SU2_TYPE::GetValue(layers->Evaluate(w,x,core).distance/layers->GetLayer(w).firstHeight));
          if(!std::isfinite(phi) || phi<0)throw std::runtime_error("Invalid layer-guide distance.");
        }
        guide<<row.first<<','<<phi<<'\n';
      }
      guide.close();if(!guide)throw std::runtime_error("Layer-guide output failed.");
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    const auto failed=world.elect(failure);INFO(failed.message);REQUIRE_FALSE(failed.any);
    std::ofstream meta("layer_probe_metadata_rank"+std::to_string(world.rank)+".json");meta<<std::setprecision(17)
        <<"{\"rows\":"<<evaluated<<",\"core\":"<<core<<",\"seconds\":"<<world.seconds()-begin
        <<",\"shape_only\":true,\"accepted_mesh\":false,\"CFD_steps\":0}\n";REQUIRE(bool(meta));
    return;
  }
