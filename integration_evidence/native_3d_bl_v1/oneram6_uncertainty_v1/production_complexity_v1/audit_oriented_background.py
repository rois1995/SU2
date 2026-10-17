"""Independent background Q18 at the production helper's positively oriented source order."""
import json,struct,sys,time
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from run import ROOT,FROZEN,sha
from support_correction import SHARED,DOMAIN
sys.path.insert(0,str(FROZEN))
from audit_sensor_mass import duffy_rule,logdet
BASE=Path(__file__).resolve().parent
manifest=json.loads((SHARED/'manifest.json').read_text());source=SHARED/'original.bin'
assert sha(source)==manifest['original_sha256']
with source.open('rb') as f:magic,count,core=struct.unpack('<QQd',f.read(24))
assert magic==0x4e41545352433344 and count==manifest['cells']
data=np.memmap(source,dtype=DOMAIN,mode='r',offset=24,shape=(count,))
rule,weights=duffy_rule();total=np.longdouble(0);negative=0;start=time.monotonic()
for begin in range(0,count,4096):
 rows=data[begin:min(count,begin+4096)];x=rows['coordinates'].astype(np.longdouble)
 a,b,c=x[:,1]-x[:,0],x[:,2]-x[:,0],x[:,3]-x[:,0]
 determinant=np.einsum('ij,ij->i',a,np.cross(b,c));assert np.all(determinant!=0)
 selected=determinant<0;negative+=int(np.count_nonzero(selected))
 raw=rows['sensor'].copy();raw[selected]=raw[selected][:,[1,0,2,3]]
 matrix=np.tril(raw)+np.swapaxes(np.tril(raw,-1),-2,-1)
 density=np.zeros(len(rows),dtype=np.longdouble)
 for beta,w in zip(rule,weights):
  sample=np.einsum('k,nkij->nij',beta,matrix)
  density+=w*np.exp(.5*logdet(sample))
 total+=np.sum(np.abs(determinant)/6*density,dtype=np.longdouble)
old=json.loads((FROZEN/'sensor_mass_audit.json').read_text())['sensor_q18']
result={'status':'PASS_INDEPENDENT_ORIENTED_SOURCE_Q18','sensor_q18':float(total),'previous_sorted_source_q18':old,
 'relative_change_of_different_point_rules':float(total/old-1),'cells':count,'swapped_cells':negative,'seconds':time.monotonic()-start,
 'source_sha256':sha(source),'helper_sha256':sha(Path(__file__)),
 'scope':'Same original RAW9 P1 field. Q18 samples change under vertex permutation; exact cubic determinant mean/bounds are invariant.'}
(BASE/'oriented_background_reference.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result),flush=True)
