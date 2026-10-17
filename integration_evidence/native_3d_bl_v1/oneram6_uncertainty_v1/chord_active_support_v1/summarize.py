"""Verify terminal active-support evidence without rerunning CFD or native fixtures."""
from pathlib import Path
import hashlib,json,re
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];cache={}
def load(p):
    sha(p);return json.loads(p.read_text())
def sha(p):
    p=p.resolve()
    if p not in cache:
        h=hashlib.sha256()
        with p.open('rb') as f:
            for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
        cache[p]=h.hexdigest()
    return cache[p]
edges=('first_layer_edge_band_reference_v1','normal_classification_first_edge_v1','normal_classification_surface_edge_v1','interior_first_edge_final_v1','interior_surface_edge_final_v1')
for mode in ('serial','mpi','runtime','cell88','cell157','cell329')+edges:
    result=load(HERE/mode/'result.json');proof=load(HERE/mode/'provenance.json')
    assert result['exit_code']==0
    assert result['provenance_sha256']==sha(HERE/mode/'provenance.json')
    assert result['log_sha256']==sha(HERE/mode/'run.log')
    for path,digest in proof['pins'].items():assert sha(ROOT/path)==digest,path
serial=(HERE/'serial/run.log').read_text();assert 'Passed all 23 test cases with 502489 assertions' in serial
edge_rows=[]
for mode in edges:
    line=next(s for s in (HERE/mode/'run.log').read_text().splitlines() if s.startswith('FROZEN_EDGE'))
    r=dict(re.findall(r'(\w+)=("[^"]*"|\S+)',line));assert r['failure']=='""'
    if mode in ('interior_first_edge_final_v1','interior_surface_edge_final_v1'):
        reference=load(HERE.parent/mode/'result.json')['edge'];assert r['length']==reference['length']
    edge_rows.append(dict(case=mode,**r))
cells=[]
for cell in (88,157,329):
    item=load(HERE/('cell%d'%cell)/'result.json')['estimate']
    refs=HERE.parents[1]/'oneram6_density_reference_v1'/('cell%d'%cell)
    orders={int(p.parent.name.split('_')[0][5:]) for p in refs.glob('order*_apex*/result.json')}
    order=max(o for o in orders if all((refs/('order%d_apex%d'%(o,a))/'result.json').exists() for a in (0,3)))
    values=[load(refs/('order%d_apex%d'%(order,a))/'result.json')['integral'] for a in (0,3)]
    reference=sum(values)/2;error=abs(item['value']-reference)
    cells.append(dict(cell=cell,estimate=item,reference_order=order,reference=reference,
        reference_chart_spread=abs(values[0]-values[1]),absolute_error=error,relative_error=error/reference,
        indicator_covers_observed_error=item['indicator']>=error,reference_is_proven_bound=False))
runtime=load(HERE/'runtime/independent_runtime.json')
assert len(runtime)==2 and all(r['transport_error']==0 and r['first_height_error']==0 and r['restart_flow_defect']==0 and r['SU2_and_CGNS_reload'] for r in runtime)
for r in runtime:assert r['geometry_and_full_field']['strict_metric_gate']
(HERE/'qualified_envelope.hpp').write_bytes((ROOT/'Common/include/adaptation/CNativeChordEnvelope.hpp').read_bytes())
summary=dict(status='SCOPED_ACTIVE_SUPPORT_QUALIFIED_NOT_COMPLETE_M6_OR_FULL_GOAL3',
    current_source_build_binary_input_helper_pins_verified=True,
    serial_cases=23,serial_assertions=502489,MPI2_volume_and_envelope='PASS',
    actual_N4_M2_RANS_SA_current_BDF_conservation_SU2_CGNS_restart='PASS',
    actual_runtime=runtime,edge_replays=edge_rows,difficult_cell_references=cells,
    component_replay=load(HERE/'result.json')['summary'],
    seed_shape_diagnosis=load(HERE/'seed_shape_audit.json'),
    indicator_is_proven_bound=False,full_M6_rerun=False,accepted_M6=False,whole_event_speedup_established=False,
    progressive_measurement='DESIGN_ONLY_NOT_IMPLEMENTED',goal_complete=False,
    limits=['Bounded geometry sample, not a seed-wide or complete adaptation speedup.',
            'Cell157 error exceeds its heuristic indicator; default accuracy/global allocation remains uncalibrated.',
            'Active-support geometry is numerical, not an outward-rounded coverage proof.',
            'No new complete M6 attempt or CFD advancement. Final construction and affordability remain open.'])
sha(HERE/'qualified_envelope.hpp')
summary['artifact_sha256']={str(p.relative_to(ROOT)):sha(p) for p in sorted(cache) if ROOT in p.parents}
summary['helper_sha256']=sha(Path(__file__))
(HERE/'summary.json').write_text(json.dumps(summary,indent=2,allow_nan=False)+'\n')
print(json.dumps(dict(status=summary['status'],cell_reference_errors=[{k:r[k] for k in ('cell','relative_error','indicator_covers_observed_error')} for r in cells],goal_complete=False),indent=2))
