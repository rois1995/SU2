#!/usr/bin/env python3
import hashlib,json,pathlib
root=pathlib.Path(__file__).resolve().parents[5];out=pathlib.Path(__file__).resolve().parent
source=root/'Common/src/adaptation/CNativeAffineReference.cpp'
previous=out.parent/'distance141/probe.cpp'; checks=out.parent/'distance141/checks.inc'; interval=out.parent/'interval135/probe.cpp'
new=source.read_text(); old=previous.read_text()
def part(s,a,b):return s[s.index(a):s.index(b)]
prefix='''#include <algorithm>\n#include <array>\n#include <cassert>\n#include <chrono>\n#include <cmath>\n#include <cstdint>\n#include <cstring>\n#include <iostream>\n#include <limits>\n#include <random>\n#include <stdexcept>\n#include <boost/multiprecision/cpp_int.hpp>\nstruct Point { double x,y,z; };\nusing Vec=std::array<double,3>;\nVec Values(Point p){return {p.x,p.y,p.z};}\nPoint ToPoint(Vec p){return {p[0],p[1],p[2]};}\nvoid Finite(Point p){for(auto x:Values(p))if(!std::isfinite(x))throw std::invalid_argument("nonfinite");}\nstruct AtlasOps {void Work(size_t=1){}};\n'''
tri=part(new,'struct TriangleDistanceBounds', 'struct UnionTriangle')
wrap='''TriangleDistanceBounds OldTriangleDistance(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o,true);}\nTriangleDistanceBounds NewFull(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o,true);}\nTriangleDistanceBounds NewUpper(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o,false);}\n'''
def group(s,name,adj):
  helpers=part(s,'struct Range {','Box ParameterBox(') if name=='v142' else part(s,'struct Range {','double NormBound(')
  extra=part(s,'double NormBound(', 'std::array<Box,15> CubicNormalControls(') if name=='v142' else part(s,'double NormBound(', 'Point BoxMiddle(')
  middle=part(new,'Point BoxMiddle(', 'struct TriangleDistanceBounds')
  code=part(new,'long double Adjacent(', '// Each operation') if adj else ''
  return 'namespace '+name+' {\n'+code+helpers+extra+middle+tri+wrap+checks.read_text().replace('int main(){','int distance_check(){').replace('<<sink<<"\\n";','<<sink<<"\\n";return 0;')+'\n}\n'
# Distance141 standalone already excludes ParameterBox and contains helpers through DotBox.
body=prefix+group(old,'v141',False)+group(new,'v142',True)
intervaltest=interval.read_text()[interval.read_text().index('using Rational='):]
intervaltest=intervaltest.replace('int main()', 'int interval_check()').replace('checks','interval_checks').replace('int interval_check()', 'void interval_check()')
body+='namespace v142 {\n'+intervaltest+'\n}\n'+(out/'neighbors.inc').read_text()
(out/'probe.cpp').write_text(body)
inputs=[source,previous,checks,interval,out/'generate.py',out/'neighbors.inc']
(out/'provenance.json').write_text(json.dumps({'sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},'generated_sha256':hashlib.sha256(body.encode()).hexdigest()},indent=2)+'\n')
