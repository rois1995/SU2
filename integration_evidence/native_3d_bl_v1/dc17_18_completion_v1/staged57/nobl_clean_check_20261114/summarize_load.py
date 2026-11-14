"""Summarize actual 10s host receipts in emitted marker windows; no epoch estimate."""
import json,re,statistics,time
from collections import defaultdict
from pathlib import Path
from extract_markers import ROOT,STAMP,PHASE
case=ROOT/'run';markers=json.loads((ROOT/'current_markers.json').read_text())
receipts=[json.loads(s) for s in (case/'host_resources.jsonl').read_text().splitlines()]
assert all(b['unix']>a['unix'] for a,b in zip(receipts,receipts[1:]))
partitions=[]
for f in case.glob('rank_output/*/rank.0/stdout'):
    for line in f.open(errors='replace'):
        if 'NATIVE3D_PARTITION ' in line:
            stamps=re.findall(STAMP+r'(?=<stdout>:)',line)
            if stamps:partitions.append(time.mktime(time.strptime(stamps[-1],'%a %b %d %H:%M:%S %Y')))
def summary(values):
    return {'n':len(values),'min':min(values),'median':statistics.median(values),'max':max(values)} if values else {'n':0}
events=[]
for event in markers['ranks']['0']['events']:
    values=defaultdict(list);windows=[]
    for gap in event['intervals']:
        phase=PHASE.get(gap['phase'],gap['phase']);left=gap['start_log_unix_s'];right=gap['end_log_unix_s']
        if left is None and right is not None:left=max((t for t in partitions if t<right),default=None)
        if left is None or right is None:continue
        assert left<=right
        windows.append({'phase':phase,'start_log_unix_s':left,'end_log_unix_s':right})
        values[phase].extend(float(r['loadavg'].split()[0]) for r in receipts if left<=r['unix']<right)
    events.append({'invocation':event['invocation'],'windows':windows,'phases':{p:summary(v) for p,v in values.items()}})
cpu=defaultdict(list)
for a,b in zip(receipts,receipts[1:]):
    prior={r['pid']:r for r in a['processes']}
    for row in b['processes']:
        old=prior.get(row['pid'])
        if row['comm']!='SU2_CFD' or not old or old['start_ticks']!=row['start_ticks']:continue
        assert row['ticks']>=old['ticks']
        cpu[str(row['pid'])].append(100*(row['ticks']-old['ticks'])/100/(b['unix']-a['unix']))
output={'scope':'Load receipts inside rank0 timestamped marker windows, not a reconstructed native epoch. First C* starts at preceding PARTITION log and includes any post-partition prelude.','precision':'10s polls; MPI log timestamps have1s precision; forwarding delay unmeasured. Subsecond phases can have no samples.','events':events,'observed_native_CPU_percent_of_one_core':{p:summary(v) for p,v in cpu.items()},'cpu_accounting':'Retained /proc user+system ticks, host CLK_TCK100, divided by measured wall interval; not a wall-wait measurement.'}
(ROOT/'load_summary.json').write_text(json.dumps(output,indent=2)+'\n');print([(e['invocation'],e['phases']) for e in events])
