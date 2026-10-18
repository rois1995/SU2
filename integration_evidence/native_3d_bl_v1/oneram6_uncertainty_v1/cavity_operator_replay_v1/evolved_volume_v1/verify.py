#!/usr/bin/env python3
"""Verify scoped controls, exact rejected state and the geometry certificate; never rerun CFD."""
import gzip,hashlib,json,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[4]

def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def binary(p):
 if p.exists():return digest(p)
 h=hashlib.sha256()
 with gzip.open(p.with_suffix('.gz'),'rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def pinned(d,executed=None):
 for name,expected in d.items():
  p=ROOT/name
  assert digest(p)==expected or (executed and p.name=='run.py' and digest(executed)==expected),name

def main():
 for mode in ['serial','mpi4','enabled_mpi4','disabled_runtime']:
  folder=HERE/'qualification_v1'/mode;r=json.loads((folder/'result.json').read_text());assert r['exit_code']==0
  assert r['provenance_sha256']==digest(folder/'provenance.json') and r['log_sha256']==digest(folder/'run.log')
  p=json.loads((folder/'provenance.json').read_text());pinned(p['pins'])
  capsule=json.loads(gzip.decompress((folder/'executed_source.json.gz').read_bytes()))
  assert all((ROOT/name).read_text()==text for name,text in capsule.items())
 for folder,exe,legacy in [(HERE,'replay',True),(HERE/'reconnect_probe_v1','replay',True),(HERE/'full_state_v1','context',False),(HERE/'full_state_v1/geometry_lattice_v1','replay',False)]:
  r=json.loads((folder/'receipt.json').read_text());assert r['compile']['exit']==r['link']['exit']==0
  assert binary(folder/exe)==r['binary_sha256']
  pinned(r['hashes'],folder/'run_executed_v1.py' if (folder/'run_executed_v1.py').exists() else None)
  if not legacy:
   assert r['run']['exit']==0
   for p,expected in r['linked_archives'].items():assert digest(Path(p))==expected,p
  else:
   # Executed local binaries/source/inputs are sealed; the engine archive was subsequently rebuilt.
   assert folder==HERE or r['run']['exit']==0
 cases=json.loads((HERE/'receipt.json').read_text())['cases'];assert len(cases)==10 and sum(c['any_local_progress'] for c in cases)==9
 for c in cases:
  assert c['exit']==0 and c['work']['failures']==c['work']['invalid_queries']==0
  for name,expected in c['input_hashes'].items():assert digest(HERE/c['stem']/name)==expected
 state=HERE/'full_state_v1';summary=json.loads((state/'state_summary.json').read_text())
 assert summary['cells']==541027 and summary['points']==139884 and summary['active_original_edge_cells']==summary['active_original_blocker_cells']==0
 assert digest(state/'m6_rejected.su2.gz')==summary['grid_sha256'] and digest(state/'inspect_state.py')==summary['helper_sha256']
 for row in summary['owned_pieces']:
  p=state/f"rejected_working_rank{row['rank']}.bin.gz";assert digest(p)==row['compressed_sha256'];h=hashlib.sha256()
  with gzip.open(p,'rb') as f:
   header=f.read(16);assert header[:8]==b'NATOWN1\0';assert int.from_bytes(header[8:],'little')==row['cells'];h.update(header)
   count=0
   for b in iter(lambda:f.read(2**20),b''):h.update(b);count+=len(b)
  assert h.hexdigest()==row['raw_sha256'] and count==180*row['cells']
 assert all(not x['accepted'] and x['atomic_input'] and x['CFD_steps']==0 for x in summary['rank_results'])
 profiles=[json.loads(p.read_text()) for p in state.glob('profile_rank*.json')];assert len(profiles)==4 and all(x==profiles[0] for x in profiles)
 assert profiles[0]['parameter']==.875 and profiles[0]['core']==.003215747252591252 and profiles[0]['runtime_version']==2
 proof=state/'geometry_lattice_v1/certificate.json';certificate=json.loads(proof.read_text())
 pinned({str((proof.parent/name).relative_to(ROOT)):expected for name,expected in certificate['hashes'].items()})
 assert certificate['exact_edge_lattice']['maximum_interior_binary64_points']==1 and certificate['left_no_interior_point']['maximum_interior_binary64_points']==0
 assert certificate['native_frozen_field_measurement']['left']>6*certificate['native_frozen_field_measurement']['edge_limit']
 before=digest(state/'run.log');guard=subprocess.run([sys.executable,str(state/'run.py')],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=10)
 assert guard.returncode!=0 and 'Saved event exists' in guard.stdout and digest(state/'run.log')==before
 print('PASS: current source/build/binary scoped controls; sealed local replays; four exact terminal owner states; MPI frozen profile; immutable input; lattice certificate; long-event overwrite guard. No accepted M6/full-goal/speedup claim.')
if __name__=='__main__':main()
