"""Align the protected-band reference with the actual represented query, before tests."""
from pathlib import Path
p=Path('UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp');s=p.read_text()
old='''    if(d<=.03)CHECK(sample.hn==original[k].hn);'''
new='''    if(sample.distance<=.03)CHECK(sample.hn==original[k].hn);
    else CHECK(sample.hn==Approx(SU2_TYPE::GetValue(original[k].hn)+.5*(2./3-2*.2/2.2)*(SU2_TYPE::GetValue(sample.distance)-.03)).epsilon(1e-13));'''
assert s.count(old)==1;s=s.replace(old,new,1);p.write_text(s)
