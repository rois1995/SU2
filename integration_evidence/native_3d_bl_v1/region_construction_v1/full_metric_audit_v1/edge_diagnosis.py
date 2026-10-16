"""Convergence diagnosis for the saved full-edge audit discrepancy; no native rerun."""
import csv,json,math,shutil,sys,time,warnings
from pathlib import Path
import numpy as np
from scipy.integrate import quad
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent))
from audit_full_metric import CylinderField
from construct import sha

start=time.monotonic();case=HERE.parent/'native_core_rotated_v10';field=CylinderField(case)
d=json.loads((case/'core_accepted.json').read_text());nodes={n[0]:np.array(n[1:]) for n in d['nodes']}
row=next(r for r in csv.DictReader((case/'core_edges.csv').open()) if (int(r['a']),int(r['b']))==(482,483))
a,b=nodes[482],nodes[483];direction=b-a
cuts=field.cuts(a,b)
def speed(t):return math.sqrt(direction@field(a+t*direction)@direction)
results=[]
for tolerance in (1e-9,1e-11,1e-13):
    value=indicator=0.;evaluations=0
    with warnings.catch_warnings(record=True) as messages:
        warnings.simplefilter('always')
        for low,high in zip(cuts,cuts[1:]):
            v,e,info=quad(speed,low,high,epsabs=tolerance*(high-low),epsrel=tolerance,limit=512,full_output=1)[:3]
            value+=v;indicator+=e;evaluations+=info['neval']
    results.append(dict(method='Gauss-Kronrod21',tolerance=tolerance,value=value,indicator=indicator,evaluations=evaluations,warnings=[str(w.message) for w in messages]))
    print(results[-1],flush=True)
points,weights=np.polynomial.legendre.leggauss(32)
for count in (4,16,64,256):
    value=0.
    for low,high in zip(cuts,cuts[1:]):
        for i in range(count):
            left=low+(high-low)*i/count;right=low+(high-low)*(i+1)/count
            value+=(right-left)/2*sum(w*speed((left+right)/2+(right-left)/2*t) for t,w in zip(points,weights))
    results.append(dict(method='Composite Gauss-Legendre32',panels_per_shell=count,value=value));print(results[-1],flush=True)
report=dict(status='CONVERGENCE_DIAGNOSIS_ONLY',edge=[482,483],a=a.tolist(),b=b.tolist(),native_length=float(row['length']),cuts=cuts,results=results,
            helper_sha256=sha(Path(__file__)),field_helper_sha256=sha(HERE.parent/'audit_full_metric.py'),elapsed_seconds=time.monotonic()-start,
            scope='Independent field models agree with all saved centroids. Multiple quadrature checks of the first failed edge only; no native point-query comparison along this edge or full mesh metric qualification.')
(HERE/'edge_diagnosis.json').write_text(json.dumps(report,indent=2)+'\n')
