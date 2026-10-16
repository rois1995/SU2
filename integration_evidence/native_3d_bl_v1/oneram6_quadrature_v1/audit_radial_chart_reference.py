#!/usr/bin/env python3
"""Audit completed independent M6 normal-density radial/chart references."""
from pathlib import Path
import contextlib,hashlib,io,json,runpy
import numpy as np
HERE=Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):old=runpy.run_path(str(HERE/'check_normal_integral.py'))
script=hashlib.sha256((HERE/'check_radial_chart_reference.py').read_bytes()).hexdigest()
oracle=hashlib.sha256((HERE/'check_normal_integral.py').read_bytes()).hexdigest()
fixture=hashlib.sha256((HERE/'wing_cell13.dat').read_bytes()).hexdigest()
refs=[];inputs={}
for apex in range(4):
 path=HERE/('radial_chart_apex%d_a72_r24_b1p25.json'%apex);r=json.loads(path.read_text())
 assert r['complete'] and r['volume_relative_error']<2e-12
 assert r['identity']=={'script_sha256':script,'oracle_sha256':oracle,'fixture_sha256':fixture,'numpy':np.__version__,'parameters':{'apex':apex,'angular':72,'radial':24,'bands':1.25}}
 refs.append(r);inputs[path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
radial=HERE/'radial_chart_apex3_a72_r16_b1p5.json';earlier=json.loads(radial.read_text())
assert earlier['complete'] and earlier['identity']['script_sha256']==script and earlier['identity']['oracle_sha256']==oracle and earlier['identity']['fixture_sha256']==fixture
spread=(max(r['integral'] for r in refs)-min(r['integral'] for r in refs))/max(r['integral'] for r in refs)
change=abs(refs[3]['integral']/earlier['integral']-1);assert spread<1e-4 and change<1e-4
native={}
for suffix in ('150','125'):
 path=HERE/'global_ray_v1'/('replay_rule_'+suffix+'.csv');table=np.genfromtxt(path,delimiter=',',names=True)
 bary=np.column_stack([table['b'+str(k)] for k in range(4)]);distance=old['distance'](bary@old['cell']);values=old['density'](distance)
 assert np.max(abs(distance-table['distance']))<4e-14 and np.max(abs(values/table['normal_density']-1))<2e-8
 assert np.all(table['weight']>0) and abs(table['weight'].sum()/old['volume']-1)<2e-10
 integral=float(table['weight']@values);errors=[abs(integral/r['integral']-1) for r in refs];assert max(errors)<5e-4
 native[suffix]={'points':len(table),'normal_integral':integral,'relative_errors_by_apex':errors,'worst_relative_error':max(errors),'rule_sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
report={'normal_reference_gate_pass':True,'scope':'Independent exact-triangle normal-density reference only. Radial16/24 convergence is checked on apex3; all4 apexes agree at72x24. This is numerical agreement, not a rigorous enclosure or each-chart separate angular convergence. Actual CFD sensors/full tensor integration/complete M6 mesh acceptance remain unqualified.','reference_agreement_tolerance':1e-4,'normal_tolerance':5e-4,'radial_relative_change_apex3':change,'four_chart_relative_spread':spread,'native':native,'references':refs,'sha256':dict(inputs,reference_script=script,oracle=oracle,fixture=fixture,audit_script=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())}
(HERE/'global_ray_v1/four_chart_audit.json').write_text(json.dumps(report,indent=2)+'\n')
print('NORMAL_REFERENCE_GATE_PASS radial_change=',change,'chart_spread=',spread,'worst_native_error=',native['150']['worst_relative_error'])
