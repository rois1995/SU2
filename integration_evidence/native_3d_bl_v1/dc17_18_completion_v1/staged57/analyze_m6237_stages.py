"""Reuse existing first-event stage parsing and exact matching-stage comparison."""
from pathlib import Path
import json,hashlib,time,re

def first_native_lines(raw):
 end=raw.rfind(b'\n')+1;raw=raw[:end];used=0;parsed=[]
 for binary in raw.splitlines(keepends=True):
  used+=len(binary);line=re.sub(r'\x1b\[[0-9;]*m','',binary.decode(errors='replace')).strip()
  if not line.startswith('NATIVE3D_STAGE rank='):continue
  kv=dict(item.split('=',1) for item in line.split()[1:] if '=' in item);parsed.append(kv)
  if kv.get('stage','').lower()=='exit':break
 return parsed,raw[:used]
probe=b'NATIVE3D_STAGE rank=0 stage=exit rounds=1 phase=improvement\nNATIVE3D_STAGE rank=0 stage=quality_entry rounds=2 phase=improvement\n'
rows,prefix=first_native_lines(probe);assert len(rows)==1 and prefix==probe.splitlines(keepends=True)[0]
r=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57');a=r/'m6_runs/euler_build237_signed_bounds_real_cfd50k';groups={};sources={}
for rank in range(6):
 p=a/f'rank_output/1/rank.{rank}/stdout';parsed,raw=first_native_lines(p.read_bytes());sources[str(p)]={'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
 for kv in parsed:
  key=(kv['stage'],kv.get('rounds'),kv.get('phase'));groups.setdefault(key,{})[rank]=kv
out=[]
for (stage,rounds,phase),rows in groups.items():
 if len(rows)!=6:continue
 totals={name:sum(int(x[name]) for x in rows.values()) for name in ['cells','measured','q_measured','j_measured','q_bad','j_bad','geometry_bad','size_bad']}
 flags={name:all(x[name]=='1' for x in rows.values()) for name in ['cache_consistent','census_complete','shape_complete','geometry_complete']}
 out.append({'stage':stage,'round':int(rounds),'phase':phase,'maximum_native_elapsed':max(float(x['elapsed']) for x in rows.values()),'totals':totals,'complete_flags':flags,'replicated_global_work':max(int(x['global_work']) for x in rows.values()),'rank_rows':rows})
result={'time':time.time(),'scope':'First native event only; complete six-rank stage groups. Initial target equality must be verified separately; no source-only speedup or quality closure claim. Partial final EXIT counts require complete flags.','events':out,'source_prefixes':sources}
(a/'construction_progress_assessment.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'complete_groups':len(out),'latest':{k:v for k,v in out[-1].items() if k!='rank_rows'}} if out else {'complete_groups':0}))

import json,pathlib,hashlib,time
r=pathlib.Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs');names={'build235':'euler_build235_geometry_coupled_real_cfd50k','build237':'euler_build237_signed_bounds_real_cfd50k'};data={}
for label,name in names.items():
 d=json.loads((r/name/'construction_progress_assessment.json').read_text())
 for f,receipt in d['source_prefixes'].items():
  with open(f,'rb') as fp:raw=fp.read(receipt['bytes'])
  assert hashlib.sha256(raw).hexdigest()==receipt['sha256'],f
 data[label]=d
old={(x['stage'],x['round'],x['phase']):x for x in data['build235']['events']};matches=[]
work=['accepted','length_splits','length_collapses','geometry_trials','guidance_trials','guidance_faces','upper_calls','upper_work','traversal_work','scope_build_work','facet_build_work','piece_selection_work','scope_hits','scope_misses','cache_evictions']
for b in data['build237']['events']:
 key=(b['stage'],b['round'],b['phase'])
 if key not in old:continue
 a=old[key];row={'stage':key[0],'round':key[1],'phase':key[2],'counts_equal':a['totals']==b['totals'],'flags_equal':a['complete_flags']==b['complete_flags'],'totals237':b['totals'],'complete_flags237':b['complete_flags']}
 for label,event in [('build235',a),('build237',b)]:
  row[label]={'native_seconds':event['maximum_native_elapsed'],'work':{k:sum(int(v[k]) for v in event['rank_rows'].values()) for k in work},'rank_prepared_limits':[int(v['prepared_limit']) for v in event['rank_rows'].values()]}
 row['work_differences237_minus235']={k:row['build237']['work'][k]-row['build235']['work'][k] for k in work};matches.append(row)
receipt={'time':time.time(),'scope':'Matching complete first-event stage/round groups, prefix hashes verified. Unmeasured q/J printed zeros are UNKNOWN. Wall time is contention/cache/ownership sensitive; not causal kernel speedup.','matching':matches,'source_prefixes':{k:v['source_prefixes'] for k,v in data.items()},'accepted_mesh_proven':False}
(r/names['build237']/'comparison235_matching_stages.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(matches[-1:] ,indent=2))
