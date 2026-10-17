#!/usr/bin/env python3
"""Native complete-support preparation, independent PDF check, then original-field MPI replay."""
import csv,json,math,os,struct,sys,time
from pathlib import Path
import numpy as np
import run as replay
from run import HERE,ROOT,OLD,BINARY,sha,invoke
from correction_support import Proposal
CAMPAIGN=HERE/'native_support_provider_v1'


def proof(folder,extra=()):
    folder.mkdir(parents=True,exist_ok=True)
    names=['Common/include/adaptation/CNativeSupport3D.hpp','Common/include/adaptation/CNativeDonor3D.hpp',
        'Common/src/adaptation/CNativeDonor3D.cpp','SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp',
        'UnitTests/SU2_CFD/adaptation/NativeCorrectionReplay3D.hpp','Common/include/adaptation/CNativeDistributed2D.hpp']
    paths=[ROOT/n for n in names]+[BINARY]+list(extra)
    value={str(p.relative_to(ROOT)):sha(p) for p in paths};path=folder/'extra_provenance.json'
    if path.exists():assert json.loads(path.read_text())==value
    else:path.write_text(json.dumps(value,indent=2)+'\n')


def prepare(label,fixture,count=8192,seed=2641):
    folder=CAMPAIGN/label/'prepare';proof(folder,[fixture])
    invoke(folder,{'SU2_NATIVE_BL_PREPARE_CORRECTION':str(count),'SU2_NATIVE_BL_PROPOSAL_SEED':str(seed)},fixture)
    metadata=json.loads((folder/'native_proposal.json').read_text())
    with (folder/'native_queries.bin').open('rb') as f:magic,n=struct.unpack('<QQ',f.read(16))
    assert magic==0x4e41545155455259 and n==count==metadata['samples']
    assert (folder/'native_queries.bin').stat().st_size==16+32*n
    data=np.fromfile(folder/'native_queries.bin',dtype='<f8',offset=16).reshape(n,4)
    text=np.atleast_1d(np.genfromtxt(folder/'native_queries.csv',delimiter=',',names=True))
    assert np.array_equal(data[:,:3],np.column_stack([text[k] for k in ('x','y','z')]))
    assert np.array_equal(data[:,3],text['inverse_pdf']) and np.all(np.isfinite(data)) and np.all(data[:,3]>0)
    start=time.monotonic();independent=Proposal(fixture);pdf=independent.pdf(data[:,:3]);difference=float(np.max(abs(pdf*data[:,3]-1)))
    comparison=dict(maximum_relative_PDF_difference=difference,compared_points=count,
        native_query_sha256=sha(folder/'native_queries.bin'),independent_helper_sha256=sha(HERE/'correction_support.py'))
    (folder/'pdf_comparison.json').write_text(json.dumps(comparison,indent=2)+'\n')
    if difference>=1e-8:
        error=abs(pdf*data[:,3]-1);ids=np.argsort(error)[-20:]
        np.savetxt(folder/'pdf_discrepancies.csv',np.column_stack([ids,data[ids],pdf[ids],error[ids]]),delimiter=',',header='row,x,y,z,inverse_native_pdf,python_pdf,relative_error')
        raise AssertionError('Independent KD-tree proposal PDF disagrees with native ADT proposal.')
    result=dict(status='PASS_NATIVE_SUPPORT_AND_INDEPENDENT_PDF',native=metadata,independent_maximum_relative_PDF_difference=difference,
        independent_validation_seconds=time.monotonic()-start,independent_validation_is_production_cost=False,
        query_sha256=sha(folder/'native_queries.bin'),text_sha256=sha(folder/'native_queries.csv'),fixture=str(fixture.relative_to(ROOT)),fixture_sha256=sha(fixture),
        independent_helper_sha256=sha(HERE/'correction_support.py'),verification_helper_sha256=sha(Path(__file__)),extra_provenance_sha256=sha(folder/'extra_provenance.json'))
    if 'planar' in label:
        r=.2;volume=2*r+2*np.pi*r*r+4*np.pi*r**3/3
        assert np.max(abs(data[:,3]/volume-1))<1e-11
        result['independent_square_uniform_tube']=True
    (folder/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(label,json.dumps(result),flush=True)


def integrate(label,domain,fixture,sensor_interval,target,ranks):
    base=CAMPAIGN/label/'prepare';prepared=json.loads((base/'verification.json').read_text())
    assert sha(base/'native_queries.bin')==prepared['query_sha256'] and sha(fixture)==prepared['fixture_sha256']
    folder=CAMPAIGN/label/('np%d'%ranks);proof(folder,[domain,fixture,base/'native_queries.bin'])
    parameters={'SU2_NATIVE_BL_CORRECTION_DOMAIN':str(domain),'SU2_NATIVE_BL_CORRECTION_QUERIES':str(base/'native_queries.bin'),
        'SU2_NATIVE_BL_NORMALIZE':str(target),'SU2_NATIVE_BL_SENSOR_INTERVAL':'%.17g %.17g'%sensor_interval,'SU2_NATIVE_BL_NORMALIZE_RTOL':'.05'}
    invoke(folder,parameters,fixture,ranks)
    value=json.loads((folder/'correction_global.json').read_text());normal=json.loads((folder/'correction_normalization.json').read_text())
    assert value['samples']==prepared['native']['samples'] and value['source_ownership_shift']==1
    assert value['cached_direct_tensor_checks']==value['inside'] and value['different_cached_direct_tensors']==0
    assert normal['status'] in ('estimated_tolerance','integration_limited') and normal['trials']<=24
    if normal['status']=='estimated_tolerance':assert abs(normal['value']-target)+normal['uncertainty']<=.05*target
    rows=[]
    for rank in range(ranks):rows.extend(csv.DictReader((folder/('correction_rank%d.csv'%rank)).open()))
    rows.sort(key=lambda row:int(row['row']));assert [int(row['row']) for row in rows]==list(range(value['samples']))
    weights=np.array([float(row['weighted_correction']) for row in rows]);assert abs(weights.mean()-value['correction'])<1e-11*max(1.,abs(value['correction']))
    assert abs(weights.std(ddof=1)/math.sqrt(len(weights))-value['empirical_standard_error'])<1e-11*max(1.,value['empirical_standard_error'])
    result=dict(status='PASS_NATIVE_SUPPORT_ORIGINAL_FIELD_REPLAY',native=value,normalization=normal,
        full_goal_complete=False,production_normalization_enabled=False,shifted_source_ownership=True,
        composed_estimate=value['correction']+sum(sensor_interval)/2,heuristic_total_uncertainty=3*value['empirical_standard_error']+(sensor_interval[1]-sensor_interval[0])/2,
        query_sha256=prepared['query_sha256'],correction_sha256=sha(folder/'correction_global.json'),normalization_sha256=sha(folder/'correction_normalization.json'),
        rows_sha256={p.name:sha(p) for p in folder.glob('correction_rank*.csv')},verification_helper_sha256=sha(Path(__file__)))
    other=CAMPAIGN/label/('np%d'%(1 if 'planar' in label else 2))/'verification.json'
    if other.exists() and other.parent!=folder:
        previous=json.loads(other.read_text());old=[]
        for name,digest in previous['rows_sha256'].items():
            assert sha(other.parent/name)==digest;old.extend(csv.DictReader((other.parent/name).open()))
        old.sort(key=lambda row:int(row['row']));assert old==rows
        assert previous['normalization']['status']==normal['status']
        for key in ('log_scale','value','uncertainty'):assert abs(previous['normalization'][key]-normal[key])<1e-12*max(1.,abs(normal[key]))
        result['all_MPI_rows_and_normalization_agree']=True
    if 'planar' in label:
        from normalize_support_correction import planar_integral
        references=[]
        for row in csv.DictReader((folder/'correction_normalization_trace.csv').open()):
            ref=planar_integral(math.exp(float(row['log_scale'])));assert abs(ref-float(row['value']))<=float(row['uncertainty']);references.append(ref)
        result['independent_planar_references']=references
    (folder/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(label,'np%d'%ranks,json.dumps(result),flush=True)


def faults(domain,fixture,sensor_interval,query):
    common={'SU2_NATIVE_BL_CORRECTION_DOMAIN':str(domain),'SU2_NATIVE_BL_CORRECTION_QUERIES':str(query),
        'SU2_NATIVE_BL_NORMALIZE':'16','SU2_NATIVE_BL_SENSOR_INTERVAL':'%.17g %.17g'%sensor_interval,'SU2_NATIVE_BL_NORMALIZE_RTOL':'.05'}
    folder=CAMPAIGN/'blocked_global_output';proof(folder,[domain,fixture,query]);(folder/'correction_global.json').mkdir(exist_ok=True)
    invoke(folder,common,fixture,4,expected_failure=True)
    assert 'Cannot save global correction result' in (folder/'run.log').read_text()
    assert not (folder/'correction_normalization.json').exists()
    folder=CAMPAIGN/'mismatched_controls';folder.mkdir(parents=True,exist_ok=True);wrapper=folder/'different_target.sh'
    body='#!/bin/sh\nif [ "${OMPI_COMM_WORLD_RANK:-0}" = 0 ]; then export SU2_NATIVE_BL_NORMALIZE=17; fi\nexec "${SU2_NATIVE_TEST_DRIVER:?}" "$@"\n'
    if wrapper.exists():assert wrapper.read_text()==body
    else:wrapper.write_text(body);wrapper.chmod(0o700)
    proof(folder,[domain,fixture,query,wrapper]);saved=replay.BINARY
    try:
        replay.BINARY=wrapper;invoke(folder,dict(common,SU2_NATIVE_TEST_DRIVER=str(BINARY)),fixture,4,expected_failure=True)
    finally:replay.BINARY=saved
    assert 'Inconsistent correction normalization controls across ranks' in (folder/'run.log').read_text()
    assert not (folder/'correction_normalization.json').exists()
    result=dict(status='PASS_COLLECTIVE_OUTPUT_AND_CONTROL_FAILURES',output_result_sha256=sha(CAMPAIGN/'blocked_global_output/result.json'),
        controls_result_sha256=sha(folder/'result.json'),real_binary_sha256=sha(BINARY),no_normalization_after_failure=True)
    (CAMPAIGN/'faults.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)


if __name__=='__main__':raise SystemExit('Import prepare/integrate/faults from the saved campaign driver; no CFD.')
