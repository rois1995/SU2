"""Small sequential native 3D geometry/incidence checks; no SU2/CFD build or MPI launch."""
import fractions
import hashlib
import json
import math
import os
from pathlib import Path
import random
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[2]
folder = Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'integration_evidence/native_3d_core_v1/geometry_controls_v1'
print('Case working and retained folder:',folder,flush=True)
folder.mkdir(exist_ok=False)
env = dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
records = []
def run(command,name,input_text=None):
    start = time.monotonic()
    p = subprocess.run(command,cwd=root,env=env,input=input_text,text=True,capture_output=True)
    (folder/(name+'.stdout')).write_text(p.stdout)
    (folder/(name+'.stderr')).write_text(p.stderr)
    records.append(dict(stage=name,command=list(map(str,command)),exit_code=p.returncode,seconds=time.monotonic()-start))
    assert p.returncode==0, name+' failed; evidence retained in '+str(folder)
    return p.stdout
status = 'FAIL'
try:
    (folder/'catch_main.cpp').write_text('#define CATCH_CONFIG_MAIN\n#include "catch.hpp"\n')
    (folder/'orientation_driver.cpp').write_text(
        '#include "Common/include/adaptation/CNativeMesh3D.hpp"\n'
        '#include <iostream>\n'
        'int main() { SU2Native3D::Tetrahedron t; SU2Native3D::KernelStats s;\n'
        'while(std::cin >> t[0].x >> t[0].y >> t[0].z >> t[1].x >> t[1].y >> t[1].z\n'
        ' >> t[2].x >> t[2].y >> t[2].z >> t[3].x >> t[3].y >> t[3].z) {\n'
        'auto d=SU2Native3D::Orientation(t[0],t[1],t[2],t[3],&s);std::cout << (d>0)-(d<0) << "\\n"; }\n'
        'std::cerr << "filtered " << s.filtered_orientations << " exact " << s.exact_orientations << "\\n"; }\n')
    compiler = ['nice','-n','19','g++','-std=c++17']
    strict = ['-O2','-fno-fast-math','-fno-unsafe-math-optimizations','-ffp-contract=off','-Wall','-Wextra','-Werror']
    obj = folder/'predicates.o'
    run(compiler+strict+['-c','Common/src/adaptation/CNativePredicates3D.cpp','-o',str(obj)],'compile_predicates')
    topology_obj = folder/'topology.o'
    run(compiler+strict+['-c','Common/src/adaptation/CNativeTopology3D.cpp','-o',str(topology_obj)],'compile_topology')
    run(compiler+['-O0','-Iexternals/catch2',str(folder/'catch_main.cpp'),
                 'UnitTests/Common/adaptation/CNativeMesh3D_tests.cpp',
                 'UnitTests/Common/adaptation/CNativeTopology3D_tests.cpp',str(obj),str(topology_obj),'-o',str(folder/'test_kernel')],'compile_tests')
    output = run(['nice','-n','19',str(folder/'test_kernel'),'[NativeMesh3D]','--use-colour','no'],'unit_tests')
    assert 'All tests passed' in output
    run(compiler+strict+['-I.',str(folder/'orientation_driver.cpp'),str(obj),'-o',str(folder/'orientation_driver')],'compile_orientation_driver')
    rng = random.Random(3102026)
    samples = []
    for _ in range(64):
        samples.append([[float(rng.randrange(-32,33)) for _ in range(3)] for _ in range(4)])
    for _ in range(64):
        samples.append([[math.ldexp(rng.uniform(-1,1),rng.randrange(-900,901)) for _ in range(3)] for _ in range(4)])
    epsilon = math.ldexp(1.,-52)
    for _ in range(96):
        scale = math.ldexp(1.,rng.randrange(-500,501))
        q = [[0.,0.,0.],[scale,scale*(1+epsilon),0.],[scale*(1-epsilon),scale,0.],[0.,0.,scale]]
        if rng.randrange(2):q[1],q[2]=q[2],q[1]
        samples.append(q)
    for _ in range(64):
        q = [[float(rng.randrange(-8,9)) for _ in range(3)] for _ in range(4)]
        if rng.randrange(2):q[3]=list(q[1])
        else:
            for p in q:p[2]=0.
        samples.append(q)
    tiny = float.fromhex('0x0.0000000000001p-1022')
    huge = sys.float_info.max
    samples.extend([[[0.,0.,0.],[tiny,0.,0.],[0.,tiny,0.],[0.,0.,tiny]],
                    [[-huge,0.,0.],[huge,0.,0.],[0.,huge,0.],[0.,0.,huge]]])
    def exact_sign(points):
        p = [[fractions.Fraction(x) for x in row] for row in points]
        a,b,c = [[x-y for x,y in zip(row,p[0])] for row in p[1:]]
        d = a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0])
        return (d>0)-(d<0)
    text = '\n'.join(' '.join(format(x,'.17g') for p in q for x in p) for q in samples)+'\n'
    (folder/'orientation_samples.txt').write_text(text)
    expected = [exact_sign(q) for q in samples]
    (folder/'orientation_expected.json').write_text(json.dumps(expected)+'\n')
    actual = list(map(int,run(['nice','-n','19',str(folder/'orientation_driver')],'exact_orientation_oracle',text).split()))
    assert actual==expected and len(actual)==290
    status = 'PASS'
finally:
    files = ['Common/include/adaptation/CNativeMesh3D.hpp','Common/src/adaptation/CNativePredicates3D.cpp',
             'UnitTests/Common/adaptation/CNativeMesh3D_tests.cpp',
             'Common/include/adaptation/CNativeTopology3D.hpp','Common/src/adaptation/CNativeTopology3D.cpp',
             'UnitTests/Common/adaptation/CNativeTopology3D_tests.cpp','Common/src/meson.build','UnitTests/meson.build',
             str(Path(__file__).resolve().relative_to(root))]
    result = dict(status=status,stages=records,source_sha256={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in files},
                  scope='Isolated geometry/incidence Catch controls and 290 binary64 tetrahedral signs against exact Fraction references. One-core sequential low-priority compilation; no full SU2 build, CFD, MPI, global embedding, complete distributed-star, surface/field or performance scaling qualification.')
    result['artifact_sha256'] = {str(p.relative_to(folder)):hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.iterdir() if p.is_file()}
    (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(status,folder,flush=True)
