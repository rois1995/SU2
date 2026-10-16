/* Complete frozen-field construction controls; no CFD evolution or solver-specific remeshing mode. */
TEST_CASE("Metric-directed regional core resolves stronger curved sensor fields", "[.][NativeRegionCore3D]") {
  World world;const char* selected=std::getenv("SU2_NATIVE_CORE_FIELD");const int kind=selected?std::stoi(selected):0;
  REQUIRE(kind>=0);REQUIRE(kind<=2);const bool tilted=kind==2;
  const auto seed=PrimitiveMesh(false,tilted);
  std::stringstream text("SOLVER= RANS\nKIND_TURB_MODEL= SA\nMACH_NUMBER= .2\nREYNOLDS_NUMBER= 1000\nREYNOLDS_LENGTH= 1\n"
    "MESH_FORMAT= SU2\nMESH_FILENAME= core_initial.su2\nMARKER_FAR= (cap0,cap1)\nMARKER_HEATFLUX= (cylinder,0)\nMGLEVEL= 0\n"
    "COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
    "ADAP_BL_MARKER= (cylinder)\nADAP_BL_FIRST_HEIGHT= (.05)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.2)\nADAP_HAUSD= .04\n");
  if(!world.rank){simplex_test::WriteSU2Mesh(seed,"core_initial.su2");std::ofstream cfg("core.cfg");cfg<<text.str();}
  std::unique_ptr<CConfig> config;{Mute mute;config=std::make_unique<CConfig>(text,SU2_COMPONENT::SU2_CFD,false);}
  MeshSolution mesh(config.get(),seed,0);su2activematrix sensor(mesh.Fine().GetnPointDomain(),6);
  const double q=1/std::sqrt(3.);const Point axis=tilted?Point{q,q,q}:Point{0,0,1};
  auto expected=[&](Point p) {
    const double z=tilted?((p.x-2)+(p.y+1)+(p.z-.5))*q:p.z;
    const double strength=kind==1?4+24*z:16;
    return Tensor{1+(strength-1)*axis.x*axis.x,(strength-1)*axis.x*axis.y,(strength-1)*axis.x*axis.z,
      1+(strength-1)*axis.y*axis.y,(strength-1)*axis.y*axis.z,1+(strength-1)*axis.z*axis.z};
  };
  for(unsigned long p=0;p<sensor.rows();++p) {
    const auto* x=mesh.Fine().nodes->GetCoord(p);const auto m=expected({SU2_TYPE::GetValue(x[0]),SU2_TYPE::GetValue(x[1]),SU2_TYPE::GetValue(x[2])});
    const std::array<double,6> packed{m.xx,m.xy,m.xz,m.yy,m.yz,m.zz};for(unsigned k=0;k<6;++k)sensor(p,k)=packed[k];
  }
  ReferenceState reference;ImportGeometry(*config,mesh.Fine(),nullptr,reference);DeclarePrimitive(reference,false,tilted);
  const auto input=ImportGeometry(*config,mesh.Fine(),&sensor,reference);const auto before=Signature(world,input.cells,reference);
  const auto source=world.metadata(input.original);Save(world,"core_initial",input.cells,input.original,reference);
  WriteReference("core_initial.su2.native_ref",reference);
  auto layers=CBoundaryLayerMetric::FromNativeReference(reference,*config);
  MetricComposition compose=[layers](Point p,Tensor s) {
    CBoundaryLayerMetric::Tensor m;const su2double x[3]={p.x,p.y,p.z};
    m.m[0][0]=s.xx;m.m[0][1]=m.m[1][0]=s.xy;m.m[0][2]=m.m[2][0]=s.xz;
    m.m[1][1]=s.yy;m.m[1][2]=m.m[2][1]=s.yz;m.m[2][2]=s.zz;layers->ApplyPoint(x,m,1.,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
      SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
  EngineOptions options;options.surface_tolerance=.04;options.maximum_work=4096;options.maximum_seconds=300;options.timings=true;
  for(unsigned marker=0;marker<reference.marker_names.size();++marker)if(reference.marker_names[marker]=="cylinder") {
    options.wall_heights[marker]=.05;options.region_walls[marker]={.05,1.2,.2};
  }
  options.edge_breakpoints=[layers](Point a,Point b){return layers->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
  options.rejected_trial=[&](const Patch&,const Patch& candidate,Feature,const MetricDiagnosis&,bool,const std::string& why) {
    SaveRecords("core_construction_rejected",PatchRecords(candidate),source,reference);
    std::ofstream out("core_rejection.txt");out<<why<<'\n';
  };
  const auto result=Refine(world,input.cells,input.original,*reference.original,compose,options);
  Save(world,result.valid?"core_accepted":"core_input_retained",result.cells,input.original,reference);
  CHECK(Signature(world,input.cells,reference)==before);
  const auto regions=world.maximum(result.stats.region_core_regions),work=world.maximum(result.stats.global_work);
  if(!world.rank)std::cout<<"REGION_CORE field="<<kind<<" valid="<<result.valid<<" regions="<<regions<<" work="<<work
    <<" trials="<<result.stats.region_core_trials<<" surface_trials="<<result.stats.region_surface_trials
    <<" junction_trials="<<result.stats.region_junction_trials<<" reason="<<result.reason<<std::endl;
  INFO(result.reason);REQUIRE(result.valid);CHECK(result.rebuilt_domain);CHECK(regions>1);CHECK(work<=4096);
  std::vector<OwnedCell> local;for(auto row:result.cells)local.push_back(row.second);const auto all=world.metadata(local);
  if(world.rank)return;
  FrozenField field(source,compose,false,AssociatedExtensions(source,*reference.original),options.edge_breakpoints);
  std::ofstream samples("core_samples.csv"),edgesOut("core_edges.csv");samples<<std::setprecision(17)<<"cell,q,j,xx,xy,xz,yy,yz,zz\n";
  edgesOut<<std::setprecision(20)<<"a,b,length\n";std::map<Edge,std::pair<Point,Point>> edges;
  double minimumQ=1,minimumJ=1,residual=0,collarAnalyticDelta=0;long double maximum=0;
  size_t physicalSamples=0,collarSamples=0;
  std::ofstream sensors("core_sensors.csv");sensors<<std::setprecision(17)<<"cell,x,y,z,xx,xy,xz,yy,yz,zz,d0,d1,d2,d3,extension\n";
  for(auto row:all) {
    const auto& c=row.cell;const auto p=Centroid(c);const auto actual=field.Query(p);const auto wanted=expected(p);
    const std::array<double,6> a{actual.sensor.xx,actual.sensor.xy,actual.sensor.xz,actual.sensor.yy,actual.sensor.yz,actual.sensor.zz},
      b{wanted.xx,wanted.xy,wanted.xz,wanted.yy,wanted.yz,wanted.zz};
    // The declared exterior collar carries frozen boundary sensors, not an analytic continuation.
    const bool collar=actual.extension_component!=UINT64_MAX;
    if(collar)++collarSamples;else ++physicalSamples;
    for(unsigned k=0;k<6;++k) {
      auto& error=collar?collarAnalyticDelta:residual;error=std::max(error,std::abs(a[k]-b[k])/(1+std::abs(b[k])));
    }
    sensors<<c.id<<','<<p.x<<','<<p.y<<','<<p.z;
    for(auto value:a)sensors<<','<<value;
    for(auto id:actual.donor)sensors<<','<<id;
    sensors<<','<<actual.extension_component<<'\n';
    const auto m=actual.target;const auto shape=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},m);
    minimumQ=std::min(minimumQ,shape.mean_ratio);minimumJ=std::min(minimumJ,shape.minimum_scaled_jacobian);
    samples<<c.id<<','<<shape.mean_ratio<<','<<shape.minimum_scaled_jacobian<<','<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz<<'\n';
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
      const auto x=c.v[i],y=c.v[j];const Edge e{std::min(x.id,y.id),std::max(x.id,y.id)};edges.emplace(e,x.id<y.id?std::make_pair(x.p,y.p):std::make_pair(y.p,x.p));
    }
  }
  for(auto row:edges){const auto l=field.TargetEdgeLength(row.second.first,row.second.second);CHECK(l<=MaximumMetricEdge);maximum=std::max(maximum,l);edgesOut<<row.first[0]<<','<<row.first[1]<<','<<l<<'\n';}
  CHECK(minimumQ>=MinimumMeanRatio);CHECK(minimumJ>=MinimumScaledJacobian);CHECK(residual<2e-12);CHECK(physicalSamples>0);CHECK(sensors.good());CHECK(samples.good());CHECK(edgesOut.good());
  std::ofstream out("core_summary.json");out<<std::setprecision(17)<<"{\"field\":"<<kind<<",\"regions\":"<<regions<<",\"work\":"<<work
    <<",\"cells\":"<<all.size()<<",\"q\":"<<minimumQ<<",\"j\":"<<minimumJ<<",\"maximum_full_length\":"<<maximum<<",\"sensor_residual\":"<<residual<<",\"physical_sensor_samples\":"<<physicalSamples
    <<",\"collar_sensor_samples\":"<<collarSamples<<",\"collar_analytic_delta\":"<<collarAnalyticDelta<<"}\n";CHECK(out.good());
}
