from pathlib import Path
import json,hashlib,time
r=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57');a=r/'m6_runs/euler_build254_conditioned_mach_only_real_cfd50k';b=r/'m6_runs/euler_build251_upper_cutoff_mach_only_real_cfd50k';template=r/'analyze_m6244_mach_only.py';original=template.read_text();assert original.count('euler_build244_mach_only_real_cfd50k')==2;source=original.replace('euler_build244_mach_only_real_cfd50k',a.name)
needle="if e['stage'] in ('window','exit') and";assert source.count(needle)==1
source=source.replace(needle,"if all(e['complete_flags'].values()) and e['stage'] in ('window','exit') and")

try:exec(compile(source,str(template),'exec'),{'__file__':str(template)})
except SystemExit as e:assert e.code==0

p=json.loads((a/'construction_progress_assessment.json').read_text());print('complete stage groups',len(p['events']));print([{k:e[k] for k in ['stage','round','phase','totals']} for e in p['events'][-3:]])
# Join existing fixed-position drift counters to their following complete census.
drift_groups={}
complete={(e['stage'],e['round']) for e in p['events'] if e['phase']=='improvement'}
for path,receipt in p['source_prefixes'].items():
 raw=Path(path).read_bytes()[:receipt['bytes']]
 assert hashlib.sha256(raw).hexdigest()==receipt['sha256']
 pending=None
 for line in raw.decode(errors='replace').splitlines():
  if line.startswith('NATIVE3D_QUALITY_DRIFT '):
   pending=dict(item.split('=',1) for item in line.split()[1:] if '=' in item)
  elif line.startswith('NATIVE3D_QUALITY ') and pending is not None:
   row=dict(item.split('=',1) for item in line.split()[1:] if '=' in item)
   assert row['rank']==pending['rank'] and row['stage']==pending['stage']
   key=(row['stage'],int(row['round']))
   if key in complete:drift_groups.setdefault(key,{})[row['rank']]=pending
   pending=None
drift=[]
for (stage,round_),rows in drift_groups.items():
 assert set(rows)==set(map(str,range(6)))
 drift.append({'stage':stage,'round':round_,'ranks':rows})
(a/'target_drift_census.json').write_text(json.dumps({'scope':'Existing fixed-position samples from quality entry;not whole-field equality or cross-run matching sample sets. No new sampling or repair-decision changes.','censuses':drift},indent=2)+'\n')
idx={(e['stage'],e['round'],e['phase']):e for e in p['events']};pubs=[]
for e in p['events']:
 if e['stage']!='ownership_committed':continue
 staged=idx[('ownership_staged',e['round'],e['phase'])]
 assert staged['totals']==e['totals'] and staged['complete_flags']==e['complete_flags']
 pubs.append({'round':e['round'],'phase':e['phase'],'whole_field_counts_retained':True,'flags_retained':True,'totals':e['totals'],'flags':e['complete_flags']})
(a/'ownership_census_integrity.json').write_text(json.dumps({'time':time.time(),'scope':'Complete staged-to-committed aggregate census flags/counts;not row identity/dependency or acceptance proof.','publications':pubs},indent=2)+'\n')
provenance={'time':time.time(),'driver_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'template_sha256':hashlib.sha256(template.read_bytes()).hexdigest(),'transformed_sha256':hashlib.sha256(source.encode()).hexdigest(),'changes':['Replace BOTH literal244 attempt paths with254','Require complete flags for WINDOW/EXIT location selector'],'source_scope_verified':all(a.name in f for f in p['source_prefixes']),'complete_stage_groups':len(p['events']),'scope':'Existing first-event parsers;retain partial EXIT and unknown counts;no mesh acceptance or timing causality.'}
assert provenance['source_scope_verified']
(a/'live_quality254_analysis_provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
if (a/'quality_progress.json').exists():
 new=json.loads((a/'quality_progress.json').read_text())['records'];old=json.loads((r/'m6_runs/euler_build253_witness_cutoff_mach_only_real_cfd50k/quality_progress.json').read_text())['records']
 paired=[]
 for rnd,n in new['windows'].items():
  near=min(old['windows'],key=lambda z:abs(old['windows'][z]['quality_seconds']-n['quality_seconds']));o=old['windows'][near]
  paired.append({'round254':int(rnd),'round253':int(near),'quality_seconds254':n['quality_seconds'],'quality_seconds253':o['quality_seconds'],'gap254_minus253':n['quality_seconds']-o['quality_seconds'],'current254':n,'previous253':o})
 (a/'quality_comparison253.json').write_text(json.dumps({'time':time.time(),'scope':'Nearest measured complete quality-time census;not interpolated equal-time counts. Corrected target changes workload;no same-query or isolated-speedup claim. Use absolute q/J/geometry,net rates,churn and useful work,not debt/commits alone.','nearest_measured_quality_times':paired},indent=2)+'\n')
 print('windows',len(new['windows']),'complete ownership publications',len(pubs))
