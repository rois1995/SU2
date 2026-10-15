#!/usr/bin/env python3
"""Audit closed MPI protocol stages without treating their rejected meshes as accepted."""
import hashlib,json,re,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def physical_source(data):
 return sorted(json.dumps(sorted(v[1:] for v in r['vertices']),separators=(',',':')) for r in data['original'])
def main():
 rows=[];binary=None;source=None
 stages=[('curved_joint_protocol',1),('curved_joint_protocol_mpi2',2),('curved_joint_protocol_mpi4',4)]
 if len(sys.argv)>1:
  assert sys.argv[1]=='curved_joint_protocol_cap_mpi2' and len(sys.argv)==2
  stages=[(sys.argv[1],2)]
 for stage,ranks in stages:
  rp=HERE/(stage+'.json');r=json.loads(rp.read_text());folder=HERE/stage
  assert r['status']=='PASS' and r['contract']['MPI_ranks']==ranks
  assert r['contract']['environment']['SU2_NATIVE_CURVED_PROTOCOL']=='1'
  for name,digest in r['artifacts'].items():assert sha(folder/name)==digest,name
  for name,digest in r['contract']['pins'].items():
   if name.startswith(('Common/','SU2_CFD/','UnitTests/')) and name.endswith(('.cpp','.hpp','.inc')):assert sha(ROOT/name)==digest,name
  if binary is None:binary=r['contract']['pins']['test_binary']
  assert binary==r['contract']['pins']['test_binary']
  before=json.loads((folder/'audited_grid.json').read_text());after=json.loads((folder/'protocol_rejected.json').read_text())
  assert before['original']==after['original'],'actual before/after sensor donors changed'
  assert all(before[k]==after[k] for k in ('facets','facet_groups','marker_names')),'reference changed'
  canonical=physical_source(before)
  if source is None:source=canonical
  assert source==canonical,'physical original sensor donors differ across rank counts'
  match=re.search(r'JOINT_PROTOCOL ranks=(\d+) valid=(\d+) cells=(\d+) (?:classified_vertices=\d+ )?placement_trials=(\d+) placement_validated=(\d+) work=(\d+) stop=(\w+)',(folder/'native.log').read_text());assert match
  n,valid,cells,trials,validated,work=map(int,match.groups()[:6]);stop=match.group(7)
  assert n==ranks and valid==0 and stop=='work_budget' and work<=512 and trials>0
  assert cells==len(after['cells'])
  classified=re.search(r'classified_vertices=(\d+)',(folder/'native.log').read_text())
  rows.append(dict(classified_vertices=int(classified.group(1)) if classified else 0,stage=stage,ranks=ranks,cells=cells,placement_trials=trials,placement_validated=validated,work=work,source_donors=len(before['original']),native_receipt_sha256=sha(rp)))
 report=dict(status='PASS_MPI_PROTOCOL_ONLY_REJECTED_MESHES',rows=rows,binary_sha256=binary,helper_sha256=sha(Path(__file__)),
  scope='Native matched MPI'+','.join(str(n) for _,n in stages)+' execution/work-bound/input-signature checks plus independent exact before/after donor/reference preservation; physical original donors agree across the checked stages. Different trajectories are retained. No final mesh acceptance, independent full geometry/metric, CFD/current-BDF transfer/restart, M<N or performance qualification.')
 (HERE/('joint_protocol_cap_mpi2_audit.json' if len(stages)==1 else 'joint_protocol_audit.json')).write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
if __name__=='__main__':main()
