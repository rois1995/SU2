#!/usr/bin/env python3
"""Exact local geometry check of the saved long-cap-edge operator replay."""
import hashlib,json
from pathlib import Path
from audit_curved_cavity import frame
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 receipt=json.loads((HERE/'curved_edge_replay.json').read_text());assert receipt['status']=='PASS';folder=HERE/'curved_edge_replay';rows=[]
 for name in ['edge_original','edge_op1','edge_op2','edge_grown_old','edge_grown_proposal','edge_apex_old','edge_joint_private']:
  path=folder/(name+'.json');assert sha(path)==receipt['artifacts'][path.name]
  data,nodes,interface,volume=frame(path);rows.append(dict(file=path.name,cells=len(data['cells']),volume=str(volume),interface_faces=len(interface),sha256=sha(path)))
 old,on,outer,ov=frame(folder/'edge_apex_old.json');fresh,fn,new_outer,fv=frame(folder/'edge_joint_private.json')
 assert outer==new_outer and all(on[n]==fn[n] for f in outer for n in f)
 assert ov==fv,'retained planar replacement changes cavity volume'
 assert old['original']==fresh['original'],'frozen sensor donors changed'
 report=dict(joint_interface_preserved=True,joint_volume_preserved=True,frozen_original_donors_preserved=True,status='PASS_LOCAL_GEOMETRY_ONLY',scope='Exact positive volumes, oriented incidence and no positive-volume overlap of saved local proposals; not full actual-field/geometry authority or adaptation acceptance. Targeted fan growth remains metric-inadmissible; the saved joint private proposal passed native original-field refinement checks, not final mesh acceptance.',rows=rows,native_receipt_sha256=sha(HERE/'curved_edge_replay.json'),helper_sha256=sha(Path(__file__)),geometry_helper_sha256=sha(HERE/'audit_curved_cavity.py'))
 (HERE/'curved_edge_geometry.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
if __name__=='__main__':main()
