from pathlib import Path
import json,sys,math,hashlib
from fractions import Fraction as F
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_coupled import sources,weights
r=json.loads((HERE/'probe.json').read_text());nodes={n[0]:tuple(F(x) for x in n[1:]) for n in r['best']['nodes']};first=nodes[0];last=nodes[12]
source=HERE.parent/'recovery_collapse_v1/mpi1/cylinder_axis1_m1_s0_source.json'
src,_=sources({'original':json.loads(source.read_text())});intervals=[]
for c in src:
 wa,wb=weights(c['points'],first),weights(c['points'],last);lo,hi=F(0),F(1)
 for x,y in zip(wa,wb):
  if x<0 and y<0:hi=F(-1);break
  if x<0:lo=max(lo,x/(x-y))
  if y<0:hi=min(hi,x/(x-y))
 if lo<hi:intervals.append((lo,hi))
end=F(0);gaps=[]
for lo,hi in sorted(intervals):
 if lo>end:gaps.append((end,lo))
 end=max(end,hi)
if end<1:gaps.append((end,F(1)))
length=math.sqrt(sum(float(y-x)**2 for x,y in zip(first,last)))
report={'edge':[0,12],'gap_intervals_exact':[[str(lo),str(hi)] for lo,hi in gaps],'gap_fraction':[float(hi-lo) for lo,hi in gaps],'gap_physical_length':[float(hi-lo)*length for lo,hi in gaps],'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'scope':'Exact rational segment clipping for scientific proposal coordinates; native reprojection may differ by ULP. No source extension or acceptance change.'}
(HERE/'cap_gap.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
