#!/usr/bin/env python3
"""Independent exact geometry/normal-foot audit of the joint operator witness, not metric acceptance."""
import hashlib,importlib.util,json,sys
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
import audit_coupled as base
spec=importlib.util.spec_from_file_location('footprints',HERE.parent/'alignment_audit_v1/baseline.py');foot=importlib.util.module_from_spec(spec);spec.loader.exec_module(foot)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def write_su2(data,path):
 nodes=data['nodes'];index={n[0]:i for i,n in enumerate(nodes)};groups={}
 for f in data['surface']:groups.setdefault(data['marker_names'][next(x['marker'] for x in data['facets'] if x['id']==f['facet'])],[]).append(f['nodes'])
 with path.open('w') as out:
  out.write('NDIME= 3\nNELEM= %d\n'%len(data['cells']))
  for c in data['cells']:out.write('10 '+' '.join(str(index[n]) for n in c['nodes'])+'\n')
  out.write('NPOIN= %d\n'%len(nodes))
  for i,n in enumerate(nodes):out.write(' '.join(format(x,'.17g') for x in n[1:])+' %d\n'%i)
  out.write('NMARK= %d\n'%len(groups))
  for marker,faces in groups.items():
   out.write('MARKER_TAG= %s\nMARKER_ELEMS= %d\n'%(marker,len(faces)))
   for f in faces:out.write('5 '+' '.join(str(index[n]) for n in f)+'\n')
def main():
 receipt=json.loads((HERE/'geometry.json').read_text());assert receipt['status']=='PASS';folder=HERE/'geometry'
 assert all(sha(folder/f)==h for f,h in receipt['artifacts'].items())
 old=json.loads((folder/'wall_split_initial.json').read_text());nodes={n[0]:n for n in old['nodes']}
 facets=[{'id':f[3],'marker':f[4],'vertices':[nodes[n] for n in f[:3]]} for f in old['surface']]
 original=[{'vertices':[nodes[n]+[[1,0,0,1,0,1]] for n in c]} for c in old['cells']]
 names=['marker_'+str(i) for i in range(1+max(f['marker'] for f in facets))];reports=[]
 walls={f['marker'] for f in facets if all(n[3]==0 for n in f['vertices'])}
 for label in ('wall_split_initial','wall_split_joint','wall_split_inherited_rejected'):
  raw=json.loads((folder/(label+'.json')).read_text())
  data={'nodes':raw['nodes'],'cells':[{'id':i,'nodes':c} for i,c in enumerate(raw['cells'])],'surface':[{'nodes':f[:3],'facet':f[3]} for f in raw['surface']],'facets':facets,'marker_names':names,'original':original}
  path=folder/(label+'_audit_input.json');path.write_text(json.dumps(data,indent=2)+'\n')
  rows=[r for marker in walls for r in foot.footprints(data,names[marker])]
  outside=sum(F(r['minimum_barycentric'])<0 for r in rows)
  if label.endswith('_rejected'):
   assert outside>0;report={'file':label,'status':'REPRODUCED_REJECTION','outside_footprints':outside,'scope':'Local inherited edge-star proposal, not a complete-domain grid'}
  else:
   assert outside==0
   report,_=base.frame(path,False,validate_shape=False)
   assert report['volume']=='1'
   report.update(status='PASS_GEOMETRY_ONLY',outside_footprints=outside,wall_faces=len(rows),minimum_footprint_barycentric=min(float(F(r['minimum_barycentric'])) for r in rows),scope='Exact embedding, positive unit volume, disjoint conforming cells and finite reference coverage; normal footprints. Unit metric values are diagnostics only, not adaptation acceptance.')
  write_su2(data,folder/(label+'_geometry_only.su2'));reports.append(report)
 result={'status':'PASS_GEOMETRY_ONLY','frames':reports,'native_receipt_sha256':sha(HERE/'geometry.json'),'helper_sha256':sha(Path(__file__)),'geometry_helper_sha256':sha(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),'footprint_helper_sha256':sha(HERE.parent/'alignment_audit_v1/baseline.py')}
 (HERE/'independent_geometry.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS_GEOMETRY_ONLY',flush=True)
if __name__=='__main__':main()
