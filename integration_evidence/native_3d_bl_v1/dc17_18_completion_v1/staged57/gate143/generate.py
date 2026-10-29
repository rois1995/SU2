#!/usr/bin/env python3
"""Generate a behavioral probe from the baseline and current production source."""
from pathlib import Path
import hashlib, json, subprocess

root = next(p for p in Path(__file__).resolve().parents if (p / 'Common/src/adaptation/CNativeEngine3D.cpp').is_file())
path = root / 'Common/src/adaptation/CNativeEngine3D.cpp'
old = subprocess.check_output(['git', 'show', '3af3a99351767f1c8ed84eed670ff0394722bb5b:Common/src/adaptation/CNativeEngine3D.cpp'], cwd=root, text=True)
new = path.read_text()

def extract(src, start, end_suffix=''):
    begin = src.index(start)
    brace = src.index('{', begin)
    depth = 0
    for pos in range(brace, len(src)):
        if src[pos] == '{': depth += 1
        elif src[pos] == '}':
            depth -= 1
            if depth == 0:
                return src[begin:pos+1] + end_suffix
    raise ValueError(start)

def variant(src, name):
    score = extract(src, 'long double GeometryGuidanceExcess(')
    gate = extract(src, 'auto geometric_change=[&](bool candidate,MetricChange kind)', ';')
    return f'''namespace {name} {{
{score}
Outcome run(bool candidate, MetricChange kind, bool curved, Phase phase, const PlanarReference& geometry, const Patch& old, const Patch& fresh) {{
  Options options; Result result; long double geometric_progress = false; int guarded_calls = 0;
  auto guarded_geometry=[&](bool v) {{ ++guarded_calls; return v; }};
  {gate}
  bool accepted=geometric_change(candidate,kind);
  return {{accepted, bool(geometric_progress), result.reason, guarded_calls}};
}}
}}
'''
header = r'''#include <array>
#include <cassert>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using Id = int;
struct Point { double x,y,z; };
struct Node { Id id; Point p; };
struct Cell { std::array<Node,4> v; };
struct Face { Id facet; std::array<Id,3> v; };
struct Patch { std::vector<Cell> cells; std::vector<Face> surface; };
struct PlanarReference {
  std::array<double,8> errors{}; mutable std::vector<int> trace; int throws=-1; bool entity=false;
  std::unique_ptr<PlanarReference> PrivateParameters(const Patch&) const { return {}; }
  double SurfaceDeviation(Id id, const std::array<Point,3>&, bool) const {
    trace.push_back(id); if(id==throws) throw std::runtime_error("mock exception"); return errors.at(id);
  }
  bool EntityGeometry() const { return entity; }
};
enum class MetricChange { Coarsen, GeometryRefine, GeometryRepair, Repair };
enum class Phase { Construction, Improvement };
struct Options { double surface_tolerance=1.; };
struct Result { std::string reason; };
struct Outcome { bool accepted, progress; std::string reason; int guarded; };
'''
tests = r'''
Patch patch(int offset) {
  Patch p; p.cells.push_back({{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}}});
  p.surface={ {offset,{0,1,2}}, {offset+1,{0,1,3}}, {offset+2,{0,2,3}}, {offset+3,{1,2,3}} }; return p;
}
int main() {
  auto old=patch(0), fresh=patch(4); constexpr double tol=1.;
  for(int code=0;code<6561;++code) {
    PlanarReference a; int n=code;
    for(auto& e:a.errors) {e=n%3; n/=3;}
    for(auto kind:{MetricChange::Coarsen,MetricChange::GeometryRefine,MetricChange::GeometryRepair,MetricChange::Repair})
    for(auto phase:{Phase::Construction,Phase::Improvement}) for(bool entity:{false,true}) for(bool candidate:{false,true}) for(bool curved:{false,true}) {
      a.entity=entity; a.trace.clear(); auto before=baseline::run(candidate,kind,curved,phase,a,old,fresh);
      PlanarReference b=a; b.trace.clear(); auto after=current::run(candidate,kind,curved,phase,b,old,fresh);
      assert(before.accepted==after.accepted); if(after.accepted) assert(before.progress==after.progress);
      if(kind!=MetricChange::Coarsen) { assert(a.trace==b.trace); assert(before.reason==after.reason && before.guarded==after.guarded); }
      if(candidate && curved && kind==MetricChange::Coarsen) {
        bool valid=true; for(int i=4;i<8;++i) valid &= a.errors[i]<=tol;
        assert(after.accepted==valid);
        if(!valid) for(int id:b.trace) assert(id>=4);
      }
    }
    a.trace.clear(); auto full=current::GeometryGuidanceExcess(fresh,a,tol);
    a.trace.clear(); auto fast=current::GeometryGuidanceExcess(fresh,a,tol,true);
    assert((full==0)==(fast==0));
    int expected=4; for(int i=4;i<8;++i) if(a.errors[i]>tol) {expected=i-4+1; break;}
    assert(int(a.trace.size())==expected);
  }
  // A nonfinite accumulated score must stop and reject immediately.
  PlanarReference inf; inf.errors[5]=std::numeric_limits<double>::infinity();
  auto nonfinite=current::run(true,MetricChange::Coarsen,true,Phase::Construction,inf,old,fresh);
  assert(!nonfinite.accepted && (inf.trace==std::vector<int>{4,5}));
  // A bad early fresh facet makes old and later fresh lookups unnecessary.
  PlanarReference x; x.errors[4]=2; x.throws=0;
  auto rejected=current::run(true,MetricChange::Coarsen,true,Phase::Construction,x,old,fresh);
  assert(!rejected.accepted && x.trace==std::vector<int>{4});
  x.trace.clear(); x.throws=7; rejected=current::run(true,MetricChange::Coarsen,true,Phase::Construction,x,old,fresh);
  assert(!rejected.accepted && x.trace==std::vector<int>{4});
  // A valid fresh patch still evaluates the old patch to establish progress.
  x.errors[4]=0; x.trace.clear(); x.throws=0;
  rejected=current::run(true,MetricChange::Coarsen,true,Phase::Construction,x,old,fresh);
  assert(!rejected.accepted && rejected.reason=="mock exception" && x.trace.back()==0);
  std::cout << "exhaustive geometry admission and trace assertions passed\n";
}
'''
out = Path(__file__).with_name('probe.cpp')
out.write_text(header + variant(old,'baseline') + variant(new,'current') + tests)
provenance = {'baseline_commit':'3af3a99351767f1c8ed84eed670ff0394722bb5b', 'baseline_sha256':hashlib.sha256(old.encode()).hexdigest(), 'current_sha256':hashlib.sha256(new.encode()).hexdigest(), 'probe_sha256':hashlib.sha256(out.read_bytes()).hexdigest(), 'generator_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
Path(__file__).with_name('provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
print(json.dumps(provenance,indent=2))
