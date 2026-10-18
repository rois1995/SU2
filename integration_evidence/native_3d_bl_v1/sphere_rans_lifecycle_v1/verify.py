#!/usr/bin/env python3
"""Verify sealed evidence and executed-source provenance; no CFD or geometry rerun."""
import gzip,hashlib,json,os
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];CACHE={}
def read(p):return json.loads(p.read_text())
def sha(p):
 p=p.resolve()
 if p not in CACHE:
  h=hashlib.sha256()
  with p.open('rb') as f:
   for b in iter(lambda:f.read(2**20),b''):h.update(b)
  CACHE[p]=h.hexdigest()
 return CACHE[p]
def run(folder,current,exit_code):
 receipt=read(folder/'result.json');proof=read(folder/'provenance.json')
 assert receipt['exit_code']==exit_code,folder
 assert sha(folder/'provenance.json')==receipt['provenance_sha256'] and sha(folder/'run.log')==receipt['log_sha256']
 capsule=json.loads(gzip.decompress((folder/'executed_source.json.gz').read_bytes()))
 historical_binary={}
 for name,wanted in proof['pins'].items():
  if name in capsule:assert hashlib.sha256(capsule[name].encode()).hexdigest()==wanted,name
  elif not current and name.endswith('/UnitTests/test_driver'):historical_binary[name]=wanted
  else:assert sha(ROOT/name)==wanted,name
  if current and name in capsule:assert sha(ROOT/name)==wanted,name
 for name,wanted in proof.get('inputs',{}).items():assert sha(ROOT/name)==wanted,name
 for name,wanted in receipt.get('artifacts',{}).items():assert sha(folder/name)==wanted,name
 return {'result_sha256':sha(folder/'result.json'),'executed_source_sha256':sha(folder/'executed_source.json.gz'),'historical_binary_record_only':historical_binary}
def main():
 seal=read(HERE/'seal.json')
 for name,wanted in seal['files'].items():assert sha(ROOT/name)==wanted,name
 for name,target in seal['links'].items():
  p=ROOT/name;assert p.is_symlink() and os.readlink(str(p))==target,name
  assert os.path.commonpath([str(ROOT),str(p.resolve())])==str(ROOT) and p.exists(),name
 results={}
 for name,current,code in [('n4_m2_v2/construct',False,0),('n4_m2_v2/release',False,1),('n4_m2_query_v1/release',True,0),('n4_m2_query_v1/update',True,1)]:results[name]=run(HERE/name,current,code)
 for mode in ['serial','mpi2','mpi4','rans_mpi4']:results[mode]=run(HERE/'query_controls_v2'/mode,True,0)
 cold=read(HERE/'n4_m2_v2/construct/independent.json');release=read(HERE/'n4_m2_query_v1/release/independent.json')
 for report in [cold,release]:
  assert report['status']=='PASS_ACTUAL_RANS_STATE_TRANSFER_RESTART_AND_OUTPUT'
  assert report['maximum_actual_integral_defect']<1e-10 and report['transported_sensor_error']==0
 assert cold['auditor_sha256']==sha(HERE/'audit_producer_executed.py')
 assert release['auditor_sha256']==sha(HERE/'audit.py')
 assert cold['cells']==5440 and release['cells']==320 and release['geometry_verification']['pairs']==50560
 for rank in range(4):
  row=read(HERE/('n4_m2_query_v1/update/status_rank%d.json'%rank));assert not row['complete'] and row['unchanged_input'] and row['work']==1338
 failed=(HERE/'n4_m2_query_v1/update/run.log').read_text();assert 'Original donor union exceeds patch budget.' in failed
 assert read(HERE/'strict_independent_adapter/independent_mpi4_m2.json')['status']=='PASS_LIMITED_SCOPE'
 enabled=read(HERE/'query_controls_v2/rans_mpi4/independent_enabled.json')
 assert len(enabled)==2 and {row['budget'] for row in enabled}=={3,20}
 assert all(row['accepted'] and row['frozen'] and row['SU2_and_CGNS_reload'] and row['SA_restart_fields'] and row['projection_defect']<1e-10 and row['transported_sensor_residual']<2e-14 for row in enabled)
 summary={'status':'PARTIAL_ACCEPTED_COLD_AND_RESTARTED_REMOVAL_UPDATE_ATOMICALLY_REJECTED','goal':'ACTIVE_INCOMPLETE','results':results,'cold_actual_integral_defect':cold['maximum_actual_integral_defect'],'release_actual_integral_defect':release['maximum_actual_integral_defect'],'release_native_seconds':release['clocks'][0]['native_seconds'],'release_fixture_seconds':release['clocks'][0]['total_seconds'],'scope':'Actual initialized RANS/SA current+BDF, prescribed original identity P1, cold historical producer and current N4/M2 CGNS restart/removal; current strict/enabled planar controls. Native spherical BL oracle; independent released full field and exact geometry. No CFD advance, normalization/budget selection, unchanged-BL update success, viscous/scaling/affordability claim.'}
 (HERE/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(summary['status'],flush=True)
if __name__=='__main__':main()
