/* Test-only replay: reference projection and independent SA clipping on saved physical meshes. */
TEST_CASE("Saved curved RANS SA discrepancy is reproduced by solver bounds", "[.][NativeSavedSAProjection3D]") {
  REQUIRE(SU2_MPI::GetSize()==1);
  std::ifstream input("sa_replay.dat");REQUIRE(input.good());
  std::string magic;unsigned version=0,stages=0;double lower=0,absolute=0,fieldScale=0;
  input>>magic>>version>>stages>>lower>>absolute>>fieldScale;
  REQUIRE(magic=="SU2_SA_BOUND_REPLAY");REQUIRE(version==1);REQUIRE(stages==2);
  REQUIRE(lower==1e-16);REQUIRE(absolute==.08);REQUIRE(fieldScale>0);
  std::ofstream out("sa_replay.csv");REQUIRE(out.good());out<<std::setprecision(17)
    <<"stage,point,x,y,z,volume,raw_n,bounded_n,saved_n,raw_nm1,bounded_nm1,saved_nm1\n";
  for(unsigned stage=0;stage<stages;++stage) {
    auto read=[&](std::vector<su2double>& values) {
      CSimplexMesh mesh;mesh.nDim=3;size_t points=0,cells=0,markers=0;input>>points>>cells>>markers;
      REQUIRE(points>0);REQUIRE(points<10000);REQUIRE(cells>0);REQUIRE(cells<10000);REQUIRE(markers==3);
      mesh.coord.resize(3*points);mesh.volume.resize(points);values.resize(2*points);
      for(size_t i=0;i<points;++i) {
        for(unsigned k=0;k<3;++k)input>>mesh.coord[3*i+k];
        input>>mesh.volume[i]>>values[2*i]>>values[2*i+1];REQUIRE(mesh.volume[i]>0);
        REQUIRE(std::isfinite(SU2_TYPE::GetValue(values[2*i])));REQUIRE(std::isfinite(SU2_TYPE::GetValue(values[2*i+1])));
      }
      mesh.elem.resize(4*cells);for(auto& n:mesh.elem){input>>n;REQUIRE(n<points);}
      mesh.markers.resize(markers);
      for(auto& m:mesh.markers) {
        size_t faces=0;input>>m.name>>faces;REQUIRE(faces>0);REQUIRE(faces<10000);
        m.elem.resize(3*faces);for(auto& n:m.elem){input>>n;REQUIRE(n<points);}
      }
      REQUIRE(input.good());return mesh;
    };
    std::vector<su2double> source,saved;const auto donor=read(source),target=read(saved);
    std::array<double,2> wanted{},scale{},final{};unsigned long expectedLimited=0;
    input>>wanted[0]>>wanted[1]>>scale[0]>>scale[1]>>final[0]>>final[1]>>expectedLimited;
    REQUIRE(input.good());REQUIRE(expectedLimited>0);
    CConservativeProjection::Options options;options.absoluteLimit=absolute;options.openMarkers={"cap0","cap1"};
    CConservativeProjection projection(donor,target,options);std::vector<su2double> raw;
    projection.Project(2,source,raw);const auto& summary=projection.GetSummary();
    unsigned long limited=0;std::array<double,2> totals{};double maximum=0;
    for(unsigned f=0;f<2;++f) {
      REQUIRE(scale[f]>0);CHECK(std::abs(summary.targetTotal[f]-wanted[f])/scale[f]<1e-12);
      CHECK(std::abs(SU2_TYPE::GetValue(projection.NewTotal(f,raw))-wanted[f])/scale[f]<1e-12);
    }
    for(size_t i=0;i<target.GetnPoint();++i) {
      std::array<double,2> bounded{};
      for(unsigned f=0;f<2;++f) {
        const double value=SU2_TYPE::GetValue(raw[2*i+f]);bounded[f]=std::max(lower,value);
        limited+=value<lower && std::abs(value-lower)>1e-10*lower;
        maximum=std::max(maximum,std::abs(bounded[f]-SU2_TYPE::GetValue(saved[2*i+f]))/fieldScale);
        totals[f]+=target.volume[i]*bounded[f];
      }
      out<<stage<<','<<i;for(unsigned k=0;k<3;++k)out<<','<<target.coord[3*i+k];out<<','<<target.volume[i];
      for(unsigned f=0;f<2;++f)out<<','<<SU2_TYPE::GetValue(raw[2*i+f])<<','<<bounded[f]<<','<<SU2_TYPE::GetValue(saved[2*i+f]);out<<'\n';
    }
    INFO("stage="<<stage<<" clipped="<<limited<<" maximum field-scaled node error="<<maximum);
    CHECK(limited==expectedLimited);CHECK(maximum<2e-10);
    for(unsigned f=0;f<2;++f) {
      CHECK(std::abs(totals[f]-final[f])/scale[f]<1e-11);
      CHECK((final[f]-wanted[f])/scale[f]>1e-5);
    }
  }
  input>>std::ws;CHECK(input.eof());CHECK(out.good());
}
