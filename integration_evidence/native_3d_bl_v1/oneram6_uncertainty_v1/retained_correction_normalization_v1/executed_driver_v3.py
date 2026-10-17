#!/usr/bin/env python3
"""Resumable full-field normalization experiment using immutable retained query/PDF files."""
import csv,json,math,sys
from pathlib import Path
import numpy as np
from run import HERE,ROOT,OLD,sha,invoke
CAMPAIGN=HERE/'retained_correction_normalization_v1'


def rows(folder):
    result=[]
    for path in folder.glob('correction_rank*.csv'):result.extend(csv.DictReader(path.open()))
    return sorted(result,key=lambda row:int(row['row']))


def planar_integral(scale):
    # Both tangent eigenvalues are sensor-controlled and the normal request remains stronger in this interval.
    assert .75<=scale<=50
    volume=lambda r:2*r+2*np.pi*r*r+4*np.pi*r**3/3
    base=(2*scale)**1.5;full=20*scale-base
    x,w=np.polynomial.legendre.leggauss(64);t=(x+1)/2;fade=1-3*t*t+2*t*t*t;r=.18+.02*t
    correction=volume(.18)*full+.01*float(w@(base*np.expm1(.5*np.log(50/scale)*fade)*(2+4*np.pi*r+4*np.pi*r*r)))
    return 1.125*base+correction


def run(label,proposal,target,tolerance,ranks,expected):
    original=HERE/proposal;prep=json.loads((original/'preparation.json').read_text())
    planar=proposal.startswith('planar_')
    if planar:
        source=original/'original.bin';fixture=original/'wall.dat';low=high=prep['sensor_integral']
        for name,digest in prep['sha256'].items():assert sha(original/name)==digest
    else:
        source=ROOT/prep['domain'];fixture=OLD/'cell329/replay.dat'
        assert sha(source)==prep['domain_sha256'] and sha(original/'queries.bin')==prep['queries_sha256']
        low,high=prep['sensor']['sensor_lower'],prep['sensor']['sensor_upper']
        for name,digest in prep['sha256'].items():assert sha(ROOT/name)==digest
    dest=CAMPAIGN/label/('np%d'%ranks);dest.mkdir(parents=True,exist_ok=True)
    paths=[Path(__file__).resolve(),ROOT/'UnitTests/SU2_CFD/adaptation/NativeCorrectionReplay3D.hpp',
        ROOT/'Common/include/adaptation/CNativeDonor3D.hpp',ROOT/'Common/src/adaptation/CNativeDonor3D.cpp',original/'preparation.json']
    identity=dict(original_domain_sha256=sha(source),queries_sha256=sha(original/'queries.bin'),
        sha256={str(p.relative_to(ROOT)):sha(p) for p in paths})
    extra=dest/'extra_provenance.json'
    if extra.exists():
        previous=json.loads(extra.read_text());key=str(Path(__file__).resolve().relative_to(ROOT))
        saved=previous['sha256'].pop(key);current=dict(identity,sha256=dict(identity['sha256']));current['sha256'].pop(key)
        assert previous==current and saved in (sha(Path(__file__)),sha(CAMPAIGN/'rejected_driver_v1.py'))
    else:extra.write_text(json.dumps(identity,indent=2)+'\n')
    invoke(dest,{'SU2_NATIVE_BL_CORRECTION_DOMAIN':str(source),'SU2_NATIVE_BL_CORRECTION_QUERIES':str(original/'queries.bin'),
        'SU2_NATIVE_BL_NORMALIZE':str(target),'SU2_NATIVE_BL_SENSOR_INTERVAL':'%.17g %.17g'%(low,high),
        'SU2_NATIVE_BL_NORMALIZE_RTOL':str(tolerance)},fixture,ranks)
    result=json.loads((dest/'correction_normalization.json').read_text());trace=list(csv.DictReader((dest/'correction_normalization_trace.csv').open()))
    assert result['status']==expected and result['trials']<=24
    assert result['distinct_fields']==len(trace)
    base=json.loads((dest/'correction_global.json').read_text())
    assert result['actual_compositions']==base['inside']*result['distinct_fields']
    assert result['samples']==prep['samples'] and not result['production_normalization_changed']
    assert result['donor_searches_during_trials']==result['geometry_queries_during_trials']==0
    if expected=='estimated_tolerance':assert abs(result['value']-target)+result['uncertainty']<=tolerance*target
    if expected=='integration_limited':assert result['same_field_reuses']>0
    if expected=='outside_bracket':assert result['log_scale']==-math.log(64) and result['value']-result['uncertainty']>target
    before=original/('np%d'%ranks)
    if not (before/'report.json').exists():before=original/'np4'
    previous=json.loads((before/'report.json').read_text())
    for name,digest in previous['rows_sha256'].items():assert sha(before/name)==digest
    current=rows(dest);old=rows(before);assert len(current)==len(old)==prep['samples']
    maximum=0.;absoluteCorrection=0.;weightedDrift=0.
    inversePDF=np.fromfile(original/'queries.bin',dtype='<f8',offset=16).reshape(-1,4)[:,3]
    for i,(a,b) in enumerate(zip(current,old)):
        assert a['row']==b['row'] and a['donor']==b['donor']
        densityScale=max(1.,abs(float(b['sensor'])),abs(float(b['composed'])))
        for key in ('sensor','composed','correction','weighted_correction'):
            scale=densityScale*(inversePDF[i] if key=='weighted_correction' else 1)
            maximum=max(maximum,abs(float(a[key])-float(b[key]))/scale)
        absoluteCorrection=max(absoluteCorrection,abs(float(a['correction'])-float(b['correction'])))
        weightedDrift+=abs(float(a['weighted_correction'])-float(b['weighted_correction']))/len(current)
    assert maximum<1e-11
    assert weightedDrift<64*np.finfo(float).eps*max(1.,abs(base['correction']))
    checks=dict(status='PASS_RETAINED_NORMALIZATION_REPLAY',native=result,base_row_maximum_density_scaled_difference=maximum,
        base_correction_maximum_absolute_difference=absoluteCorrection,mean_absolute_weighted_correction_drift=weightedDrift,
        verification_helper_sha256=sha(Path(__file__)),
        heuristic_uncertainty=True,production_clipping_regradation_and_stages_tested=False,
        native_sha256=sha(dest/'correction_normalization.json'),trace_sha256=sha(dest/'correction_normalization_trace.csv'),
        extra_provenance_sha256=sha(extra),base_rows_sha256={p.name:sha(p) for p in dest.glob('correction_rank*.csv')})
    if planar:
        references=[planar_integral(math.exp(float(row['log_scale']))) for row in trace]
        discrepancies=[abs(value-float(row['value'])) for row,value in zip(trace,references)]
        checks.update(independent_planar_integrals=references,independent_planar_maximum_discrepancy=max(discrepancies),
            independent_reference_covered_all_trials=all(e<=float(row['uncertainty']) for e,row in zip(discrepancies,trace)))
        assert checks['independent_reference_covered_all_trials']
        if expected=='estimated_tolerance':assert abs(references[-1]-target)<=tolerance*target
    other=CAMPAIGN/label/('np%d'%(1 if planar else 2))/'verification.json'
    if other.exists() and other.parent!=dest:
        previous=json.loads(other.read_text())['native']
        assert previous['status']==result['status'] and previous['distinct_fields']==result['distinct_fields']
        for key in ('log_scale','value','uncertainty'):assert abs(result[key]-previous[key])<=1e-12*max(1.,abs(previous[key]))
        checks['MPI_normalization_agrees']=True
    (dest/'verification.json').write_text(json.dumps(checks,indent=2)+'\n');print(label,'np%d'%ranks,json.dumps(checks),flush=True)


if __name__=='__main__':
    run(sys.argv[1],sys.argv[2],float(sys.argv[3]),float(sys.argv[4]),int(sys.argv[5]),sys.argv[6])
