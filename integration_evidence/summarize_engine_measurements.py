"""Aggregate raw engine samples without treating incomplete contracts as passes."""
import argparse, collections, hashlib, json, statistics
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('directory', type=Path)
p.add_argument('--output', type=Path, required=True)
a=p.parse_args();root=a.directory.resolve(strict=True)
if a.output.exists():raise SystemExit('Preserve earlier summaries; choose a fresh output.')
groups=collections.defaultdict(list);inputs={}
for path in sorted(root.rglob('native_scaling.json')):
 m=json.loads(path.read_text());rel=str(path.relative_to(root));inputs[rel]=hashlib.sha256(path.read_bytes()).hexdigest()
 category=path.relative_to(root).parts[0]
 key=(category,m['tiles'],m['layout'],m['anisotropy'],m['matched'],m['ranks'])
 groups[key].append(m)
rows=[]
fields=('input_cells','output_cells','init_seconds','adapt_seconds','selection_seconds_max','collective_seconds_max',
        'commits','cross_rank','conflicts','collective_calls_max','traffic_bytes_sum','exchange_work_bytes_max',
        'memory_rejected','max_owned','rss_hwm_kib_max','rounds','max_patch','max_donors','qmin','lmax','height_error')
for (category,tiles,layout,ar,matched,ranks),samples in sorted(groups.items()):
 row=dict(category=category,tiles=tiles,layout=layout,anisotropy=ar,matched=matched,ranks=ranks,samples=len(samples),
          complete=sum(m['complete'] for m in samples),all_complete=all(m['complete'] for m in samples))
 row['statistics']={}
 for f in fields:
  values=[m[f] for m in samples]
  row['statistics'][f]={'min':min(values),'median':statistics.median(values),'max':max(values)}
 if all('input_cut_edges' in m for m in samples):row['input_cut_edges']=samples[0]['input_cut_edges']
 rows.append(row)
a.output.write_text(json.dumps({'scope':'raw samples only; independent audit and timeout logs must also be assessed',
 'directory':str(root),'input_sha256':inputs,'groups':rows},indent=2)+'\n')
for r in rows:
 t=r['statistics']['adapt_seconds'];c=r['statistics']['output_cells']
 print(f"{r['category']} layout{r['layout']} AR{r['anisotropy']} p{r['ranks']}: complete{r['complete']}/{r['samples']} "
       f"adapt{t['median']:.4f}s [{t['min']:.4f},{t['max']:.4f}], cells[{c['min']},{c['max']}]")
