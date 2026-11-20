from pathlib import Path
import shutil
from run_g1 import ITEM,cut,verify_build,run_case
from build_fp1 import build
root=ITEM/'resume3'
repo=cut.REPO
meson=repo/'UnitTests/meson.build'
source=repo/'UnitTests/Common/adaptation/CNativeLocalCore3DGBaseline_tests.cpp'
temp=Path('/tmp/fp1_g609_owned_baseline.cpp')
before=meson.read_bytes()
anchor=b"                       'Common/adaptation/CNativeLocalCore3DResumeBox_tests.cpp',\n"
assert before.count(anchor)==1 and not source.exists()
assert not (root/'g_baseline_build610').exists()
shutil.copyfile(temp,source)
meson.write_bytes(before.replace(anchor,anchor+b"                       'Common/adaptation/CNativeLocalCore3DGBaseline_tests.cpp',\n",1))
try:
    assert build(root/'g_baseline_build610',610,False)
    b=root/'g_baseline_build610'
    status=cut.read(b/'build_status.json')
    baseline=cut.read(ITEM/'resume2/build609/build_status.json')
    binary,_=verify_build(status,b/'source_pins.json')
    assert status['binary_sha256']==baseline['binary_sha256']
    case=root/'g_baseline'
    wait=run_case(case,binary,1,'[.][NativeLocalCoreG609Baseline]',5400)
    assert wait['returncode']==0
    payload=case/'f4_1_build609_owned.bin'
    cut.save(case/'baseline.json',{'passed':True,'part_g_commit':'9940a85314','path':str(payload),
             'sha256':cut.sha(payload),'bytes':payload.stat().st_size,'source_sha256':cut.sha(source),
             'test_binary_sha256':status['test_binary_sha256']})
    shutil.copyfile(temp,root/'capture_g_baseline.cpp')
finally:
    meson.write_bytes(before)
    source.unlink()
