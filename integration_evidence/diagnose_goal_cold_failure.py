"""Replay the failed goal cycle as an ordinary static-mesh cold adjoint."""
from pathlib import Path
import hashlib, json, os, re, shutil, signal, subprocess, sys, time

root = Path(__file__).resolve().parent
source = root.parent
prior = root/'integrated_goal_runtime_v7/cold_p2'
case = root/'goal_cold_failure_diagnostic_v1'
case.mkdir()
env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', PYTHONDONTWRITEBYTECODE='1')
def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''): h.update(block)
    return h.hexdigest()

sys.path.insert(0, str(source/'TestCases/adaptation/capability'))
import capcheck
from audit_native_bl import mesh
from check_airfoil_phase_timing import validate_solution
from run_native_production_checks_v1 import flow_rows
binary = root/'integrated_goal_runtime_v7/SU2_CFD_AD'
expected = json.loads((root/'goal_runtime_v7.json').read_text())['binary_sha256']
assert sha(binary) == expected
for source_name, destination in [('mesh_out_adap_00002.su2','input.su2'),
                                 ('restart_flow_adap_00002.dat','solution_flow.dat')]:
    shutil.copy2(prior/source_name, case/destination)
points, fields, precision = capcheck.read_restart(case/'solution_flow.dat')
flow = validate_solution(flow_rows(points, fields), mesh(case/'input.su2')[0])
assert precision == 'Float64'
lines = []
for line in (prior/'run.cfg').read_text().splitlines():
    if re.match(r'\s*(ADAP_|COMPUTE_METRIC|WRT_ADAP_MESH|MESH_OUT)', line): continue
    if re.match(r'\s*MESH_FILENAME\s*=', line): line = 'MESH_FILENAME= input.su2'
    if re.match(r'\s*ITER\s*=', line): line = 'ITER= 150'
    lines.append(line)
(case/'run.cfg').write_text('\n'.join(lines)+'\n')
record = dict(scope='Same saved flow/mesh and CFL50; static adjoint, no adaptation or mesh replacement',
              original_failed_case=str(prior), original_evidence_sha256=sha(prior/'evidence.json'),
              binary=str(binary), binary_sha256=expected, flow=flow,
              inputs_sha256={p.name:sha(p) for p in case.iterdir() if p.is_file()},
              runner_sha256=sha(__file__), ranks=2)
report=case/'evidence.json'
report.write_text(json.dumps(record, indent=2)+'\n')
since=None
while True:
    busy=[]
    for line in subprocess.check_output(['ps','-eo','pid,stat,comm'],text=True).splitlines()[1:]:
        pid,flags,name=line.split(maxsplit=2)
        if 'Z' not in flags and pid!='918696' and (name in ('ninja','cc1plus','test_driver','test_driver_AD','test_memory') or name.startswith('SU2_CFD')): busy.append(int(pid))
    if busy: since=None
    elif since is None: since=time.monotonic()
    elif time.monotonic()-since >= 15: break
    time.sleep(5)
command=['mpiexec','-n','2',str(binary),'run.cfg']; begun=time.monotonic()
with (case/'runtime.log').open('x') as log:
    child=subprocess.Popen(command,cwd=case,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
    record.update(command=command, child_pid=child.pid, phase='running');report.write_text(json.dumps(record,indent=2)+'\n')
    try: code=child.wait(timeout=240)
    except subprocess.TimeoutExpired:
        os.killpg(child.pid,signal.SIGTERM)
        try: child.wait(timeout=10)
        except subprocess.TimeoutExpired: os.killpg(child.pid,signal.SIGKILL);child.wait()
        code=124
text=(case/'runtime.log').read_text()
record.update(phase='terminal',exit=code,elapsed_seconds=time.monotonic()-begun,
              residual_divergence='SU2 has diverged (Residual > 10^20 detected).' in text,
              runtime_sha256=sha(case/'runtime.log'))
record.pop('child_pid',None)
report.write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps(record,indent=2))
