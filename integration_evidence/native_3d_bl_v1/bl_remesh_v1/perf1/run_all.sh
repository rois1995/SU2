#!/bin/bash
# Full perf1 campaign on the final binaries (sequential, one job at a time).
E=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/perf1
T=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/build-integrated-v2/compile-tmp
A=$T/test_driver_after_perf1;B=$T/test_driver_before_perf1
cd $E
rm -rf d2_after after_dc8
./run_d2.sh $A after
python3 compare_d2.py > d2_comparison.json
./run_validation.sh $A after unit3d unit2d mesh3d metric airfoil
./run_validation.sh $B before mesh3d metric airfoil
cp run_dc8_case.sh /dev/null
SU2_DUMMY=1 ./run_dc8_case.sh m6_le after $E/after_dc8
echo ALL-DONE >> $E/campaign.log
