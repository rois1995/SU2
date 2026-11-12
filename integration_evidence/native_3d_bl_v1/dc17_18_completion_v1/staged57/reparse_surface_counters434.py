from pathlib import Path
import hashlib,json,re,shutil
r=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57')
p=r/'surface_stage432_assessment.json';d=json.loads(p.read_text());case=Path(d['case']);keys=['rank','rounds','updated_cells','update_seconds','election_calls','election_seconds','geometry_trials','geometry_rejected','guidance_trials','guidance_rejected','guidance_seconds','upper_calls','upper_work','upper_seconds','scope_misses','scope_build_seconds','facet_build_seconds','traversal_seconds','retention_refusals','cache_evictions']
rows=[];gaps=[]
for line in (case/'run.log').read_text().splitlines():
 if not line.startswith('NATIVE3D_STAGE rank=') or ' stage=exit ' not in line:continue
 pairs=re.findall(r'(\w+)=([^ ]+)',line)
 if line.count('NATIVE3D_STAGE rank=')!=1 or len({k for k,v in pairs})!=len(pairs):gaps.append('Interleaved/duplicate-key exit line discarded.');continue
 values=dict(pairs);row={k:float(values[k]) for k in keys};rows.append(row)
assert sorted(int(x['rank']) for x in rows)==[0,2,3,5]
backup=r/'surface_stage432_assessment_before_counter_repair434.json';assert not backup.exists();shutil.copyfile(p,backup)
original_aggregate=json.dumps(d['aggregate'],sort_keys=True);d['existing_exit_counters']=rows;d['counter_attribution_gaps']=gaps
d['counter_parser_correction434']={'reason':'Reject duplicate keys caused by MPI stdout interleaving; original rank labels could be overwritten. Per-rank JSON aggregate and payload validation unchanged.','original_assessment_sha256':hashlib.sha256(backup.read_bytes()).hexdigest(),'raw_log_sha256':hashlib.sha256((case/'run.log').read_bytes()).hexdigest(),'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
assert json.dumps(d['aggregate'],sort_keys=True)==original_aggregate;p.write_text(json.dumps(d,indent=2)+'\n')
q=r/'surface_demand432_qualification.json';v=json.loads(q.read_text());v['surface_stage_assessment_sha256']=hashlib.sha256(p.read_bytes()).hexdigest();q.write_text(json.dumps(v,indent=2)+'\n')
print('Reparsed432: four clean rank counter rows; aggregate unchanged.')
