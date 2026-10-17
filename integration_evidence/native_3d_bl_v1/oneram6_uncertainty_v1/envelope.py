#!/usr/bin/env python3
"""One connected geometry milestone: strict controls and saved difficult original-field replays."""
import json
from pathlib import Path
from run import HERE,OLD,invoke,sha

def main():
 folder=HERE/'chord_envelope_v1';folder.mkdir(exist_ok=True)
 fixture=OLD/'cell329/replay.dat';identity={'HELPER_SHA256':sha(Path(__file__))}
 controls='[NativeChordEnvelope],[NativeComplexityNormalization],[NativeComplexityEstimate3D],[NativeComposite3D],[NativeVolumeBL3D],[NativeParallelBL3D],[NativeCurvedBL3D],[NativePlanarTangentEdge3D]'
 invoke(folder/'controls',identity,fixture,test_filter=controls)
 report={'scope':'Current strict3D controls and three saved original M6 cells; not whole-field normalization or accepted mesh', 'cells':[]}
 for cell in (88,157,329):
  case=folder/('cell%d'%cell)
  invoke(case,dict(identity,SU2_NATIVE_BL_CORRECTION='1'),OLD/('cell%d'%cell)/'replay.dat')
  result=json.loads((case/'correction.json').read_text())
  refdir=HERE.parent/'oneram6_density_reference_v1'/('cell%d'%cell);order=72 if cell==157 else 128
  ref=sum(json.loads((refdir/('order%d_apex%d'%(order,a))/'result.json').read_text())['integral'] for a in (0,3))/2
  for offset in (0,2):
   a,b=result['runs'][offset:offset+2]
   for key in ('value','indicator','evaluations'):assert a[key]==b[key],(cell,key,a,b)
  for row in result['runs']:
   row['absolute_reference_error']=abs(row['value']-ref);row['relative_reference_error']=abs(row['value']/ref-1)
  result.update(cell=cell,reference=ref,output_sha256=sha(case/'correction.json'))
  report['cells'].append(result);print('ENVELOPE',cell,json.dumps(result),flush=True)
 (folder/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
