#!/usr/bin/env python3
"""Generate an unapplied S11 synchronous-finalizer extraction and proof."""
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
    if path == CPP: return (HERE / "CNativeEngine3D_S10.cpp.proposed").read_text()
    raise ValueError(path)


def digest(text):
    """Return source SHA256."""
    return hashlib.sha256(text.encode()).hexdigest()


def format_helper(text):
    """Format only the new method."""
    return subprocess.check_output(
        ["clang-format-18", "--style=file"], input=text.encode()
    ).decode()


def main():
    """Write source proposals and enforce caller/body reversal."""
    cpp = read(CPP)
    first = "  // Freeze donors first; normalization changes working associations, not the source interpretation.\n"
    last = "  // A complete visible radial region is constructed as one transaction, never as a larger local cavity.\n"
    if cpp.count(first) != 1 or cpp.count(last) != 1:
        raise ValueError("S11 block anchors are not unique")
    a = cpp.index(first)
    b = cpp.index(last, a)
    old_body = cpp[a:b]
    if old_body.count("return finish();") != 7:
        raise AssertionError("S11 direct-finalizer count differs from source inventory")
    expected_body = old_body.replace(
        "return finish();",
        "return std::optional<EngineResult>(finish());",
    )
    if expected_body.count("std::optional<EngineResult>(finish())") != 7:
        raise AssertionError("not all S11 finalizer branches were adapted")
    aliases = (
        "  auto& world = context.world;\n"
        "  auto& options = context.options;\n"
        "  auto& geometry = context.geometry;\n"
        "  auto& result = *context.control.result;\n"
        "  auto& stats = result.stats;\n"
        "  auto& working = *context.ownership.working;\n"
        "  auto& source = *context.exchange.source;\n"
        "  auto& directory = *context.exchange.directory;\n"
        "  auto& atlas_geometry = *context.setup.atlas_geometry;\n"
        "  const auto& atlas_work_initial = *context.setup.control_prefix.atlas_work_initial;"
    )
    tail_start = old_body.index("      std::set<Face> affected;")
    tail_end = old_body.index("\n    }\n  }\n", tail_start)
    tail_original = old_body[tail_start:tail_end]
    tail_expected = tail_original.replace(
        "return finish();",
        "return std::optional<EngineResult>(finish());",
    )
    tail_call = (
        "      // BEGIN_TAIL_CALL\n"
        "      auto done = EngineSetup::PublishRefreshedSurface(\n"
        "          context, atlas_budget, refreshed, failure, before_work, finish);\n"
        "      if (done) return done;\n"
        "      // END_TAIL_CALL"
    )
    main_expected = expected_body.replace(tail_expected, tail_call, 1)
    helper = (
        "template <class Finish>\n"
        "std::optional<EngineResult> EngineSetup::RefreshWorkingSurface(\n"
        "    EngineContext& context, SU2NativeGeometry::AtlasBudget& atlas_budget, Finish& finish) {\n"
        + aliases
        + "\n  // BEGIN_MOVED_SOURCE\n"
        + main_expected.rstrip()
        + "\n  // END_MOVED_SOURCE\n"
        "  return std::nullopt;\n"
        "}\n"
    )
    helper = format_helper(helper)
    moved = helper.split("// BEGIN_MOVED_SOURCE", 1)[1].split(
        "// END_MOVED_SOURCE", 1
    )[0]
    before_tail, tail_remainder = moved.split("// BEGIN_TAIL_CALL", 1)
    _, after_tail = tail_remainder.split("// END_TAIL_CALL", 1)
    reconstructed = before_tail + tail_expected + after_tail
    if TOKEN.findall(expected_body) != TOKEN.findall(reconstructed):
        raise AssertionError("S11 main and tail token reconstruction differs")
    tail_aliases = (
        "  auto& world = context.world;\n"
        "  auto& options = context.options;\n"
        "  auto& result = *context.control.result;\n"
        "  auto& working = *context.ownership.working;\n"
        "  auto& source = *context.exchange.source;\n"
        "  auto& directory = *context.exchange.directory;"
    )
    tail_helper = format_helper(
        "template <class Finish>\n"
        "std::optional<EngineResult> EngineSetup::PublishRefreshedSurface(\n"
        "    EngineContext& context, SU2NativeGeometry::AtlasBudget& atlas_budget,\n"
        "    SU2NativeGeometry::BoundaryCoverage& refreshed, CLocalFailure& failure,\n"
        "    size_t before_work, Finish& finish) {\n"
        + tail_aliases
        + "\n  // BEGIN_MOVED_SOURCE\n"
        + tail_expected.rstrip()
        + "\n  // END_MOVED_SOURCE\n  return std::nullopt;\n}\n"
    )
    tail_moved = tail_helper.split("// BEGIN_MOVED_SOURCE", 1)[1].split(
        "// END_MOVED_SOURCE", 1
    )[0]
    if TOKEN.findall(tail_expected) != TOKEN.findall(tail_moved):
        raise AssertionError("S11 tail tokens differ after explicit finish transport")
    old_caller = old_body
    new_caller = (
        "  if (auto done = EngineSetup::RefreshWorkingSurface(context, atlas_budget, finish)) {\n"
        "    return std::move(*done);\n"
        "  }\n"
    )
    proposed_cpp = cpp.replace(old_caller, new_caller, 1)
    if proposed_cpp == cpp or proposed_cpp.replace(new_caller, old_caller, 1) != cpp:
        raise AssertionError("S11 caller does not reverse byte-exactly")

    hpp = (HERE / "EngineSetup_S10.hpp.proposed").read_text()
    declaration = (
        "  template <class Finish>\n"
        "  static std::optional<EngineResult> RefreshWorkingSurface(\n"
        "      EngineContext&, SU2NativeGeometry::AtlasBudget&, Finish&);\n"
        "  template <class Finish>\n"
        "  static std::optional<EngineResult> PublishRefreshedSurface(\n"
        "      EngineContext&, SU2NativeGeometry::AtlasBudget&,\n"
        "      SU2NativeGeometry::BoundaryCoverage&, CLocalFailure&, size_t, Finish&);\n"
    )
    anchor = "  EngineSetup();\n"
    if hpp.count(anchor) != 1:
        raise AssertionError("S10 header proposal lacks unique constructor anchor")
    proposed_hpp = hpp.replace(anchor, declaration + anchor, 1)
    impl = (HERE / "EngineSetup_S10.cpp.proposed").read_text()
    proposed_impl = impl + "\n" + helper + "\n" + tail_helper

    (HERE / "CNativeEngine3D_S11.cpp.proposed").write_text(proposed_cpp)
    (HERE / "EngineSetup_S11.hpp.proposed").write_text(proposed_hpp)
    (HERE / "EngineSetup_S11.cpp.proposed").write_text(proposed_impl)
    proof = {
        "status": "unapplied S11 proposal on immutable ce252 main",
        "source_revision": REV,
        "source_sha256": digest(cpp),
        "proposed_main_sha256": digest(proposed_cpp),
        "reversed_main_sha256": digest(proposed_cpp.replace(new_caller, old_caller, 1)),
        "caller_byte_exact_reversal": True,
        "body_token_equality_after_explicit_finish_transport": True,
        "tail_function_token_equal": True,
        "direct_finish_sites_preserved_in_helper": 7,
        "synchronous_finish_before_helper_local_unwind": True,
        "caller_uses_nonconst_auto_and_move": True,
        "formatted_function_lines": [helper.count("\n"), tail_helper.count("\n")],
        "helper_under_80_lines": max(helper.count("\n"), tail_helper.count("\n")) <= 80,
        "external_borrowed_bindings": [
            "context.world",
            "context.options",
            "context.geometry",
            "context.control.result/stats",
            "context.ownership.working",
            "context.exchange.source/directory",
            "context.setup.atlas_geometry",
            "context.setup.control_prefix.atlas_work_initial",
            "atlas_budget argument",
            "existing finish closure by reference",
        ],
        "limitations": [
            "No build or runtime validation; proposal is unapplied.",
            "Future static EngineControl::Finish binding remains a later read-only rebind.",
            "Nothrow move trait for EngineResult remains unverified.",
        ],
    }
    if not proof["helper_under_80_lines"]:
        raise AssertionError("S11 methods exceed 80 formatted lines")
    (HERE / "b8_s11_reversal_proof.json").write_text(json.dumps(proof, indent=2) + "\n")


if __name__ == "__main__":
    main()
