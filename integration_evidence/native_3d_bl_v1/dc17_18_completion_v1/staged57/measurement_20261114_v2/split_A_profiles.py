"""Measured serial public-engine boundaries, independent of buffered log arrival."""
import json,sys
from pathlib import Path
from summarize_samples import summarize
case=Path(sys.argv[1]);profile=case/'profile'
clocks=[json.loads(line) for line in (profile/'engine_clocks.jsonl').read_text().splitlines()]
events={row['event']:row for row in clocks}
assert all(key in events for key in ('process_starti','PrepareSurface_entry','PrepareSurface_return'))
start,entry,end=[events[key]['monotonic_ns']/1e9 for key in ('process_starti','PrepareSurface_entry','PrepareSurface_return')]
assert start<entry<end
windows={'preparation':(start,entry),'native':(entry,end)}
result={name:dict(wall_seconds=b-a,profile=summarize(profile/'rank0.samples.txt',a,b)) for name,(a,b) in windows.items()}
result['clocks']=events
(case/'split_profiles.json').write_text(json.dumps(result,indent=2)+'\n')
for name in windows:print(name,result[name]['wall_seconds'],result[name]['profile']['native_samples'])
