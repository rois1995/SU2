"""Join every raw cached metric record to the independently read exported cell ID."""
from pathlib import Path
import hashlib,json,time
import meshio
import numpy as np
R=Path(__file__).resolve().parent;A=R/'m6_runs/euler_build274_metric_cleanup_mach_only_real_cfd50k';D=A/'inspection'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
started=time.monotonic();out=A/'inspection_raw_metric_reader_status.json';assert not out.exists()
prior=json.loads((A/'inspection_reader_status.json').read_text());assert prior['all_reader_checks_passed']
ready=json.loads((A/'capture_ready.json').read_text());inputs=[Path(__file__),D/'rejected_mesh.vtu',A/'capture_ready.json',A/'inspection_reader_status.json'];pins={str(p):sha(p) for p in inputs}
v=meshio.read(D/'rejected_mesh.vtu');f={k:blocks[0] for k,blocks in v.cell_data.items()};ids=f['global_cell_id'];index={int(cid):i for i,cid in enumerate(ids)};seen=set()
assert ready['native_totals']['size_bad']==0
for p in sorted(A.glob('adapted_mesh_inspection_rank*_manifest.json')):
 pins[str(p)]=sha(p);assert pins[str(p)]==ready['files'][str(p.resolve())]['sha256']
 m=json.loads(p.read_text());assert m['metrics_complete'];raw=A/m['files']['metrics'];pins[str(raw)]=sha(raw);assert pins[str(raw)]==ready['files'][str(raw.resolve())]['sha256']
 count=0
 for line in raw.open():
  row=json.loads(line);assert len(row)==9
  cid,version,mask,q,j,g,edges,bindings,metrics=row;i=index[cid];assert cid not in seen;seen.add(cid);count+=1
  assert mask&15==15 and f['cache_valid'][i]==1
  for name,value in [('q',q),('J',j),('geometry_excess_l2',g)]:assert float(f[name][i])==value,(cid,name)
  assert len(edges)==6
  assert all(np.isfinite(e[2]) for e in edges)
  assert float(f['metric_edge_min'][i])==min(e[2] for e in edges)
  assert float(f['metric_edge_max'][i])==max(e[2] for e in edges)
 assert count==m['written_rows']
assert len(seen)==len(index)==ready['owned_rows']==734806
assert pins=={p:sha(p) for p in pins}
result={'state':'terminal','exit_code':0,'all_raw_metrics_match_exported_cell_ids':True,'cells':len(seen),'seconds':time.monotonic()-started,'size_height_failures':0,'global_two_sided_geometry_certificate_complete':False,'accepted':False,'pins':pins,'scope':'All raw q/J/local geometry and edge extrema match their exported global cell IDs exactly; no geometry acceptance proof.'}
out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='pins'}))
