#!/usr/bin/env python3
"""Classify source-pinned native full-edge measurements; no independent quadrature claim."""
import collections,csv,hashlib,json,statistics,sys
from decimal import Decimal
from pathlib import Path
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 stage=sys.argv[1] if len(sys.argv)>1 else 'curved_interior_residual'
 assert stage in ('curved_interior_residual','curved_reconnection_probe')
 parent_stage='curved_interior_engine' if stage=='curved_interior_residual' else 'curved_interior_continue'
 folder=HERE/stage;rp=HERE/(stage+'.json');receipt=json.loads(rp.read_text());assert receipt['status'] in ('PASS','FAIL')
 for name in ('audited_grid.json','grid_edges.csv','grid_metric_summary.json'):assert sha(folder/name)==receipt['artifacts'][name]
 data=json.loads((folder/'audited_grid.json').read_text());summary=json.loads((folder/'grid_metric_summary.json').read_text())
 parent=json.loads((HERE/parent_stage/'curved_grid_partial_rejected.json').read_text())
 assert data['original']==parent['original'] and all(data[k]==parent[k] for k in ('facets','facet_groups','marker_names'))
 facets={f['id']:data['marker_names'][f['marker']] for f in data['facets']};physical=collections.defaultdict(set)
 for f in data['surface']:
  a,b,c=f['nodes']
  for edge in ((a,b),(b,c),(c,a)):physical[tuple(sorted(edge))].add(facets[f['facet']])
 threshold=Decimal.from_float(1.8);groups=collections.defaultdict(list);excess=Decimal(0)
 for row in csv.DictReader((folder/'grid_edges.csv').open()):
  length=Decimal(row['length'])
  if length<=threshold:continue
  markers=physical.get((int(row['a']),int(row['b'])),set());groups['+'.join(sorted(markers)) if markers else 'interior'].append(length);excess+=(length-threshold)**2
 count=sum(map(len,groups.values()));assert count==summary['oversized_edges']
 assert summary['shape_failures']==0
 parent_summary=json.loads((HERE/parent_stage/'grid_refine_summary.json').read_text())
 assert abs(float(excess)-parent_summary['remaining_excess'])<=1e-10
 report=dict(status='NATIVE_FIELD_RESIDUAL_CLASSIFIED',native_status=receipt['status'],cells=summary['cells'],oversized_edges=count,maximum_native_length=summary['maximum_length'],
  groups={k:dict(count=len(v),maximum_csv=str(max(v)),median_csv=str(statistics.median(v))) for k,v in sorted(groups.items())},
  squared_excess_from_csv=str(excess),native_receipt_sha256=sha(rp),helper_sha256=sha(Path(__file__)),original_donors_and_reference_unchanged=True,
  scope='The complete residual export precedes any separately failed local replay; receipt failure is preserved. Classifies native original-field full composed-edge lengths against the actual binary64 1.8 threshold. Export arithmetic agrees with the previous engine result. Independent geometry/height/quality audit is separate; no independent composed integration, accepted mesh, CFD or performance claim.')
 (HERE/('interior_residual_classification.json' if stage=='curved_interior_residual' else 'continued_residual_classification.json')).write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
if __name__=='__main__':main()
