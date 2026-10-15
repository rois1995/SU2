#!/usr/bin/env python3
"""Independent exact geometry check of the field-directed joint placement replay."""
import hashlib,json,sys,math
from pathlib import Path
from audit_curved_cavity import frame
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent/'alignment_audit_v1'))
from baseline import footprints
from audit_cavities import sub,dot,cross

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 stage=sys.argv[1] if len(sys.argv)>1 else 'curved_joint_search'
 assert stage in ('curved_joint_search','curved_interior_search','curved_cover_probe','curved_cap_cover_probe','curved_cap_point_probe','curved_wall_refit_probe','curved_rim_refit_probe','curved_current_wall_refit_probe','curved_wall_front_probe','curved_refit_interior_point','curved_refit_interior_cover','curved_refit_interior_dependencies')
 receipt_path=HERE/(stage+'.json');receipt=json.loads(receipt_path.read_text());assert receipt['status'] in ('PASS','FAIL')
 folder=HERE/stage;wall=stage in ('curved_wall_refit_probe','curved_rim_refit_probe','curved_current_wall_refit_probe','curved_wall_front_probe')
 candidate='wall_front_repaired' if stage=='curved_wall_front_probe' and (folder/'wall_front_repaired.json').exists() else 'wall_refit_proposal' if wall else 'joint_private' if receipt['status']=='PASS' else 'joint_best_rejected'
 paths=[folder/(name+'.json') for name in ('wall_refit_original' if wall else 'joint_original',candidate)]
 for path in paths:assert sha(path)==receipt['artifacts'][path.name]
 old,on,outer,ov=frame(paths[0]);fresh,fn,new_outer,fv=frame(paths[1])
 assert outer==new_outer and all(on[n]==fn[n] for f in outer for n in f),'artificial interface changed'
 if not wall:assert ov==fv,'planar cap replacement changed cavity volume'
 assert old['original']==fresh['original'],'original sensor donors changed'
 assert old['facets']==fresh['facets'] and old['marker_names']==fresh['marker_names'],'geometry authority changed'
 rows=footprints(fresh,'cylinder');height_error=0.
 for row in rows:
  a,b,c=[fn[n] for n in row['face']];normal=cross(sub(b,a),sub(c,a))
  height=abs(float(dot(sub(fn[row['apex']],a),normal)))/math.sqrt(float(dot(normal,normal)))
  height_error=max(height_error,abs(height-.05)/.05)
 assert height_error<=1e-10
 exact_feet=all(not row['outside'] for row in rows)
 report=dict(status='PASS_LOCAL_GEOMETRY_ONLY' if exact_feet else 'FAIL_EXACT_NORMAL_FOOT',normal_feet=rows,native_status=receipt['status'],old_cells=len(old['cells']),new_cells=len(fresh['cells']),
   wall_faces=len(rows),maximum_height_relative_error=height_error,interface_faces=len(outer),old_volume=str(ov),new_volume=str(fv),input_sha256={p.name:sha(p) for p in paths},
   native_receipt_sha256=sha(receipt_path),helper_sha256=sha(Path(__file__)),geometry_helper_sha256=sha(HERE/'audit_curved_cavity.py'),
   footprint_helper_sha256=sha(HERE.parent/'alignment_audit_v1/baseline.py'),
   scope='Local wall normal feet/height, exact positive volumes, oriented manifold incidence, no positive-volume pair overlap, fixed artificial interface and unchanged original donors/reference. Volume equality is required for flat caps/interior only; curved projection changes physical skin/volume. Exact normal-foot containment is reported separately from the native numerical tolerance; a tiny negative weight still fails this exact audit. Native status is separate from geometric evidence; consult the native metric rejection when present. No whole-grid completion, independent smooth-wall/full-field integration, transfer or performance qualification.')
 (HERE/('joint_search_geometry.json' if stage=='curved_joint_search' else 'cover_geometry.json' if stage=='curved_cover_probe' else 'cap_cover_geometry.json' if stage=='curved_cap_cover_probe' else 'cap_point_geometry.json' if stage=='curved_cap_point_probe' else 'front_refit_geometry.json' if stage=='curved_wall_front_probe' else 'rim_refit_geometry.json' if stage=='curved_rim_refit_probe' else 'current_wall_refit_geometry.json' if stage=='curved_current_wall_refit_probe' else 'wall_refit_geometry.json' if wall else stage+'_geometry.json' if stage.startswith('curved_refit_interior_') else 'interior_search_geometry.json')).write_text(json.dumps(report,indent=2)+'\n');print(report['status']);sys.exit(0 if exact_feet else 1)
if __name__=='__main__':main()
