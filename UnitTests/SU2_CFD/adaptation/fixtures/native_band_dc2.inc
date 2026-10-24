// CONSTRUCTION_STRATEGY_PLAN rev 2 DC-2: TEST-ONLY native-gate replay of the Python SWBI band
// (integration_evidence/native_3d_bl_v1/construction_strategy_v1). Nothing here is production code or promoted.
// Inputs come from dc2/dc2_export.py. Every value is measured with the unchanged native gates:
// actual-centroid Measure on FrozenField::Query (CBoundaryLayerMetric vertex-averaged tensor law, same composition as
// the WP-0(b) replay), FrozenField::TargetEdgeLength, CheckFirstHeight per wall face, and the REFINE changed-edge
// excess rule of ValidateMetricChange (CNativeMetric3D.cpp:337) replicated over the whole replacement, because the
// bounded validator itself refuses more than MaximumReplacementCells cells. Resumable: every stage/chunk is a file.
#include <cctype>
#include <cstdio>
namespace swbi_dc2 {
using namespace SU2Native3D;
// DC-14: Clock, Seconds and Category moved to SU2_CFD/include/adaptation/native_bl3d/BLState.inl (production).
template<class T> std::vector<T> ReadRaw(const std::string& path,size_t count) {
  std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("DC2 missing "+path);
  if(size_t(in.tellg())!=count*sizeof(T))throw std::runtime_error("DC2 size mismatch "+path);
  in.seekg(0);std::vector<T> v(count);in.read(reinterpret_cast<char*>(v.data()),std::streamsize(count*sizeof(T)));
  if(!in)throw std::runtime_error("DC2 short read "+path);return v;
}
template<class T> void WriteRaw(const std::string& path,const std::vector<T>& v) {
  {std::ofstream out(path+".tmp",std::ios::binary);out.write(reinterpret_cast<const char*>(v.data()),std::streamsize(v.size()*sizeof(T)));
   if(!out)throw std::runtime_error("DC2 cannot write "+path);}
  if(std::rename((path+".tmp").c_str(),path.c_str()))throw std::runtime_error("DC2 cannot rename "+path);
}
bool Complete(const std::string& path,size_t bytes){std::ifstream in(path,std::ios::binary|std::ios::ate);return in && size_t(in.tellg())==bytes;}
void WriteText(const std::string& path,const std::string& text) {
  {std::ofstream out(path+".tmp");out<<text;if(!out)throw std::runtime_error("DC2 cannot write "+path);}
  if(std::rename((path+".tmp").c_str(),path.c_str()))throw std::runtime_error("DC2 cannot rename "+path);
}
struct Case {
  std::string name;double h0=0,growth=0,thickness=0,hs=0,core=0;int wall=0;
  size_t npts=0,nP0=0,nband=0,nadd=0,nrem=0,nfacets=0,nwall=0,old_cells=0,fresh_cells=0;
  std::vector<std::string> markers;
  std::vector<double> pts,P0;std::vector<int64_t> band,kind,add,rem,facets,wallchild;std::vector<uint8_t> addband;
};
Case Load(const std::string& dir) {
  Case c;std::ifstream in(dir+"/meta.txt");std::string tag;int version=0;in>>tag>>version;
  if(tag!="SU2_DC2_CASE"||version!=1)throw std::runtime_error("DC2 bad meta "+dir);
  for(std::string key;in>>key;) {
    if(key=="end")break;
    if(key=="name")in>>c.name;else if(key=="h0")in>>c.h0;else if(key=="growth")in>>c.growth;else if(key=="thickness")in>>c.thickness;
    else if(key=="hs")in>>c.hs;else if(key=="core")in>>c.core;else if(key=="wall_marker")in>>c.wall;else if(key=="npts")in>>c.npts;
    else if(key=="nP0")in>>c.nP0;else if(key=="nband")in>>c.nband;else if(key=="nadd")in>>c.nadd;else if(key=="nrem")in>>c.nrem;
    else if(key=="nfacets")in>>c.nfacets;else if(key=="nwall")in>>c.nwall;else if(key=="old_cells")in>>c.old_cells;
    else if(key=="fresh_cells")in>>c.fresh_cells;
    else if(key=="markers"){size_t n;in>>n;c.markers.resize(n);for(auto& m:c.markers)in>>m;}
    else throw std::runtime_error("DC2 unknown meta key "+key);
    if(!in)throw std::runtime_error("DC2 truncated meta "+dir);
  }
  c.pts=ReadRaw<double>(dir+"/pts.f64",3*c.npts);c.P0=ReadRaw<double>(dir+"/P0.f64",3*c.nP0);
  c.band=ReadRaw<int64_t>(dir+"/band.i64",4*c.nband);c.kind=ReadRaw<int64_t>(dir+"/kind.i64",c.nband);
  c.add=ReadRaw<int64_t>(dir+"/add.i64",2*c.nadd);c.addband=ReadRaw<uint8_t>(dir+"/addband.u8",c.nadd);
  c.rem=ReadRaw<int64_t>(dir+"/rem.i64",2*c.nrem);c.facets=ReadRaw<int64_t>(dir+"/facets.i64",5*c.nfacets);
  c.wallchild=ReadRaw<int64_t>(dir+"/wallchild.i64",2*c.nwall);
  return c;
}
Point At(const std::vector<double>& x,int64_t i){return {x[3*i],x[3*i+1],x[3*i+2]};}
struct Native {
  std::shared_ptr<PlanarReference> reference;
  std::shared_ptr<CBoundaryLayerMetric> layers;
  std::unique_ptr<FrozenField> field;
  std::map<Id,Facet> facet;
};
// Same construction as junction_wp0::LoadCase: faceted authority, uniform sensor on six covering donors (exact P1),
// BL composition at every actual query through junction_wp0::Compose/Bands (the production ApplyPoint call).
Native Build(const Case& c) {
  Native n;std::vector<Facet> facets;
  for(size_t i=0;i<c.nfacets;++i){Facet f;f.id=Id(c.facets[5*i]);f.marker=int(c.facets[5*i+1]);
    for(unsigned k=0;k<3;++k){const auto v=c.facets[5*i+2+k];f.v[k]={Id(v),At(c.P0,v)};}facets.push_back(f);n.facet[f.id]=f;}
  n.reference=std::make_shared<PlanarReference>(facets);
  Point lo=At(c.P0,0),hi=lo;
  auto grow=[&](const std::vector<double>& x){for(size_t i=0;i<x.size()/3;++i){const auto p=At(x,i);
    lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}};
  grow(c.P0);grow(c.pts);
  const double pad=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z});lo={lo.x-pad,lo.y-pad,lo.z-pad};hi={hi.x+pad,hi.y+pad,hi.z+pad};
  Tensor sensor;sensor.xx=sensor.yy=sensor.zz=1/(c.hs*c.hs);sensor.xy=sensor.xz=sensor.yz=0;
  std::vector<DonorCell> donors;
  auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{b&1?hi.x:lo.x,b&2?hi.y:lo.y,b&4?hi.z:lo.z}};};
  const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
  for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);
    for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
    if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);
    d.sensor.fill(sensor);donors.push_back(d);}
  CBoundaryLayerMetric::Wall w;w.layer.marker=c.markers.at(c.wall);w.layer.firstHeight=c.h0;w.layer.growth=c.growth;w.layer.thickness=c.thickness;
  std::map<Id,unsigned long> index;
  for(const auto& f:facets)if(f.marker==c.wall){for(auto v:f.v){const auto ins=index.emplace(v.id,index.size());
    if(ins.second){w.coord.push_back(v.p.x);w.coord.push_back(v.p.y);w.coord.push_back(v.p.z);}w.conn.push_back(ins.first->second);}w.conn.push_back(w.conn.back());}
  std::vector<CBoundaryLayerMetric::Wall> walls;walls.push_back(std::move(w));
  n.layers=std::make_shared<CBoundaryLayerMetric>(3,std::move(walls));
  n.field=std::make_unique<FrozenField>(std::move(donors),junction_wp0::Compose(n.layers,c.core),false,std::vector<Id>{},junction_wp0::Bands(n.layers));
  return n;
}
Cell BandCell(const Case& c,size_t i,bool& swapped) {
  Cell cell;cell.id=Id(i+1);for(unsigned k=0;k<4;++k){const auto v=c.band[4*i+k];cell.v[k]={Id(v),At(c.pts,v)};}
  swapped=Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0;if(swapped)std::swap(cell.v[0],cell.v[1]);return cell;
}
struct Budget {Clock::time_point start=Clock::now();double limit=0;bool Over()const{return limit>0 && Seconds(start)>limit;}};
// Stage 1: q/J of every band cell at its actual centroid.
bool Shape(const Case& c,Native& n,const std::string& out,std::ostream& log) {
  if(Complete(out+"/q.f64",8*c.nband)&&Complete(out+"/J.f64",8*c.nband)){log<<"DC2 "<<c.name<<" shape: resumed\n";return true;}
  const auto t=Clock::now();std::vector<double> q(c.nband,NAN),J(c.nband,NAN);size_t swapped=0,degenerate=0,errors=0;std::map<std::string,size_t> why;
  for(size_t i=0;i<c.nband;++i) {
    bool s=false;const auto cell=BandCell(c,i,s);swapped+=s;
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)==0){++degenerate;continue;}
    try{const auto m=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},n.field->Query(Centroid(cell)).target);q[i]=m.mean_ratio;J[i]=m.minimum_scaled_jacobian;}
    catch(const std::exception& e){++errors;++why[Category(e.what())];}
  }
  WriteRaw(out+"/q.f64",q);WriteRaw(out+"/J.f64",J);
  std::ostringstream j;j<<std::setprecision(17)<<"{\"cells\":"<<c.nband<<",\"swapped_to_native_orientation\":"<<swapped<<",\"degenerate\":"<<degenerate
    <<",\"errors\":"<<errors<<",\"error_reasons\":{";bool comma=false;for(const auto& r:why){j<<(comma?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;comma=true;}
  j<<"},\"seconds\":"<<Seconds(t)<<"}\n";WriteText(out+"/shape.json",j.str());
  log<<"DC2 "<<c.name<<" shape: "<<j.str();return true;
}
// Stage 2: CheckFirstHeight on each wall face with its unique band child.
bool Height(const Case& c,Native& n,const std::string& out,std::ostream& log) {
  if(Complete(out+"/height.f64",8*4*c.nwall)){log<<"DC2 "<<c.name<<" height: resumed\n";return true;}
  const auto t=Clock::now();std::vector<double> h(4*c.nwall,NAN);size_t pass=0,fail=0,nochild=0;std::map<std::string,size_t> why;
  const WallHeights heights{{c.wall,c.h0}};
  for(size_t i=0;i<c.nwall;++i) {
    const Id fid=Id(c.wallchild[2*i]);const auto child=c.wallchild[2*i+1];
    if(child<0){++nochild;++why["no unique band child"];continue;}
    bool s=false;const auto cell=BandCell(c,size_t(child),s);const auto& f=n.facet.at(fid);
    std::set<Id> want{f.v[0].id,f.v[1].id,f.v[2].id};Face face{};bool found=false;
    for(auto g:junction_wp0::CellFaces(cell))if(std::set<Id>(g.begin(),g.end())==want){face=g;found=true;}
    if(!found)throw std::runtime_error("DC2 wall child does not contain its face.");
    Patch p;p.cells={cell};p.surface={SurfaceFace{face,fid}};FirstHeightReport report;std::string reason;
    const bool ok=CheckFirstHeight(p,heights,*n.reference,report,reason);
    h[4*i]=ok;h[4*i+1]=report.maximum_relative_error;h[4*i+2]=report.minimum_barycentric;h[4*i+3]=report.maximum_tilt;
    if(ok)++pass;else{++fail;++why[Category(reason)];}
  }
  WriteRaw(out+"/height.f64",h);
  std::ostringstream j;j<<std::setprecision(17)<<"{\"wall_faces\":"<<c.nwall<<",\"pass\":"<<pass<<",\"fail\":"<<fail<<",\"no_child\":"<<nochild<<",\"reasons\":{";
  bool comma=false;for(const auto& r:why){j<<(comma?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;comma=true;}
  j<<"},\"seconds\":"<<Seconds(t)<<"}\n";WriteText(out+"/height.json",j.str());log<<"DC2 "<<c.name<<" height: "<<j.str();return true;
}
// Stage 3: native TargetEdgeLength of every added (after) and removed (before) unique edge, in resumable chunks.
bool Edges(const Case& c,Native& n,const std::string& out,const std::string& tag,const std::vector<int64_t>& e,size_t count,
           const std::vector<double>& x,size_t chunk,const Budget& budget,std::ostream& log) {
  bool all=true;
  for(size_t s=0,k=0;s<count;s+=chunk,++k) {
    const auto m=std::min(chunk,count-s);const std::string file=out+"/"+tag+"_"+std::to_string(k)+".f64";
    if(Complete(file,8*m))continue;
    if(budget.Over()){all=false;break;}
    const auto t=Clock::now();std::vector<double> L(m,NAN);size_t errors=0;std::map<std::string,size_t> why;
    for(size_t i=0;i<m;++i) {
      const auto a=e[2*(s+i)],b=e[2*(s+i)+1];
      try{L[i]=double(n.field->TargetEdgeLength(At(x,a),At(x,b)));}catch(const std::exception& err){++errors;++why[Category(err.what())];}
    }
    WriteRaw(file,L);
    std::ostringstream j;j<<std::setprecision(17)<<"{\"edges\":"<<m<<",\"errors\":"<<errors<<",\"seconds\":"<<Seconds(t)<<",\"reasons\":{";
    bool comma=false;for(const auto& r:why){j<<(comma?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;comma=true;}
    j<<"}}\n";WriteText(file+".json",j.str());
    log<<"DC2 "<<c.name<<" "<<tag<<" chunk "<<k<<": "<<j.str()<<std::flush;
  }
  return all;
}
std::vector<double> Gather(const std::string& out,const std::string& tag,size_t count,size_t chunk) {
  std::vector<double> L;L.reserve(count);
  for(size_t s=0,k=0;s<count;s+=chunk,++k){const auto part=ReadRaw<double>(out+"/"+tag+"_"+std::to_string(k)+".f64",std::min(chunk,count-s));L.insert(L.end(),part.begin(),part.end());}
  return L;
}
}  // namespace swbi_dc2

TEST_CASE("DC-2 native gates on the Python SWBI band", "[.][NativeBandDC2]") {
  using namespace swbi_dc2;
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC2_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* only=std::getenv("SU2_DC2_CASES");REQUIRE(only);
  const char* limit=std::getenv("SU2_DC2_MAX_SECONDS");Budget budget;budget.limit=limit?std::atof(limit):0;
  const size_t chunk=100000;
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty())continue;
    const std::string dir=root+"/"+name,out=dir+"/native";
    if(std::system(("mkdir -p '"+out+"'").c_str()))FAIL("DC2 cannot create "+out);
    auto& log=std::cout;const auto t=Clock::now();
    const auto c=Load(dir+"/input");REQUIRE(c.name==name);
    auto n=Build(c);log<<"DC2 "<<name<<" built field/reference in "<<Seconds(t)<<" s\n"<<std::flush;
    Shape(c,n,out,log);Height(c,n,out,log);
    const bool added=Edges(c,n,out,"add",c.add,c.nadd,c.pts,chunk,budget,log);
    const bool removed=added && Edges(c,n,out,"rem",c.rem,c.nrem,c.P0,chunk,budget,log);
    if(!added||!removed){log<<"DC2 "<<name<<" stopped at the time budget; rerun resumes from the saved chunks.\n";break;}
    // REFINE changed-edge excess, exactly the rule of ValidateMetricChange (CNativeMetric3D.cpp:333-337).
    const auto La=Gather(out,"add",c.nadd,chunk),Lr=Gather(out,"rem",c.nrem,chunk);
    auto excess=[](long double l){const auto e=std::max(0.L,l-(long double)MaximumMetricEdge);return e*e;};
    long double removedExcess=0,addedExcess=0,addedBand=0;size_t nanA=0,nanR=0,overBand=0,overCollar=0;double maxBand=0,maxAll=0,maxRem=0;
    for(size_t i=0;i<La.size();++i){if(!std::isfinite(La[i])){++nanA;continue;}addedExcess+=excess(La[i]);maxAll=std::max(maxAll,La[i]);
      if(c.addband[i]){addedBand+=excess(La[i]);maxBand=std::max(maxBand,La[i]);overBand+=La[i]>MaximumMetricEdge;}else overCollar+=La[i]>MaximumMetricEdge;}
    for(double l:Lr){if(!std::isfinite(l)){++nanR;continue;}removedExcess+=excess(l);maxRem=std::max(maxRem,l);}
    const bool valid=!nanA && !nanR && c.fresh_cells>c.old_cells && removedExcess>0 && addedExcess<removedExcess*(1-1e-12L);
    std::ostringstream j;j<<std::setprecision(17)<<"{\"case\":"<<junction_wp0::Json(name)<<",\"old_cells\":"<<c.old_cells<<",\"fresh_cells\":"<<c.fresh_cells
      <<",\"added_edges\":"<<c.nadd<<",\"removed_edges\":"<<c.nrem<<",\"added_nonfinite\":"<<nanA<<",\"removed_nonfinite\":"<<nanR
      <<",\"added_over_1.8_band\":"<<overBand<<",\"added_over_1.8_collar\":"<<overCollar<<",\"added_max_band\":"<<maxBand<<",\"added_max\":"<<maxAll
      <<",\"removed_max\":"<<maxRem<<",\"removed_excess\":"<<double(removedExcess)<<",\"added_excess\":"<<double(addedExcess)
      <<",\"added_excess_band\":"<<double(addedBand)<<",\"refine_valid\":"<<valid<<",\"field_stats\":{\"compositions\":"<<n.field->Statistics().compositions
      <<",\"edge_requests\":"<<n.field->Statistics().edge_requests<<",\"edge_failures\":"<<n.field->Statistics().edge_failures<<"},\"seconds_this_run\":"<<Seconds(t)<<"}\n";
    WriteText(out+"/refine.json",j.str());log<<"DC2 "<<name<<" refine: "<<j.str()<<std::flush;
  }
}
