"""Diagnostic only: test a normal-spacing cap using the existing field probe and production compile flags."""
import hashlib
import json
import shlex
import subprocess
import sys
from pathlib import Path
root = Path.cwd()
evidence = root / 'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1'
build = root / 'integration_evidence/build-integrated-v2'
previous = evidence / 'validation20'
out = evidence / 'validation21'
out.mkdir(exist_ok=True)
source = root / 'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp'
s = source.read_text()
old = 'const std::array<double,3>* curvedDirection=nullptr) {'
assert s.count(old) == 1
s = s.replace(old, 'const std::array<double,3>* curvedDirection=nullptr, su2double minimumNormal=0) {')
old = '  const auto ln=-2*std::log(SU2_TYPE::GetValue(sample.hn));'
assert s.count(old) == 1
s = s.replace(old, '''  if(minimumNormal>0)sample.hn=min(sample.hn,max(minimumNormal,su2double(std::exp(std::min(l0,l1)/2))));
''' + old)
old = 'WallMetric3D(tangentSize,sample,curvature,curvedDirection);'
assert s.count(old) == 1
s = s.replace(old, 'WallMetric3D(tangentSize,sample,curvature,curvedDirection,transitionCore>0?d.h0:su2double(0));')
copy = out / 'boundary_layer_diagnostic.cpp'
copy.write_text(s)
rows = json.loads((build / 'compile_commands.json').read_text())
row = next(r for r in rows if r['file'].endswith('/CBoundaryLayerMetric.cpp'))
args = shlex.split(row['command'])
command = []
i = 0
while i < len(args):
    arg = args[i]; i += 1
    if arg in ('-o', '-MF', '-MQ', '-MT'):
        i += 1; continue
    if arg in ('-MD', '-MMD', '-c', '-g') or arg.endswith('.cpp'):
        continue
    command.append(arg)
obj = out / 'boundary_layer_diagnostic.o'
command += ['-I' + str(source.parent), '-g0', '-c', str(copy), '-o', str(obj)]
link = json.loads((previous / 'diagnostic_build.json').read_text())['link']
link[2] = str(out / 'probe')
link.insert(4, str(obj))
files = [source, Path(__file__), copy, previous/'probe_budget.cpp',previous/'complexity_diagnostic.cpp', build/'compile_commands.json']
for arg in link:
    f = build / arg
    if f.is_file(): files.append(f.resolve())
(out/'source_runtime_pins.json').write_text(json.dumps({str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in files},indent=2)+'\n')
(out/'commands.json').write_text(json.dumps([command,link],indent=2)+'\n')
subprocess.run(command,cwd=build,check=True)
subprocess.run(link,cwd=build,check=True)
