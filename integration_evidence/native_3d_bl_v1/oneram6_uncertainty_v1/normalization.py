#!/usr/bin/env python3
"""Bounded distributed normalization experiment on saved fields; no CFD or mesh acceptance claim."""
import json
from pathlib import Path
import numpy as np
from run import HERE,OLD,invoke,sha
DTYPE=np.dtype([('id','<u8'),('coordinates','<f8',(4,3)),('sensor','<f8',(4,3,3))])
def main():
 folder=HERE/'normalization_policy_v1';folder.mkdir(exist_ok=True)
 fixture=OLD/'cell329/replay.dat'
 invoke(folder/'contracts',{},fixture,test_filter='[NativeComplexityNormalization]')
 manifest=json.loads((HERE/'field_inputs/manifest.json').read_text());selected=[]
 for block in manifest['blocks']:
  if block['begin']>329:break
  p=HERE/block['path'];assert sha(p)==block['sha256']
  rows=np.fromfile(p,dtype=DTYPE,offset=8);keep=np.isin(rows['id'],[88,157,329])
  if np.any(keep):selected.append(rows[keep])
 rows=np.concatenate(selected);assert list(rows['id'])==[88,157,329]
 packet=folder/'original_cells.bin'
 payload=np.array([len(rows)],dtype='<u8').tobytes()+rows.tobytes()
 if packet.exists():assert packet.read_bytes()==payload
 else:packet.write_bytes(payload)
 results=[]
 for ranks in [1,4]:
  case=folder/('np%d'%ranks)
  env={'SU2_NATIVE_BL_FIELD':str(packet),'SU2_NATIVE_BL_COMPLEXITY':'8192','SU2_NATIVE_BL_NORMALIZE':'10','SU2_NATIVE_BL_NORMALIZE_RTOL':'.01','INPUT_SHA256':sha(packet)}
  r=invoke(case,env,fixture,ranks=ranks);n=r['normalization'];assert n['cells']==3 and n['ranks']==ranks
  assert not n['proven_error_bound'] and n['CFD_steps']==0 and not n['mesh_constructed']
  results.append(n)
  print('NORMALIZATION',ranks,json.dumps(n),flush=True)
 for key in ['value','uncertainty','log_scale']:
  assert abs(results[0][key]-results[1][key])<=1e-10*max(1,abs(results[0][key])),(key,results)
 assert results[0]['status']==results[1]['status']
 malformed=folder/'malformed_original_cells.bin';bad=rows.copy();bad['coordinates'][0]=0
 malformed.write_bytes(np.array([len(bad)],dtype='<u8').tobytes()+bad.tobytes())
 case=folder/'malformed_np4'
 invoke(case,{'SU2_NATIVE_BL_FIELD':str(malformed),'SU2_NATIVE_BL_NORMALIZE':'10','INPUT_SHA256':sha(malformed)},fixture,ranks=4,expected_failure=True)
 assert 'NORMALIZATION_INPUT cell=88' in (case/'run.log').read_text()
 report={'scope':'Three original frozen cells, uniform sensor/core scaling; no production bounds/regradation or mesh construction',
   'ranks':[1,4],'results':results,'input_sha256':sha(packet),'MPI_agreement_relative_tolerance':1e-10,
   'malformed_input_collectively_rejected':True,'guard_is_proven':False,'whole_frozen_M6_complete':False}
 (folder/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
