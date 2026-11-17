from pathlib import Path
import hashlib
import json
import re
import subprocess

ROOT = Path.cwd()
PREP = ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/B4_preparation/proposal"
OUT = PREP / "generated"
OUT.mkdir(parents=True, exist_ok=True)
MAIN = ROOT / "Common/src/adaptation/CNativeEngine3D.cpp"
HEADER = ROOT / "Common/src/adaptation/native_engine3d/EngineQueues.hpp"
CPP = ROOT / "Common/src/adaptation/native_engine3d/EngineQueues.cpp"
EXPECTED = {
    "CNativeEngine3D.cpp": "806e13eccc41df3a6e19602b38e1297dc8b9852fd4a6a8804af7a8b67d4d80e5",
    "EngineQueues.hpp": "359d3ef321fa1a6d2722f9ad4fe1667d970f031c5c51c6a1266510b580fc5a55",
    "EngineQueues.cpp": "701f2fe570f59a2d289611f504ca59205cdc14fe98e67855dc50fea301c784e5",
}

def sha(data):
    return hashlib.sha256(data).hexdigest()

def mask_lex(text):
    lex = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
    return lex.sub(lambda m: "".join("\n" if c == "\n" else " " for c in m[0]), text)

def close_brace(text, opening):
    masked = mask_lex(text)
    depth = 0
    for i in range(opening, len(text)):
        if masked[i] == "{": depth += 1
        elif masked[i] == "}":
            depth -= 1
            if depth == 0: return i
    raise AssertionError("unclosed block")

def body_tokens(text):
    text = re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.S)
    return re.findall(r'"(?:\\.|[^"\\])*"|\w+|[^\s]', text)

def lambda_span(text, name):
    match = re.search(r"^  auto " + re.escape(name) + r"=\[&\].*?\{", text, re.M | re.S)
    assert match, name
    begin = match.start()
    opening = match.end() - 1
    finish = close_brace(text, opening)
    assert text[finish:finish + 3] == "};\n", name
    return {"name": name, "start": begin, "end": finish + 3, "old": text[begin:finish + 3],
            "body": text[opening + 1:finish]}

main_bytes, header_bytes, cpp_bytes = MAIN.read_bytes(), HEADER.read_bytes(), CPP.read_bytes()
assert sha(main_bytes) == EXPECTED["CNativeEngine3D.cpp"]
assert sha(header_bytes) == EXPECTED["EngineQueues.hpp"]
assert sha(cpp_bytes) == EXPECTED["EngineQueues.cpp"]
source = main_bytes.decode()
removed = [lambda_span(source, name) for name in ("coalesce", "service_frontier")]
without = source
for item in sorted(removed, key=lambda r: r["start"], reverse=True):
    without = without[:item["start"]] + without[item["end"]:]
assert without.count("coalesce(choices)") == 2
assert without.count("service_frontier(") == 7
without = without.replace("coalesce(choices)", "context.queues.Coalesce(context, choices, curved)")
without = without.replace("service_frontier(", "context.queues.ServiceFrontier(context, ")

# Methods borrow the exact live Adapt objects through EngineContext; `curved` stays explicit because
# it is a local const computed once from the geometry immediately before context construction.
ALIASES = {
    "Coalesce": [
        "  auto& world = context.world;",
        "  auto& options = context.options;",
        "  auto& source = *context.exchange.source;",
        "  auto& directory = *context.exchange.directory;",
        "  auto& result = *context.control.result;",
        "  auto& selection = *context.census.selection;",
        "  auto& repairing = *context.control.repairing;",
        "  auto& coarsening = *context.control.coarsening;",
        "  auto& composition = context.composition;",
        "  auto& bidirectional = context.bidirectional;",
    ],
    "ServiceFrontier": [
        "  auto& world = context.world;",
        "  auto& options = context.options;",
        "  auto& source = *context.exchange.source;",
        "  auto& directory = *context.exchange.directory;",
        "  auto& result = *context.control.result;",
        "  auto& repairing = *context.control.repairing;",
        "  auto& coarsening = *context.control.coarsening;",
        "  auto& checkpoint = *context.control.checkpoint;",
    ],
}

def member(name, signature, aliases, old):
    body = "".join(line + "\n" for line in aliases) + old["body"]
    return signature + " {\n" + body + "}\n\n"

coalesce_body = removed[0]["body"]
coalesce_split = coalesce_body.index("    try {", coalesce_body.index("if(!admitted)"))
coalesce_order_body = coalesce_body[coalesce_split:]
coalesce_prefix = coalesce_body[:coalesce_split]
coalesce_call = "    return CoalesceOrder(context, choices, all, concurrent_refine, failure, feature);\n"
coalesce_prefix += coalesce_call

service_body = removed[1]["body"]
service_validation_start = service_body.index("    for(const auto& receipt:receipts)")
service_loop_start = service_body.index("    for(const auto& receipt:receipts)", service_validation_start + 1)
service_loop_open = service_body.index("{", service_loop_start)
service_loop_end = close_brace(service_body, service_loop_open)
service_tail = service_body[service_loop_end + 1:]
assert "return certify_support || context.exchange.Vote(context, failure);" in service_tail, repr(service_tail)
service_receipt_body = service_body[service_loop_start:]
service_prefix = service_body[:service_loop_start] + "    return ServiceReceipts(context, receipts, certify_support, failure);\n"

ALIASES["CoalesceOrder"] = [
    "  auto& world = context.world;",
    "  auto& stats = context.control.result->stats;",
    "  auto& repairing = *context.control.repairing;",
]
ALIASES["ServiceReceipts"] = [
    "  auto& world = context.world;",
    "  auto& selection = *context.census.selection;",
    "  auto& repairing = *context.control.repairing;",
    "  auto& checkpoint = *context.control.checkpoint;",
]

method_coalesce = "bool EngineQueues::Coalesce(EngineContext& context, std::vector<Choice>& choices, bool curved) {\n" + \
    "\n".join(ALIASES["Coalesce"]) + "\n" + coalesce_prefix + "}\n\n"
method_order = "template <class FeatureFn>\nbool CoalesceOrder(EngineContext& context, std::vector<Choice>& choices, std::vector<ProposalKey>& all, bool concurrent_refine, CLocalFailure& failure, FeatureFn& feature) {\n" + \
    "\n".join(ALIASES["CoalesceOrder"]) + "\n" + coalesce_order_body + "}\n\n"
method_service = "bool EngineQueues::ServiceFrontier(EngineContext& context, Feature feature, bool active, bool accepted, const std::array<RepairReceipt, MaximumCandidateBatch>* extra, size_t count, unsigned disposition) {\n" + \
    "\n".join(ALIASES["ServiceFrontier"]) + "\n" + service_prefix + "}\n\n"
method_receipts = "bool EngineQueues::ServiceReceipts(EngineContext& context, std::vector<RepairReceipt>& receipts, bool certify_support, CLocalFailure& failure) {\n" + \
    "\n".join(ALIASES["ServiceReceipts"]) + "\n" + service_receipt_body + "}\n\n"

def declarations(text):
    insertion = "  void LengthDirection(EngineContext& context, bool collapse);"
    assert text.count(insertion) == 1
    replacement = insertion + "\n  bool Coalesce(EngineContext&, std::vector<Choice>& choices, bool curved);\n  bool ServiceFrontier(EngineContext&, Feature feature, bool active, bool accepted,\n                       const std::array<RepairReceipt, MaximumCandidateBatch>* extra = nullptr, size_t count = 0,\n                       unsigned disposition = 4);\n  bool ServiceReceipts(EngineContext&, std::vector<RepairReceipt>& receipts, bool certify_support,\n                       CLocalFailure& failure);"
    return text.replace(insertion, replacement, 1)

new_header = declarations(header_bytes.decode())
new_header = new_header.replace(new_header.splitlines()[0], "// Chooses and services candidates through canonical queue state; preserves container and protocol order.", 1)
new_header = subprocess.run(["clang-format-18"], input=new_header, text=True, capture_output=True, check=True).stdout.rstrip() + "\n"
# Keep the local function template before EngineQueues::Coalesce, its first
# non-dependent caller. C++ name lookup cannot rely on a later definition.
ordered_methods = method_order + method_coalesce + method_service + method_receipts
base_cpp = cpp_bytes.decode()
base_cpp = base_cpp.replace(base_cpp.splitlines()[0], "// Runs the original coalescing/frontier transactions; owns no additional queue payloads.", 1)
new_cpp = subprocess.run(["clang-format-18"], input=base_cpp + "\n" + ordered_methods, text=True, capture_output=True, check=True).stdout.rstrip() + "\n"

# Strip only the newly introduced alias prefix and compare each generated member body to the actual
# removed lambda body. No statement edits or semantic substitutions are permitted.
def member_body(text, name):
    masked = mask_lex(text)
    target = r"bool CoalesceOrder" if name == "CoalesceOrder" else r"bool EngineQueues::" + name
    match = re.search(target + r"\s*\([^;]*?\)\s*\{", masked, re.S)
    assert match, name
    opening = match.end() - 1
    return text[opening + 1:close_brace(text, opening)]

def strip_alias_prefix(body):
    lines = body.splitlines(keepends=True)
    out, prefix, still_aliases = [], [], True
    for line in lines:
        m = re.match(r"\s*auto\s*&\s*(\w+)\s*=.*;\s*$", line)
        if still_aliases and (not line.strip() or m):
            if m: prefix.append((m.group(1), line.strip()))
            continue
        still_aliases = False
        out.append(line)
    return "".join(out), prefix

proofs, capture_audit = [], {}
capture_candidates = {"world", "options", "source", "directory", "result", "stats", "selection", "repairing",
                      "coarsening", "composition", "bidirectional", "checkpoint"}
params_by_method = {
    "Coalesce": ["context", "choices", "curved"],
    "CoalesceOrder": ["context", "choices", "all", "concurrent_refine", "failure", "feature"],
    "ServiceFrontier": ["context", "feature", "active", "accepted", "extra", "count", "disposition"],
    "ServiceReceipts": ["context", "receipts", "certify_support", "failure"],
}
for name in params_by_method:
    generated_body = member_body(new_cpp, name)
    generated_body, aliases = strip_alias_prefix(generated_body)
    alias_bindings = [line for _, line in aliases]
    assert alias_bindings == [line.strip() for line in ALIASES[name]], ("alias binding mismatch", name,
        alias_bindings, ALIASES[name])
    names = [n for n, _ in aliases]
    assert len(names) == len(set(names)), f"duplicate alias: {name}"
    references = set(body_tokens(re.sub(r"(?:\.|->)\s*\w+", " .", generated_body))) & capture_candidates
    free = sorted(references - set(names) - set(params_by_method[name]))
    assert not free, (name, "unbound capture", free)
    unused = [n for n in names if not re.search(r"\b" + re.escape(n) + r"\b", generated_body)]
    assert not unused, (name, "unused alias", unused, generated_body[:800])
    capture_audit[name] = {"bindings_and_types": alias_bindings, "parameters": params_by_method[name],
                           "translation_unit_constants": ["MaximumCandidateBatch"],
                           "free_capture_names": free, "unused_aliases": unused}

coalesce_actual = member_body(new_cpp, "Coalesce")
coalesce_actual, _ = strip_alias_prefix(coalesce_actual)
order_actual = member_body(new_cpp, "CoalesceOrder")
order_actual, _ = strip_alias_prefix(order_actual)
coalesce_actual, count = re.subn(r"return\s+CoalesceOrder\(context,\s*choices,\s*all,\s*concurrent_refine,\s*failure,\s*feature\);",
                                lambda _: order_actual, coalesce_actual)
assert count == 1
if body_tokens(coalesce_actual) != body_tokens(removed[0]["body"]):
    actual, wanted = body_tokens(coalesce_actual), body_tokens(removed[0]["body"])
    at = next((i for i, pair in enumerate(zip(actual, wanted)) if pair[0] != pair[1]), min(len(actual), len(wanted)))
    raise AssertionError(("coalesce with CoalesceOrder inlined differs", at, actual[at:at + 14], wanted[at:at + 14],
                          len(actual), len(wanted)))
proofs.append({"lambda": "coalesce", "members": ["Coalesce", "CoalesceOrder"],
               "actual_removed_lambda_body_token_equal_after_inlining": True})

service_actual = member_body(new_cpp, "ServiceFrontier")
service_actual, _ = strip_alias_prefix(service_actual)
receipts_actual = member_body(new_cpp, "ServiceReceipts")
receipts_actual, _ = strip_alias_prefix(receipts_actual)
service_actual, count = re.subn(r"return\s+ServiceReceipts\(context,\s*receipts,\s*certify_support,\s*failure\);",
                               lambda _: receipts_actual, service_actual)
assert count == 1
assert body_tokens(service_actual) == body_tokens(removed[1]["body"]), "service_frontier with ServiceReceipts inlined differs from actual lambda"
proofs.append({"lambda": "service_frontier", "members": ["ServiceFrontier", "ServiceReceipts"],
               "actual_removed_lambda_body_token_equal_after_inlining": True})

# Member lines, no more than the plan's 80-line cap.
for method in params_by_method:
    target = r"bool CoalesceOrder" if method == "CoalesceOrder" else r"bool EngineQueues::" + method
    match = re.search(target + r"\s*\([^;]*?\)\s*\{", mask_lex(new_cpp), re.S)
    opening = match.end() - 1
    closing = close_brace(new_cpp, opening)
    begin = new_cpp.rfind("template <class FeatureFn>", 0, match.start()) if method == "CoalesceOrder" else match.start()
    line_count = new_cpp[begin:closing + 1].count("\n") + 1
    capture_audit[method]["formatted_lines"] = line_count
    assert line_count <= 80, (method, line_count)

# Reverse routes and restore lambdas; this must reconstruct the complete pinned main file byte-for-byte.
reversed_main = without.replace("context.queues.Coalesce(context, choices, curved)", "coalesce(choices)")
reversed_main = reversed_main.replace("context.queues.ServiceFrontier(context, ", "service_frontier(")
for row in sorted(removed, key=lambda r: r["start"]):
    reversed_main = reversed_main[:row["start"]] + row["old"] + reversed_main[row["start"]:]
assert reversed_main == source, "main file exact reversal failed"

files = {"CNativeEngine3D.cpp": without, "EngineQueues.hpp": new_header, "EngineQueues.cpp": new_cpp}
for name, text in files.items(): OUT.joinpath(name + ".proposed").write_text(text)
manifest = {
    "unapplied": True,
    "base": EXPECTED,
    "proposed_sha256": {name: sha(text.encode()) for name, text in files.items()},
    "whole_main_file_byte_reversal": True,
    "removed_lambdas": [{"name": row["name"], "start": row["start"], "end": row["end"],
                         "body_token_sha256": sha(" ".join(body_tokens(row["body"])).encode())} for row in removed],
    "call_replacements": {"coalesce": 2, "service_frontier": 7},
    "member_token_proofs": proofs,
    "capture_binding_audit": capture_audit,
    "no_new_fields_or_payload_copies": True,
    "compiled_or_run": False,
    "rebind_required": "Regenerate against the post-B3 accepted source before any B4 edit.",
}
OUT.joinpath("binding_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
