#!/usr/bin/env python3
"""Exact local geometry check of the saved long-cap-edge operator replay."""
import hashlib,json
from pathlib import Path
from audit_curved_cavity import frame
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 receipt=json.loads((HERE/'curved_edge_replay.json').read_text());assert receipt['status']=='PASS';folder=HERE/'curved_edge_replay';rows=[]
 for name in ['edge_original','edge_op1','edge_op2','edge_grown_old','edge_grown_proposal']:
  path=folder/(name+'.json');assert sha(path)==receipt['artifacts'][path.name]
  data,nodes,interface,volume=frame(path);rows.append(dict(file=path.name,cells=len(data['cells']),volume=str(volume),interface_faces=len(interface),sha256=sha(path)))
 report=dict(status='PASS_LOCAL_GEOMETRY_ONLY',scope='Exact positive volumes, oriented incidence and no positive-volume overlap of saved local proposals; not full actual-field/geometry authority or adaptation acceptance. Targeted fan growth remains metric-inadmissible.',rows=rows,native_receipt_sha256=sha(HERE/'curved_edge_replay.json'),helper_sha256=sha(Path(__file__)),geometry_helper_sha256=sha(HERE/'audit_curved_cavity.py'))
 (HERE/'curved_edge_geometry.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
if __name__=='__main__':main()
