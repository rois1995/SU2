import json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from native_support_provider import prepare,integrate,faults,CAMPAIGN
from run import HERE,ROOT,OLD,FROZEN,invoke
from support_correction import SHARED,domain
domain()
control='[NativeSupport3D],[NativeComposite3D],[NativeComposite2D],[NativeComplexityNormalization]'
invoke(CAMPAIGN/'controls_np1',{},OLD/'cell329/replay.dat',1,test_filter=control)
for ranks in (1,4):
 invoke(CAMPAIGN/('donor_controls_np%d'%ranks),{},OLD/'cell329/replay.dat',ranks,test_filter='[NativeDonorReuse3D],[NativeDonorPointProbe3D],[NativeRegionField3D]')
planar=HERE/'planar_support_correction_v2';fixture=planar/'wall.dat';source=planar/'original.bin'
volume=json.loads((planar/'preparation.json').read_text())['sensor_integral'];bounds=(volume,volume)
prepare('planar_seed449',fixture,8192,449)
for ranks in (1,4):integrate('planar_seed449',source,fixture,bounds,16,ranks)
audit=json.loads((FROZEN/'sensor_mass_audit.json').read_text());bounds=(audit['sensor_lower'],audit['sensor_upper'])
fixture=OLD/'cell329/replay.dat';source=SHARED/'original.bin'
for seed in (2641,9919):
 label='m6_seed%d'%seed;prepare(label,fixture,8192,seed)
 for ranks in ((2,4) if seed==2641 else (4,)):integrate(label,source,fixture,bounds,1e6,ranks)
planar=HERE/'planar_support_correction_v2';volume=json.loads((planar/'preparation.json').read_text())['sensor_integral']
faults(planar/'original.bin',planar/'wall.dat',(volume,volume),CAMPAIGN/'planar_seed449/prepare/native_queries.bin')
