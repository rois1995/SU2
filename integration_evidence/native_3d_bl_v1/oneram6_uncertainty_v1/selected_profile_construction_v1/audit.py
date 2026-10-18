#!/usr/bin/env python3
"""Audit actual selection/construction outcome and preserve an exact frozen replay field."""
import gzip,hashlib,json,re,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];folder=HERE/'m6_budget300000_np4'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
def load(p):return json.loads(p.read_text())
r=load(folder/'result.json');proof=load(folder/'provenance.json');assert r['exit_code']==0
assert r['log_sha256']==sha(folder/'run.log') and r['provenance_sha256']==sha(folder/'provenance.json')
for name,digest in proof['pins'].items():assert sha(ROOT/name)==digest,name
for name,source in json.loads(gzip.decompress((folder/'executed_source.json.gz').read_bytes())).items():assert source==(ROOT/name).read_text(),name
prior=folder.parents[1]/'adaptive_budget_evidence_v1/m6_growth2_budget300000_np4'
assert proof['prior_selection_receipt_sha256']==sha(prior/'result.json')
old=load(prior/'saved_metric_budget.json');initial=load(folder/'saved_metric_budget.json');construction=load(folder/'selected_construction.json')
assert initial['CFD_steps']==0 and initial['flow_and_turbulence_unchanged'] and initial['reference_unchanged'] and initial['frozen']
for k in ('parameter','version','trials','evidence_stages'):assert initial[k]==old[k],k
for k in ('estimate','uncertainty'):assert abs(initial[k]-old[k])<=1e-10*max(1,abs(old[k])),k
assert construction['CFD_steps']==0 and construction['construction_attempted'] and construction['profile_frozen']
assert construction['atomic_original_state_check'] and construction['workers']==4
assert construction['parameter']==initial['parameter'] and construction['version']==initial['version']
assert construction['transfer_performed']==construction['accepted_mesh']
log=(folder/'run.log').read_text();assert len(re.findall(r'Passed .*test case',log))==4
normal=[dict(re.findall(r'(\w+)=([^\s;]+)',line)) for line in log.splitlines() if line.startswith('Native3D complexity normalization:')]
assert len(normal)==1;normal=normal[0]
# The binary files retain every literal selected tensor component, without refitting or scaling.
assert sys.byteorder=='little'
dtype=np.dtype([('id','<u8'),('coordinates','<f8',(3,)),('sensor','<f8',(3,3))]);parts=[]
for rank in range(4):
 p=folder/('selected_sensor_rank%d.bin'%rank);assert r['owned_sensor_files'][p.name]==sha(p)
 with p.open('rb') as f:count=int.from_bytes(f.read(8),'little')
 assert 0<count<=10000000 and p.stat().st_size==8+count*dtype.itemsize
 part=np.fromfile(p,dtype=dtype,offset=8);assert len(part)==count;parts.append(part)
data=np.concatenate(parts);data.sort(order='id');assert np.array_equal(data['id'],np.arange(len(data),dtype=np.uint64))
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'));from capcheck import read_su2
mesh=read_su2(folder/'Coarse.su2');assert len(data)==len(mesh.P) and np.array_equal(data['coordinates'],mesh.P)
assert np.all(np.isfinite(data['sensor'])) and np.all(np.isfinite(data['coordinates']))
assert np.max(np.abs(data['sensor']-data['sensor'].transpose(0,2,1)))<=1e-12*np.max(np.abs(data['sensor']))
np.linalg.cholesky(data['sensor'])
p=folder/'selected_sensor.bin';payload=len(data).to_bytes(8,'little')+data.tobytes()
if p.exists():assert p.read_bytes()==payload
else:p.write_bytes(payload)
manifest={'format':'uint64 count; count records of uint64 id,3 binary64 xyz,9 binary64 row-major sensor',
 'byte_order':'little','count':len(data),'selected_sensor_sha256':sha(p),'source_mesh_sha256':sha(folder/'Coarse.su2'),
 'config_sha256':sha(folder/'run.cfg'),'provenance_sha256':sha(folder/'provenance.json'),
 'profile_parameter':initial['parameter'],'profile_version':initial['version'],'core':float(normal['frozen_core']),
 'log_scale':float(normal['log_scale']),'sensor_only_original_P1':True,'adapted_sample_refit':False,
 'clipped_sensor_rescaling_permitted':False,'raw_family_replay_requires_original_unclipped_frames':True,
 'scope':'Exact frozen construction/diagnostic replay only; this snapshot cannot replace the original family during correction.'}
(folder/'selected_field_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
engine=[dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in log.splitlines() if line.startswith('NATIVE3D_DETAIL stage=refine ')]
rejections=[line for line in log.splitlines() if line.startswith('Native3D rejected event:')]
parents=[dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in log.splitlines() if line.startswith('NATIVE3D rank=')]
assert len(engine)==len(parents)==4
global_counts={k:sum(int(e[k]) for e in engine) for k in ('accepted','layer_published','remaining_edges','cached_invalid_cells')}
summary={'qualified_fixture_execution':True,'full_goal_complete':False,'initialization':initial,'construction':construction,
 'selected_field':manifest,'engine_rank_diagnostics':engine,'parent_rank_diagnostics':parents,
 'global_engine_counts':global_counts,'raw_construction_counter_scope':'private_edits is rank0 only; global counts are reduced from all rank diagnostics',
 'rejections':rejections,'receipt':r,
 'audit_helper_sha256':sha(Path(__file__)),
 'limits':['Initialization/budget feasibility separate from construction','No converged physical-accuracy or complete-event speedup claim']}
if not construction['accepted_mesh']:
 assert not construction['transfer_performed'] and rejections
 summary['next_action']='Reconsider complete construction/dependency coordination; do not add another local fallback or ceiling ladder.'
else:summary['limits'].append('Independent accepted geometry/full-composed/transfer audit remains required before qualification')
(HERE/'summary.json').write_text(json.dumps(summary,indent=2,allow_nan=False)+'\n')
print(json.dumps({k:v for k,v in summary.items() if k not in ('engine_rank_diagnostics','receipt')},indent=2))
