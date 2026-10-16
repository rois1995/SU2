#!/usr/bin/env python3
"""Stream the exact complete field once; checkpoint each cell, with MPI1/2/4 and original-field subdivision."""
import csv,json,os,subprocess,sys,time
from pathlib import Path
import numpy as np
from run import HERE,ROOT,BUILD,BINARY,OLD,sha,pins
DTYPE=np.dtype([('id','<u8'),('coordinates','<f8',(4,3)),('sensor','<f8',(4,3,3))])
def main():
 label=sys.argv[1];end=int(sys.argv[2]);ranks=int(sys.argv[3]) if len(sys.argv)>3 else 1
 depth=int(sys.argv[4]) if len(sys.argv)>4 else 0
 assert '/' not in label and ranks in (1,2,4) and 0<=depth<=6
 manifest=json.loads((HERE/'field_inputs/manifest.json').read_text());assert 0<end<=manifest['count']
 folder=HERE/label;folder.mkdir(exist_ok=True)
 selection=set(json.loads(Path(sys.argv[5]).read_text())) if len(sys.argv)>5 else None
 identity={'producer':pins(),'runner_sha256':sha(Path(__file__).resolve()),'manifest_sha256':sha(HERE/'field_inputs/manifest.json'),
           'fixture_sha256':sha(OLD/'cell329/replay.dat'),'budget':8192,'subdivision_depth':depth,'ranks':ranks,
           'absolute_tolerance':os.environ.get('SU2_NATIVE_BL_ABSOLUTE','-1')}
 proof=folder/'provenance.json'
 if proof.exists():assert json.loads(proof.read_text())==identity,'Different source/input/binary; preserve this evidence and select another folder.'
 else:proof.write_text(json.dumps(identity,indent=2)+'\n')
 completed={}
 for receipt in sorted(folder.glob('attempt*/result.json')):
  r=json.loads(receipt.read_text());assert r['provenance_sha256']==sha(proof)
  assert r['exit_code'] in (0,124),'Prior numerical failure requires investigation: '+str(receipt)
  for name,digest in r['csv_sha256'].items():
   path=receipt.parent/name;assert sha(path)==digest
   for row in csv.DictReader(path.open()):
    i=int(row['cell']);assert i not in completed;completed[i]=row
 requested=[]
 for item in manifest['blocks']:
  if item['begin']>=end:break
  p=HERE/item['path'];assert sha(p)==item['sha256']
  block=np.fromfile(p,dtype=DTYPE,offset=8)
  keep=[int(row['id'])<end and int(row['id']) not in completed and (selection is None or int(row['id']) in selection) for row in block]
  if any(keep):requested.append(block[keep])
 if not requested:print('REUSE_RANGE_COMPLETE',label,len(completed),flush=True);return
 rows=np.concatenate(requested);del requested
 case=folder/('attempt%06d'%int(rows['id'][0]));case.mkdir(exist_ok=True)
 assert not (case/'run.log').exists(),'Inspect incomplete prior attempt before resuming: '+str(case)
 packet=case/'input.bin'
 with packet.open('wb') as stream:
  stream.write(np.array([len(rows)],dtype='<u8').tobytes());rows.tofile(stream)
 input_sha=sha(packet)
 env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(BUILD/'compile-tmp'),
          SU2_NATIVE_BL_REPLAY=str(OLD/'cell329/replay.dat'),SU2_NATIVE_BL_COMPLEXITY='8192',SU2_NATIVE_BL_FIELD=str(packet),SU2_NATIVE_BL_SUBDIVISIONS=str(depth))
 base=[str(BINARY),'[NativeFacetedVolumeReplay3D]','--abort','--reporter','compact']
 if ranks>1:base=['mpirun','-np',str(ranks),'--bind-to','none']+base
 command=['taskset','-c','0-'+str(ranks-1) if ranks>1 else '0','nice','-n','19','timeout','--kill-after=5s',os.environ.get('SU2_NATIVE_STREAM_SECONDS','7200')+'s']+base
 start=time.monotonic()
 with (case/'run.log').open('w') as log:run=subprocess.run(command,cwd=case,env=env,stdout=log,stderr=subprocess.STDOUT)
 csvs={};parts=[]
 for rank in range(ranks):
  output=case/('field_estimates_rank%d.csv'%rank)
  if output.exists():
   csvs[output.name]=sha(output);parts+=list(csv.DictReader(output.open()))
 ids=[int(p['cell']) for p in parts];assert len(set(ids))==len(ids)
 assert set(ids)<=set(map(int,rows['id']))
 for p in parts:
  assert np.isfinite(float(p['value'])) and float(p['value'])>0 and float(p['indicator'])>=0
  assert int(p['evaluations'])<=8192*(2**depth)
  assert int(p['cell']) not in completed;completed[int(p['cell'])]=p
 receipt={'provenance_sha256':sha(proof),'input_sha256':input_sha,'csv_sha256':csvs,'log_sha256':sha(case/'run.log'),
          'exit_code':run.returncode,'elapsed_seconds':time.monotonic()-start,'requested':len(rows),'completed':len(parts),'command':command}
 (case/'result.json').write_text(json.dumps(receipt,indent=2)+'\n');packet.unlink()
 errors=[float(p['indicator']) for p in completed.values()]
 report={'complete_whole_field':len(completed)==manifest['count'],'complete_requested':run.returncode==0 and len(parts)==len(rows),
  'cells':len(completed),'total_cells':manifest['count'],'value':float(np.sum([float(p['value']) for p in completed.values()],dtype=np.longdouble)),
  'indicator':float(np.sum(errors,dtype=np.longdouble)) if np.all(np.isfinite(errors)) else None,
  'evaluations':sum(int(p['evaluations']) for p in completed.values()),'limited_subcells':sum(int(p['limited']) for p in completed.values()),
  'unresolved_subcells':sum(int(p['unresolved_support']) for p in completed.values()),'provenance_sha256':sha(proof),'indicator_is_proven_bound':False}
 (folder/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report),flush=True)
 if run.returncode or len(parts)!=len(rows):raise RuntimeError('Partial field saved: '+str(case))
if __name__=='__main__':main()
