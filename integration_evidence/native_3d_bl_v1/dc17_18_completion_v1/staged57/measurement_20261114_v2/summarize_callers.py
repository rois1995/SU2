#!/usr/bin/env python3
"""Summarize conditional caller shares from already decoded LBR sample stacks."""
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path


PROFILE = Path(__file__).resolve().parent / "run_A" / "profile"
SOURCES = {
    "preparation": PROFILE / "rank0.preparation.summary.json",
    "native_surface": PROFILE / "rank0.native_surface.summary.json",
}
OFFSET = re.compile(r"\+0x[0-9a-fA-F]+$")
UNKNOWN = re.compile(r"\[unknown\]|\?\?|PERF_CONTEXT|\[truncated\]|\.\.\.", re.I)
ARITH = ("Multiply", "Add", "Adjacent", "Blossom")


def normalize(frame):
    return OFFSET.sub("", frame.strip())


def group_for_leaf(leaf):
    name = normalize(leaf)
    if any(f"::{op}(" in name for op in ARITH) and name.startswith("SU2NativeGeometry::"):
        return "interval_arithmetic"
    if "__nexttowardl" in name or "nextafterl" in name:
        return "libm_nextafter"
    if "OptionA::InStrip(" in name:
        return "in_strip"
    if "BoundaryGuidance(" in name and "lambda" in name:
        return "boundary_guidance_lambda"
    return None


def caller_evidence(frames):
    ancestors = [normalize(f) for f in frames[1:]]
    rules = (
        ("BuildMobilityPolicy", r"BuildMobilityPolicy"),
        ("ContainsEntityFace_or_BoundaryParameterState", r"ContainsEntityFace|BoundaryParameterState"),
        ("wall_upper_bound_proof", r"EntityFaceError|QuerySurfaceUnion|Bernstein|Subdiv|Bounds?|UpperBound|SurfaceError"),
        ("closest_point_or_normal", r"Closest|Normal|SurfaceNormal"),
    )
    for label, pattern in rules:
        match = next((frame for frame in ancestors if re.search(pattern, frame, re.I)), None)
        if match:
            return label, match
    if not ancestors or any(UNKNOWN.search(f) for f in ancestors):
        return "unresolved", "<unresolved>"
    return "other_visible", ancestors[0]


def caller_category(frames):
    return caller_evidence(frames)[0]


def outer_guidance_caller(frames):
    for frame in frames[1:]:
        name = normalize(frame)
        if "BoundaryGuidance(" in name or "std::_Function_handler" in name or "_M_invoke" in name:
            continue
        return name
    return "<unresolved>"


def step_category(frames):
    names = [normalize(f) for f in frames[1:]]
    checks = (
        ("seed_scoring", r"seed|score"),
        ("candidate_trial", r"candidate|trial|evaluate"),
        ("census", r"census"),
    )
    for label, pattern in checks:
        if any(re.search(pattern, frame, re.I) for frame in names):
            return label
    if not names or any(UNKNOWN.search(f) for f in names):
        return "unresolved"
    return "other_visible"


def add_distribution(counter, key, samples):
    counter[key] += samples


def summarize_file(path):
    data = json.loads(path.read_text())
    stacks = data.get("stacks", [])
    totals = Counter()
    group_leaf = defaultdict(Counter)
    group_category = defaultdict(Counter)
    category_evidence = defaultdict(Counter)
    group_nearest = defaultdict(Counter)
    diagnostics = defaultdict(Counter)
    operation_samples = defaultdict(Counter)
    operation_categories = defaultdict(Counter)
    operation_evidence = defaultdict(Counter)
    nextafter_origin = Counter()
    instrip_parent = Counter()
    guidance_step = Counter()
    guidance_ancestry = Counter()
    for stack in stacks:
        samples = int(stack.get("samples", 0))
        frames = stack.get("leaf_to_root", [])
        if not frames or samples <= 0:
            continue
        leaf = normalize(frames[0])
        group = group_for_leaf(leaf)
        if not group:
            continue
        totals[group] += samples
        group_leaf[group][leaf] += samples
        ancestors = [normalize(f) for f in frames[1:]]
        if not ancestors:
            diagnostics[group]["no_parent_frame"] += samples
        if any(UNKNOWN.search(f) for f in frames):
            diagnostics[group]["restricted_or_unknown_frame"] += samples
        if stack.get("truncated") is True or any("[truncated]" in f for f in frames):
            diagnostics[group]["explicitly_truncated"] += samples
        else:
            diagnostics[group]["truncation_marker_absent"] += samples
        diagnostics[group]["max_stack_depth"] = max(diagnostics[group]["max_stack_depth"], len(frames))
        if group == "interval_arithmetic":
            op = next((op for op in ARITH if f"::{op}(" in leaf), "unknown")
            operation_samples[op]["samples"] += samples
            category, evidence = caller_evidence(frames)
            group_category[group][category] += samples
            category_evidence[group][evidence] += samples
            operation_categories[op][category] += samples
            operation_evidence[op][evidence] += samples
            group_nearest[group][ancestors[0] if ancestors else "<no parent>"] += samples
        elif group == "libm_nextafter":
            if any("Adjacent(" in f for f in ancestors):
                origin = "confirmed_Adjacent_ancestry"
            elif any("nextafter" in f and "nextafterl" not in f for f in ancestors):
                origin = "confirmed_nextafter_ancestry"
            else:
                origin = "unknown_ancestry"
            nextafter_origin[origin] += samples
            group_nearest[group][ancestors[0] if ancestors else "<no parent>"] += samples
        elif group == "in_strip":
            instrip_parent[ancestors[0] if ancestors else "<no parent>"] += samples
            category_evidence[group][ancestors[0] if ancestors else "<no parent>"] += samples
            group_nearest[group][ancestors[0] if ancestors else "<no parent>"] += samples
        elif group == "boundary_guidance_lambda":
            guidance_step[step_category(frames)] += samples
            outer = outer_guidance_caller(frames)
            guidance_ancestry[outer] += samples
            category_evidence[group][outer] += samples
            group_nearest[group][outer] += samples

    def distribution(counter, denominator):
        return [{"label": key, "samples": count,
                 "share": count / denominator if denominator else None}
                for key, count in counter.most_common()]

    groups = {}
    for group, denom in totals.items():
        groups[group] = {
            "sample_denominator": denom,
            "leaf_functions": distribution(group_leaf[group], denom),
            "nearest_visible_parent": distribution(group_nearest[group], denom),
            "chain_diagnostics": dict(diagnostics[group]),
        }
        if group == "interval_arithmetic":
            groups[group]["caller_categories"] = distribution(group_category[group], denom)
            groups[group]["caller_functions_by_sampled_category"] = distribution(category_evidence[group], denom)
            groups[group]["operators"] = {
                op: {"sample_denominator": count["samples"],
                     "caller_categories": distribution(operation_categories[op], count["samples"]),
                     "caller_functions": distribution(operation_evidence[op], count["samples"])}
                for op, count in operation_samples.items()}
        elif group == "libm_nextafter":
            groups[group]["ancestry_classification"] = distribution(nextafter_origin, denom)
            groups[group]["interpretation"] = "Only visible symbol ancestry is classified; otherwise unknown."
        elif group == "in_strip":
            groups[group]["parent_ancestry"] = distribution(instrip_parent, denom)
            groups[group]["caller_functions"] = distribution(category_evidence[group], denom)
        elif group == "boundary_guidance_lambda":
            groups[group]["visible_engine_step_ancestry"] = distribution(guidance_step, denom)
            groups[group]["nearest_ancestry"] = distribution(guidance_ancestry, denom)
            groups[group]["outer_visible_caller"] = distribution(category_evidence[group], denom)
            groups[group]["interpretation"] = "Step buckets require visible matching names; no source-only inference."
    return {"input": str(path), "decode_exit": data.get("decode_exit"),
            "sample_stacks": data.get("sample_stacks"), "unique_stacks": data.get("unique_stacks"),
            "sampled_leaf_groups": groups}


def self_check():
    assert normalize("ns::f(int)+0xabc") == "ns::f(int)"
    assert group_for_leaf("SU2NativeGeometry::(anonymous namespace)::Multiply(int)+0x0") == "interval_arithmetic"
    assert caller_category(["leaf", "EntityFaceError()", "outer"]) == "wall_upper_bound_proof"
    assert step_category(["lambda", "RunCandidateTrial()"]) == "candidate_trial"


def main(out_path):
    results = {name: summarize_file(path) for name, path in SOURCES.items()}
    out = Path(out_path)
    report = {"method": "LBR decoded sampled stacks; each stack contributes its sample weight once per matching leaf group",
              "not_calls": "Shares/counts are sampled appearances, not function invocation counts.",
              "limits": ["No exact calls per guidance evaluation can be derived from sampled stacks.",
                         "Unmatched, restricted, or explicitly truncated chains remain counted as diagnostics.",
                         "A missing truncation marker does not prove a complete call chain.",
                         "BoundaryGuidance step classification uses only visible names; source context is not counted as evidence."],
              "profiles": results}
    out.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(out)


if __name__ == "__main__":
    self_check()
    target = Path(sys.argv[1]) if len(sys.argv) > 1 else PROFILE.parent / "caller_summary.json"
    main(target)
