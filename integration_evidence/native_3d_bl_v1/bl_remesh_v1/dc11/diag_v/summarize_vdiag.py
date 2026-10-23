#!/usr/bin/env python3
"""Summarize vdiag_<case>.jsonl (Q4: refused splits re-examined with swap / swap+move on enlarged rings)."""
import sys, json, collections
for path in sys.argv[1:]:
    rows = [json.loads(l) for l in open(path) if l.strip()]
    S = [r for r in rows if "levels" in r]
    print(f"== {path}: {len(S)} samples")
    for r in rows:
        if "census" in r:
            print("  census (all floor refusals in V) columns:", r["census_columns"])
            tot = [0] * 6
            for op, c in r["census"].items():
                print(f"    {op:28s} {c}"); tot = [a + b for a, b in zip(tot, c)]
            print(f"    {'TOTAL':28s} {tot}")
        if r.get("summary"): print("  summary", {k: r[k] for k in r if k != "case"})
    abs_old = sum(1 for r in S if r["old_q"] >= .2 and r["old_J"] >= .05)
    print(f"  samples with old cavity above the gates (absolute refusals): {abs_old}/{len(S)}; fresh q median "
          f"{sorted(r['fresh_q'] for r in S)[len(S)//2] if S else 0:.3f}")
    any_abs = any_rel = any_adm = 0
    stat = collections.defaultdict(lambda: [0, 0, 0, 0, 0])  # n, improved, absolute, relative, admitted
    for r in S:
        a = rl = ad = False
        for L in r["levels"]:
            k = (L["level"], L["mode"]); st = stat[k]
            if "skipped" in L or "error" in L: continue
            st[0] += 1; st[1] += L["improved"]; st[2] += L["absolute_ok"]; st[3] += L["relative_ok"]; st[4] += L.get("admit8", 0)
            a |= bool(L["absolute_ok"]); rl |= bool(L["relative_ok"]); ad |= bool(L.get("admit8", 0))
        any_abs += a; any_rel += rl; any_adm += ad
    print("  level/mode          n improved abs>=.20/.05 rel>=min(old,.2) admit8")
    for k, v in sorted(stat.items()): print(f"  {k[0]:9s} {k[1]:9s} {v[0]:3d} {v[1]:8d} {v[2]:13d} {v[3]:16d} {v[4]:6d}")
    print(f"  best over levels: absolute {any_abs}/{len(S)}, relative {any_rel}/{len(S)}, Admit8 {any_adm}/{len(S)}")
    for r in S:
        best = max((L for L in r["levels"] if "after_q" in L), key=lambda L: L["after_q"], default=None)
        print(f"   {r['op'][:16]:16s} L={r['requested']:.2f} star={r['star']:2d} old q={r['old_q']:.3f} fresh q={r['fresh_q']:.3f} J={r['fresh_J']:.3f} "
              f"phys={','.join(r['physical_markers']) or '-'}"
              + (f" -> best {best['level']}/{best['mode']} q={best['after_q']:.3f} J={best['after_J']:.3f} (old ring min q={best['old_min_q']:.3f}) flips={best['flips']} moves={best['moves']}" if best else " -> none"))
