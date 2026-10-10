"""Small sequential actual non-MPI engine build/control, with source and artifact pins."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[2]
folder = Path(sys.argv[1]).resolve()
print('Case working and retained folder:', folder, flush=True)
folder.mkdir(exist_ok=False)
env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1')
units = ['Common/src/adaptation/CNative'+n+'3D.cpp' for n in
         ['Predicates','Topology','Cavity','Boundary','Field','Metric','Distributed','Donor','Engine']]
house = ['Common/src/adt/CADTBaseClass.cpp','Common/src/adt/CADTElemClass.cpp',
         'Common/include/parallelization/mpi_structure.cpp','Common/src/parallelization/CPassiveComm.cpp',
         'Common/src/adaptation/CDistributedSearch.cpp','Common/src/adaptation/CTransferMemory.cpp']
control = 'integration_evidence/native_3d_core_v1/serial_engine.cpp'
# Reuse the completed MPI campaign's dependency pin list, plus this standalone control.
receipt = json.loads((folder.parent/'validation.json').read_text())
files = list(receipt['source_sha256'])+[control,str(Path(__file__).resolve().relative_to(root))]
def pins(): return {p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in files}
initial = pins()
records = []
def run(command, name):
    print('Starting', name, flush=True)
    start = time.monotonic()
    p = subprocess.run(command,cwd=root,env=env,text=True,capture_output=True)
    (folder/(name+'.stdout')).write_text(p.stdout)
    (folder/(name+'.stderr')).write_text(p.stderr)
    records.append(dict(stage=name,command=list(map(str,command)),exit_code=p.returncode,
                        seconds=time.monotonic()-start))
    if p.returncode: raise RuntimeError(name+' failed; evidence retained in '+str(folder))
    return p.stdout
for command,name in [(['uptime'],'load'),(['free','-m'],'memory'),
                     (['ps','-eo','pid,ppid,ni,pcpu,args','--sort=-pcpu'],'processes')]:
    p=subprocess.run(command,text=True,capture_output=True)
    (folder/('before_'+name+'.txt')).write_text(p.stdout if name!='processes' else
        '\n'.join(p.stdout.splitlines()[:16])+'\n')
status='FAIL'
try:
    assert receipt['status']=='PASS'
    assert all(initial[p]==h for p,h in receipt['source_sha256'].items()), 'MPI receipt sources changed'
    compiler=['nice','-n','19','g++','-std=c++17','-O2','-I.','-Iexternals/eigen',
              '-fno-fast-math','-fno-unsafe-math-optimizations','-ffp-contract=off',
              '-ffunction-sections','-fdata-sections']
    objects=[]
    for source in units+house:
        obj=folder/(Path(source).stem+'.o')
        flags=['-Wall','-Wextra','-Werror','-Wno-unused-parameter'] if source in units else []
        run(compiler+flags+['-c',source,'-o',str(obj)],'compile_'+Path(source).stem)
        objects.append(str(obj))
    binary=folder/'serial_engine'
    run(compiler+[control]+objects+['-Wl,--gc-sections','-o',str(binary)],'link')
    assert 'PASS actual non-MPI engine' in run(['nice','-n','19','/usr/bin/time','-v',str(binary)],'run')
    status='PASS'
finally:
    final=pins()
    changed=[p for p in files if initial[p]!=final[p]]
    if changed: status='FAIL'
    result=dict(status=status,base_commit=receipt['base_commit'],source_sha256=initial,
                final_source_sha256=final,changed_during_run=changed,stages=records,
                scope='Actual non-MPI engine refine/general-coarsen/refine, planar skin, strict metric gates and accepted-input rejection preservation. No CFD, BL, CGNS or scaling claim.')
    result['artifact_sha256']={str(p.relative_to(folder)):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in folder.rglob('*') if p.is_file() and p.suffix!='.o' and p.name!='serial_engine'}
    if (folder/'serial_engine').exists(): result['binary_sha256']=hashlib.sha256((folder/'serial_engine').read_bytes()).hexdigest()
    (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(status,folder,flush=True)
