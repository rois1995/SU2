#!/usr/bin/env python3
"""Reuse the existing exact primitive lifecycle oracle without its historical CLI driver."""
import hashlib,json,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
reference=ROOT/'integration_evidence/native_3d_curved_v1/primitive_audit_v1/audit_primitives_final.py'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 namespace={'__file__':str(reference),'__name__':'retained_primitive_oracle'}
 definitions=reference.read_text().split('\ncase=HERE.parent/sys.argv[1];')[0]
 exec(compile(definitions,str(reference),'exec'),namespace)
 namespace['HERE']=HERE/'retained_primitive_oracle'
 start=time.monotonic();cases=[]
 for mode,ranks in [('serial',1),('mpi4',4)]:
  folder=HERE/'qualification'/mode;result=json.loads((folder/'result.json').read_text());assert result['exit_code']==0
  assert sha(folder/'provenance.json')==result['provenance_sha256'] and sha(folder/'run.log')==result['log_sha256']
  for workers in range(1,ranks+1):
   for sphere,tilted in [(False,False),(False,True),(True,False)]:
    name=('sphere' if sphere else 'cylinder')+'_axis'+str(int(tilted))+'_m'+str(workers)
    events=[namespace['audit_event'](folder,name+'_s'+str(cycle),sphere,tilted,'SU2') for cycle in range(2)]
    restart=namespace['read'](folder/(name+'_restart_states.json'));wanted={tuple(r['point']):r['states'] for r in restart['donor']}
    assert all(max(abs(a-b)/max(1.,abs(a)) for a,b in zip(r['states'],wanted[tuple(r['point'])]))<2e-10 for r in restart['accepted'])
    final=namespace['read'](folder/(name+'_s1_accepted.json'))
    for fmt in ['SU2','CGNS']:
     file=folder/(name+'_s1_written_'+fmt+('.cgns' if fmt=='CGNS' else '.su2'))
     reader=namespace['cgns_signature'] if fmt=='CGNS' else namespace['su2_signature']
     assert reader(file,set(final['marker_names']))==namespace['signature'](final,set(final['marker_names']))
     namespace['inputs'][str(file.relative_to(HERE))]=sha(file)
    cases.append({'mode':mode,'CFD_ranks':ranks,'workers':workers,'name':name,'events':events})
    (HERE/'primitive_independent_v2_partial.json').write_text(json.dumps({'complete':False,'cases':cases,'elapsed_seconds':time.monotonic()-start},indent=2)+'\n')
    print('PASS',mode,name,flush=True)
 report={'status':'PASS_15_ACTUAL_PRIMITIVE_LIFECYCLES_30_EVENTS','cases':cases,'elapsed_seconds':time.monotonic()-start,'input_hashes':namespace['inputs'],'reference_oracle_sha256':sha(reference),'helper_sha256':sha(Path(__file__)),'scope':'Existing exact primitive Euler lifecycle checks, source metric/geometry/current+BDF/open-domain accounting/transport/restart/SU2+CGNS; serial and N4 M1/2/3/4. This is not the5440-cell BL sphere CFD lifecycle or MPI generation qualification.'}
 assert len(cases)==15;(HERE/'primitive_independent_v2.json').write_text(json.dumps(report,indent=2)+'\n')
 print(report['status'],flush=True)
if __name__=='__main__':main()
