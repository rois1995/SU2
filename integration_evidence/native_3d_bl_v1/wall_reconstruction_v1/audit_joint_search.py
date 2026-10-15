#!/usr/bin/env python3
"""Independent exact geometry check of the field-directed joint placement replay."""
import hashlib,json
from pathlib import Path
from audit_curved_cavity import frame
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 receipt_path=HERE/'curved_joint_search.json';receipt=json.loads(receipt_path.read_text());assert receipt['status']=='PASS'
 folder=HERE/'curved_joint_search'
 paths=[folder/(name+'.json') for name in ('joint_original','joint_private')]
 for path in paths:assert sha(path)==receipt['artifacts'][path.name]
 old,on,outer,ov=frame(paths[0]);fresh,fn,new_outer,fv=frame(paths[1])
 assert outer==new_outer and all(on[n]==fn[n] for f in outer for n in f),'artificial interface changed'
 assert ov==fv,'planar cap replacement changed cavity volume'
 assert old['original']==fresh['original'],'original sensor donors changed'
 assert old['facets']==fresh['facets'] and old['marker_names']==fresh['marker_names'],'geometry authority changed'
 report=dict(status='PASS_LOCAL_GEOMETRY_ONLY',old_cells=len(old['cells']),new_cells=len(fresh['cells']),
   interface_faces=len(outer),volume=str(ov),input_sha256={p.name:sha(p) for p in paths},
   native_receipt_sha256=sha(receipt_path),helper_sha256=sha(Path(__file__)),geometry_helper_sha256=sha(HERE/'audit_curved_cavity.py'),
   scope='Exact positive volumes, oriented manifold incidence, no positive-volume pair overlap, fixed artificial interface, equal cavity volume and unchanged original donors/reference. Native success requires full actual-field Refine/first-height gates. This does not establish whole-grid completion, independent smooth-wall/full-field integration, solution transfer or performance.')
 (HERE/'joint_search_geometry.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
if __name__=='__main__':main()
