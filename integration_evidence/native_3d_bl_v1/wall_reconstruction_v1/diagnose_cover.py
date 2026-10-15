#!/usr/bin/env python3
"""Independently enumerate support covers on the actual failed interior replay."""
import collections,hashlib,itertools,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 folder=HERE/'curved_reconnection_probe';rp=HERE/'curved_reconnection_probe.json';receipt=json.loads(rp.read_text());assert receipt['status']=='FAIL'
 for name in ('audited_grid.json','joint_original.json','native.log'):assert sha(folder/name)==receipt['artifacts'][name]
 data=json.loads((folder/'audited_grid.json').read_text());old=json.loads((folder/'joint_original.json').read_text())
 match=re.search(r'EDGE_REPLAY edge=(\d+),(\d+)',(folder/'native.log').read_text());assert match;edge=set(map(int,match.groups()))
 cells=[set(c['nodes']) for c in old['cells']];assert all(edge<=c for c in cells)
 global_physical={n for f in data['surface'] for n in f['nodes']}
 endpoint_cells=[set(c['nodes']) for c in data['cells'] if edge.intersection(c['nodes'])]
 known_physical={n for f in data['surface'] if any(set(f['nodes'])<=c for c in endpoint_cells) for n in f['nodes']}
 def union(ids):return len([c for c in data['cells'] if set(ids).intersection(c['nodes']) or edge<=set(c['nodes'])])
 todo=cells[:];greedy=[]
 while todo:
  counts=collections.Counter(n for c in todo for n in c if n not in edge|known_physical)
  assert counts;n=max(sorted(counts),key=counts.get);greedy.append(n);todo=[c for c in todo if n not in c]
 candidates=sorted(set.union(*cells)-edge-global_physical);costs={n:sum(n in c['nodes'] for c in data['cells']) for n in candidates};covers=[]
 for count in range(1,min(8,len(candidates))+1):
  for ids in itertools.combinations(candidates,count):
   if all(set(ids).intersection(c) for c in cells):covers.append(dict(vertices=ids,cost=sum(costs[n] for n in ids),cells=union(ids)))
 assert covers;best=min(covers,key=lambda c:(c['cost'],c['vertices']));assert union(greedy)>64 and best['cells']<=64
 report=dict(status='GREEDY_SUPPORT_BLOCKAGE_REPRODUCED',edge=sorted(edge),old_cells=len(cells),greedy=dict(vertices=greedy,cells=union(greedy)),best=best,free_star_costs=costs,
  native_receipt_sha256=sha(rp),helper_sha256=sha(Path(__file__)),scope='Independent exhaustive cover enumeration on source-pinned actual mesh; global physical vertices are excluded from authoritative surface records. Reproduces an oversized greedy dependency set and a smaller admissible set. This is a support-selection diagnosis, not proof of metric reconstruction or final mesh acceptance.')
 (HERE/'cover_diagnosis.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],report['greedy'],report['best'])
if __name__=='__main__':main()
