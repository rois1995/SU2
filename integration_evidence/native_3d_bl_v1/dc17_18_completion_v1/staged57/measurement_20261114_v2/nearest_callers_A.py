#!/usr/bin/env python3
"""Nearest-symbol caller distributions from saved Run A LBR stack summaries."""
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parent
PROFILE = ROOT / "run_A" / "profile"
SOURCES = {"preparation": PROFILE / "rank0.preparation.summary.json",
           "native_surface": PROFILE / "rank0.native_surface.summary.json"}
OFFSET = re.compile(r"\+0x[0-9a-fA-F]+$")
UNKNOWN = re.compile(r"\[unknown\]|\?\?|PERF_CONTEXT|\[truncated\]|\.\.\.", re.I)
OPS = ("Multiply", "Add", "Adjacent", "Blossom")


def normalize(name):
    return OFFSET.sub("", name.strip())


def leaf_group(name):
    n = normalize(name)
    if n.startswith("SU2NativeGeometry::") and any(f"::{op}(" in n for op in OPS):
        return "interval_arithmetic"
    if "__nexttowardl" in n or "nextafterl" in n:
        return "libm_nextafter"
    if "OptionA::InStrip(" in n:
        return "in_strip"
    if "BoundaryGuidance(" in n and "lambda" in n:
        return "boundary_guidance_lambda"
    return None


def distinct_callers(frames, limit):
    result = []
    for raw in frames[1:]:
        name = normalize(raw)
        if result and name == result[-1]:
            continue
        result.append(name)
        if len(result) == limit:
            break
    return result


def geometry_category(frames):
    """Use only the first five caller frames; distant LBR tails are unreliable."""
    near = distinct_callers(frames[:6], 5)
    prefix = " | ".join(near)
    if "NormBound" in prefix and "ErrorBound" in prefix and re.search(r"Admissible|Realize", prefix):
        return "point_admissibility_normal_realization", near[0] if near else "<unresolved>"
    if "CubicDeviation" in prefix and "SurfaceError" in prefix:
        return "local_face_model_deviation", near[0] if near else "<unresolved>"
    if re.search(r"TriangleDistance|Bernstein|QuerySurfaceUnion|BoundPhysicalSurface", prefix):
        return "wall_union_proof", next((x for x in near if re.search(
            r"TriangleDistance|Bernstein|QuerySurfaceUnion|BoundPhysicalSurface", x)), near[0])
    if re.search(r"evaluateSurface", prefix, re.I) and re.search(
            r"EvaluateAtSurfaceAssociation|IntrinsicSurfaceGuidance", prefix):
        return "target_surface_evaluation", next((x for x in near if re.search(
            r"EvaluateAtSurfaceAssociation|IntrinsicSurfaceGuidance", x)), near[0])
    if "BoundaryParameterState" in prefix and re.search(r"ContainsEntityFace|BoundaryParameterState", prefix):
        return "parameter_coverage", next((x for x in near if "BoundaryParameterState" in x or "ContainsEntityFace" in x), near[0])
    mobility_call = next((x for x in near if re.search(r"EdgeNormal|CubicNormalControls|NormalControls", x)), None)
    mobility_owner = next((x for x in near if "BuildMobilityPolicy" in x), None)
    if mobility_call and mobility_owner:
        return "mobility_policy", mobility_call
    return "unresolved", near[0] if near else "<no visible caller>"


def nextafter_origin(frames):
    """Only positive Adjacent ancestry when it is the nearest non-libm frame."""
    for raw in frames[1:]:
        name = normalize(raw)
        if "nextafterl" in name or "__nexttowardl" in name or "@plt" in name or "libm" in name:
            continue
        return "observed_Adjacent_ancestry" if "Adjacent(" in name else "unresolved_direct_vs_inlined"
    return "unresolved_direct_vs_inlined"


def caller_label(counter, count, denominator):
    return {"caller": counter, "samples": count,
            "sample_share": count / denominator if denominator else None,
            "resolution": "not resolved" if count < 100 else "resolved"}


def profile_summary(path):
    data = json.loads(path.read_text())
    groups = defaultdict(lambda: {"denominator": 0, "leaves": Counter(),
                                  "prefixes": {1: Counter(), 2: Counter(), 3: Counter()},
                                  "categories": Counter(), "category_evidence": Counter(),
                                  "operations": defaultdict(lambda: {"samples": 0,
                                      "categories": Counter(), "evidence": Counter()})})
    diagnostics = defaultdict(Counter)
    for stack in data.get("stacks", []):
        samples = int(stack.get("samples", 0))
        frames = stack.get("leaf_to_root", [])
        if samples <= 0 or not frames:
            continue
        leaf = normalize(frames[0])
        group = leaf_group(leaf)
        if not group:
            continue
        d = groups[group]
        d["denominator"] += samples
        d["leaves"][leaf] += samples
        calls = distinct_callers(frames, 3)
        for depth in (1, 2, 3):
            prefix = " → ".join(calls[:depth]) if len(calls) >= depth else " → ".join(calls + ["<missing>"] * (depth - len(calls)))
            d["prefixes"][depth][prefix] += samples
        if len(frames) == 1:
            diagnostics[group]["no_visible_caller"] += samples
        if any(UNKNOWN.search(x) for x in frames):
            diagnostics[group]["restricted_or_unknown_chain"] += samples
        if stack.get("truncated") or any("[truncated]" in x for x in frames):
            diagnostics[group]["explicitly_truncated_chain"] += samples
        else:
            diagnostics[group]["no_truncation_marker"] += samples
        diagnostics[group]["maximum_stack_depth"] = max(diagnostics[group]["maximum_stack_depth"], len(frames))

        if group == "interval_arithmetic":
            op = next((name for name in OPS if f"::{name}(" in leaf), "unknown")
            category, evidence = geometry_category(frames)
            d["categories"][category] += samples
            d["category_evidence"][evidence] += samples
            row = d["operations"][op]
            row["samples"] += samples
            row["categories"][category] += samples
            row["evidence"][evidence] += samples
        elif group == "libm_nextafter":
            d["categories"][nextafter_origin(frames)] += samples
        elif group == "in_strip":
            parent = normalize(frames[1]) if len(frames) > 1 else "<no visible caller>"
            d["categories"][parent] += samples
        elif group == "boundary_guidance_lambda":
            # Keep the nearby function/callback frames; outer engine step is not inferred.
            d["categories"]["engine_step_unresolved"] += samples

    output = {}
    for group, d in groups.items():
        denom = d["denominator"]
        result = {"sample_denominator": denom,
                  "leaf_functions": [caller_label(k, n, denom) for k, n in d["leaves"].most_common()],
                  "first_1_2_3_distinct_caller_prefixes": {
                      str(depth): [caller_label(k, n, denom) for k, n in d["prefixes"][depth].most_common()]
                      for depth in (1, 2, 3)},
                  "chain_diagnostics": dict(diagnostics[group])}
        if group == "interval_arithmetic":
            result["conservative_near_caller_categories"] = [caller_label(k, n, denom) for k, n in d["categories"].most_common()]
            result["near_symbol_evidence"] = [caller_label(k, n, denom) for k, n in d["category_evidence"].most_common()]
            result["operators"] = {op: {"sample_denominator": row["samples"],
                "categories": [caller_label(k, n, row["samples"]) for k, n in row["categories"].most_common()],
                "near_symbol_evidence": [caller_label(k, n, row["samples"]) for k, n in row["evidence"].most_common()]}
                for op, row in d["operations"].items()}
        elif group == "libm_nextafter":
            result["observed_near_ancestry"] = [caller_label(k, n, denom) for k, n in d["categories"].most_common()]
            result["limit"] = "No near Adjacent symbol is unresolved: inlining prevents a negative source-branch conclusion."
        elif group == "in_strip":
            result["immediate_parent_only"] = [caller_label(k, n, denom) for k, n in d["categories"].most_common()]
        elif group == "boundary_guidance_lambda":
            result["nearest_visible_chain"] = result["first_1_2_3_distinct_caller_prefixes"]
            result["engine_step"] = "unresolved; no distant seed/candidate/census inference"
        output[group] = result
    return {"input": str(path), "sample_stacks": data.get("sample_stacks"),
            "unique_stacks": data.get("unique_stacks"), "groups": output}


def self_check():
    chain = ["leaf", "NormBound()", "ErrorBound()", "Realize()", "BuildMobilityPolicy()"]
    assert geometry_category(chain)[0] == "point_admissibility_normal_realization"
    chain = ["leaf", "NearCaller()", "FarRoot::BuildMobilityPolicy()"]
    assert geometry_category(chain)[0] == "unresolved"
    assert nextafter_origin(["__nexttowardl", "libm.so+0x2", "otherNear()", "Adjacent()"])[0:] == "unresolved_direct_vs_inlined"
    assert nextafter_origin(["__nexttowardl", "Adjacent()+0x20", "far()"])[0:] == "observed_Adjacent_ancestry"


def main():
    profiles = {name: profile_summary(path) for name, path in SOURCES.items()}
    report = {"method": "Sample-weighted nearest visible LBR caller prefixes; no counts are function invocations.",
              "denominators": "Each leaf-group denominator is the sum of LBR stack sample weights for that leaf group.",
              "limits": ["Long ancestors are not used to force geometry or engine-step classification.",
                         "Unresolved/unmatched samples remain explicit; counts below 100 are marked not resolved.",
                         "No near Adjacent symbol does not prove direct std::nextafter because Adjacent may be inline.",
                         "BoundaryGuidance step and exact calls per evaluation are not inferable from these samples."],
              "profiles": profiles}
    target = ROOT / "run_A" / "nearest_callers.json"
    target.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(target)


if __name__ == "__main__":
    self_check()
    main()
