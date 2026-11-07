"""Bounded discrete compression-width assessment from existing same-seed cuts."""
from pathlib import Path
import csv,hashlib,json,sys
import numpy as np
HERE=Path(__file__).resolve().parent;R=HERE.parent;repo=R.parents[3]
sys.path.insert(0,str(repo/'TestCases/adaptation/capability'));import capcheck
csvpath=HERE/'surface_cuts.csv';rows=list(csv.DictReader(csvpath.open()));sections=[]
for y in [.2,.4,.6,.8,1.0]:
 selected=[r for r in rows if float(r['y'])==y and r['side']=='upper'];X=np.array([[float(r[k]) for k in ['x_over_c','Mach','Cp']] for r in selected]);assert (np.diff(X[:,0])>=0).all()
 dx=np.diff(X[:,0]);dm=-np.diff(X[:,1]);dc=np.diff(X[:,2]);eligible=(dx>1e-8)&(X[:-1,0]>.15)&(X[1:,0]<.9)&(dm>0)&(dc>0)
 slope=np.zeros(len(dx));slope[eligible]=dm[eligible]/dx[eligible];peak=int(slope.argmax());assert slope[peak]>0
 lo=hi=peak
 while lo>0 and eligible[lo-1] and slope[lo-1]>=.5*slope[peak]:lo-=1
 while hi+1<len(dx) and eligible[hi+1] and slope[hi+1]>=.5*slope[peak]:hi+=1
 width=float(X[hi+1,0]-X[lo,0]);drop=float(X[lo,1]-X[hi+1,1]);rise=float(X[hi+1,2]-X[lo,2]);assert width>0 and drop>0 and rise>0
 sections.append({'y':y,'peak_x_over_c':float(.5*(X[peak,0]+X[peak+1,0])),'maximum_negative_Mach_slope_per_chord':float(slope[peak]),'connected_half_peak_band_x_over_c':[float(X[lo,0]),float(X[hi+1,0])],'connected_half_peak_width_over_chord':width,'cut_segments_in_band':hi-lo+1,'Mach_drop_in_band':drop,'Cp_rise_in_band':rise})
seed=R/'metric_target244_mach_only/L1/baseline_euler_seed.dat';continued=R/'metric_target244_mach_only/L1/restart_flow.dat'
P,F,_=capcheck.read_restart(seed);Q,G,_=capcheck.read_restart(continued);assert np.array_equal(P,Q)
changes={k:float(np.max(abs(F[k]-G[k]))) for k in ['Mach','Pressure_Coefficient','Density','Energy']};assert all(np.isfinite(list(changes.values())))
report={'scope':'Existing coarse-grid piecewise-linear upper-surface cuts on the qualified seed continuation. A connected half-maximum negative-Mach-gradient band is a discrete compression-width diagnostic,NOT a complete lambda-shock identification or proven physical shock thickness/resolution. No new CFD/target/remeshing run.','definition':'Within x/c .15.. .9,require Mach decrease and Cp rise;choose steepest cut segment,extend adjacent eligible segments while slope>=half peak. Width is endpoint x/c difference,not an interpolation-error bound.','sections':sections,'one_CFD_continuation_max_absolute_field_change':changes,'seed_points':len(P),'sha256':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [csvpath,seed,continued,Path(__file__)]}}
(HERE/'seed_flow_resolution.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n');print([(s['y'],s['connected_half_peak_width_over_chord'],s['Mach_drop_in_band']) for s in sections]);print(changes)
