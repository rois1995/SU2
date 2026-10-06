"""Plot the completed, independently audited three-layout MPI experiment."""
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import statistics

root = Path(__file__).resolve().parent
parent = root/'robustness_campaign_v8/repeated'
audit_path = parent/'independent_audit.json'
audit = json.loads(audit_path.read_text())
assert audit['all_structural_pass'] and len(audit['cases']) == 27
samples = []
for case in audit['cases']:
    assert case['complete'] and case['structural_pass']
    directory = Path(case['directory'])
    assert directory.parent.resolve() == parent.resolve()
    for name, expected in case['files_sha256'].items():
        assert hashlib.sha256((directory/name).read_bytes()).hexdigest() == expected
    samples.append(json.loads((directory/'native_scaling.json').read_text()))
for key in ('tiles', 'input_cells', 'matched', 'anisotropy', 'h0', 'xmax', 'ymax'):
    assert len({sample[key] for sample in samples}) == 1, 'Different input/target conditions'
rows = []
for layout in (1, 2, 3):
    for ranks in (1, 2, 4):
        group = [m for m in samples if (m['layout'], m['ranks']) == (layout, ranks)]
        assert len(group) == 3
        times = [m['adapt_seconds'] for m in group]
        assert all(math.isfinite(value) and value > 0 for value in times)
        rows.append(dict(layout=layout, ranks=ranks, samples=3, minimum_s=min(times),
                         median_s=statistics.median(times), maximum_s=max(times),
                         output_cells_min=min(m['output_cells'] for m in group),
                         output_cells_max=max(m['output_cells'] for m in group),
                         initial_cut_edges=group[0]['input_cut_edges']))
output = root/'partition_comparison_v1'
output.mkdir()
os.environ['MPLCONFIGDIR'] = str(output/'mpl_cache')
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
fig, ax = plt.subplots(figsize=(8, 4.8))
for layout, label in ((1, 'Cyclic'), (2, 'Horizontal strips'), (3, 'Vertical strips (low cut)')):
    group = [r for r in rows if r['layout'] == layout]
    medians = [r['median_s'] for r in group]
    errors = [[r['median_s']-r['minimum_s'] for r in group], [r['maximum_s']-r['median_s'] for r in group]]
    ax.errorbar([r['ranks'] for r in group], medians, yerr=errors, marker='o', capsize=4, label=label)
ax.set(xlabel='MPI ranks', ylabel='Engine adaptation time (s)', xticks=[1, 2, 4], ylim=(0, None),
       title=f"Partition sensitivity: {samples[0]['input_cells']:,} input triangles")
ax.grid(alpha=.2)
ax.legend()
fig.text(.5, .025, 'Medians and min–max ranges of 3 runs; shared workstation.\nDifferent adapted meshes/work counts; initialization, CFD, transfer and audits excluded.',
         ha='center', fontsize=9)
fig.tight_layout(rect=(0, .08, 1, 1))
fig.savefig(output/'partition_times.png', dpi=180)
fig.savefig(output/'partition_times.svg')
with (output/'measurements.csv').open('w', newline='') as stream:
    writer = csv.DictWriter(stream, fieldnames=list(rows[0])); writer.writeheader(); writer.writerows(rows)
(output/'provenance.json').write_text(json.dumps(dict(audit=str(audit_path),
    audit_sha256=hashlib.sha256(audit_path.read_bytes()).hexdigest(),
    plot_source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    scope='Engine-only fixed input/target; rank-dependent final meshes/work; shared workstation',
    inputs=[dict(directory=c['directory'], files_sha256=c['files_sha256']) for c in audit['cases']]), indent=2)+'\n')
print(output)
