#!/usr/bin/env python3
"""DC-5 production re-run: classify every rejected engine trial of one A1 run by its PRODUCTION reason.

Input: the JSONL written by the [NativeSymmetryRuntime3D] annulus observer (SU2_NATIVE_TRIAL_LOG) and the run log
(NATIVE3D_DETAIL stage=refine line, engine histograms). Every record falls in exactly one class; the rules use the
engine's MetricDiagnosis for measured candidates and the production reason text for operator/geometry/dependency
rejections (unmatched texts are reported as 'unclassified' with their distinct prefixes, never dropped)."""
import collections, json, re, sys
FAILURE = {0: 'None', 1: 'Measurement', 2: 'Shape', 3: 'Progress'}
Q, J = .20, .05
RULES = [  # (class, regex on reason) for unmeasured / non-metric rejections, first match wins.
 ('limit', r'elapsed|work budget|global proposal budget|evaluation budget exhausted|trial budget|exhausted'),
 ('height', r'prescribed height fails|first-height|geometry-height incompatibility|normal foot leaves|footprint'),
 ('dependency/cap', r'dependency|reservation|exceeds budget|exceeds its|cell count|complete-star|complete star|versioned|identity|MaximumReplacement'),
 ('split operator', r'obstruct|cannot see|cone|cavity|visib|inverts|orientation|apex|cover|reconnect|split|insert[^i]|collapse|strip|flip|refit|placement|positive|invert'),
 ('geometry', r'geometry|geometric|surface|facet|chart|seam|crease|junction|marker|projected|collides|overlap|curved|clearance|immutable|authority|analytic|planar'),
]
def classify(r):
    f = FAILURE.get(r['failure'], str(r['failure']))
    if r['candidate'] and f == 'Shape':
        if r['q'] >= .18 and r['q'] < Q and r['J'] >= J: return 'shape q in [0.18,0.20)'
        return 'shape q<0.18 or J<0.05'
    if r['candidate'] and f == 'Progress': return 'progress'
    if r['candidate'] and f == 'Measurement':
        text = r['reason']
        if re.search(r'budget|exhausted', text, re.I): return 'limit'
        return 'measurement'
    for name, rule in RULES:
        if re.search(rule, r['reason'], re.I): return name
    return 'unclassified'
def main(path, log=None):
    fix = lambda l: re.sub(r":(-?)(inf|nan)\b", lambda m: ":" + ("NaN" if m.group(2) == "nan" else m.group(1) + "Infinity"), l)
    records = [json.loads(fix(l)) for l in open(path)]
    classes = collections.Counter(); prefixes = collections.defaultdict(collections.Counter)
    for r in records:
        c = classify(r); classes[c] += 1
        prefixes[c][re.sub(r'[-+]?\d[\d.eE+-]*', '#', r['reason'])[:110]] += 1
    out = {'records': len(records), 'classes': dict(classes.most_common()),
           'reasons': {c: dict(p.most_common(12)) for c, p in prefixes.items()}}
    qs = [r['q'] for r in records if r['candidate'] and FAILURE.get(r['failure']) == 'Shape']
    out['shape_q_histogram'] = dict(collections.Counter(f'{min(int(q * 20) / 20, .95):.2f}' for q in qs).most_common())
    if log:
        text = open(log, errors='replace').read()
        line = next((l for l in text.splitlines() if l.startswith('NATIVE3D_DETAIL stage=refine')), '')
        keys = ('stop', 'global_work', 'work_limit', 'rounds', 'attempted', 'accepted', 'rejected', 'height_checks', 'height_rejected',
                'footprint_rejected', 'geometry_trials', 'geometry_rejected', 'metric_trials', 'metric_shape_rejected',
                'metric_progress_rejected', 'metric_measurement_rejected', 'wall_reconstruction_trials', 'wall_reconstruction_accepted',
                'wall_joint_trials', 'wall_joint_accepted', 'placement_trials', 'placement_validated', 'oversized_stars',
                'final_owned_cells', 'remaining_edges', 'remaining_repairs', 'cached_invalid_cells', 'sample_q', 'sample_j')
        fields = dict(kv.split('=', 1) for kv in line.split()[1:] if '=' in kv)
        out['engine'] = {k: fields[k] for k in keys if k in fields}
        out['rejected_event'] = [l for l in text.splitlines() if 'rejected event' in l][-1:] 
    print(json.dumps(out, indent=1))
if __name__ == '__main__':
    main(*sys.argv[1:])
