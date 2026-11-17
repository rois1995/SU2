#!/usr/bin/env python3
"""Generate an unapplied four-method S10 extraction with token proofs."""
import hashlib
import json
import re
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REV = "ce252"
CPP = "Common/src/adaptation/CNativeEngine3D.cpp"
HPP = "Common/src/adaptation/native_engine3d/EngineSetup.hpp"
IMPL = "Common/src/adaptation/native_engine3d/EngineSetup.cpp"
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_][A-Za-z_0-9]*|\d+(?:\.\d*)?|::|->|&&|\|\||==|!=|<=|>=|\+\+|--|\+=|-=|\*=|/=|<<|>>|[^\s]')


def read(path):
    """Read only the immutable git source."""
    return subprocess.check_output(["git", "show", f"{REV}:{path}"]).decode()


def digest(text):
    """Hash source text."""
    return hashlib.sha256(text.encode()).hexdigest()


def span(text, first, last):
    """Return a unique marker-delimited source span."""
    if first not in text or last not in text:
        raise ValueError(f"missing source marker: {first[:50]!r}")
    a = text.index(first)
    return a, text.index(last, a)


def format_helper(text):
    """Format only the new helper body."""
    return subprocess.check_output(
        ["clang-format-18", "--style=file"], input=text.encode()
    ).decode()


def aliases(name):
    """Bind existing active values by reference only."""
    common = (
        "  auto& world = context.world;\n"
        "  auto& options = context.options;\n"
        "  auto& result = *context.control.result;\n"
        "  auto& stats = result.stats;"
    )
    if name == "numeric":
        return common + (
            "\n  auto& composition = context.composition;"
            "\n  auto& accepted = context.accepted;"
            "\n  auto& atlas_geometry = *context.setup.atlas_geometry;"
            "\n  auto& target_geometry_lease = *context.setup.target_geometry_lease;"
        )
    if name == "wall":
        return (
            "  auto& world = context.world;\n"
            "  auto& options = context.options;\n"
            "  auto& composition = context.composition;\n"
            "  auto& intent = context.intent;\n"
            "  auto& bidirectional = context.bidirectional;\n"
            "  auto& surface_target = context.surface_target;"
        )
    if name == "exchange":
        return common
    return common + (
        "\n  auto& accepted = context.accepted;"
        "\n  auto& original = context.original;"
        "\n  auto& geometry = context.geometry;"
        "\n  auto& bidirectional = context.bidirectional;"
        "\n  auto& surface_target = context.surface_target;"
        "\n  auto& coarsening = *context.control.coarsening;"
        "\n  auto& repairing = *context.control.repairing;"
        "\n  auto& accepted_bytes = *context.control.accepted_bytes;"
        "\n  auto& working = *context.ownership.working;"
        "\n  auto& selection = *context.census.selection;"
        "\n  auto& field_identity = *context.census.field_identity;"
        "\n  auto& initial = *context.census.initial;"
        "\n  auto& eligible_extensions = *context.census.eligible_extensions;"
        "\n  auto& directory = *context.exchange.directory;"
        "\n  auto& source = *context.exchange.source;"
    )


def main():
    """Create proposed files and enforce exact source/token reversal."""
    cpp = read(CPP)
    timer = "  {\n    Timer timer(stats.initialization_seconds,options.timings, SU2NativeTiming3D::Operation::Communication);\n"
    if cpp.count(timer) != 1:
        raise ValueError("S10 Timer anchor is not unique")
    start = cpp.index(timer)
    body_start = cpp.index("    CLocalFailure failure;\n", start) + len("    CLocalFailure failure;\n")
    body_end = cpp.index("\n  }\n  if (!ready) return finish();", body_start)
    body = cpp[body_start:body_end]
    specs = [
        ("numeric", "    const auto curved_lo=", "    const auto batch_lo="),
        ("wall", "    const auto batch_lo=", "    ready=context.exchange.Vote(context, failure);"),
        ("exchange", "    if(ready)ready=context.exchange.Admit(context, transfer_memory::Add(transfer_memory::Mul(world.size,sizeof(std::vector<HeightPolicyRow>)),", "    if (ready) ready=context.exchange.Admit(context, transfer_memory::Add(accepted_bytes,64,"),
        ("populate", "    if (ready) ready=context.exchange.Admit(context, transfer_memory::Add(accepted_bytes,64,", "\n  }\n  if (!ready) return finish();"),
    ]
    chunks = {}
    for name, left, right in specs:
        if name == "populate":
            a, b = body.index(left), len(body)
        else:
            a, b = span(body, left, right)
        chunks[name] = body[a:b]
    vote = "    ready=context.exchange.Vote(context, failure);\n"
    if body.count(vote) != 2:
        raise AssertionError("expected the shared validation and exchange votes")
    partition_tokens = (
        TOKEN.findall(chunks["numeric"])
        + TOKEN.findall(chunks["wall"])
        + TOKEN.findall(vote)
        + TOKEN.findall(chunks["exchange"])
        + TOKEN.findall(chunks["populate"])
    )
    if partition_tokens != TOKEN.findall(body):
        raise AssertionError("S10 source ranges do not cover statements in original order")

    method_names = {
        "numeric": "CheckInitialNumericPolicies",
        "wall": "CheckInitialWallPolicies",
        "exchange": "ExchangeInitialWallPolicies",
        "populate": "PopulateInitialStateAndBuildIndexes",
    }
    params = {
        "numeric": "EngineContext& context, bool curved, SU2NativeGeometry::AtlasBudget& atlas_budget, CLocalFailure& failure",
        "wall": "EngineContext& context, CLocalFailure& failure",
        "exchange": "EngineContext& context, CLocalFailure& failure, bool& ready",
        "populate": "EngineContext& context, bool curved, CLocalFailure& failure, bool& ready, Id& maximum_node, Id& maximum_cell",
    }
    methods = {}
    fragment_proofs = []
    for name, _, _ in specs:
        text = (
            f"void EngineSetup::{method_names[name]}({params[name]}) {{\n"
            + aliases(name)
            + "\n  // BEGIN_MOVED_SOURCE\n"
            + chunks[name].rstrip()
            + "\n  // END_MOVED_SOURCE\n}\n"
        )
        text = format_helper(text)
        moved = text.split("// BEGIN_MOVED_SOURCE", 1)[1].split("// END_MOVED_SOURCE", 1)[0].strip()
        old_tokens = TOKEN.findall(chunks[name])
        new_tokens = TOKEN.findall(moved)
        if old_tokens != new_tokens:
            raise AssertionError(f"moved statements differ in {name}")
        methods[name] = text
        fragment_proofs.append({
            "name": method_names[name],
            "old_sha256": digest(chunks[name]),
            "moved_sha256": digest(moved),
            "tokens_equal": True,
            "token_count": len(old_tokens),
            "formatted_lines": text.count("\n"),
        })

    old_caller = cpp[start:body_end] + "\n  }\n"
    new_caller = (
        "  {\n"
        "    Timer timer(stats.initialization_seconds,options.timings, SU2NativeTiming3D::Operation::Communication);\n"
        "    CLocalFailure failure;\n"
        "    EngineSetup::CheckInitialNumericPolicies(context, curved, atlas_budget, failure);\n"
        "    EngineSetup::CheckInitialWallPolicies(context, failure);\n"
        "    ready=context.exchange.Vote(context, failure);\n"
        "    EngineSetup::ExchangeInitialWallPolicies(context, failure, ready);\n"
        "    EngineSetup::PopulateInitialStateAndBuildIndexes(\n"
        "        context, curved, failure, ready, maximum_node, maximum_cell);\n"
        "  }\n"
    )
    if cpp.count(old_caller) != 1:
        raise AssertionError("S10 caller range is not unique")
    candidate = cpp.replace(old_caller, new_caller, 1)
    reversed_cpp = candidate.replace(new_caller, old_caller, 1)
    if reversed_cpp != cpp:
        raise AssertionError("whole Adapt source failed byte-exact reversal")

    hpp = read(HPP)
    declarations = (
        "  static void CheckInitialNumericPolicies(EngineContext&, bool, SU2NativeGeometry::AtlasBudget&, CLocalFailure&);\n"
        "  static void CheckInitialWallPolicies(EngineContext&, CLocalFailure&);\n"
        "  static void ExchangeInitialWallPolicies(EngineContext&, CLocalFailure&, bool&);\n"
        "  static void PopulateInitialStateAndBuildIndexes(EngineContext&, bool, CLocalFailure&, bool&, Id&, Id&);\n"
    )
    anchor = "  EngineSetup();\n"
    if hpp.count(anchor) != 1:
        raise AssertionError("header constructor anchor is not unique")
    proposed_hpp = hpp.replace(anchor, declarations + anchor, 1)
    proposed_impl = read(IMPL) + "\n" + "\n".join(methods.values())

    (HERE / "CNativeEngine3D_S10.cpp.proposed").write_text(candidate)
    (HERE / "EngineSetup_S10.hpp.proposed").write_text(proposed_hpp)
    (HERE / "EngineSetup_S10.cpp.proposed").write_text(proposed_impl)
    proof = {
        "status": "unapplied S10-only extraction",
        "source_revision": REV,
        "source_sha256": digest(cpp),
        "proposed_main_sha256": digest(candidate),
        "reversed_main_sha256": digest(reversed_cpp),
        "whole_main_byte_exact_reversal": True,
        "timer_and_failure_declarations_retained_in_caller": True,
        "final_ready_finish_after_timer_unchanged": "  if (!ready) return finish();" in candidate,
        "all_s10_statements_partitioned_in_original_order": True,
        "functions": fragment_proofs,
    }
    (HERE / "b8_s10_token_reversal_proof.json").write_text(json.dumps(proof, indent=2) + "\n")


if __name__ == "__main__":
    main()
