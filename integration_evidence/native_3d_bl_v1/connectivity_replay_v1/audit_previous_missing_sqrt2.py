#!/usr/bin/env python3
"""Independent fixed-interface, actual frozen-field and wall-footprint replay audit; partial progress only."""
import hashlib,importlib.util,json,math
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;OLD=HERE.parent/'wall_reconstruction_v1'
spec=importlib.util.spec_from_file_location('oracle',OLD/'audit_sensor.py');oracle=importlib.util.module_from_spec(spec);spec.loader.exec_module(oracle)
spec=importlib.util.spec_from_file_location('geo',OLD/'audit_geometry.py');geo=importlib.util.module_from_spec(spec);spec.loader.exec_module(geo)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 receipt=json.loads((HERE/'replay.json').read_text());assert receipt['status']=='PASS';folder=HERE/'replay'
 assert all(sha(folder/f)==h for f,h in receipt['artifacts'].items())
 reference=json.loads((OLD/'geometry/wall_split_initial_audit_input.json').read_text());source=json.loads((OLD/'feasibility_control.json').read_text())['original'];reports=[];skins=[]
 for label in ('before','after'):
  raw=json.loads((folder/('connectivity_replay_'+label+'.json')).read_text());data={'nodes':raw['nodes'],'cells':[{'id':i,'nodes':c} for i,c in enumerate(raw['cells'])],'surface':[{'nodes':f[:3],'facet':f[3]} for f in raw['surface']],'facets':reference['facets'],'marker_names':reference['marker_names'],'original':source}
  path=folder/('independent_'+label+'.json');path.write_text(json.dumps(data,indent=2)+'\n')
  report,_=oracle.base.frame(path,False,validate_shape=False,metric_query=oracle.tensor,edge_query=oracle.edge);assert report['volume']=='1'
  nodes={n[0]:tuple(map(F,n[1:])) for n in raw['nodes']};loss=0;original,_=oracle.base.sources(data)
  for c in data['cells']:
   t=[nodes[n] for n in c['nodes']];center=tuple(F(float(sum(p[k] for p in t)/4)) for k in range(3));m=oracle.tensor(original,center)
   det=oracle.base.determinant(t);volume=float(det)*math.sqrt(float(oracle.base.spd(m)));edges=[oracle.base.sub(t[j],t[i]) for i in range(4) for j in range(i+1,4)];q=12*(volume/2)**(2/3)/sum(float(oracle.base.quad(m,e)) for e in edges)
   j=min(float(det)*math.sqrt(float(oracle.base.spd(m)))/math.sqrt(math.prod(float(oracle.base.quad(m,oracle.base.sub(t[b],t[a]))) for b in range(4) if b!=a)) for a in range(4))
   loss+=max(0,1-q/.2)**2+max(0,1-j/.05)**2
  rows=[r for marker in reference['marker_names'] for r in geo.foot.footprints(data,marker)]
  # Only bottom-wall markers10/12 are prescribed in this witness.
  walls=[r for r in rows if next(f['marker'] for f in data['facets'] if f['id']==r['facet']) in (10,12)]
  assert walls and all(F(r['minimum_barycentric'])>=0 for r in walls)
  report.update(shape_loss=loss,minimum_wall_footprint_barycentric=min(float(F(r['minimum_barycentric'])) for r in walls),full_admissible=report['minimum_mean_ratio']>=.2 and report['minimum_scaled_jacobian']>=.05 and report['maximum_length']<=1.8)
  skins.append(sorted((tuple(f[:3]),f[3],f[4]) for f in raw['surface']));reports.append(report);geo.write_su2(data,folder/('connectivity_'+label+'_private.su2'))
 assert skins[0]==skins[1];assert reports[1]['shape_loss']<reports[0]['shape_loss']
 result={'status':'PASS_PRIVATE_PROGRESS_ONLY','frames':reports,'fixed_skin':True,'native_receipt_sha256':sha(HERE/'replay.json'),'helper_sha256':sha(Path(__file__)),'metric_helper_sha256':sha(OLD/'audit_sensor.py'),'scope':'Exact global coverage, fixed physical interface, independent actual-field centroid shape and full composed edge measurements and prescribed wall footprints. Partial shape progress is not an accepted adaptation or production/MPI/CFD qualification.'}
 (HERE/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
if __name__=='__main__':main()
