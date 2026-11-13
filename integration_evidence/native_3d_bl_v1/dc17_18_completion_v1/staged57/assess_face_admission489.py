#!/usr/bin/env python3
"""Exact interval comparison for staged face admission; never certifies a mesh."""
import json, math, sys
from fractions import Fraction
from pathlib import Path

R = Path(__file__).resolve().parent
sys.path.insert(0, str(R))
from assess_surface_transaction470 import sha


def f(x):
    x = float(x)
    if not math.isfinite(x):
        raise ValueError(f"nonfinite number: {x!r}")
    return Fraction.from_float(x)


def score(xs, tau):
    return sum((max(f(x) - tau, Fraction()) ** 2 for x in xs), Fraction())


def self_test():
    t = f(1e-4)
    assert score([.0002, .0001], t) == (f(.0002) - t) ** 2
    assert score([.00009, .0001], t) == 0
    assert score([.0002] * 3, t) < score([.0003] * 3, t)
    identities = {("a", "old", 1), ("a", "old", 2), ("a", "fresh", 1)}
    bounds = identities.copy()
    assert sum(k[1] == "old" for k in identities) != sum(k[1] == "fresh" for k in identities)
    assert bounds == identities
    print("exact Fraction score self-check passed")


def assess(case):
    status = json.loads((case / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    pins = json.loads((case / "pins.json").read_text())
    assert pins and all(sha(p) == h for p, h in pins.items())
    handles = json.loads((case / "terminal_handles_actual.json").read_text())
    assert handles["all_owned_handles_terminal"]
    rows = [json.loads(s) for s in (case / "actual_bl_cavity_planning.jsonl").read_text().splitlines() if s]
    inputs = json.loads((R / "reverse_surface486_inputs.json").read_text())
    for name in ("owned", "parameters"):
        assert sha(inputs[name]["path"]) == inputs[name]["sha256"]
    owned, params = case / "trajectory_working_owned.bin", case / "trajectory_working_parameters.bin"
    assert sha(owned) == inputs["owned"]["sha256"] and sha(params) == inputs["parameters"]["sha256"]

    ids = [r for r in rows if r["stage"] == "face_admission_identity"]
    bounds = [r for r in rows if r["stage"] == "face_admission_bound"]
    passes = [r for r in rows if r["stage"] == "face_admission_pass"]
    trials = [r for r in rows if r["stage"] == "saved_correction_trial"]
    result = next(r for r in rows if r["stage"] == "saved_correction_result")
    assert [(r["vertex"], r["retained"]) for r in trials] == [(7594,7592),(17814,64065077),(6070,6072)]
    assert result["constructor_calls"] == 3 and result["constructor_calls_upper_bound"] <= 9
    assert result["published"] == 0

    # The native decisions are logged before admission probes; active failures stay refused, control stays feasible.
    refs = json.loads((R / "corrected_surface488_assessment.json").read_text())["trials"]
    near = json.loads((R / "reverse_surface486_assessment.json").read_text())["trials"][6]
    for row, baseline in zip(trials, (near, refs[1], refs[2])):
        for key in ("geometry_pass", "private_feasible", "old_geometry_bad", "new_geometry_bad", "old_q_bad", "new_q_bad", "old_J_bad", "new_J_bad"):
            assert row[key] == baseline[key], (row.get("context"), key, row[key], baseline[key])
    assert [r["geometry_pass"] for r in trials] == [False, False, True]
    assert [r["private_feasible"] for r in trials] == [False, False, True]

    identities = {}
    for r in ids:
        face = tuple(sorted(map(int, r["face"])))
        assert len(face) == 3 and len(set(face)) == 3
        key = (r["context"], r["side"], face, str(r["facet"]))
        assert key not in identities
        identities[key] = r
        assert len(r["rotation_upper"]) == 3
        numeric = [f(x) for x in [*r["rotation_upper"], r["canonical_upper"], r["affine_floor"]]]
        assert min(numeric) >= 0
    levels = {}
    for r in bounds:
        key = (r["context"], r["side"], tuple(sorted(map(int, r["face"]))), str(r["facet"]))
        slot = (key, f(r["precision"]))
        assert slot not in levels and isinstance(r["scope_complete"], bool)
        tau, lower, upper = (f(r[n]) for n in ("physical_tolerance", "derived_lower", "combined_upper"))
        assert tau == f(1e-4) and min(tau, lower, upper, f(r["geometry_work"]), f(r["seconds"])) >= 0
        if r["scope_complete"]: assert lower <= upper
        if r.get("query_upper") is not None: assert f(r["query_upper"]) >= 0
        assert isinstance(r["query_evaluated"], bool)
        assert all(isinstance(n, int) for n in r["support"])
        if not r["query_evaluated"]:
            ident = next(v for k, v in identities.items() if k == key)
            assert f(ident["canonical_upper"]) <= f(r["physical_tolerance"])
            assert r["scope_complete"] and f(r["derived_lower"]) == 0
            assert f(r["combined_upper"]) == f(ident["canonical_upper"]) and not r["support"]
        levels[slot] = r
    expected_p = {f(2.5e-5), f(6.25e-6)}
    observed_p = {p for _, p in levels}
    contexts = sorted({r["context"] for r in trials})
    complete_passes = len(contexts) == 3 and result["contexts_completed"] == 3
    complete_passes &= len(passes) == 3 and {p["pass"] for p in passes} == {0,1,2} and all(p["complete"] for p in passes)
    complete_passes &= all(set(p["contexts_completed"]) == set(contexts) if isinstance(p["contexts_completed"], list)
                           else p["contexts_completed"] == 3 for p in passes)
    exact_coverage = all({k for (k,p) in levels if p == precision and k[1] == side} ==
                         {k for k in identities if k[1] == side} for precision in expected_p for side in ("old", "fresh"))
    complete_coverage = observed_p == expected_p and exact_coverage
    complete_coverage &= all(any(k[0] == c and k[1] == side for k in identities)
                             for c in contexts for side in ("old", "fresh"))
    report = dict(case=str(case), complete=bool(complete_passes and complete_coverage), passes=passes,
                  status=status, verified_pin_count=len(pins), all_owned_handles_terminal=True,
                  native_refusal_unchanged=True, published=False, final_mesh_accepted=False,
                  face_count=len(identities), precisions={})
    if not report["complete"]:
        report["reason"] = "three complete passes and exact identity/bound coverage at both precisions are required; no verdict emitted"
        return report

    for precision in sorted(expected_p):
        per_context = {}
        for context in contexts:
            sides = {}
            for side in ("old", "fresh"):
                keys = [k for k in identities if k[0] == context and k[1] == side]
                lo = hi = Fraction()
                canonical_score = Fraction()
                rotation_scores = [Fraction()] * 3
                order_variation = []
                scope_proof = True
                affine_floor_safe = True
                for key in keys:
                    ident, b = identities[key], levels[(key, precision)]
                    tau, affine = f(b["physical_tolerance"]), f(ident["affine_floor"])
                    canonical = f(ident["canonical_upper"])
                    derived, combined = f(b["derived_lower"]), f(b["combined_upper"])
                    assert tau == f(1e-4) and min(affine, canonical, derived, combined) >= 0
                    if b["scope_complete"]: assert derived <= combined
                    if canonical <= tau:
                        assert b["scope_complete"] and derived == 0 and combined == canonical
                    lower = max(affine, derived) if b["scope_complete"] else Fraction()
                    upper = max(affine, combined)
                    lo += max(lower-tau, 0) ** 2
                    hi += max(upper-tau, 0) ** 2
                    scope_proof &= b["scope_complete"]
                    affine_floor_safe &= affine <= tau
                    canonical_score += max(canonical-tau, 0) ** 2
                    for j, x in enumerate(ident["rotation_upper"]): rotation_scores[j] += max(f(x)-tau, 0) ** 2
                    order_variation.append(max(map(f, ident["rotation_upper"])) - min(map(f, ident["rotation_upper"])))
                sides[side] = dict(lower=str(lo), upper=str(hi), lower_float=float(lo), upper_float=float(hi),
                                   canonical_guidance_score=str(canonical_score),
                                   rotation_guidance_scores=[str(x) for x in rotation_scores],
                                   per_face_order_variation=[str(x) for x in order_variation],
                                   scope_proof=scope_proof, affine_floor_below_tolerance=affine_floor_safe)
            fl, fu, ol, ou = map(Fraction, (sides["fresh"]["lower"], sides["fresh"]["upper"], sides["old"]["lower"], sides["old"]["upper"]))
            fresh_lower_valid = sides["fresh"]["scope_proof"] and sides["fresh"]["affine_floor_below_tolerance"]
            old_lower_valid = sides["old"]["scope_proof"] and sides["old"]["affine_floor_below_tolerance"]
            verdict = "genuine_one_sided_regression" if fresh_lower_valid and fl > ou else \
                      "improvement_despite_native_refusal" if old_lower_valid and fu < ol else \
                      "overlapping" if fresh_lower_valid and old_lower_valid else "policy_interval_only_physical_uncertain"
            per_context[context] = dict(scores=sides, comparison=verdict,
                                        fresh_minus_old_lower=str(fl-ol), fresh_minus_old_upper=str(fu-ou))
        report["precisions"][str(float(precision))] = dict(contexts=per_context,
            raw_bound_records=[v for (k,p),v in levels.items() if p == precision])
    report["limits"] = ["rotation sums are numerical-order envelopes, not distance intervals",
                        "upper-bound score deltas are diagnostic; final mesh acceptance is not assessed"]
    return report


def main():
    if "--self-test" in sys.argv:
        self_test(); return
    cases = sorted((R / "cases").glob("curved489_face_admission_serial_*"))
    if not cases:
        print("No face-admission cases found; no assessment written."); return
    report = dict(diagnostic="face_admission489", final_mesh_accepted=False,
                  cases=[assess(case) for case in cases])
    out = R / "face_admission489_assessment.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(out)


if __name__ == "__main__": main()
