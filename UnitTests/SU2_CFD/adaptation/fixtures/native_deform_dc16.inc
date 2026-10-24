// DC-16: tests of the shared dimension-generic deformation module (CNativeDeformation.hpp, 2D triangles and 3D tetrahedra)
// and of the 3D room step of the growth layers (BLDeform16.inl) on the DC-12 synthetic boxes.
#include "../../../../Common/include/adaptation/CNativeDeformation.hpp"
namespace dc16test {
using namespace SU2NativeDeform;
// n x n unit square, each square split by its diagonal; bottom y = 0 prescribed, left x = 0 sliding (normal (1,0)) when
// slide, every other boundary vertex clamped
inline Problem<2> Square(int n, Vec<2> bottom, bool slide) {
  Problem<2> p;auto id=[&](int i,int j){return uint32_t(i+(n+1)*j);};
  for(int j=0;j<=n;++j)for(int i=0;i<=n;++i)p.x.push_back({double(i)/n,double(j)/n});
  for(int j=0;j<n;++j)for(int i=0;i<n;++i){p.cells.push_back({id(i,j),id(i+1,j),id(i+1,j+1)});p.cells.push_back({id(i,j),id(i+1,j+1),id(i,j+1)});}
  const size_t m=p.x.size();p.kind.assign(m,Constraint::Free);p.value.assign(m,Vec<2>{0,0});p.normals.assign(m,{});
  for(int j=0;j<=n;++j)for(int i=0;i<=n;++i){const auto v=id(i,j);
    if(j==0){p.kind[v]=Constraint::Fixed;if(i>0&&i<n)p.value[v]=bottom;}
    else if(i==0&&slide&&j<n){p.kind[v]=Constraint::Slide;p.normals[v]={Vec<2>{1,0}};}
    else if(i==0||i==n||j==n)p.kind[v]=Constraint::Fixed;}
  return p;
}
inline Problem<3> Cube(int n, Vec<3> bottom) {
  Problem<3> p;auto id=[&](int i,int j,int k){return uint32_t(i+(n+1)*(j+(n+1)*k));};
  for(int k=0;k<=n;++k)for(int j=0;j<=n;++j)for(int i=0;i<=n;++i)p.x.push_back({double(i)/n,double(j)/n,double(k)/n});
  const std::array<std::array<int,3>,6> perms{{{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}}};
  for(int k=0;k<n;++k)for(int j=0;j<n;++j)for(int i=0;i<n;++i)for(const auto& q:perms){std::array<int,3> c{i,j,k};std::array<uint32_t,4> v;v[0]=id(c[0],c[1],c[2]);
    for(int t=0;t<3;++t){++c[q[t]];v[t+1]=id(c[0],c[1],c[2]);}
    std::array<Vec<3>,4> x;for(int t=0;t<4;++t)x[t]=p.x[v[t]];
    const double det=(x[1][0]-x[0][0])*((x[2][1]-x[0][1])*(x[3][2]-x[0][2])-(x[2][2]-x[0][2])*(x[3][1]-x[0][1]))-(x[1][1]-x[0][1])*((x[2][0]-x[0][0])*(x[3][2]-x[0][2])-(x[2][2]-x[0][2])*(x[3][0]-x[0][0]))+(x[1][2]-x[0][2])*((x[2][0]-x[0][0])*(x[3][1]-x[0][1])-(x[2][1]-x[0][1])*(x[3][0]-x[0][0]));
    if(det<0)std::swap(v[0],v[1]);p.cells.push_back(v);}
  const size_t m=p.x.size();p.kind.assign(m,Constraint::Free);p.value.assign(m,Vec<3>{0,0,0});p.normals.assign(m,{});
  for(int k=0;k<=n;++k)for(int j=0;j<=n;++j)for(int i=0;i<=n;++i){const auto v=id(i,j,k);const bool side=i==0||i==n||j==0||j==n;
    if(k==0){p.kind[v]=Constraint::Fixed;if(!side)p.value[v]=bottom;}else if(side||k==n)p.kind[v]=Constraint::Fixed;}
  return p;
}
template<int D> long double Signed(const std::array<Vec<D>,D+1>& q) {
  if(D==2)return ((long double)q[1][0]-q[0][0])*((long double)q[2][1]-q[0][1])-((long double)q[1][1]-q[0][1])*((long double)q[2][0]-q[0][0]);
  const long double a[3]={(long double)q[1][0]-q[0][0],(long double)q[1][1]-q[0][1],(long double)q[1][D-1]-q[0][D-1]};
  const long double b[3]={(long double)q[2][0]-q[0][0],(long double)q[2][1]-q[0][1],(long double)q[2][D-1]-q[0][D-1]};
  const long double c[3]={(long double)q[D][0]-q[0][0],(long double)q[D][1]-q[0][1],(long double)q[D][D-1]-q[0][D-1]};
  return a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0]);
}
template<int D> bool AllPositive(const Problem<D>& p,const std::vector<Vec<D>>& y) {
  for(const auto& c:p.cells){std::array<Vec<D>,D+1> q;for(int k=0;k<=D;++k)q[k]=y[c[k]];if(!(Signed<D>(q)>0))return false;}return true;}
}  // namespace dc16test

TEST_CASE("DC-16 shared deformation: 2D and 3D Laplacian, sliding, pseudo-time freezing", "[NativeDeform16]") {
  using namespace dc16test;
  SECTION("2D: pushed bottom, maximum principle, exact zero tangential component") {
    auto p=Square(8,{0,.05},false);std::vector<Vec<2>> d;SolveOptions o;SolveReport r;
    REQUIRE(SolveDisplacement<2>(p,d,o,r));CHECK(r.converged);CHECK(r.unknowns==2*7*7);
    for(size_t i=0;i<p.x.size();++i){CHECK(d[i][0]==0.);CHECK(d[i][1]>=-1e-14);CHECK(d[i][1]<=.05+1e-14);}
    // decay away from the pushed edge along the middle column
    for(int j=1;j<8;++j){const auto a=uint32_t(4+9*j),b=uint32_t(4+9*(j-1));CHECK(d[a][1]<=d[b][1]+1e-14);}
    CHECK(d[4+9*1][1]>d[4+9*6][1]);
    std::vector<Vec<2>> y;std::vector<char> frozen;MoveOptions mo;MoveReport mr;
    const CellCheck<2> positive=[](size_t,const std::array<Vec<2>,3>& q){return Signed<2>(q)>0;};
    PseudoTimeMove<2>(p,d,positive,y,frozen,mo,mr);CHECK(mr.frozen==0);CHECK(mr.reached==1.);CHECK(mr.moved==49);
    for(size_t i=0;i<p.x.size();++i)if(p.kind[i]==Constraint::Fixed)CHECK(y[i]==p.x[i]);  // Dirichlet data never moves
    CHECK(AllPositive<2>(p,y));
    // deterministic
    std::vector<Vec<2>> d2;SolveReport r2;SolveDisplacement<2>(p,d2,o,r2);CHECK(d2==d);
  }
  SECTION("2D: sliding left side keeps x exactly, moves tangentially") {
    auto p=Square(8,{.04,.05},true);std::vector<Vec<2>> d;SolveOptions o;SolveReport r;
    REQUIRE(SolveDisplacement<2>(p,d,o,r));
    size_t slid=0;for(size_t i=0;i<p.x.size();++i)if(p.kind[i]==Constraint::Slide){CHECK(d[i][0]==0.);slid+=d[i][1]>0;}
    CHECK(slid>=3);
    std::vector<Vec<2>> y;std::vector<char> frozen;MoveOptions mo;MoveReport mr;
    const CellCheck<2> positive=[](size_t,const std::array<Vec<2>,3>& q){return Signed<2>(q)>0;};
    PseudoTimeMove<2>(p,d,positive,y,frozen,mo,mr);
    for(size_t i=0;i<p.x.size();++i)if(p.kind[i]==Constraint::Slide)CHECK(y[i][0]==0.);
    CHECK(AllPositive<2>(p,y));
  }
  SECTION("2D: a push through the clamped top freezes vertices and keeps every accepted state valid") {
    auto p=Square(6,{0,1.4},false);std::vector<Vec<2>> d;SolveOptions o;SolveReport r;REQUIRE(SolveDisplacement<2>(p,d,o,r));
    // the Dirichlet bottom is data only; the free vertices would run into the clamped top row
    std::vector<Vec<2>> y;std::vector<char> frozen;MoveOptions mo;MoveReport mr;
    const CellCheck<2> positive=[](size_t,const std::array<Vec<2>,3>& q){return Signed<2>(q)>0;};
    PseudoTimeMove<2>(p,d,positive,y,frozen,mo,mr);
    CHECK(mr.frozen>0);CHECK(mr.moved>0);CHECK(AllPositive<2>(p,y));CHECK(mr.failed_checks>0);
  }
  SECTION("3D: Kuhn cube, pushed bottom, exact zero tangential components, positive") {
    auto p=Cube(4,{0,0,.1});std::vector<Vec<3>> d;SolveOptions o;SolveReport r;
    REQUIRE(SolveDisplacement<3>(p,d,o,r));CHECK(r.converged);CHECK(r.unknowns==3*27);
    for(size_t i=0;i<p.x.size();++i){CHECK(d[i][0]==0.);CHECK(d[i][1]==0.);CHECK(d[i][2]>=-1e-14);CHECK(d[i][2]<=.1+1e-14);}
    std::vector<Vec<3>> y;std::vector<char> frozen;MoveOptions mo;MoveReport mr;
    const CellCheck<3> positive=[](size_t,const std::array<Vec<3>,4>& q){return Signed<3>(q)>0;};
    PseudoTimeMove<3>(p,d,positive,y,frozen,mo,mr);CHECK(mr.frozen==0);CHECK(mr.moved==27);CHECK(AllPositive<3>(p,y));
    auto q=Cube(4,{0,0,1.5});SolveDisplacement<3>(q,d,o,r);PseudoTimeMove<3>(q,d,positive,y,frozen,mo,mr);
    CHECK(mr.frozen>0);CHECK(AllPositive<3>(q,y));
  }
}

TEST_CASE("DC-16 room step of the growth layers on synthetic boxes: valid, volume and skin kept, symmetry sliding", "[NativeRoom16]") {
  using namespace dc12;World world;REQUIRE(world.size==1);
  struct NeedReset{long double need=dc16::roomNeed;~NeedReset(){dc16::roomNeed=need;dc16::roomStep=true;}} reset;
  for(int variant:{0,1,2}) {
    const double angle=variant==1?.3:0.;INFO("variant "<<variant);  // 1 rotated symmetry plane, 2 two opposing walls
    size_t heightsOff=0;
    for(bool room:{false,true}) {
      INFO("room "<<room);
      Policies policies;artificialMarkers={2};perf=Perf{};ResetCaches();cachesOn=true;
      dc16::roomStep=room;dc16::roomNeed=room?1e9L:1.5L;  // every planned top is needy
      BoxSpec b;b.angle=angle;b.g=1.25;b.topWall=variant==2;auto s=SyntheticBox(b);ApplyWallPolicies(*s);DetectCreases(*s,BoxRoles());
      size_t unbound=0;const auto areas0=MarkerAreas(*s,unbound);REQUIRE(unbound==0);
      auto field=ComputeDirectionField(*s);directionField=&field.d;
      Rows10 rows;std::ostringstream log;PhaseR1(*s,rows,4096,60,log);PhaseR2(*s,static_cast<Rows&>(rows),120,log);
      const auto G=PhaseG(*s,rows,120,log);
      long double volume=0;bool positive=true;for(const auto& row:s->mesh){const auto& c=row.second.cell;const auto o=Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p);positive=positive&&o>0;volume+=o;}
      CHECK(positive);CHECK(std::abs(double(volume/6)-1)<1e-9);
      const auto areas=MarkerAreas(*s,unbound);CHECK(unbound==0);
      for(const auto& a:areas0){const auto f=areas.find(a.first);REQUIRE(f!=areas.end());CHECK(std::abs(double(f->second-a.second))<=1e-9);}
      // rim-adjacent wall faces of the box have no first row (no completion pass here): the topology and orientation
      // parts of the independent validity must hold, and the room step may not change the first-height verdicts
      const auto v=dc15::IndependentValidity(*s);INFO(v.witness);
      CHECK(v.nonpositive==0);CHECK(v.overshared==0);CHECK(v.same_orientation==0);CHECK(v.interior_physical==0);CHECK(v.open_unbound==0);
      CHECK(v.unknown_marker==0);CHECK(v.inconsistent_points==0);CHECK(v.nonmanifold_skin_edges==0);
      if(!room)heightsOff=v.height_failures;else CHECK(v.height_failures==heightsOff);
      size_t grown=0;for(const auto& c:G.columns)grown+=c.stack.size()>=3;CHECK(grown>=8);
      if(room){CHECK(dc16::roomStats.calls>0);CHECK(dc16::roomStats.needy>0);CHECK(dc16::roomStats.commits>0);CHECK(dc16::roomStats.moved>0);
        CHECK(dc16::roomStats.slide>0);INFO(dc16::JsonRoom(dc16::roomStats));}
      directionField=nullptr;
    }
  }
}
