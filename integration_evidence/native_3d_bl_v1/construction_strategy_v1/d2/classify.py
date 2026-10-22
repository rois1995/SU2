#!/usr/bin/env python3
"""D2: classify native replay records (diagnose*.jsonl) of non-integrable edges by their residual hot spots.
Spot classes (top spot = largest residual error decides the witness class):
  tie      owners differ, distinct feet (concave medial switch) and the two owners' distances agree to <1e-10 (relative)
           or within the on-wall rounding bound 1.73e-14 (absolute, scaled by d at t*) at t*+-1e-9, +-1e-6, +-1e-3: the chord lies in the medial set; native ownership is decided by rounding.
  tie_ending_within_1e-3  the same tie holds within +-1e-6 but not at +-1e-3 (end of the medial stretch).
  root     owners differ, transversal switch (gap >=1e-10 somewhere in the window), jump not covered by a cut
           (cut_gap*jump_rel > 1e-10); 'rounded' if an envelope transition lies within 1e-6, else 'missed'.
  branch   same owner (or same foot) on both sides with an integrand jump > 1e-6: tensor branch not covered by a cut.
  other    no jump (> 1e-9) found, owner unknown, or covered jump.
Usage: classify.py diagnose.jsonl [out.json]"""
import collections, json, sys
TOL = 1.73157e-14  # d.tolerance printed by the replay log
src = sys.argv[1]; R = [json.loads(l) for l in open(src)]
def spot_class(s):
    jr = s['jump_rel'] or 0
    if jr <= 1e-9: return 'other:no_jump'
    if 'relation' not in s: return 'other:owner_unknown'
    rel = s['relation']; feet = s['feet_gap'] or 0
    gaps = [abs(g) for d, g in s['tie'] if d != 0 and g is not None]
    scale = max(s['left']['d'] or 0, s['right']['d'] or 0)
    # Relative 1e-10, or absolute within the existing on-wall rounding bound (64 eps max|x| = 1.73e-14 on M6).
    persistent = bool(gaps) and (max(gaps) < 1e-10 or max(gaps) * scale <= TOL)
    if rel == 'same' or feet <= 1e-9:
        return 'branch:' + rel if jr > 1e-6 else 'other:small_same_owner'
    if persistent: return 'tie:' + rel
    near = [abs(g) for d, g in s['tie'] if d != 0 and abs(d) <= 1e-6 and g is not None]
    if near and (max(near) < 1e-10 or max(near) * scale <= TOL): return 'tie_ending_within_1e-3:' + rel
    if s['cut_gap'] * jr <= 1e-10: return 'other:covered_' + rel
    return ('root:rounded_' if s['transition_gap'] < 1e-6 else 'root:missed_') + rel
out = {'source': src, 'records': len(R), 'native_ok': sum(r.get('native_ok', 0) for r in R),
       'native_called': sum(r.get('native_called', 1) for r in R),
       'replica_reproduces_native': sum(r.get('replica_reproduces_native', 0) for r in R),
       'cut_replica_equal': sum(r['cut_replica_equal'] for r in R), 'replica_ok': sum(r['replica_ok'] for r in R)}
top = collections.Counter(); anyspot = collections.Counter(); allspots = collections.Counter(); dih = collections.defaultdict(list)
flips = collections.defaultdict(list); jumps = collections.defaultdict(list); pure = collections.Counter()
for r in R:
    if r['replica_ok']: top['converged'] += 1; continue
    if not r['spots']: top['other:no_spot'] += 1; continue
    cls = [spot_class(s) for s in r['spots']]
    top[cls[0]] += 1; s = r['spots'][0]
    if 'dihedral' in s: dih[cls[0]].append(s['dihedral'])
    flips[cls[0]].append(s['flips']); jumps[cls[0]].append(s['jump_rel'])
    for c in set(cls): anyspot[c] += 1
    for c in cls: allspots[c] += 1
    pure['all_spots_tie' if all(c.startswith('tie') for c in cls) else 'mixed_or_other'] += 1
import statistics as st
def q(v):
    v = sorted(v); return [v[0], v[len(v) // 2], v[-1]] if v else None
out.update({'top_spot_class': dict(top.most_common()), 'witnesses_with_spot_class': dict(anyspot.most_common()),
            'spot_class_counts_top4': dict(allspots.most_common()), 'all_top4_spots_tie': dict(pure),
            'top_spot_dihedral_deg_min_med_max': {k: q(v) for k, v in dih.items()},
            'top_spot_flips_in_512_min_med_max': {k: q(v) for k, v in flips.items()},
            'top_spot_jump_rel_min_med_max': {k: q(v) for k, v in jumps.items()}})
if 'replica_evaluations' in R[0]:
    ev = [r['replica_evaluations'] for r in R if r['replica_ok']]
    out['converged_evaluations_min_med_max'] = q(ev)
txt = json.dumps(out, indent=1); print(txt)
if len(sys.argv) > 2: open(sys.argv[2], 'w').write(txt + '\n')
