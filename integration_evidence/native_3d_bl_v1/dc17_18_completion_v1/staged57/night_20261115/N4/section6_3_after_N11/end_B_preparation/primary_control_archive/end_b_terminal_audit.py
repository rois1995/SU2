from pathlib import Path
import os,json,time,hashlib,subprocess,sys
P=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11');E=P/'end_B_preparation';V=P/'B8/validation';root=Path.cwd();files=[P/'build558/primary_terminal_receipt.json']+[V/(name+'_primary_terminal_receipt.json') for name in ['G1_serial','G1_mpi2','G1_mpi4','G2','G2_R1','GQ']]+[E/'g3_primary_terminal_receipt.json',E/'g4_primary_terminal_receipt.json',E/'R3_orphan_generator/process_tree_terminal_receipt.json',E/'R3_G2_controller_recovery/adoption_launch_identity.json',E/'R3_GQ_controller_recovery/adoption_launch_identity.json',E/'R3_GQ_controller_recovery/supervisor_launch.json']
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def collect(x,rows):
 if isinstance(x,dict):
  if isinstance(x.get('pid'),int) and isinstance(x.get('start_ticks'),int):rows[(x['pid'],x['start_ticks'])]=dict(pid=x['pid'],start_ticks=x['start_ticks'])
  for value in x.values():collect(value,rows)
 elif isinstance(x,list):
  for value in x:collect(value,rows)
rows={}
for f in files:collect(json.loads(f.read_text()),rows)
for row in rows.values():
 try:
  f=(Path('/proc')/str(row['pid'])/'stat').read_text().rsplit(')',1)[1].split();live=int(f[19])==row['start_ticks'] and f[0] not in {'Z','X'}
 except FileNotFoundError:live=False
 row['alive']=live;assert not live,row
active=[]
for p in Path('/proc').iterdir():
 if not p.name.isdigit() or int(p.name)==os.getpid():continue
 try:
  f=(p/'stat').read_text().rsplit(')',1)[1].split()
  if f[0] in {'Z','X'}:continue
  args=[x.decode(errors='replace') for x in (p/'cmdline').read_bytes().split(b'\0') if x];cwd=(p/'cwd').resolve();comm=(p/'comm').read_text().strip();script=args[1] if len(args)>1 and args[1].endswith('.py') else ''
  owned_script=bool(script and ('/night_20261115/' in script or script.startswith('/tmp/end_b_') or script.startswith('/tmp/b6_') or script.startswith('/tmp/b7_') or script.startswith('/tmp/b8_')))
  owned_binary=comm in {'SU2_CFD','test_driver','ninja','cc1plus'} and (cwd==root or root in cwd.parents)
  if owned_script or owned_binary:active.append(dict(pid=int(p.name),start_ticks=int(f[19]),comm=comm,script=script))
 except (OSError,ValueError,IndexError):pass
assert not active,active
pins=json.loads((P/'build558/source_pins.json').read_text());assert len(pins)==1781 and all(sha(f)==h for f,h in pins.items());b=json.loads((P/'build558/build_status.json').read_text());assert all(sha(b[k])==b[k+'_sha256'] for k in ['binary','test_binary']);assert subprocess.run(['git','diff','--quiet','--','Common/src/adaptation/CNativeEngine3D.cpp','Common/src/adaptation/native_engine3d']).returncode==0
out=Path(sys.argv[1]);assert not out.exists();r=dict(actual_host_proc=True,unix=time.time(),all_known_owned_handles_terminal=True,identities=list(rows.values()),active_owned_jobs=active,source_pins_verified=len(pins),current_binary_hashes_verified=True,owned_CPP_matches_committed_source=True,receipt_sha256={str(f):sha(f) for f in files},source_commit=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip());out.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(dict(terminal_identities=len(rows),active_owned_jobs=0,source_pins=1781,source_commit=r['source_commit'])))
