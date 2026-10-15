#!/usr/bin/env python3
"""Independent edge-debt diagnosis of the exact shape-complete wall replay."""
import hashlib,itertools,json
from pathlib import Path
from fractions import Fraction as F
from audit_sensor import edge,base
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 full_path=HERE/'planar_wall_input.json';full=json.loads(full_path.read_text());nd={n[0]:tuple(map(F,n[1:])) for n in full['nodes']};report=json.loads((HERE/'planar_wall_residual.json').read_text());assert sha(full_path)==report['mesh_sha256']
 ap={x['apex'] for x in report['owners']};ids={c['id'] for c in full['cells'] if set(report['edge'])<=set(c['nodes']) or ap&set(c['nodes'])}
 for face in [(3,50319,31455),(8399,8403,8401),(8401,8403,50319),(13,8401,8403)]:ids|={c['id'] for c in full['cells'] if set(face)<=set(c['nodes'])}
 ids|={c['id'] for c in full['cells'] if 50319 in c['nodes']};old=[c for c in full['cells'] if c['id'] in ids];assert len(old)==31
 fresh=[];new={};log=HERE/'planar_wall_retirement_rejected_v1/native.log';receipt=json.loads((HERE/'planar_wall_retirement_rejected_v1.json').read_text());assert sha(log)==receipt['artifacts']['native.log']
 for line in log.read_text().splitlines():
  r=line.split()
  if r[:2]!=['MESH','planar_wall_replay_frame0_growth5']:continue
  nodes=[]
  for i in range(4):
   nid=int(r[4+4*i]);new[nid]=tuple(F(float(v)) for v in r[5+4*i:8+4*i]);nodes.append(nid)
  fresh.append(dict(nodes=nodes))
 assert fresh
 def edges(cells,nodes):return {tuple(sorted((a,b))):(nodes[min(a,b)],nodes[max(a,b)]) for c in cells for a,b in itertools.combinations(c['nodes'],2)}
 a,b=edges(old,nd),edges(fresh,new);removed=added=0;bad=[];original,_=base.sources(full)
 for name,rows,other in [('old',a,b),('fresh',b,a)]:
  for k,v in rows.items():
   if other.get(k)==v:continue
   length=edge(original,*v);loss=max(0.,length-1.8)**2
   if name=='old':removed+=loss
   else:
    added+=loss
    if loss:bad.append(dict(edge=k,points=[list(map(float,z)) for z in v],length=length))
 result=dict(status='REPRODUCED_REPAIRED_LENGTH_DEBT',old_cells=len(old),fresh_cells=len(fresh),removed_excess=removed,added_excess=added,introduced_oversized_edges=bad,log_sha256=sha(log),input_sha256=sha(full_path),helper_sha256=sha(Path(__file__)),metric_helper_sha256=sha(HERE/'audit_sensor.py'),scope='Independent analytic original-P1 plus unchanged square-wall BL edge integration for the shape-complete private replay; not global adaptation acceptance.')
 (HERE/'planar_wall_repaired_debt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
