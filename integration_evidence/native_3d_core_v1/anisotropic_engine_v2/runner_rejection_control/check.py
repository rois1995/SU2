import ast,hashlib,json,os,subprocess,sys,time
from pathlib import Path
root=Path.cwd();folder=Path(__file__).resolve().parent
source=root/'integration_evidence/native_3d_core_v1/run_distributed.py'
tree=ast.parse(source.read_text());function=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='run')
records=[];namespace=dict(subprocess=subprocess,time=time,folder=folder,root=root,env=dict(os.environ),records=records)
exec(compile(ast.Module(body=[function],type_ignores=[]),str(source),'exec'),namespace)
run=namespace['run'];original=folder.parent/'mpi1.stdout'
labels=('MESH coupled_initial ','MESH coupled_refined ','MESH coupled_coarsened ','MESH engine_rejected ')
fixture='\n'.join(x for x in original.read_text().splitlines() if x.startswith(labels))+'\n'
(folder/'native_rejected_records.txt').write_text(fixture)
command=[sys.executable,'-c',"import pathlib,sys;sys.stdout.write(pathlib.Path(sys.argv[1]).read_text());sys.stderr.write('forced runner command failure\\n');sys.exit(7)",str(folder/'native_rejected_records.txt')]
assert run(command,'rejected_mpi',allow_failure=True)==fixture
assert records[-1]['exit_code']==7
run([sys.executable,'integration_evidence/native_3d_core_v1/audit_distributed.py',str(folder/'rejected_mpi.stdout'),str(folder/'meshes')],'audit_rejected')
assert (folder/'meshes/engine_rejected.su2').exists()
rejected=False
try:run([sys.executable,'-c',"import sys;print('retained failure');sys.exit(7)"],'required_failure')
except RuntimeError:rejected=True
assert rejected and (folder/'required_failure.stdout').read_text()=='retained failure\n'
result=dict(status='PASS',source_sha256={str(source.relative_to(root)):hashlib.sha256(source.read_bytes()).hexdigest()},
 original_log_sha256=hashlib.sha256(original.read_bytes()).hexdigest(),stages=records,
 scope='Actual runner helper exercised with forced command exit7 and saved real native one-round rejected mesh records: failure retained, subsequent independent audit/SU2 export succeeds; default required failure still raises. This is not an MPI process-failure recovery test.')
result['artifact_sha256']={str(p.relative_to(folder)):hashlib.sha256(p.read_bytes()).hexdigest() for p in folder.rglob('*') if p.is_file()}
(folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS forced exit7 preserved records and rejected SU2 export; default failure still raises')
