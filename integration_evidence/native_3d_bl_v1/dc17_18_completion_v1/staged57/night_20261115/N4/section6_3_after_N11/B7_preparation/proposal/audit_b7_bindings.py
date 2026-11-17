from pathlib import Path
import hashlib, json, re, subprocess

ROOT = Path.cwd()
BASE = ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/B7_preparation/proposal"
SRC = ROOT / "Common/src/adaptation/CNativeEngine3D.cpp"
EXPECTED = "806e13eccc41df3a6e19602b38e1297dc8b9852fd4a6a8804af7a8b67d4d80e5"
BASE_COMMIT = "ce25219f04cb01285bfeb8b33dba1e6a3496782a"
source_bytes = subprocess.check_output(["git", "show", f"{BASE_COMMIT}:Common/src/adaptation/CNativeEngine3D.cpp"], cwd=ROOT)
source_sha = hashlib.sha256(source_bytes).hexdigest()
assert source_sha == EXPECTED, f"candidate B3 source changed: {source_sha}"
s = source_bytes.decode()
lex = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
masked = lex.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), s)

def close_brace(opening):
    depth = 0
    for i in range(opening, len(s)):
        if masked[i] == '{': depth += 1
        elif masked[i] == '}':
            depth -= 1
            if depth == 0: return i
    raise AssertionError("unbalanced source")

# Exact local-to-canonical bindings used by the six lambda bodies.
BIND = {
    "world": ("auto&", "context.world"),
    "options": ("auto&", "context.options"),
    "started": ("const auto&", "*context.setup.control_prefix.started"),
    "phase": ("auto&", "*context.setup.control_prefix.phase"),
    "selection": ("auto&", "*context.census.selection"),
    "working": ("auto&", "*context.ownership.working"),
    "result": ("auto&", "*context.control.result"),
    "stats": ("auto&", "context.control.result->stats"),
    "directory": ("auto&", "*context.exchange.directory"),
    "source": ("auto&", "*context.exchange.source"),
    "checkpoint": ("auto&", "*context.control.checkpoint"),
    "repairing": ("auto&", "*context.control.repairing"),
    "coarsening": ("const auto&", "*context.control.coarsening"),
    "bidirectional": ("bool&", "context.bidirectional"),
    "surface_target": ("auto&", "context.surface_target"),
    "geometry": ("const auto&", "context.geometry"),
    "initial": ("auto&", "*context.census.initial"),
    "seed_slot": ("auto&", "*context.census.seed_slot"),
    "construction_claims": ("auto&", "*context.exchange.construction_claims"),
    "retired_edges": ("auto&", "*context.census.retired_edges"),
    "campaign_edges": ("auto&", "*context.census.campaign_edges"),
    "inactive_blocked_edges": ("auto&", "*context.queues.inactive_blocked_edges"),
    "inactive_blocked_links": ("auto&", "*context.queues.inactive_blocked_links"),
    "length_blocks_dirty": ("auto&", "*context.queues.length_blocks_dirty"),
    "private_parameters": ("auto&", "*context.setup.private_parameters"),
    "field_identity": ("auto&", "*context.census.field_identity"),
    "construction_changed": ("auto&", "*context.setup.control_prefix.construction_changed"),
    "improvement_changed": ("auto&", "*context.setup.control_prefix.improvement_changed"),
    "construction_handoff_progress": ("auto&", "*context.setup.control_prefix.construction_handoff_progress"),
    "construction_generation_changed": ("auto&", "*context.setup.control_prefix.construction_generation_changed"),
    "phase_start_round": ("auto&", "*context.setup.control_prefix.phase_start_round"),
    "metric_cleanup": ("auto&", "*context.setup.control_prefix.metric_cleanup"),
    "construction_entry_reported": ("auto&", "*context.control.construction_entry_reported"),
    "generation_window_count": ("auto&", "*context.control.generation_window_count"),
    "progress_at_window": ("auto&", "*context.control.progress_at_window"),
    "attempted_at_window": ("auto&", "*context.control.attempted_at_window"),
    "deferred_at_window": ("auto&", "*context.control.deferred_at_window"),
}
LAMBDAS = ["begin_length_generation", "surface_ready", "enter_phase", "discard_checkpoint", "capture_checkpoint", "rollback_checkpoint"]
PARAMS = {
    "begin_length_generation": ["bool collapse"],
    "surface_ready": [],
    "enter_phase": ["EnginePhase next", "bool refine_geometry", "Observer& observe_progress"],
    "discard_checkpoint": [],
    "capture_checkpoint": [],
    "rollback_checkpoint": ["Observer& observe_progress"],
}

def get_lambda(name):
    m = re.search(r"auto\s+" + re.escape(name) + r"\s*=\s*\[&\][^\n]*?\{", masked)
    if not m: raise AssertionError(name)
    opening = m.end() - 1
    end = close_brace(opening)
    return m.start(), end, opening, s[opening+1:end]

out = {"base": {"path": str(SRC), "commit": BASE_COMMIT, "sha256": source_sha,
                  "source_check_sha256": "3fff8559d3f67b42c1314f2388fb8ba9d510fa064e625f53ef99df0d7f17f8a1",
                  "plan_sha256": "c48ae8a9c59ce693a810248806e6fe6ffa76c3400924f157d6081f5f5e16492b",
                  "part1_sha256": "034275df32e2f4b7f4ef9bb2aef0bc16453f6d0249971b90fc42ba6a33b663cd"},
       "lambda_audit": []}
for name in LAMBDAS:
    start, end, opening, body = get_lambda(name)
    caps = sorted(set(re.findall(r"\b[A-Za-z_]\w*\b", lex.sub("", body))) & set(BIND))
    # Local declarations and nested call names are explicitly separated from captures.
    declared = set(re.findall(r"\b(?:const\\s+)?(?:auto|bool|size_t|uint64_t|unsigned|std::size_t|std::string)\\s+(?:&\s*)?([A-Za-z_]\w*)", body))
    captured = [x for x in caps if x not in declared]
    alias_lines = []
    for x in caps:
        typ, expr = BIND[x]
        alias_lines.append(f"  {typ} {x} = {expr};")
    if name in ("enter_phase", "rollback_checkpoint"):
        alias_lines.insert(0, "  using Phase = EnginePhase;")
    if name == "enter_phase":
        signature = "template<class Observer>\nbool EngineControl::EnterPhase(EngineContext& context, EnginePhase next, bool refine_geometry, Observer& observe_progress) {\n"
    elif name == "rollback_checkpoint":
        signature = "template<class Observer>\nbool EngineControl::RollbackCheckpoint(EngineContext& context, Observer& observe_progress) {\n"
    else:
        signature = "bool EngineControl::" + ''.join(x.title() for x in name.split('_')) + "(EngineContext& context" + (", " + ", ".join(PARAMS[name]) if PARAMS[name] else "") + ") {\n"
    code = signature + "\n".join(alias_lines) + "\n" + body + "\n}"
    fmt = subprocess.run(["clang-format-18"], input=code, text=True, capture_output=True, check=True).stdout
    out["lambda_audit"].append({"name": name,
        "source_lines": [s.count("\n", 0, start)+1, s.count("\n", 0, end)+1],
        "raw_body_lines": body.count("\n")+1,
        "formatted_member_lines_including_required_aliases": fmt.count("\n")+1,
        "free_outer_identifiers": captured,
        "aliases_included_for_formatted_size": caps,
        "canonical_context_bindings": {x: {"declaration": BIND[x][0], "expression": BIND[x][1]} for x in caps},
        "signature_parameters": PARAMS[name],
        "formatted_preview_sha256": hashlib.sha256(fmt.encode()).hexdigest()})

# Split the large enter_phase lambda at its two existing high-level branches.
start, end, opening, body = get_lambda("enter_phase")
branch_start = body.index("    if(surface_target && next==Phase::Improvement) {")
# Find matching branch brace in masked lambda body.
body_mask = lex.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), body)
branch_open = body_mask.index("{", branch_start)
d=0
for i in range(branch_open, len(body)):
    if body_mask[i]=="{": d+=1
    elif body_mask[i]=="}":
        d-=1
        if d==0:
            branch_end=i+1; break
branch=body[branch_start:branch_end]
rest=body[:branch_start]+"    if (surface_target && next == Phase::Improvement)\n      return EnterSurfaceRepair(context, next, observe_progress);\n"+body[branch_end:]
tail_start_original=body.index("    CLocalFailure failure;", body.index("if(refine_geometry && !begin_length_generation(false))"))
tail_start=tail_start_original + len("    if (surface_target && next == Phase::Improvement)\n      return EnterSurfaceRepair(context, observe_progress);\n") - (branch_end-branch_start)
tail=rest[tail_start:]
rest=rest[:tail_start]+"    return CompletePhaseEntry(context, next, refine_geometry, observe_progress);\n"
# Move the existing bidirectional phase-direction branch as one leaf.
bidir_start=rest.index("    if(bidirectional) {")
rest_mask=lex.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), rest)
bidir_open=rest_mask.index("{",bidir_start); depth=0
for i in range(bidir_open,len(rest)):
    if rest_mask[i]=="{": depth+=1
    elif rest_mask[i]=="}":
        depth-=1
        if depth==0: bidir_end=i+1; break
bidir=rest[bidir_start:bidir_end]
rest=rest[:bidir_start]+"    PreparePhaseDirection(context, next, refine_geometry);\n"+rest[bidir_end:]

def fake_count(name, fragment):
    caps=sorted(set(re.findall(r"\b[A-Za-z_]\w*\b",lex.sub("",fragment))) & set(BIND))
    aliases=[f"  {BIND[x][0]} {x} = {BIND[x][1]};" for x in caps]
    if name == "EnterSurfaceRepair":
        signature="template<class Observer>\nbool EngineControl::EnterSurfaceRepair(EngineContext& context, EnginePhase next, Observer& observe_progress) {\n"
    elif name == "PreparePhaseDirection":
        signature="void EngineControl::PreparePhaseDirection(EngineContext& context, EnginePhase next, bool refine_geometry) {\n"
    else:
        signature="template<class Observer>\nbool EngineControl::"+name+"(EngineContext& context, EnginePhase next, bool refine_geometry, Observer& observe_progress) {\n"
    aliases.insert(0, "  using Phase = EnginePhase;")
    f=subprocess.run(["clang-format-18"],input=signature+"\n".join(aliases)+"\n"+fragment+"\n}",text=True,capture_output=True,check=True).stdout
    return f.count("\n")+1, caps
branch_count, branch_caps=fake_count("EnterSurfaceRepair",branch)
tail_count, tail_caps=fake_count("CompletePhaseEntry",tail)
bidir_count, bidir_caps=fake_count("PreparePhaseDirection",bidir)
rest_count, rest_caps=fake_count("EnterPhase",rest)
out["enter_phase_split"]={"proposed_boundary":"extract the initial `surface_target && next == Phase::Improvement` branch through its return; replace it with a direct call to `EnterSurfaceRepair(context, observe_progress)`",
  "branch_original_lines":[s.count("\n",0,opening+1+branch_start)+1,s.count("\n",0,opening+1+branch_end)+1],
  "surface_repair_helper_formatted_lines_with_aliases":branch_count,
  "normal_entry_tail_original_lines":[s.count("\n",0,opening+1+tail_start_original)+1,s.count("\n",0,end)+1],
  "normal_entry_tail_formatted_lines_with_aliases":tail_count,
  "bidirectional_setup_lines":[s.count("\n",0,opening+1+body.index("    if(bidirectional) {"))+1,s.count("\n",0,opening+1+body.index("    if(bidirectional) {")+len(bidir))+1],
  "bidirectional_helper_formatted_lines_with_aliases":bidir_count,
  "remaining_enter_phase_formatted_lines_with_aliases":rest_count,
  "helper_free_outer_identifiers":branch_caps,"tail_free_outer_identifiers":tail_caps,"bidirectional_free_outer_identifiers":bidir_caps,"remaining_free_outer_identifiers":rest_caps,
  "caveat":"line counts are measured on a mechanical preview using alias references; generated helper body proof is not part of this read-only audit"}

json_path=BASE/"binding_audit.json"
json_path.write_text(json.dumps(out,indent=2)+"\n")
lines=["# B7 frozen B3 binding audit", "", f"Source commit: `{BASE_COMMIT}`; SHA256: `{source_sha}`.",
       "Controlling source check SHA256: `3fff8559d3f67b42c1314f2388fb8ba9d510fa064e625f53ef99df0d7f17f8a1`.",
       "Plan/PART1 SHA256 values are recorded in `binding_audit.json`. This is read-only; no production or execution files were changed.", "",
       "| Lambda | Current lines | Raw body | Formatted member preview + live aliases | Free outer identifiers |", "|---|---:|---:|---:|---|"]
for row in out["lambda_audit"]:
    lines.append(f"| `{row['name']}` | {row['source_lines'][0]}–{row['source_lines'][1]} | {row['raw_body_lines']} | {row['formatted_member_lines_including_required_aliases']} | {', '.join('`'+x+'`' for x in row['free_outer_identifiers'])} |")
split=out["enter_phase_split"]
lines += ["", "## Canonical bindings", ""]
for row in out["lambda_audit"]:
    lines.append(f"### `{row['name']}`")
    lines.append("")
    for name, binding in row["canonical_context_bindings"].items():
        lines.append(f"- `{name}` ({binding['declaration']}) → `{binding['expression']}`")
    if row["signature_parameters"]: lines.append("- Explicit parameters: " + ", ".join(f"`{x}`" for x in row["signature_parameters"]))
    lines.append("")
lines += ["## Size and split", "", f"`enter_phase` is the oversized body. Extract its early wall-repair branch (source lines {split['branch_original_lines'][0]}–{split['branch_original_lines'][1]}), bidirectional direction setup (lines {split['bidirectional_setup_lines'][0]}–{split['bidirectional_setup_lines'][1]}), and normal-entry tail (lines {split['normal_entry_tail_original_lines'][0]}–{split['normal_entry_tail_original_lines'][1]}). Formatted previews are {split['surface_repair_helper_formatted_lines_with_aliases']} lines for `EnterSurfaceRepair`, {split['bidirectional_helper_formatted_lines_with_aliases']} for `PreparePhaseDirection`, {split['normal_entry_tail_formatted_lines_with_aliases']} for `CompletePhaseEntry`, and {split['remaining_enter_phase_formatted_lines_with_aliases']} for `EnterPhase`; each includes current capture aliases.",
          "", "These are existing ordered blocks. The wall-repair branch remains before the normal transition; direction setup stays before construction-entry reporting and state transitions; the final measurement/priority/ownership/observer tail stays after admission and optional length-generation setup. `begin_length_generation`, `surface_ready`, `discard_checkpoint`, `capture_checkpoint`, and `rollback_checkpoint` fit under 80 lines with their current required aliases in the preview. Rebind if B5 changes `observe_progress`; keep the observer synchronous and passed by reference.",
          "", "The method previews measure formatting and borrowed aliases only. They are not generated C++ bodies and make no token-equivalence claim. Exact line counts and canonical bindings are in `binding_audit.json`.", ""]
(BASE/"binding_inventory.md").write_text("\n".join(lines))
print(json_path)
print(BASE/"binding_inventory.md")
