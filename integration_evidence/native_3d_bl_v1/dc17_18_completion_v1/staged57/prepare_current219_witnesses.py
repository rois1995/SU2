"""Serialize complete current M6 witness supports using the qualified capture reader."""
from pathlib import Path
import importlib.util,json,math,re,shutil,struct,time
HERE=Path(__file__).resolve().parent
A=HERE/'m6_runs/euler_build219_local_metricL1_real_cfd50k'
OUT=A/'current_witnesses_v1'
spec=importlib.util.spec_from_file_location('boundary_preparation',HERE/'prepare_boundary202.py')
old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
reader=old.reader

def input_hashes():
    receipt=json.loads((A/'capture_ready.json').read_text())
    result={p:old.sha256(Path(p)) for p in receipt['files']}
    assert all(result[p]==v['sha256'] and Path(p).stat().st_size==v['bytes'] for p,v in receipt['files'].items())
    for p in (A/'capture_ready.json',A/'current_witness_selection.json',A/'m6.su2.native_ref',A/'run.cfg',Path(__file__),HERE/'prepare_boundary202.py',HERE/'inspect_native_failure3d.py'):
        result[str(p.resolve())]=old.sha256(p)
    return result

def main():
    started=time.monotonic();assert not OUT.exists();assert json.loads((A/'terminal_handles.json').read_text())['all_owned_terminal']
    selection=json.loads((A/'current_witness_selection.json').read_text())
    assert len(selection)==4
    selected=set()
    for label,case in selection.items():
        assert re.fullmatch('[a-z][a-z0-9_]{0,50}',label)
        assert set(case['support_cells'])=={cell for star in case['vertex_stars'].values() for cell in star}
        selected.update(case['support_cells'])
    before=input_hashes();manifests,missing=reader._load_manifests([str(A/'*_inspection_rank*_manifest.json')])
    assert not missing and len(manifests)==6 and sum(x['written_rows'] for x in manifests)==1011596
    assert all(all(x[k] for k in ('owned_complete','metrics_complete','reference_complete','rejections_complete')) for x in manifests)
    points={};physical={};metrics={};records={};core_rows={};rows=0
    for manifest in manifests:
        files,caps=manifest['_resolved_files'],manifest['_caps'];paired=reader._jsonl(files['metrics'],caps['metrics'],'metrics')
        with files['owned'].open('rb') as raw:
            for index,cell in enumerate(reader._read_owned(files['owned'],manifest['written_rows'],caps['owned'])):
                m=reader._validate_metric(next(paired),cell,files['metrics']);rows+=1
                for node,binding in zip(cell['nodes'],m['bindings']):
                    if binding is None:continue
                    assert binding['id']==node['id']
                    value=(tuple(node['xyz']),(binding['feature'],binding['version'],binding['dimension'],binding['u'],binding['v']))
                    assert points.setdefault(node['id'],value)==value
                for slot,face in enumerate(cell['physical']):
                    if not face['present']:continue
                    ordered=tuple(cell['nodes'][k]['id'] for k in old.FACES[slot]);key=tuple(sorted(ordered));assert key not in physical
                    physical[key]=(ordered,face['facet'])
                if cell['id'] not in selected:continue
                assert old.exact_positive(cell['nodes']);assert cell['id'] not in records
                raw.seek(8+index*416);record=raw.read(416);assert len(record)==416;records[cell['id']]=record
                assert m['q'] is not None and m['J'] is not None
                values=[cell['id'],m['q'],m['J']]
                for first,second,length in m['edges']:
                    assert length is not None and math.isfinite(length);values.extend((first,second,length))
                metrics[cell['id']]=old.METRIC.pack(*values)
                if any(cell['id']==c['cell'] for c in selection.values()):core_rows[str(cell['id'])]=m
        assert next(paired,None) is None
    assert rows==1011596 and set(records)==selected and set(metrics)==selected
    assert {node for face in physical for node in face}==set(points)
    control=selection['interior_control_candidate']['cell'];assert all(x is None for x in core_rows[str(control)]['bindings'])
    OUT.mkdir();old.encode(OUT/'witness_boundary_points.bin',[old.POINT.pack(node,*xyz,*binding) for node,(xyz,binding) in sorted(points.items())],old.POINT)
    old.encode(OUT/'witness_boundary_surface.bin',[old.SURFACE.pack(*ordered,facet) for _,(ordered,facet) in sorted(physical.items())],old.SURFACE)
    outputs=['witness_boundary_points.bin','witness_boundary_surface.bin']
    for label,case in selection.items():
        ordered=sorted(case['support_cells']);names=[label+'_owned.bin',label+'_expected_metrics.bin']
        old.encode(OUT/names[0],[records[cell] for cell in ordered],struct.Struct('416s'))
        old.encode(OUT/names[1],[metrics[cell] for cell in ordered],old.METRIC);outputs.extend(names)
    shutil.copyfile(A/'run.cfg',OUT/'run.cfg');(OUT/'m6.su2.native_ref').symlink_to((A/'m6.su2.native_ref').resolve());outputs.extend(['run.cfg','m6.su2.native_ref'])
    after=input_hashes();assert before==after
    receipt={'schema':'native_current_witness_preparation_v1','source_build':'build219','source_event':1,'paired_rows':rows,'boundary_points':len(points),'physical_faces':len(physical),'selected_support_cells':len(selected),'cases':selection,'core_diagnostics':core_rows,'input_hashes_before':before,'input_hashes_after':after,'outputs':{name:{'bytes':(OUT/name).stat().st_size,'sha256':old.sha256(OUT/name)} for name in outputs},'seconds':time.monotonic()-started,'scope':'Exact captured state and complete neighbor supports for bounded existing-operator tests; fresh association reconstruction, not historical replay or mesh acceptance. Supports above operator bounds are context only; use existing smaller active stars.'}
    (OUT/'witness_preparation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ('paired_rows','boundary_points','physical_faces','selected_support_cells','seconds')}))
if __name__=='__main__':main()
