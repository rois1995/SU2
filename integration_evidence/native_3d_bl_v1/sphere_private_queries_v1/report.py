#!/usr/bin/env python3
"""Derive partial-event outcomes and inclusive/nested clocks from the saved native logs."""
import json,math,re
from pathlib import Path
HERE=Path(__file__).resolve().parent;BASE=HERE.with_name('sphere_rans_lifecycle_v1')
def read(p):return json.loads(p.read_text())
def tokens(line):
 row={}
 for key,value in re.findall(r'(\w+)=([^\s]+)',line):
  try:
   number=float(value);assert math.isfinite(number);row[key]=int(number) if number.is_integer() else number
  except ValueError:row[key]=value
 return row
def main():
 cases=[]
 for name in ['n4_m2_query_v1','n4_m2_query_v2','n4_m2_query_v3','n4_m2_query_v4']:
  folder=BASE/name/'update';result=read(folder/'result.json');log=re.sub(r'\x1b\[[0-9;]*m','',(folder/'run.log').read_text())
  statuses=[read(folder/('status_rank%d.json'%r)) for r in range(4)]
  assert result['exit_code']==1 and all(not s['complete'] and s['unchanged_input'] for s in statuses)
  assert log.count('Failed 1 test case, failed 1 assertion.')==4,'Unexpected state/transfer assertion failure'
  reason=next(line for line in log.splitlines() if line.startswith('Native3D rejected event:'))
  stages=[tokens(line) for line in log.splitlines() if line.startswith('NATIVE3D_DETAIL stage=') and ' rank=0 ' in line]
  ranks=[tokens(line) for line in log.splitlines() if line.startswith('NATIVE3D rank=')]
  assert sorted(row['rank'] for row in ranks)==[0,1,2,3]
  main=next(row for row in ranks if row['rank']==0)
  private_edits=sum(row['coarsen_accepted'] for row in ranks)
  cases.append({'folder':str(folder.relative_to(HERE.parents[2])),'exit_code':result['exit_code'],'launcher_seconds':result['seconds'],'status_rank0':statuses[0],'reason':reason,'rank0_native':main,'rank0_details':stages,'per_rank_native':ranks,'total_private_accepted_edits':private_edits})
 before=read(HERE/'endpoint_filter/before.json');after=read(HERE/'endpoint_filter/after.json')
 assert all(before[k]==after[k] for k in ['source_cells','closed','positive_per_pass','repeats'])
 controls={mode:read(HERE/'controls_v3'/mode/'result.json') for mode in ['serial','mpi2','mpi4','rans_mpi4']};assert all(r['exit_code']==0 for r in controls.values())
 assert read(HERE/'strict_independent_adapter/independent_mpi4_m2.json')['status']=='PASS_LIMITED_SCOPE'
 enabled=read(HERE/'controls_v3/rans_mpi4/independent_enabled.json');assert {r['budget'] for r in enabled}=={3,20} and all(r['accepted'] and r['frozen'] for r in enabled)
 out={'status':'PARTIAL_QUERY_AND_ADMISSION_CONTROLS_PASS_ACTUAL_UPDATE_TIME_REJECTED','goal':'ACTIVE_INCOMPLETE','cases':cases,'endpoint_before':before,'endpoint_after':after,'observed_probe_seconds_ratio':after['seconds']/before['seconds'],'exact_orientation_ratio':after['exact_orientations']/before['exact_orientations'],'current_controls':controls,'strict_and_enabled_independent':True,'scope':'No accepted large repeated-BL mesh or CFD advance, viscous convergence or affordability/scaling claim. Native private edits remain uncommitted when the event rejects. Same-field endpoint probe is a local observation; event work differs between rejected trials. Default/opt-in planar RANS controls are independently audited; enabled curved/interacting workflows and global uncertainty allocation remain open.'}
 (HERE/'summary.json').write_text(json.dumps(out,indent=2)+'\n');print(out['status'])
if __name__=='__main__':main()
