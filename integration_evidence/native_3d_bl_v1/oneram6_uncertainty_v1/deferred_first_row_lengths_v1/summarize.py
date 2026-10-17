"""Verify saved terminal evidence and compare exact traces; never launch a native job."""
import hashlib,json
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
BASE=HERE.parent/'original_parent_support_v1'
CONSTRUCTION=HERE.parent/'construction_deferred_lengths_v1'
checked={}
def sha(path):
    path=path.resolve()
    if path not in checked:
        h=hashlib.sha256()
        with path.open('rb') as stream:
            for block in iter(lambda:stream.read(1024*1024),b''):h.update(block)
        checked[path]=h.hexdigest()
    return checked[path]
def load(path):return json.loads(path.read_text())
def receipt(folder,exit_code):
    result=load(folder/'result.json');proof=load(folder/'provenance.json')
    assert result['exit_code']==exit_code
    assert result['provenance_sha256']==sha(folder/'provenance.json')
    assert result['log_sha256']==sha(folder/'run.log')
    for name,digest in proof.get('pins',proof).items():assert sha(ROOT/name)==digest,name
    for name,source in load_capsule(folder).items():
        assert hashlib.sha256(source.encode()).hexdigest()==sha(ROOT/name),name
    return result

def load_capsule(folder):
    import gzip
    return json.loads(gzip.decompress((folder/'executed_source.json.gz').read_bytes()))

cases={mode:receipt(HERE/mode,1 if mode=='coupled' else 0)
       for mode in ('serial','mpi','frozen','runtime','coupled')}
receipt(CONSTRUCTION,0)
rows=[]
assert (HERE/'frozen/first_row_rejected.dat').read_bytes()==(BASE/'canonical_frozen/first_row_rejected.dat').read_bytes()
for rank in range(4):
    new=np.loadtxt(HERE/'frozen'/('query_trace_rank%d.csv'%rank),delimiter=',')
    old=np.loadtxt(BASE/'canonical_frozen'/('query_trace_rank%d.csv'%rank),delimiter=',')
    assert new.shape[1]==old.shape[1]==15
    old_bits={row.tobytes() for row in np.ascontiguousarray(old).view('V120').reshape(-1)}
    assert all(row.tobytes() in old_bits for row in new)
    assert load(HERE/'frozen'/('first_row_rank%d.json'%rank))['edge_evaluations']==0
    rows.append(dict(rank=rank,new_unique_queries=len(new),old_unique_queries=len(old),exact_old_query_subset=True))

baseline=load(BASE/'summary.json')['baseline_finer']
finer=[]
for item in baseline['geometric_files']:
    path=HERE/'coupled'/item['file']
    assert sha(path)==item['sha256'],str(path)
    finer.append(dict(file=item['file'],sha256=sha(path),byte_identical_to_qualified_baseline=True))
engine=[]
for line in (HERE/'coupled/run.log').read_text().splitlines():
    if line.startswith('ENGINE rank=0 '):
        r=dict(s.split('=',1) for s in line.split()[1:] if '=' in s)
        engine.append({k:r[k] for k in ('case','valid','stop','global_work','rounds','accepted','final')})
assert engine[-1]['case']=='finer_grouped_coupled_BL' and engine[-1]['valid']=='0'
assert engine[-1]['global_work']=='4096' and engine[-1]['rounds']=='162'
assert all(r['valid']=='1' for r in engine[:-1])

phase=[]
for line in (CONSTRUCTION/'run.log').read_text().splitlines():
    if line.startswith('NATIVE3D_DETAIL stage=refine '):
        row=dict(s.split('=',1) for s in line.split()[1:] if '=' in s)
        phase.append({k:row[k] for k in ('rank','first_layer','seed','layer_published','layer_edge_evaluations',
                    'cached_measured_cells','cached_invalid_cells','final_owned_cells','accepted','stop','nested_edge_geometry')})
assert len(phase)==4 and all(r['stop']=='time_budget' and r['accepted']=='0' for r in phase)
assert sum(int(r['layer_published']) for r in phase)==38800
assert sum(int(r['cached_measured_cells']) for r in phase)==32832
assert sum(int(r['final_owned_cells']) for r in phase)==536159
construction=load(CONSTRUCTION/'construction_result.json');assert not construction['accepted']
runtime=load(HERE/'runtime/independent_runtime.json')
assert len(runtime)==2 and all(r['SU2_and_CGNS_reload'] and r['SA_restart_fields'] and r['restart_flow_defect']==0 for r in runtime)
summary=dict(status='SCOPED_RANKING_QUALIFIED_FULL_GOAL3_INCOMPLETE',
    source_build_binary_and_helper_pins_match=True,serial_cases=71,serial_assertions=14440,
    affected_MPI_curved_reader_subset='PASS',geometry_byte_identical=True,query_rows=rows,
    exact_unique_query_count_before=sum(r['old_unique_queries'] for r in rows),
    exact_unique_query_count_after=sum(r['new_unique_queries'] for r in rows),
    independent_N4_M2_RANS_BDF_SA_SU2_CGNS_restart='PASS',coupled_rank0=engine,
    coupled_campaign='FAIL at preexisting stronger finer-sensor case; not a passing whole campaign',
    finer_baseline_attribution=finer,complete_M6=construction,complete_M6_phase=phase,
    full_M6_seed_complete=False,full_M6_volume_edits=0,full_M6_transfer=False,
    goal_complete=False,whole_event_speedup_established=False,
    limits=['No accepted complete M6 grid or construction-feasibility result.',
            'Frozen64-parent geometry/query equivalence is scoped, not a full-event speedup.',
            'Only optional ranking work is deferred; selected cells retain mandatory full composed checks.',
            'No CFD advancement, cluster execution or new global uncertainty calibration.'])
summary['artifact_sha256']={str(p.relative_to(ROOT)):sha(p) for p in sorted(checked) if ROOT in p.parents}
summary['helper_sha256']=sha(Path(__file__))
(HERE/'summary.json').write_text(json.dumps(summary,indent=2,allow_nan=False)+'\n')
print(json.dumps({k:summary[k] for k in ('status','source_build_binary_and_helper_pins_match','exact_unique_query_count_before','exact_unique_query_count_after','complete_M6','goal_complete')},indent=2))
