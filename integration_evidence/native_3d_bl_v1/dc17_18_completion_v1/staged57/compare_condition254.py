"""Same-seed before/after target assessment using the existing diagnostic only."""
from pathlib import Path
import re,json,hashlib,sys,numpy as np
r=Path(__file__).resolve().parent;repo=r.parents[3]
sys.path.insert(0,str(repo/'TestCases/adaptation/capability'));import capcheck
old=r/'metric_target253_hessian_trace_v1/L1';new=r/'metric_target254_conditioned_v1/L1'
P,F,_=capcheck.read_restart(old/'restart_flow.dat');Q,G,_=capcheck.read_restart(new/'restart_flow.dat')
assert np.array_equal(P,Q) and F.keys()==G.keys()
unchanged={k:np.array_equal(v,G[k]) for k,v in F.items() if not k.startswith(('Hessian_','Metric_'))}
assert all(unchanged.values()),unchanged
out=r/'target_delivery254_v1';out.mkdir(exist_ok=True)
base=r/'target_delivery253_v1/analyze_target.py';code=base.read_text();header='HERE=Path(__file__).resolve().parent;R=HERE.parent;REPO=R.parents[3]'
assert code.count(header)==1
code=code.replace(header,"R=Path(__file__).resolve().parent.parent;HERE=R/'target_delivery254_v1';REPO=R.parents[3]")
assert code.count('metric_target253_hessian_trace_v1/L1')==1
code=code.replace('metric_target253_hessian_trace_v1/L1','metric_target254_conditioned_v1/L1')
text=(new/'run.log').read_text();pre=float(re.search(r'Sensor metric complexity before gradation: ([0-9.e+-]+)',text)[1].rstrip('.'))
assert code.count('11961.4')==2;code=code.replace('11961.4',repr(pre))
exec(compile(code,str(base),'exec'),{'__file__':str(base),'__name__':'conditioned_target_analysis'})
Hkeys=[k for k in F if k.startswith('Hessian_')]
changed=np.any(np.column_stack([F[k]!=G[k] for k in Hkeys]),axis=1)
r0=json.loads((r/'target_delivery253_v1/target_diagnosis.json').read_text());r1=json.loads((out/'target_diagnosis.json').read_text())
report={'scope':'Same CFD seed,config/constraints bit-identical,one base-fit conditioning correction;no adaptation acceptance or speed claim.',
'unchanged_non_hessian_metric_fields_bit_identical':unchanged,'positions_bit_identical':True,'hessian_changed_points_exact':int(changed.sum()),
'hessian_relative_change_points_gt_1e8':int((np.sqrt(sum((F[k]-G[k])**2 for k in Hkeys))>1e-8*np.maximum(np.sqrt(sum(F[k]**2 for k in Hkeys)),1e-300)).sum()),
'raw_hessian_frobenius_max_before':float(max(np.sqrt(sum(F[k]**2*(2 if k.endswith(('XY','XZ','YZ')) else 1) for k in Hkeys)))),
'raw_hessian_frobenius_max_after':float(max(np.sqrt(sum(G[k]**2*(2 if k.endswith(('XY','XZ','YZ')) else 1) for k in Hkeys)))),
'old_shells':r0['shells'],'new_shells':r1['shells'],
'compression_strip_median_spacing_before':[s['final_spacing_over_chord']['median'] for s in r0['directional_compression_strips']],
'compression_strip_median_spacing_after':[s['final_spacing_over_chord']['median'] for s in r1['directional_compression_strips']],
'metric_log_lines':[line for line in text.splitlines() if any(k in line for k in ['Quadratic Hessian fits','Quadratic Hessian relative','Sensor metric complexity before','Mesh complexity after'])],
'analysis_transform':{'base_sha256':hashlib.sha256(base.read_bytes()).hexdigest(),'transformed_sha256':hashlib.sha256(code.encode()).hexdigest(),'changes':['Redirect output directory','Use corrected target input','Use actual rounded pre-gradation complexity from current log']},
'sha256':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [old/'restart_flow.dat',new/'restart_flow.dat',old/'run.cfg',new/'run.cfg',Path(__file__)]}}
# Independently check final SPD/bounds/gradation, reusing the established directed-edge audit.
mesh=capcheck.read_su2(new/'m6.su2');M=np.zeros((len(P),3,3))
for a,b,k in [(0,0,'XX'),(0,1,'XY'),(0,2,'XZ'),(1,1,'YY'),(1,2,'YZ'),(2,2,'ZZ')]:M[:,a,b]=M[:,b,a]=G['Metric_'+k]
eig,V=np.linalg.eigh(M);assert np.isfinite(M).all() and (eig>0).all()
assert eig.min()>=1e-4*(1-1e-8) and eig.max()<=1e12*(1+1e-8)
aspect=np.sqrt(eig[:,-1]/eig[:,0]);assert aspect.max()<=1e6*(1+1e-8)
N=len(P);pairs=np.concatenate([mesh.E[:,[a,b]] for a in range(4) for b in range(4) if a!=b]);keys=np.unique(pairs[:,0]*N+pairs[:,1]);src=keys//N;dst=keys%N
inverse=(V*(1/np.sqrt(eig))[:,None,:])@V.transpose(0,2,1);bad=0;maximum=1.
for begin in range(0,len(keys),32768):
 i=src[begin:begin+32768];j=dst[begin:begin+32768];dx=P[i]-P[j]
 scale=1/(1+np.log(1.3)*np.sqrt(np.maximum(0,np.einsum('ni,nij,nj->n',dx,M[j],dx))))**2
 ratio=np.linalg.eigvalsh(inverse[i]@M[j]@inverse[i])[:,-1]*scale
 bad+=int(np.count_nonzero(ratio>1+1e-5));maximum=max(maximum,float(ratio.max()))
assert bad==0
report['final_target_constraints']={'complexity':r1['final_complexity'],'min_eigenvalue':float(eig.min()),'max_eigenvalue':float(eig.max()),'max_aspect_ratio':float(aspect.max()),'directed_edges':len(keys),'gradation_violations':bad,'maximum_transported_ratio':maximum,'threshold':1+1e-5}
(out/'comparison.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
print({k:report[k] for k in ['hessian_changed_points_exact','hessian_relative_change_points_gt_1e8','raw_hessian_frobenius_max_before','raw_hessian_frobenius_max_after']})
print('shock before',report['compression_strip_median_spacing_before']);print('shock after',report['compression_strip_median_spacing_after'])
