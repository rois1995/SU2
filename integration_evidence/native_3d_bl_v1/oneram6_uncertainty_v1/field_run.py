#!/usr/bin/env python3
"""Resumable exact-field scalar quadrature; retain completed rows even on an interrupted block."""
import csv,hashlib,json,os,subprocess,sys,time
from pathlib import Path
import numpy as np
from run import HERE,ROOT,BUILD,BINARY,OLD,sha,pins
DTYPE=np.dtype([('id','<u8'),('coordinates','<f8',(4,3)),('sensor','<f8',(4,3,3))])
def main():
 manifest=json.loads((HERE/'field_inputs/manifest.json').read_text())
 begin=int(sys.argv[1]) if len(sys.argv)>1 else 0
 end=int(sys.argv[2]) if len(sys.argv)>2 else manifest['count']
 assert 0<=begin<end<=manifest['count']
 folder=HERE/os.environ.get('SU2_NATIVE_FIELD_OUTPUT','field_v3');folder.mkdir(exist_ok=True)
 identity={'producer':pins(),'runner_sha256':sha(Path(__file__).resolve()),'manifest_sha256':sha(HERE/'field_inputs/manifest.json'),
           'fixture_sha256':sha(OLD/'cell329/replay.dat'),'budget':8192}
 proof=folder/'provenance.json'
 if proof.exists():assert json.loads(proof.read_text())==identity,'Field identity changed; use another evidence folder.'
 else:proof.write_text(json.dumps(identity,indent=2)+'\n')
 completed={}
 for receipt in sorted(folder.glob('batch*/result.json')):
  r=json.loads(receipt.read_text());assert r['provenance_sha256']==sha(proof)
  path=receipt.parent/'field_estimates_rank0.csv';assert sha(path)==r['csv_sha256']
  with path.open() as stream:
   for row in csv.DictReader(stream):
    i=int(row['cell']);assert i not in completed;completed[i]=row
 start=time.monotonic();new=0
 values_sum=np.sum([float(p['value']) for p in completed.values()],dtype=np.longdouble)
 indicators_sum=np.sum([float(p['indicator']) for p in completed.values()],dtype=np.longdouble)
 evaluations=sum(int(p['evaluations']) for p in completed.values())
 limited=sum(int(p['limited']) for p in completed.values())
 unresolved=sum(int(p['unresolved_support']) for p in completed.values())
 for item in manifest['blocks']:
  if item['end']<=begin or item['begin']>=end:continue
  source=HERE/item['path'];assert sha(source)==item['sha256']
  rows=np.fromfile(source,dtype=DTYPE,offset=8)
  for at in range(0,len(rows),32):
   records=rows[at:at+32];records=records[[begin<=int(r['id'])<end and int(r['id']) not in completed for r in records]]
   if not len(records):continue
   case=folder/('batch%06d'%int(records['id'][0]));case.mkdir(exist_ok=True)
   assert not (case/'run.log').exists(),'An unrecorded prior attempt needs inspection: '+str(case)
   packet=case/'input.bin';packet.write_bytes(np.array([len(records)],dtype='<u8').tobytes()+records.tobytes())
   env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(BUILD/'compile-tmp'),
            SU2_NATIVE_BL_REPLAY=str(OLD/'cell329/replay.dat'),SU2_NATIVE_BL_COMPLEXITY='8192',SU2_NATIVE_BL_FIELD=str(packet))
   command=['taskset','-c','0','nice','-n','19','timeout','--kill-after=5s','180s',str(BINARY),
            '[NativeFacetedVolumeReplay3D]','--abort','--reporter','compact']
   tick=time.monotonic()
   with (case/'run.log').open('w') as log:run=subprocess.run(command,cwd=case,env=env,stdout=log,stderr=subprocess.STDOUT)
   output=case/'field_estimates_rank0.csv';parts=[]
   if output.exists():
    with output.open() as stream:parts=list(csv.DictReader(stream))
   expected=records['id'][:len(parts)]
   assert [int(p['cell']) for p in parts]==list(expected)
   for p in parts:
    assert np.isfinite(float(p['value'])) and float(p['value'])>0 and 0<=float(p['indicator'])
    assert int(p['evaluations'])<=8192
    completed[int(p['cell'])]=p
    values_sum+=np.longdouble(p['value']);indicators_sum+=np.longdouble(p['indicator'])
    evaluations+=int(p['evaluations']);limited+=int(p['limited']);unresolved+=int(p['unresolved_support'])
   receipt={'provenance_sha256':sha(proof),'input_sha256':sha(packet),'csv_sha256':sha(output) if output.exists() else None,
            'log_sha256':sha(case/'run.log'),'exit_code':run.returncode,'elapsed_seconds':time.monotonic()-tick,
            'requested':len(records),'completed':len(parts),'command':command}
   (case/'result.json').write_text(json.dumps(receipt,indent=2)+'\n');packet.unlink()
   new+=len(parts)
   report={'complete':len(completed)==manifest['count'],'cells':len(completed),'total_cells':manifest['count'],
           'value':float(values_sum),
           'indicator':float(indicators_sum) if np.isfinite(indicators_sum) else None,
           'evaluations':evaluations,
           'limited_cells':limited,
           'unresolved_support':unresolved,
           'provenance_sha256':sha(proof),'indicator_is_proven_bound':False}
   (folder/'progress.json').write_text(json.dumps(report,indent=2)+'\n')
   print('FIELD',int(records['id'][0]),len(parts),receipt['elapsed_seconds'],json.dumps(report),flush=True)
   if run.returncode or len(parts)!=len(records):raise RuntimeError('Incomplete block retained: '+str(case))
   if time.monotonic()-start>7200:print('Time ceiling reached; completed rows retained',flush=True);return
 print('FIELD_RANGE_DONE',begin,end,'new',new,flush=True)
if __name__=='__main__':main()
