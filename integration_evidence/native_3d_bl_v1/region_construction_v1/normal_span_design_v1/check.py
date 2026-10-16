"""Exact-form radial BL span lower bound for planning intermediate fronts."""
import hashlib,json,math
from pathlib import Path
import mpmath as mp
HERE=Path(__file__).resolve().parent
mp.mp.dps=70
rows=[]
for value in ('0.05','0.01','0.001'):
    h,growth,thickness=mp.mpf(value),mp.mpf('1.2'),mp.mpf('.2')
    full=max(h,mp.mpf('.9')*thickness);limit=mp.mpf('1.8');requested=mp.mpf('.9')*limit
    def height(distance):return max(h,2*(h+(growth-1)*distance)/(growth+1))
    def primitive(distance):
        if distance<=h/2:return distance/h
        return mp.mpf('.5')+(growth+1)/(2*(growth-1))*mp.log((h+(growth-1)*distance)/(h+(growth-1)*h/2))
    depth=h*(mp.mpf('.5')*(growth-1)+mp.mpf('.5')*(growth+1)*mp.expm1((requested-mp.mpf('.5'))*2*(growth-1)/(growth+1)))/(growth-1)
    # Independent high-precision integration of the piecewise normal-size definition.
    numerical=mp.quad(lambda d:1/height(d),[0,h/2,full]);assert abs(numerical-primitive(full))<mp.mpf('1e-65')
    assert abs(primitive(depth)-requested)<mp.mpf('1e-65')
    remaining=primitive(full)-primitive(depth);segments=int(mp.ceil(remaining/limit))
    assert segments==(1 if value=='0.05' else 4 if value=='0.01' else 11)
    rows.append({'first_height':value,'growth':str(growth),'thickness':str(thickness),'unfaded_end':str(full),
                 'ideal_first_front_depth':str(depth),'normal_span_to_unfaded_end':str(primitive(full)),
                 'span_after_ideal_first_front':str(remaining),'minimum_remaining_normal_segments':segments})
result={'status':'PASS_NORMAL_SPAN_LOWER_BOUND_ONLY','precision_digits':mp.mp.dps,'rows':rows,
        'helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'construction_source_sha256':hashlib.sha256((HERE.parents[3]/'Common/src/adaptation/CNativeRegion3D.cpp').read_bytes()).hexdigest(),
        'scope':'Ideal radial column on the cylindrical wall with the recorded normal growth law and current first-band metric span1.62. Tensor intersection preserves this BL normal demand. Ignoring the outer fade gives a lower bound: a single direct transition edge cannot span6.749/18.216 metric units under1.8. At least4/11 remaining segments are necessary, not sufficient for complete q/J/geometry/height/work-feasible tetrahedra. This is not an actual high-ratio mesh run, a global feasibility proof or a production mode.'}
(HERE/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'])
