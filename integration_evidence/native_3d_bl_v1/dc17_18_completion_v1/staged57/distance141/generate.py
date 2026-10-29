#!/usr/bin/env python3
import hashlib, json, pathlib, subprocess
root = pathlib.Path(__file__).resolve().parents[5]
out = pathlib.Path(__file__).resolve().parent
path = 'Common/src/adaptation/CNativeAffineReference.cpp'
old = subprocess.check_output(['git', 'show', '7fe65b3b95:' + path], cwd=root, text=True)
new = (root / path).read_text()
def part(s, begin, end): return s[s.index(begin):s.index(end)]
helpers = part(new, 'struct Range {', 'Box ParameterBox(') + part(new, 'double NormBound(', 'std::array<Box,15> CubicNormalControls(') + part(new, 'Point BoxMiddle(', 'struct TriangleDistanceBounds')
oldfn = part(old, 'TriangleDistanceBounds TriangleDistance(', 'struct UnionTriangle').replace('TriangleDistance(', 'OldTriangleDistance(', 1)
newfn = part(new, 'TriangleDistanceBounds TriangleDistance(', 'struct UnionTriangle')
prefix = '''#include <algorithm>\n#include <array>\n#include <cassert>\n#include <chrono>\n#include <cmath>\n#include <iostream>\n#include <limits>\n#include <stdexcept>\n#include <boost/multiprecision/cpp_int.hpp>\nstruct Point { double x,y,z; };\nusing Vec=std::array<double,3>;\nVec Values(Point p){return {p.x,p.y,p.z};}\nPoint ToPoint(Vec p){return {p[0],p[1],p[2]};}\nvoid Finite(Point p){for(auto x:Values(p))if(!std::isfinite(x))throw std::invalid_argument("nonfinite");}\nstruct AtlasOps {void Work(size_t=1){}};\n'''
signature = newfn[:newfn.index('{')]
has_flag = 'bool ' in signature
wrappers = 'TriangleDistanceBounds NewFull(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o' + (',true' if has_flag else '') + '); }\n'
wrappers += 'TriangleDistanceBounds NewUpper(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o' + (',false' if has_flag else '') + '); }\n'
probe = prefix + helpers + 'struct TriangleDistanceBounds {double lower=0,upper=INFINITY;};\n' + oldfn + newfn + wrappers + (out/'checks.inc').read_text()
(out/'probe.cpp').write_text(probe)
inputs = [root/path, out/'generate.py', out/'checks.inc', root/'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build140_prepared_surface/live_query.txt', root/'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build140_prepared_surface/live_bound_state.txt']
(out/'provenance.json').write_text(json.dumps({'old_revision':'7fe65b3b95','sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},'generated_sha256':hashlib.sha256(probe.encode()).hexdigest()},indent=2)+'\n')
