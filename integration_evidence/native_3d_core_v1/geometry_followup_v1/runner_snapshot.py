from pathlib import Path
import hashlib,json,os,subprocess,time,sys
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');base=root/'integration_evidence/native_3d_core_v1'
folder=base/'geometry_followup_v1';folder.mkdir(exist_ok=False)
(folder/'runner_snapshot.py').write_bytes(Path(__file__).read_bytes());print('Follow-up evidence folder:',folder,flush=True)
build=base/'geometry_build_v1';target=base/'geometry_bridge_v6';pid=2673220
expected=Path('/proc/'+str(pid)+'/cmdline').read_bytes();assert b'/tmp/native_3d_geometry_build.py' in expected
paths=['Common/src/adaptation/CNativeImport3D.cpp','Common/include/adaptation/CNativeImport3D.hpp','Common/src/adaptation/CNativeBoundary3D.cpp','Common/include/adaptation/CNativeBoundary3D.hpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/TransferTestCase.hpp','integration_evidence/native_3d_core_v1/run_geometry_bridge.py','integration_evidence/native_3d_core_v1/audit_geometry_bridge.py']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
pins={p:sha(root/p) for p in paths};(folder/'source_sha256.json').write_text(json.dumps(pins,indent=2)+'\n')
(folder/'active.json').write_text(json.dumps(dict(pid=os.getpid(),waiting_for=pid,target=str(target)),indent=2)+'\n')
status='FAIL';reason='';code=None
try:
 while not (build/'validation.json').exists():
  proc=Path('/proc/'+str(pid)+'/cmdline')
  assert proc.exists() and proc.read_bytes()==expected,'Owned build ended without a terminal receipt; inspect before restarting.'
  time.sleep(20)
 receipt=json.loads((build/'validation.json').read_text());assert receipt['status']=='PASS', 'Actual SU2 rebuild failed.'
 deadline=time.monotonic()+3600
 while os.getloadavg()[0]>7.5:
  if time.monotonic()>deadline:raise RuntimeError('Host remained busy; bridge campaign deferred, not restarted.')
  time.sleep(20)
 memory={line.split(':',1)[0]:int(line.split()[1]) for line in Path('/proc/meminfo').read_text().splitlines() if line.startswith('MemAvailable:')}
 assert memory['MemAvailable']>=4*1024*1024,'Insufficient available memory for the small bridge campaign.'
 assert all(sha(root/p)==h for p,h in pins.items()),'Pending bridge sources changed while waiting; review before launching.'
 assert not target.exists(),'Target exists; inspect its handle/receipt rather than restart.'
 command=['nice','-n','19',sys.executable,str(base/'run_geometry_bridge.py'),str(target)]
 (folder/'launch.json').write_text(json.dumps(dict(command=command,load=os.getloadavg(),available_kib=memory['MemAvailable'],started=time.time()),indent=2)+'\n')
 with (folder/'bridge.stdout').open('w') as out,(folder/'bridge.stderr').open('w') as err:
  p=subprocess.Popen(command,cwd=root,stdout=out,stderr=err)
  (folder/'active.json').write_text(json.dumps(dict(pid=os.getpid(),child_pid=p.pid,target=str(target)),indent=2)+'\n');code=p.wait()
 status='PASS' if code==0 else 'FAIL';reason='Bridge campaign completed; inspect its validation and independent audit receipts.'
except Exception as error:reason=str(error)
finally:
 (folder/'validation.json').write_text(json.dumps(dict(status=status,reason=reason,exit_code=code,source_sha256=pins,scope='One bounded local geometry bridge campaign queued after the actual owned rebuild and host admission. No automatic retries, cluster launches, configured driver or performance claim.'),indent=2)+'\n')
 print(status,reason,folder,flush=True)
