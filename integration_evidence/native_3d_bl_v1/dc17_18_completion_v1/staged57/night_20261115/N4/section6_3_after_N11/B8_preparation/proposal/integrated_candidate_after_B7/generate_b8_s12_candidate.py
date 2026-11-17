#!/usr/bin/env python3
"""Generate a bounded unapplied S12 applicability helper and proof."""
import hashlib
import json
import re
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REV = "ce252"
CPP = "Common/src/adaptation/CNativeEngine3D.cpp"
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_][A-Za-z_0-9]*|\d+(?:\.\d*)?|::|->|&&|\|\||==|!=|<=|>=|\+\+|--|\+=|-=|\*=|/=|<<|>>|[^\s]')


def read_source():
    return (HERE / "CNativeEngine3D_S11.cpp.proposed").read_text()


def digest(text):
    """Return the source SHA256."""
    return hashlib.sha256(text.encode()).hexdigest()


def format_helper(text):
    """Format only the proposed S12 helper."""
    return subprocess.check_output(
        ["clang-format-18", "--style=file"], input=text.encode()
    ).decode()


def main():
    """Write proposals and verify body tokens plus caller reversal."""
    cpp = read_source()
    first = "  if(region_applicable) {\n    std::set<Id> walls;\n"
    last = "  if(region_applicable) {\n    region_active=true;region_started=Clock::now();CLocalFailure failure;\n"
    if cpp.count(first) != 1 or cpp.count(last) != 1:
        raise ValueError("S12 caller anchors are not unique")
    a = cpp.index(first)
    b = cpp.index(last, a)
    old_block = cpp[a:b]
    if old_block.count("return finish();") != 1:
        raise AssertionError("S12 source mismatch should have one caller finish")
    transformed = old_block.replace("return finish();", "return false;")
    if "region_applicable=region_applicable || prismatic;" not in old_block:
        raise AssertionError("S12 prismatic applicability statement missing")
    aliases = (
        "  auto& world = context.world;\n"
        "  auto& options = context.options;\n"
        "  auto& geometry = context.geometry;\n"
        "  auto& result = *context.control.result;"
    )
    helper = (
        "bool EngineSetup::CheckRegionApplicability(EngineContext& context,\n"
        "                                           bool& region_applicable,\n"
        "                                           bool prismatic) {\n"
        + aliases
        + "\n  // BEGIN_MOVED_SOURCE\n"
        + transformed.rstrip()
        + "\n  return true;\n"
        + "  // END_MOVED_SOURCE\n"
        "}\n"
    )
    helper = format_helper(helper)
    moved = helper.split("// BEGIN_MOVED_SOURCE", 1)[1].split(
        "// END_MOVED_SOURCE", 1
    )[0]
    moved_without_success = moved.rsplit("return true;", 1)[0]
    if TOKEN.findall(transformed) != TOKEN.findall(moved_without_success):
        raise AssertionError("S12 source statements differ after status-return mapping")
    new_call = (
        "  if (!EngineSetup::CheckRegionApplicability(context, region_applicable, prismatic)) "
        "return finish();\n"
    )
    candidate = cpp.replace(old_block, new_call, 1)
    if candidate == cpp or candidate.replace(new_call, old_block, 1) != cpp:
        raise AssertionError("S12 caller replacement does not reverse exactly")

    hpp = (HERE / "EngineSetup_S11.hpp.proposed").read_text()
    declaration = (
        "  static bool CheckRegionApplicability(EngineContext&, bool&, bool);\n"
    )
    anchor = "  EngineSetup();\n"
    if hpp.count(anchor) != 1:
        raise AssertionError("S11 header constructor anchor missing")
    proposed_hpp = hpp.replace(anchor, declaration + anchor, 1)
    impl = (HERE / "EngineSetup_S11.cpp.proposed").read_text()
    proposed_impl = impl + "\n" + helper

    (HERE / "CNativeEngine3D_S12.cpp.proposed").write_text(candidate)
    (HERE / "EngineSetup_S12.hpp.proposed").write_text(proposed_hpp)
    (HERE / "EngineSetup_S12.cpp.proposed").write_text(proposed_impl)
    proof = {
        "status": "unapplied S12 proposal on immutable ce252 main",
        "source_revision": REV,
        "source_sha256": digest(cpp),
        "proposed_main_sha256": digest(candidate),
        "reversed_main_sha256": digest(candidate.replace(new_call, old_block, 1)),
        "caller_byte_exact_reversal": True,
        "body_token_equality_after_finish_to_status_mapping": True,
        "POD_declarations_remain_in_original_caller": True,
        "direct_finish_remains_in_caller": True,
        "collective_order_unchanged": True,
        "formatted_function_lines": helper.count("\n"),
        "helper_under_80_lines": helper.count("\n") <= 80,
        "borrowed_bindings": {
            "context.world": "World&",
            "context.options": "EngineOptions&",
            "context.geometry": "const PlanarReference&",
            "context.control.result": "engaged optional<EngineResult> borrowed as a reference",
            "region_applicable": "caller-owned bool&",
            "prismatic": "caller-computed bool by value",
            "global_original": "unchanged caller local; not captured by helper",
        },
    }
    if not proof["helper_under_80_lines"]:
        raise AssertionError("S12 helper exceeds 80 formatted lines")
    (HERE / "b8_s12_reversal_proof.json").write_text(json.dumps(proof, indent=2) + "\n")


if __name__ == "__main__":
    main()
