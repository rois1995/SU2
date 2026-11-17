#!/usr/bin/env python3
"""Generate an unapplied S13 helper chain and source-reversal proofs."""
import hashlib
import json
import re
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REV = "ce252"
CPP = "Common/src/adaptation/CNativeEngine3D.cpp"
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_][A-Za-z_0-9]*|\d+(?:\.\d*)?|::|->|&&|\|\||==|!=|<=|>=|\+\+|--|\+=|-=|\*=|/=|<<|>>|[^\s]')


def read(path):
    if path == CPP: return (HERE / "CNativeEngine3D_S12.cpp.proposed").read_text()
    raise ValueError(path)


def digest(text):
    """Hash UTF-8 source."""
    return hashlib.sha256(text.encode()).hexdigest()


def format_cpp(text):
    """Format only generated C++ proposal snippets."""
    return subprocess.check_output(
        ["clang-format-18", "--style=file"], input=text.encode()
    ).decode()


def tokens(text):
    """Return lexical tokens for structural source comparison."""
    return TOKEN.findall(text)


def make_method(signature, aliases, body):
    """Format one method with markers around the transferred body."""
    aliases = prune_aliases(aliases, body)
    draft = (
        signature + " {\n" + aliases + "\n  // BEGIN_SOURCE_BODY\n"
        + body.rstrip() + "\n  // END_SOURCE_BODY\n}\n"
    )
    return format_cpp(draft)


def prune_aliases(aliases, body):
    """Keep only borrowed aliases referenced by this helper body."""
    kept = []
    for line in aliases.splitlines():
        match = re.search(r"(?:const )?auto&\s+(\w+)\s*=", line)
        if match and (re.search(rf"\b{re.escape(match.group(1))}\b", body)
                      or (match.group(1) == "result" and re.search(r"\bstats\b", body))):
            kept.append(line)
    return "\n".join(kept)


def moved_body(method):
    """Get the body region from a formatted helper."""
    return method.split("// BEGIN_SOURCE_BODY", 1)[1].split(
        "// END_SOURCE_BODY", 1
    )[0].strip()


def replace_finish(text):
    """Transport direct finish results through optional helper returns."""
    return text.replace(
        "return finish();", "return std::optional<EngineResult>(finish());"
    )


def main():
    """Generate S13 proposals and enforce recursive token reconstruction."""
    cpp = read(CPP)
    block_start = "    // Fresh regional identities must also exceed frozen donor identities after earlier coarsening.\n"
    block_end = "    region_rebuilt=changed;stats.global_work=TimedAllreduce(stats.global_work,CPassiveComm::Op::MAX,world.comm);\n"
    if cpp.count(block_start) != 1 or cpp.count(block_end) != 1:
        raise ValueError("S13 main anchors are not unique")
    start = cpp.index(block_start)
    end = cpp.index(block_end, start)
    original_chain = cpp[start:end]
    post_end_marker = "\n  }\n  if(!surface_target&&!options.wall_heights.empty())"
    post_end = cpp.index(post_end_marker, end)
    post_raw = cpp[end:post_end]
    original_span = cpp[start:post_end]

    # Split the owner/gather/service/finalization stages at original statement boundaries.
    head_end = original_chain.index("    std::vector<std::vector<OwnedCell>> buckets(world.size);")
    head_raw = original_chain[:head_end]
    gather_start = head_end
    prepare_start = original_chain.index("    DonorPatch imported;imported.valid=true;", gather_start)
    gather_raw = original_chain[gather_start:prepare_start]
    service_start = original_chain.index("    RegionFieldService service(", prepare_start)
    run_start = original_chain.index("    try {service.Run([&](const RegionMetric& target){", service_start)
    prep_prefix = original_chain[prepare_start:run_start]
    callback_start = run_start + len("    try {service.Run([&](const RegionMetric& target){\n")
    callback_end = original_chain.index("    },imported.cells);}", callback_start)
    callback_raw = original_chain[callback_start:callback_end]
    callback_close = "    },imported.cells);}catch(const std::exception& e){failure.Set(2,0,e.what());}\n"
    if original_chain.count(callback_close) != 1:
        raise ValueError("S13 service callback close anchor mismatch")
    stats_start = original_chain.index("    const auto& field_stats=service.Statistics();", callback_end)
    publish_start = original_chain.index("    const bool changed=world.maximum(!replacement.empty());", stats_start)
    post_start = len(original_chain)
    run_stats_raw = original_chain[stats_start:publish_start]
    publish_raw = original_chain[publish_start:post_start]

    # Callback source gets three nested helpers while the one existing service.Run lambda remains.
    first_nested = "std::map<Id,Point> points;"
    skin_start = callback_raw.index(first_nested)
    skin_decl_start = callback_raw.index("std::vector<Facet> skin;", skin_start)
    volume_start = callback_raw.index("        // Surface integrals give one partition-independent region kernel.")
    record_start = callback_raw.index("        if(!constructed) {")
    points_raw = callback_raw[skin_start:skin_decl_start]
    skin_raw = callback_raw[skin_decl_start:volume_start]
    volume_raw = callback_raw[volume_start:record_start]
    record_raw = callback_raw[record_start:]
    if not record_raw.endswith("      }\n"):
        raise AssertionError("expected outer callback condition close")
    record_raw = record_raw[:-len("      }\n")]
    # Caller owns points/skin at their original callback point; helper fills them by reference.
    skin_body = skin_raw.replace("std::vector<Facet> skin;", "", 1)
    # Construct writes the original status into caller storage instead of introducing a second decision.
    volume_body = volume_raw.replace("const bool constructed=ConstructWallRegion", "constructed=ConstructWallRegion", 1)
    if volume_body == volume_raw:
        raise AssertionError("ConstructWallRegion output mapping was not applied")

    # Exact prelude with direct finish branches transported by reference continuation.
    head_body = replace_finish(head_raw)
    gather_body = replace_finish(gather_raw)
    prep_body = replace_finish(prep_prefix)
    stats_body = replace_finish(run_stats_raw)
    publish_body = replace_finish(publish_raw).replace("const bool changed=world.maximum(!replacement.empty());", "changed=world.maximum(!replacement.empty());", 1)
    callback_method = (
        "      EngineSetup::EvaluateRegionTarget(\n"
        "          context, region_cells, target, rows, old, fresh, report, core_declined,\n"
        "          region_parameter_edits, replacement);"
    )
    if original_chain.count("return finish();") != (
        head_raw.count("return finish();") + gather_raw.count("return finish();")
        + prep_prefix.count("return finish();") + run_stats_raw.count("return finish();")
        + publish_raw.count("return finish();")
    ):
        raise AssertionError("S13 direct finish branches not fully assigned")

    common = (
        "  auto& world = context.world;\n"
        "  auto& options = context.options;\n"
        "  auto& geometry = context.geometry;\n"
        "  auto& composition = context.composition;\n"
        "  auto& original = context.original;\n"
        "  auto& result = *context.control.result;\n"
        "  auto& stats = result.stats;\n"
        "  auto& working = *context.ownership.working;\n"
        "  auto& selection = *context.census.selection;\n"
        "  auto& initial = *context.census.initial;\n"
        "  auto& directory = *context.exchange.directory;\n"
        "  auto& source = *context.exchange.source;\n"
        "  auto& maximum_node = *context.commit.maximum_node;\n"
        "  auto& maximum_cell = *context.commit.maximum_cell;\n"
        "  auto& coarsening = *context.control.coarsening;\n"
        "  auto& field_identity = *context.census.field_identity;\n"
        "  auto& last_rejection = *context.control.last_rejection;\n"
        "  auto& wall_counters = *context.setup.wall_counters;\n"
        "  const auto& started = *context.setup.control_prefix.started;"
    )

    # Head owns scratch/count temporaries while every descendant runs synchronously.
    head_call = (
        head_body
        + "\n    auto done = EngineSetup::GatherRegion(\n"
        + "        context, global_original, region_cells, scratch, failure, finish);\n"
        + "    if (done) return done;\n    return std::nullopt;"
    )
    head_method = make_method(
        "template <class Finish>\nstd::optional<EngineResult> EngineSetup::ConstructRegion(\n"
        "    EngineContext& context, uint64_t global_original, CLocalFailure& failure,\n"
        "    Finish& finish)",
        common,
        head_call,
    )

    # Gather keeps every original region aggregate alive through PrepareRegionField.
    gather_call = (
        gather_body
        + "\n    auto done = EngineSetup::PrepareRegionField(\n"
        + "        context, global_original, region_cells, scratch, failure, admitted,\n"
        + "        rows, regions, old, fresh, replacement, region_parameter_edits, core_declined, finish);\n"
        + "    if (done) return done;\n    return std::nullopt;"
    )
    gather_method = make_method(
        "template <class Finish>\nstd::optional<EngineResult> EngineSetup::GatherRegion(\n"
        "    EngineContext& context, uint64_t global_original, size_t region_cells,\n"
        "    size_t scratch, CLocalFailure& failure, Finish& finish)",
        common,
        gather_call,
    )

    # Prepare keeps imported/report/tick/service alive during the synchronous tail call.
    prep_body_full = (
        prep_body
        + "    try {service.Run([&](const RegionMetric& target){\n"
        + callback_method
        + "\n    },imported.cells);}catch(const std::exception& e){failure.Set(2,0,e.what());}\n"
        + "    auto done = EngineSetup::RunAndFinalizeRegion(\n"
        + "        context, service, report, failure, scratch, admitted, fresh, replacement,\n"
        + "        region_parameter_edits, finish);\n"
        + "    if (done) return done;\n    return std::nullopt;"
    )
    prep_alias = common + "\n  auto& directory = *context.exchange.directory;"
    # Remove the duplicate directory alias from common at this method only.
    prep_alias = prep_alias.replace(
        "  auto& directory = *context.exchange.directory;\n  auto& source",
        "  auto& source",
    )
    prep_method = make_method(
        "template <class Rows, class Finish>\nstd::optional<EngineResult> EngineSetup::PrepareRegionField(\n"
        "    EngineContext& context, uint64_t global_original, size_t region_cells, size_t scratch,\n"
        "    CLocalFailure& failure, bool& admitted, Rows& rows,\n"
        "    std::vector<DonorRegion>& regions, Patch& old, Patch& fresh,\n"
        "    std::vector<OwnedCell>& replacement, std::vector<BoundaryParameterEdit>& region_parameter_edits,\n"
        "    bool& core_declined, Finish& finish)",
        prep_alias,
        prep_body_full,
    )

    # Existing callback operations are copied into the helpers below.
    callback_prefix = callback_raw[:skin_start]
    evaluate_body = (
        callback_prefix
        + points_raw
        + "        std::vector<Facet> skin;\n"
        + "        EngineSetup::PrepareRegionSkin(geometry, old, points, skin);\n"
        + "        bool constructed=false;\n"
        + "        EngineSetup::ConstructRegionVolume(context, region_cells, old, skin, target, fresh, report, constructed);\n"
        + "        EngineSetup::RecordRegionResult(context, old, fresh, rows, report, already_valid, short_edges, constructed,\n"
        + "                                        core_declined, region_parameter_edits, replacement);\n"
        + "      }"
    )
    evaluate_method = make_method(
        "template <class Rows>\nvoid EngineSetup::EvaluateRegionTarget(\n"
        "    EngineContext& context, size_t region_cells, const RegionMetric& target, Rows& rows, Patch& old, Patch& fresh,\n"
        "    RegionReport& report, bool& core_declined,\n"
        "    std::vector<BoundaryParameterEdit>& region_parameter_edits,\n"
        "    std::vector<OwnedCell>& replacement)",
        common,
        evaluate_body,
    )
    skin_method = make_method(
        "void EngineSetup::PrepareRegionSkin(const PlanarReference& geometry, const Patch& old,\n"
        "                                    std::map<Id, Point>& points, std::vector<Facet>& skin)",
        "",
        skin_body,
    )
    volume_aliases = (
        "  auto& geometry = context.geometry;\n"
        "  auto& options = context.options;\n"
        "  auto& result = *context.control.result;\n"
        "  auto& stats = result.stats;\n"
        "  auto& maximum_node = *context.commit.maximum_node;\n"
        "  auto& maximum_cell = *context.commit.maximum_cell;\n"
        "  const auto& started = *context.setup.control_prefix.started;"
    )
    volume_method = make_method(
        "void EngineSetup::ConstructRegionVolume(EngineContext& context, size_t region_cells, Patch& old,\n"
        "                                         const std::vector<Facet>& skin,\n"
        "                                         const RegionMetric& target, Patch& fresh,\n"
        "                                         RegionReport& report, bool& constructed)",
        volume_aliases,
        volume_body,
    )
    record_method = make_method(
        "template <class Rows>\nvoid EngineSetup::RecordRegionResult(\n"
        "    EngineContext& context, Patch& old, Patch& fresh, Rows& rows, RegionReport& report,\n"
        "    bool already_valid, bool short_edges, bool constructed, bool& core_declined,\n"
        "    std::vector<BoundaryParameterEdit>& region_parameter_edits,\n"
        "    std::vector<OwnedCell>& replacement)",
        common,
        record_raw,
    )

    # Final report and failure vote stay before the changed publication stage.
    post_body = replace_finish(post_raw)
    run_stats_and_call = (
        stats_body
        + "    return EngineSetup::FinalizeRegionTail(\n"
        + "        context, scratch, admitted, failure, fresh, replacement, region_parameter_edits, finish);"
    )
    run_aliases = common + "\n  auto& composition = context.composition;\n  auto& surface_target = context.surface_target;\n  auto& region_rebuilt = *context.setup.control_prefix.region_rebuilt;\n  auto& region_active = *context.control.region_active;\n  auto& region_started = *context.control.region_started;"
    run_aliases = run_aliases.replace(
        "  auto& composition = context.composition;\n  auto& original",
        "  auto& original",
    )
    run_method = make_method(
        "template <class Finish>\nstd::optional<EngineResult> EngineSetup::RunAndFinalizeRegion(\n"
        "    EngineContext& context, RegionFieldService& service, RegionReport& report,\n"
        "    CLocalFailure& failure, size_t scratch, bool& admitted, Patch& fresh,\n"
        "    std::vector<OwnedCell>& replacement,\n"
        "    std::vector<BoundaryParameterEdit>& region_parameter_edits, Finish& finish)",
        run_aliases,
        run_stats_and_call,
    )
    tail_body = (
        "    bool changed=false;\n"
        "    auto done = EngineSetup::PublishRegionResult(\n"
        "        context, scratch, admitted, failure, fresh, replacement, region_parameter_edits, changed, finish);\n"
        "    if (done) return done;\n"
        + post_body.rstrip()
        + "\n    return std::nullopt;"
    )
    tail_aliases = common + (
        "\n  auto& surface_target = context.surface_target;\n"
        "  auto& region_rebuilt = *context.setup.control_prefix.region_rebuilt;\n"
        "  auto& region_active = *context.control.region_active;\n"
        "  auto& region_started = *context.control.region_started;"
    )
    tail_method = make_method(
        "template <class Finish>\nstd::optional<EngineResult> EngineSetup::FinalizeRegionTail(\n"
        "    EngineContext& context, size_t scratch, bool& admitted, CLocalFailure& failure, Patch& fresh,\n"
        "    std::vector<OwnedCell>& replacement,\n"
        "    std::vector<BoundaryParameterEdit>& region_parameter_edits, Finish& finish)",
        tail_aliases,
        tail_body,
    )
    publish_method = make_method(
        "template <class Finish>\nstd::optional<EngineResult> EngineSetup::PublishRegionResult(\n"
        "    EngineContext& context, size_t scratch, bool& admitted, CLocalFailure& failure, Patch& fresh,\n"
        "    std::vector<OwnedCell>& replacement,\n"
        "    std::vector<BoundaryParameterEdit>& region_parameter_edits, bool& changed, Finish& finish)",
        common,
        publish_body + "\n    return std::nullopt;",
    )

    # Main call replaces only the construction/service/publication span; caller-owned tail stays byte-identical.
    new_call = (
        "    auto region_done=EngineSetup::ConstructRegion(context, global_original, failure, finish);\n"
        "    if(region_done)return std::move(*region_done);\n"
    )
    candidate = cpp[:start] + new_call + cpp[post_end:]
    reversed_cpp = candidate.replace(new_call, original_span, 1)
    if reversed_cpp != cpp:
        raise AssertionError("whole Adapt S13 replacement does not reverse byte-exactly")

    # Recursive callback proof: build the transformed callback back from helper bodies.
    eval_body_moved = moved_body(evaluate_method)
    call_skin = "EngineSetup::PrepareRegionSkin(geometry, old, points, skin);"
    call_volume = "EngineSetup::ConstructRegionVolume(context, region_cells, old, skin, target, fresh, report, constructed);"
    call_record = "EngineSetup::RecordRegionResult("
    eval_rebuilt = eval_body_moved
    if call_skin not in eval_rebuilt or call_volume not in eval_rebuilt or call_record not in eval_rebuilt:
        raise AssertionError("callback helper call markers missing")
    # Normalize format-only caller declarations and explicit constructed out-parameter mapping.
    expected_callback = callback_raw.replace(
        "const bool constructed=ConstructWallRegion", "constructed=ConstructWallRegion", 1
    ).replace(
        "        // Surface integrals give one partition-independent region kernel.",
        "        bool constructed=false;\n        // Surface integrals give one partition-independent region kernel.",
        1,
    )
    # Inline ACTUAL formatted leaf bodies rather than source fragments.
    eval_rebuilt = eval_rebuilt.replace(call_skin, moved_body(skin_method), 1)
    eval_rebuilt = eval_rebuilt.replace(call_volume, moved_body(volume_method), 1)
    eval_rebuilt, count = re.subn(r"EngineSetup::RecordRegionResult\s*\([^;]*\)\s*;", lambda _: moved_body(record_method), eval_rebuilt, count=1, flags=re.S)
    assert count == 1
    reconstructed_tokens = tokens(eval_rebuilt)
    if reconstructed_tokens != tokens(expected_callback):
        actual_tokens = reconstructed_tokens
        expected_tokens = tokens(expected_callback)
        mismatch = next((i for i, pair in enumerate(zip(actual_tokens, expected_tokens)) if pair[0] != pair[1]), 0)
        raise AssertionError(f"callback token mismatch at {mismatch}: {actual_tokens[mismatch:mismatch+8]} != {expected_tokens[mismatch:mismatch+8]}; lengths {len(actual_tokens)} {len(expected_tokens)}")

    methods = [
        head_method, gather_method, prep_method, run_method, tail_method, publish_method,
        evaluate_method, skin_method, volume_method, record_method,
    ]
    for actual, expected in ((head_method, head_call), (gather_method, gather_call),
                             (prep_method, prep_body_full), (run_method, run_stats_and_call),
                             (tail_method, tail_body), (publish_method, publish_body + "\n    return std::nullopt;")):
        assert tokens(moved_body(actual)) == tokens(expected), "actual generated chain body differs"
    lengths = [m.count("\n") for m in methods]
    if max(lengths) > 80:
        raise AssertionError(f"S13 method exceeds 80 lines: {lengths}")
    if "bool& changed" not in publish_method.split("{", 1)[0]:
        raise AssertionError("publication helper must return the original changed value by reference")
    if "bool& changed" in gather_method.split("{", 1)[0] or "bool& changed" in prep_method.split("{", 1)[0]:
        raise AssertionError("changed must be declared at the original post-report point")
    if "FinalizeRegionTail" not in moved_body(run_method):
        raise AssertionError("service owner must synchronously call the final tail helper")
    tail_moved = moved_body(tail_method)
    tail_after_publish = tail_moved[
        tail_moved.index("region_rebuilt = changed") :
    ].rsplit("return std::nullopt;", 1)[0].strip()
    if "PublishRegionResult" not in tail_moved or tokens(post_body) != tokens(tail_after_publish):
        raise AssertionError("the original post-publication tail must remain inside the service-owned tail")
    finish_returns = sum(
        moved_body(method).count("std::optional<EngineResult>(finish())")
        for method in (head_method, gather_method, prep_method, run_method, tail_method, publish_method)
    )
    if finish_returns != original_span.count("return finish();"):
        raise AssertionError("every original direct finish return must be transported exactly once")

    hpp = (HERE / "EngineSetup_S12.hpp.proposed").read_text()
    decls = (
        "  template <class Finish>\n  static std::optional<EngineResult> ConstructRegion(EngineContext&, uint64_t, CLocalFailure&, Finish&);\n"
        "  template <class Finish>\n  static std::optional<EngineResult> GatherRegion(EngineContext&, uint64_t, size_t, size_t, CLocalFailure&, Finish&);\n"
        "  template <class Rows, class Finish>\n  static std::optional<EngineResult> PrepareRegionField(EngineContext&, uint64_t, size_t, size_t, CLocalFailure&, bool&, Rows&, std::vector<DonorRegion>&, Patch&, Patch&, std::vector<OwnedCell>&, std::vector<BoundaryParameterEdit>&, bool&, Finish&);\n"
        "  template <class Finish>\n  static std::optional<EngineResult> RunAndFinalizeRegion(EngineContext&, RegionFieldService&, RegionReport&, CLocalFailure&, size_t, bool&, Patch&, std::vector<OwnedCell>&, std::vector<BoundaryParameterEdit>&, Finish&);\n"
        "  template <class Finish>\n  static std::optional<EngineResult> FinalizeRegionTail(EngineContext&, size_t, bool&, CLocalFailure&, Patch&, std::vector<OwnedCell>&, std::vector<BoundaryParameterEdit>&, Finish&);\n"
        "  template <class Finish>\n  static std::optional<EngineResult> PublishRegionResult(EngineContext&, size_t, bool&, CLocalFailure&, Patch&, std::vector<OwnedCell>&, std::vector<BoundaryParameterEdit>&, bool&, Finish&);\n"
        "  template <class Rows>\n  static void EvaluateRegionTarget(EngineContext&, size_t, const RegionMetric&, Rows&, Patch&, Patch&, RegionReport&, bool&, std::vector<BoundaryParameterEdit>&, std::vector<OwnedCell>&);\n"
        "  static void PrepareRegionSkin(const PlanarReference&, const Patch&, std::map<Id, Point>&, std::vector<Facet>&);\n"
        "  static void ConstructRegionVolume(EngineContext&, size_t, Patch&, const std::vector<Facet>&, const RegionMetric&, Patch&, RegionReport&, bool&);\n"
        "  template <class Rows>\n  static void RecordRegionResult(EngineContext&, Patch&, Patch&, Rows&, RegionReport&, bool, bool, bool, bool&, std::vector<BoundaryParameterEdit>&, std::vector<OwnedCell>&);\n"
    )
    ctor = "  EngineSetup();\n"
    if hpp.count(ctor) != 1:
        raise AssertionError("S12 header anchor mismatch")
    proposed_hpp = hpp.replace(ctor, decls + ctor, 1)
    impl = (HERE / "EngineSetup_S12.cpp.proposed").read_text()
    proposed_impl = impl + "\n" + "\n".join(methods)

    (HERE / "CNativeEngine3D_S13.cpp.proposed").write_text(candidate)
    (HERE / "EngineSetup_S13.hpp.proposed").write_text(proposed_hpp)
    (HERE / "EngineSetup_S13.cpp.proposed").write_text(proposed_impl)
    proof = {
        "status": "unapplied full S13 proposal on ce252",
        "source_revision": REV,
        "source_sha256": digest(cpp),
        "proposed_main_sha256": digest(candidate),
        "reversed_main_sha256": digest(reversed_cpp),
        "whole_main_byte_exact_reversal": True,
        "recursive_callback_token_reconstruction": True,
        "all_s13_direct_finish_sites_transported": original_span.count("return finish();"),
        "transported_finish_return_count": finish_returns,
        "finish_is_synchronous_through_gather_service_run_and_publication": True,
        "all_helper_formatted_lines": lengths,
        "all_helpers_under_80_lines": True,
        "single_original_service_run_callback_retained": True,
        "single_original_region_service_and_tick_retained": True,
        "service_remains_alive_through_original_in_region_tail": "PrepareRegionField owns service while RunAndFinalizeRegion synchronously calls FinalizeRegionTail",
        "post_publication_tail_reconstructed_inside_service_owner": tokens(post_body) == tokens(tail_after_publish),
        "borrowed_bindings": {
            "context.control.result": "engaged optional<EngineResult> reference",
            "context.ownership.working": "engaged optional<Owned> reference",
            "context.exchange.directory": "engaged optional<unique_ptr<Directory>>; one dereference retains unique_ptr&",
            "context.exchange.source": "engaged optional<CavityMetricSource> reference",
            "context.census.selection/initial/field_identity": "engaged existing references",
            "context.commit.maximum_node/maximum_cell": "engaged existing Id references",
            "context.setup.control_prefix.started": "engaged Clock::time_point reference",
            "context.setup.wall_counters": "engaged counters reference",
            "failure/region_active/region_started": "caller-owned active references",
            "RegionFieldService/RegionReport/rows/patches/replacement/edits": "borrowed references through nested synchronous calls",
            "finish": "existing local finalizer borrowed by template reference",
        },
        "limitations": [
            "No build or runtime validation; unapplied proposal only.",
            "Future static EngineControl::Finish rebind remains a later source-only step.",
            "EngineResult move traits remain unverified.",
        ],
    }
    (HERE / "b8_s13_recursive_reversal_proof.json").write_text(
        json.dumps(proof, indent=2) + "\n"
    )
    repairs = {
        "status": "unapplied proposal setup repairs",
        "source_revision": REV,
        "cause_corrections": [
            {
                "cause": "The initial split passed the publication result through GatherRegion and PrepareRegionField before its original declaration point.",
                "correction": "Changed is now declared inside FinalizeRegionTail at the original post-report point and passed by reference only to PublishRegionResult.",
            },
            {
                "cause": "The first tail extraction left region_rebuilt, final votes, identity reductions, timing, and active/result clearing in Adapt after the service owner unwound.",
                "correction": "The entire original post-publication tail now runs synchronously in FinalizeRegionTail nested under PrepareRegionField's live RegionFieldService.",
            },
            {
                "cause": "Combining report accumulation, publication, and finalization exceeded the 80-line helper limit.",
                "correction": "Split only at the original post-report boundary into RunAndFinalizeRegion and FinalizeRegionTail; the service and caller owners remain live across the synchronous call.",
            },
            {
                "cause": "A shared alias prelude emitted unused borrowed references in every split helper.",
                "correction": "The generator now keeps only aliases referenced by each moved helper body.",
            },
        ],
        "verification": {
            "generator_selfchecks_passed": True,
            "whole_main_byte_exact_reversal": True,
            "recursive_callback_token_reconstruction": True,
            "post_tail_token_reconstruction_inside_service_owner": True,
            "all_11_direct_finish_returns_transported": True,
            "all_helpers_at_most_80_formatted_lines": True,
            "build_or_runtime_validation": False,
        },
    }
    (HERE / "b8_s13_setup_repairs.json").write_text(json.dumps(repairs, indent=2) + "\n")


if __name__ == "__main__":
    main()
