#!/usr/bin/env python3
"""Assess saved M6 artifacts; a solver exit alone never implies adapted-mesh acceptance."""
from pathlib import Path
import csv, hashlib, json, re, sys
import numpy as np
CASE = Path(__file__).resolve().parent
ROOT = CASE.parents[2]
sys.path.insert(0, str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2, read_restart

def sha(p):
    h = hashlib.sha256()
    with p.open('rb') as stream:
        for block in iter(lambda:stream.read(1048576),b''): h.update(block)
    return h.hexdigest()

def history(path):
    rows = list(csv.reader(path.open()))
    names = [s.strip().strip('"') for s in rows[0]]
    return {k:np.array([float(r[i]) for r in rows[1:]]) for i,k in enumerate(names)}

def main():
    retry = CASE/'retry_workspace_fix'; flow = CASE/'flow_checkpoint'
    assert int((CASE/'exit_code.txt').read_text()) == 134
    assert int((retry/'exit_code.txt').read_text()) == 1
    assert int((flow/'exit_code.txt').read_text()) == 0
    text = (retry/'native.log').read_text()
    assert 'Native geometric complexity cell 13: Native3D BL volume quadrature point budget exhausted.' in text
    assert '0 WLS fallbacks.' in text and 'NATIVE3D rank=' not in text
    assert not list(CASE.rglob('adapted_mesh*.su2'))
    a,b,c = [history(p/'history.csv') for p in (CASE,retry,flow)]
    common = sorted(set(a)&set(b)&set(c))
    for k in common:
        assert len(a[k]) == 120 and np.array_equal(a[k],b[k]) and np.array_equal(a[k],c[k]), k
    assert (CASE/'run.cfg').read_bytes() == (retry/'run.cfg').read_bytes()
    mesh = read_su2(CASE/'Coarse.su2'); points, fields, precision = read_restart(flow/'restart_flow.dat')
    coord_error = float(np.max(abs(points-mesh.P))); assert coord_error <= 1e-12
    assert all(np.isfinite(v).all() for v in fields.values())
    rho = fields['Density']; assert rho.min() > 0
    momentum2 = sum(fields['Momentum_'+d]**2 for d in 'xyz')
    pressure = .4*(fields['Energy']-.5*momentum2/rho); assert pressure.min() > 0
    pins = json.loads((retry/'provenance.json').read_text())
    assert sha(Path(pins['binary'])) == pins['binary_sha256']
    for name,digest in pins['source_sha256'].items(): assert sha(ROOT/name) == digest, name
    for p in [CASE,retry,flow]:
        provenance = json.loads((p/'provenance.json').read_text())
        assert sha(p/'Coarse.su2') == provenance['mesh_sha256']
        assert sha(p/'run.cfg') == provenance['config_sha256']
    report = {'status':'FAIL_BEFORE_REMESH','first_attempt':'120 CFD iterations, then QLS missing-workspace assertion; fixed shared allocation.',
              'fixed_retry':'120 CFD iterations and full MPI QLS recovery pass; thin-BL complexity integration exhausts65536 points on original global cell13.',
              'native_remesh_entered':False,'candidate_generated':False,'accepted_adapted_grid':False,
              'flow_checkpoint':'PASS_FLOW_EXPORT_ONLY','history_common_columns':common,'history_common_columns_exact_equal':True,
              'iterations':120,'restart_precision':precision,'restart_mesh_coordinate_max_error':coord_error,
              'restart_all_fields_finite':True,'minimum_density':float(rho.min()),'minimum_ideal_gas_pressure':float(pressure.min()),
              'quadratic_workspace_regression':'PASS:1 case/32260 assertions',
              'hessian_max_rank_seconds':float(re.search(r'Hessian recovery: ([\d.]+)',text)[1]),
              'failure_location':json.loads((CASE/'failure_cell_13.json').read_text()),
              'limitations':['Unconverged flow; no viscous accuracy, remeshing, mesh-acceptance or solution-transfer result.',
                             'No geometry, first-height, fade, quality, integration tolerance or work-limit relaxation. No MMG substitution.'],
              'source_binary_input_pins_verified':True}
    (CASE/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    paths = [p for p in CASE.rglob('*') if p.is_file() and p.name not in {'artifacts.json'}]
    (CASE/'artifacts.json').write_text(json.dumps({str(p.relative_to(CASE)):sha(p) for p in sorted(paths)},indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='failure_location'},indent=2))
if __name__ == '__main__': main()
