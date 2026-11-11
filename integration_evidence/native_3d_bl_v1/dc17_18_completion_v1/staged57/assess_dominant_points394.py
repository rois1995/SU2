"""Assess the bounded prepared production-target probe; no mesh acceptance claim."""
import json,math
from pathlib import Path
R=Path(__file__).resolve().parent
case=sorted((R/'cases').glob('curved394_dominant_panel_points_*'),key=lambda p:p.stat().st_mtime)[-1]
status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
terminal=json.loads((case/'terminal_handles.json').read_text());assert terminal['all_owned_handles_terminal']
rows=[json.loads(line) for line in (case/'actual_bl_front.jsonl').read_text().splitlines()]
evals={(r['sample'],r['phase']):r for r in rows if r.get('stage')=='prepared_target_stability'}
certs={r['sample']:r for r in rows if r.get('stage')=='prepared_target_certificate'}
assert len(evals)==36 and len(certs)==12
norm=lambda a: math.sqrt(sum(x*x for x in a))
diff=lambda a,b:norm([x-y for x,y in zip(a,b)])
def compare(a,b):
 return dict(normal_difference=diff(a['full_normal'],b['full_normal']),
             tensor_relative_difference=diff(a['combined'],b['combined'])/max(norm(a['combined']),1e-300),
             chord_speed_relative_difference=abs(a['chord_speed']-b['chord_speed'])/a['chord_speed'],
             first_height_relative_difference=abs(a['hn']-b['hn'])/a['hn'])
local=[]
for k,c in certs.items():
 assert c['upper']>=c['lower']>=0
 assert c['upper']-c['lower']<=c['requested_error']
 assert c['support_count']>0 and c['facet'] in c['supports']
 local.append(dict(sample=k,before_after=compare(evals[k,'before'],evals[k,'after']),
                   repeat=compare(evals[k,'before'],evals[k,'repeat']),certificate=c))
pairs=[]
for low,high in [(k,k+6) for k in range(6)]:
 assert evals[low,'before']['point']==evals[high,'before']['point']
 assert evals[low,'before']['sensor']==evals[high,'before']['sensor']
 pairs.append(dict(low=low,high=high,coarse_tight=compare(evals[low,'after'],evals[high,'after']),
                   foot_difference=diff(certs[low]['point'],certs[high]['point']),
                   raw_normal_difference=diff(certs[low]['normal'],certs[high]['normal']),
                   bindings=[certs[low]['binding'],certs[high]['binding']],
                   facets=[certs[low]['facet'],certs[high]['facet']]))
for pair in pairs:
 assert pair['coarse_tight']['tensor_relative_difference']<1e-9,pair
 assert pair['coarse_tight']['chord_speed_relative_difference']<1e-9,pair
resources=[json.loads(x) for x in (case/'resources.jsonl').read_text().splitlines()]
last={}
for row in resources:
 for p in row['processes']:last[p['pid']]=p
result=dict(case=str(case),status=status,terminal=terminal,points=local,pairs=pairs,
            work=sum(r['geometry_work'] for r in rows if r.get('stage')=='prepared_target_point_complete'),
            point_seconds=sum(r['seconds'] for r in rows if r.get('stage')=='prepared_target_point_complete'),
            last_sample_process_cpu_ticks={pid:dict(user=p.get('user_cpu_ticks'),system=p.get('system_cpu_ticks')) for pid,p in last.items()},
            limitations=['Bounded prepared-target probe, no complete edge integral or constructed patch.',
                         'Sensor parameter reconstructed from rounded point; identical fixed sensor within each comparison.',
                         'Distance interval does not bound foot or normal error.',
                         'No MPI wait attribution from CPU ticks; this is serial.'])
(R/'dominant_points394_assessment.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(dict(seconds=status['seconds'],work=result['work'],point_seconds=result['point_seconds'],pairs=pairs),indent=2))
