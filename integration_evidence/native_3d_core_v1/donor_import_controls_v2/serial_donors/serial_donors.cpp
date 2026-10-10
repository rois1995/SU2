#include "Common/include/adaptation/CNativeDonor3D.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace SU2Native3D;
int main() {
  World world;
  DonorCell row; row.cell={0, {{{0,{0,0,0}}, {1,{1,0,0}}, {2,{0,1,0}}, {3,{0,0,1}}}}};
  for(size_t i=0;i<4;++i) {auto p=row.cell.v[i].p; row.sensor[i]={2+p.x,.2,.1,3+p.y,.15,4+p.z};}
  const std::vector<DonorCell> original{row};
  DonorImport source(world); std::string reason;
  assert(source.Build(original,reason));
  assert(!source.Build(original,reason));
  const std::vector<DonorRegion> regions{{0,0,0,1,1,1}};
  auto patch=source.Import(regions); assert(patch.valid && patch.cells.size()==1);
  int count=0;
  FrozenField field(patch.cells,[&](Point p,Tensor m){++count;if(p.z<.0625)m.zz+=100*(1-16*p.z);return m;});
  for(int i=0;i<2;++i){auto s=field.Query({.125,.25,.03125});assert(s.sensor.xx==2.125 && s.sensor.yy==3.25 && s.sensor.zz==4.03125);assert(s.target.zz==54.03125);}
  assert(count==2 && field.Statistics().hits==1);
  assert(source.Import({}).valid);
  assert(!source.Import(regions,1).valid);
  assert(!source.Import(regions,SIZE_MAX,SIZE_MAX).valid);
  auto boxed=source.Import({{.8,.8,.8,.8,.8,.8}});assert(boxed.valid && boxed.cells.size()==1);
  bool rejected=false;try{field.Query({.8,.8,.8});}catch(const std::exception&){rejected=true;}assert(rejected);
  assert(!source.Import({{2,2,2,3,3,3}}).valid);
  assert(source.Import(regions).valid);
  DonorImport retry(world); auto bad=original;bad[0].sensor[0].zz=-1;
  assert(!retry.Build(bad,reason) && retry.ResidentBytes()==0);
  assert(retry.Build(original,reason));
  std::cout<<"PASS serial immutable donor import, P1, actual composition/cache, atomic rejection and exact outside query\n";
}
