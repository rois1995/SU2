from pathlib import Path
import hashlib, json, re, subprocess

ROOT = Path.cwd()
PREP = ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/B7_preparation/proposal"
OUT = PREP / "generated"
OUT.mkdir(parents=True, exist_ok=True)
MAIN = ROOT / "Common/src/adaptation/CNativeEngine3D.cpp"
HEADER = ROOT / "Common/src/adaptation/native_engine3d/EngineControl.hpp"
CPP = ROOT / "Common/src/adaptation/native_engine3d/EngineControl.cpp"
EXPECTED = {
    "main": "806e13eccc41df3a6e19602b38e1297dc8b9852fd4a6a8804af7a8b67d4d80e5",
    "header": "4d9ba9cacce2c4103745e9711211091bcb9ae56642f67ddb5ebd27ac172fbd7c",
    "cpp": "8a49a7de40d846ad4e7f6164ed2c8b1d345e3939b2aa972408eb9db87a3a4b11",
}
BASE_COMMIT = "ce25219f04cb01285bfeb8b33dba1e6a3496782a"
def git_bytes(path):
    return subprocess.check_output(["git", "show", f"{BASE_COMMIT}:{path}"], cwd=ROOT)
def sha(data): return hashlib.sha256(data).hexdigest()
main_b = git_bytes("Common/src/adaptation/CNativeEngine3D.cpp")
header_b = git_bytes("Common/src/adaptation/native_engine3d/EngineControl.hpp")
cpp_b = git_bytes("Common/src/adaptation/native_engine3d/EngineControl.cpp")
assert sha(main_b) == EXPECTED["main"], "Current candidate B3 main changed; rebind this unapplied proposal."
assert sha(header_b) == EXPECTED["header"], "EngineControl header changed; rebind this unapplied proposal."
assert sha(cpp_b) == EXPECTED["cpp"], "EngineControl implementation changed; rebind this unapplied proposal."
s = main_b.decode(); header = header_b.decode(); cpp = cpp_b.decode()
lex = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
def mask(text): return lex.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)
masked = mask(s)
def close_brace(text, masked_text, opening):
    depth = 0
    for i in range(opening, len(text)):
        if masked_text[i] == '{': depth += 1
        elif masked_text[i] == '}':
            depth -= 1
            if depth == 0: return i
    raise AssertionError("unclosed brace")
def lambda_span(name):
    m = re.search(r"auto\s+" + re.escape(name) + r"\s*=\s*\[&\][^\n]*?\{", masked)
    assert m, name
    opening = m.end()-1; end = close_brace(s, masked, opening)
    assert s[end:end+3] == "};\n", (name, repr(s[end:end+10]))
    return {"name":name, "start":m.start(), "end":end+3, "opening":opening,
            "old":s[m.start():end+3], "body":s[opening+1:end]}
NAMES = ["begin_length_generation", "surface_ready", "enter_phase", "discard_checkpoint", "capture_checkpoint", "rollback_checkpoint"]
removed = [lambda_span(n) for n in NAMES]

BIND = {
    "world": ("auto&", "context.world"), "options": ("auto&", "context.options"),
    "started": ("const auto&", "*context.setup.control_prefix.started"),
    "phase": ("auto&", "*context.setup.control_prefix.phase"),
    "selection": ("auto&", "*context.census.selection"), "working": ("auto&", "*context.ownership.working"),
    "result": ("auto&", "*context.control.result"), "stats": ("auto&", "context.control.result->stats"),
    "directory": ("auto&", "*context.exchange.directory"), "source": ("auto&", "*context.exchange.source"),
    "checkpoint": ("auto&", "*context.control.checkpoint"), "repairing": ("auto&", "*context.control.repairing"),
    "coarsening": ("const auto&", "*context.control.coarsening"), "bidirectional": ("bool&", "context.bidirectional"),
    "surface_target": ("auto&", "context.surface_target"), "geometry": ("const auto&", "context.geometry"),
    "composition": ("auto&", "context.composition"),
    "initial": ("auto&", "*context.census.initial"), "seed_slot": ("auto&", "*context.census.seed_slot"),
    "construction_claims": ("auto&", "*context.exchange.construction_claims"),
    "retired_edges": ("auto&", "*context.census.retired_edges"), "campaign_edges": ("auto&", "*context.census.campaign_edges"),
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

def caps(body): return sorted(set(re.findall(r"\b[A-Za-z_]\w*\b", lex.sub("", body))) & set(BIND))
def aliases(body, type_phase=False):
    rows=[f"  {BIND[x][0]} {x} = {BIND[x][1]};" for x in caps(body)]
    if type_phase: rows.insert(0,"  using Phase = EnginePhase;")
    return "\n".join(rows)
def tok(text): return re.findall(r'"(?:\\.|[^"\\])*"|\w+|[^\s]', re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.S))
def split_braced(body, marker):
    start=body.index(marker); bm=mask(body); opening=bm.index("{",start)
    end=close_brace(body,bm,opening)+1
    return start,end,body[start:end]

def replace_call(text, old, new):
    # Replace the exact spelling with balanced arguments, ignoring comments/strings.
    mm=mask(text); pattern=re.compile(r"\b"+re.escape(old)+r"\s*\(")
    matches=list(pattern.finditer(mm)); out=text; pairs=[]
    for m in reversed(matches):
        op=mm.find("(",m.start()); cl=close_paren(mm,op)
        args=text[op+1:cl]
        old_call=text[m.start():cl+1]; new_call=new(args)
        pairs.append((old_call,new_call))
        out=out[:m.start()]+new_call+out[cl+1:]
    return out, len(matches), pairs
def close_paren(mm, opening):
    d=0
    for i in range(opening,len(mm)):
        if mm[i]=='(': d+=1
        elif mm[i]==')':
            d-=1
            if not d:return i
    raise AssertionError("unclosed call")
def split_args(args):
    mm=mask(args); d=0; out=[]; start=0
    for i,c in enumerate(mm):
        if c in "([{": d+=1
        elif c in ")]}": d-=1
        elif c=="," and d==0:
            out.append(args[start:i].strip()); start=i+1
    tail=args[start:].strip()
    if tail: out.append(tail)
    return out

# Produce the three existing logical leaves of enter_phase, keeping statement text intact.
enter_old=next(x for x in removed if x["name"]=="enter_phase")["body"]
branch_a,branch_b,branch=split_braced(enter_old,"    if(surface_target && next==Phase::Improvement) {")
# The existing normal-entry tail starts after optional length-generation setup.
tail_start=enter_old.index("    CLocalFailure failure;", enter_old.index("if(refine_geometry && !begin_length_generation(false))"))
tail=enter_old[tail_start:]
pre=enter_old[:tail_start]
pre=pre.replace("begin_length_generation(false)", "BeginLengthGeneration(context, false)")
# Direction block is inside pre; move just that existing block.
dir_a,dir_b,direction=split_braced(pre,"    if(bidirectional) {")
main_enter=pre[:dir_a]+"    PreparePhaseDirection(context, next, refine_geometry);\n"+pre[dir_b:]+"    return CompletePhaseEntry(context, next, refine_geometry, curved, observe_progress);\n"
main_enter=main_enter[:branch_a]+"    if (surface_target && next == Phase::Improvement) {\n      return EnterSurfaceRepair(context, observe_progress);\n    }\n"+main_enter[branch_b:]
# Keep current condition's exact body for helper; only nested lambda name changes to member.
branch_mask=mask(branch); branch_open=branch_mask.index("{")
branch=branch[branch_open+1:-1]
branch=branch.replace("surface_ready()", "SurfaceReady(context)")
tail=tail.replace("begin_length_generation(false)", "BeginLengthGeneration(context, false)")
tail=tail.replace("phase_ownership(true)", "context.ownership.PhaseOwnership(context, true, curved)")
# Rewire the methods called from moved bodies.
rollback_old=next(x for x in removed if x["name"]=="rollback_checkpoint")["body"]
rollback=rollback_old.replace("discard_checkpoint()", "DiscardCheckpoint(context)")
rollback=rollback.replace("enter_phase(Phase::Improvement)", "EnterPhase(context, Phase::Improvement, false, curved, observe_progress)")

# Rewrite Adapt's calls only after removing all six lambda definitions.
main_without=s
for row in sorted(removed,key=lambda x:x["start"],reverse=True):
    main_without=main_without[:row["start"]]+main_without[row["end"]:]
calls=[]; call_pairs=[]
def routed(name, fn):
    global main_without
    main_without,count,pairs=replace_call(main_without,name,fn)
    calls.append({"old_name":name,"count":count})
    call_pairs.extend(pairs)
routed("begin_length_generation",lambda a:"context.control.BeginLengthGeneration(context, "+a.strip()+")")
def enter_route(a):
    args=split_args(a)
    if len(args)==1: args.append("false")
    assert len(args)==2,args
    return "context.control.EnterPhase(context, "+args[0]+", "+args[1]+", curved, observe_progress)"
routed("enter_phase",enter_route)
routed("capture_checkpoint",lambda a:"context.control.CaptureCheckpoint(context)")
routed("rollback_checkpoint",lambda a:"context.control.RollbackCheckpoint(context, curved, observe_progress)")
routed("discard_checkpoint",lambda a:"context.control.DiscardCheckpoint(context)")

# Undo call routing to prove the changed main file reverses exactly to the pinned source.
reversed_main=main_without
for old_call,new_call in reversed(call_pairs):
    assert reversed_main.count(new_call)>=1,("routed call missing",new_call)
    reversed_main=reversed_main.replace(new_call,old_call,1)
for row in sorted(removed,key=lambda x:x["start"]):
    reversed_main=reversed_main[:row["start"]]+row["old"]+reversed_main[row["start"]:]
if reversed_main != s:
    mismatch=next((i for i,(a,b) in enumerate(zip(reversed_main,s)) if a!=b),min(len(reversed_main),len(s)))
    raise AssertionError(("main call/lambda reversal mismatch",mismatch,repr(reversed_main[mismatch-120:mismatch+220]),repr(s[mismatch-120:mismatch+220]),len(reversed_main),len(s)))

# Member definitions. Lambdas with an existing synchronous observer use a template reference;
# it is not stored or copied. B5 can rebind these calls to ObserveProgress(context, curved, stage).
methods = []
def method(sig, name, body, type_phase=False):
    return sig+" {\n"+aliases(body,type_phase)+"\n"+body+"\n}\n\n"
methods.append(method("bool EngineControl::BeginLengthGeneration(EngineContext& context, bool collapse)","BeginLengthGeneration",next(x for x in removed if x["name"]=="begin_length_generation")["body"]))
methods.append(method("bool EngineControl::SurfaceReady(EngineContext& context)","SurfaceReady",next(x for x in removed if x["name"]=="surface_ready")["body"]))
methods.append(method("template<class Observer>\nbool EngineControl::EnterSurfaceRepair(EngineContext& context, Observer& observe_progress)","EnterSurfaceRepair",branch))
methods.append(method("void EngineControl::PreparePhaseDirection(EngineContext& context, EnginePhase next, bool refine_geometry)","PreparePhaseDirection",direction,True))
methods.append(method("template<class Observer>\nbool EngineControl::CompletePhaseEntry(EngineContext& context, EnginePhase next, bool refine_geometry, bool curved, Observer& observe_progress)","CompletePhaseEntry",tail,True))
methods.append(method("template<class Observer>\nbool EngineControl::EnterPhase(EngineContext& context, EnginePhase next, bool refine_geometry, bool curved, Observer& observe_progress)","EnterPhase",main_enter,True))
methods.append(method("void EngineControl::DiscardCheckpoint(EngineContext& context)","DiscardCheckpoint",next(x for x in removed if x["name"]=="discard_checkpoint")["body"]))
methods.append(method("bool EngineControl::CaptureCheckpoint(EngineContext& context)","CaptureCheckpoint",next(x for x in removed if x["name"]=="capture_checkpoint")["body"]))
methods.append(method("template<class Observer>\nbool EngineControl::RollbackCheckpoint(EngineContext& context, bool curved, Observer& observe_progress)","RollbackCheckpoint",rollback,True))
formatted=subprocess.run(["clang-format-18"],input="".join(methods),text=True,capture_output=True,check=True).stdout

# Header declarations: templates are defined in the included EngineControl.cpp in this TU.
decls='''
  bool BeginLengthGeneration(EngineContext&, bool collapse);
  bool SurfaceReady(EngineContext&);
  template<class Observer>
  bool EnterSurfaceRepair(EngineContext&, Observer& observe_progress);
  void PreparePhaseDirection(EngineContext&, EnginePhase next, bool refine_geometry);
  template<class Observer>
  bool CompletePhaseEntry(EngineContext&, EnginePhase next, bool refine_geometry, bool curved, Observer& observe_progress);
  template<class Observer>
  bool EnterPhase(EngineContext&, EnginePhase next, bool refine_geometry, bool curved, Observer& observe_progress);
  void DiscardCheckpoint(EngineContext&);
  bool CaptureCheckpoint(EngineContext&);
  template<class Observer>
  bool RollbackCheckpoint(EngineContext&, bool curved, Observer& observe_progress);
'''
assert header.count("  size_t SelectionBound(EngineContext& context, bool repairs);")==1
new_header=header.replace("  size_t SelectionBound(EngineContext& context, bool repairs);",decls+"\n  size_t SelectionBound(EngineContext& context, bool repairs);",1)
new_cpp=cpp+"\n"+formatted

# Capture audit and <=80-line audit on formatted, actual generated member definitions.
masked_cpp=mask(new_cpp)
methods_audit=[]
for name in ["BeginLengthGeneration","SurfaceReady","EnterSurfaceRepair","PreparePhaseDirection","CompletePhaseEntry","EnterPhase","DiscardCheckpoint","CaptureCheckpoint","RollbackCheckpoint"]:
    m=re.search(r"(?:bool|void) EngineControl::"+name+r"\s*\([^;]*?\)\s*\{",masked_cpp,re.S)
    assert m,name
    op=m.end()-1; cl=close_brace(new_cpp,masked_cpp,op)
    text=new_cpp[m.start():cl+1]; n=text.count("\n")+1
    assert n<=80,(name,n)
    methods_audit.append({"name":name,"formatted_lines":n,"sha256":sha(text.encode())})

# Inlining helpers and restoring only audited aliases must recover every old lambda token stream.
def member_body(name):
    m=re.search(r"(?:bool|void) EngineControl::"+name+r"\s*\([^;]*?\)\s*\{",masked_cpp,re.S); op=m.end()-1
    return new_cpp[op+1:close_brace(new_cpp,masked_cpp,op)]
def strip_alias(body):
    rows=[]; tail=[]; leading=True
    for line in body.splitlines(keepends=True):
        if leading and (not line.strip() or re.match(r"\s*(?:using Phase\s*=\s*EnginePhase;|(?:const auto&|auto&|bool&)\s+\w+\s*=.*;)\s*$",line)):
            if line.strip(): rows.append(line)
            continue
        leading=False;tail.append(line)
    return "".join(tail)
def reverse_method_calls(body):
    body=body.replace("SurfaceReady(context)","surface_ready()")
    body=body.replace("BeginLengthGeneration(context, false)","begin_length_generation(false)")
    body=body.replace("context.ownership.PhaseOwnership(context, true, curved)","phase_ownership(true)")
    body=body.replace("DiscardCheckpoint(context)","discard_checkpoint()")
    body=body.replace("EnterPhase(context, Phase::Improvement, false, curved, observe_progress)","enter_phase(Phase::Improvement)")
    return body
# Every direct method is token-equal after dropping its new aliases and restoring nested spelling.
proofs=[]
member_by_old={"begin_length_generation":"BeginLengthGeneration","surface_ready":"SurfaceReady","discard_checkpoint":"DiscardCheckpoint","capture_checkpoint":"CaptureCheckpoint","rollback_checkpoint":"RollbackCheckpoint"}
old_map={x["name"]:x["body"] for x in removed}
for old,member in member_by_old.items():
    actual=reverse_method_calls(strip_alias(member_body(member)))
    assert tok(actual)==tok(old_map[old]), (old,"direct body tokens differ")
    proofs.append({"old_lambda":old,"method":member,"token_equal":True})
# Rebuild enter_phase body by inlining the three helper bodies at their exact call sites.
enter=strip_alias(member_body("EnterPhase"))
branch_body=reverse_method_calls(strip_alias(member_body("EnterSurfaceRepair")))
direction_body=strip_alias(member_body("PreparePhaseDirection"))
tail_body=reverse_method_calls(strip_alias(member_body("CompletePhaseEntry")))
enter=enter.replace("return EnterSurfaceRepair(context, observe_progress);",branch_body,1)
enter=enter.replace("PreparePhaseDirection(context, next, refine_geometry);",direction_body,1)
enter=enter.replace("return CompletePhaseEntry(context, next, refine_geometry, curved, observe_progress);",tail_body,1)
enter=reverse_method_calls(enter)
if tok(enter)!=tok(old_map["enter_phase"]):
    a,b=tok(enter),tok(old_map["enter_phase"]); k=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
    raise AssertionError(("inlined enter_phase body differs",k,a[k:k+18],b[k:k+18],len(a),len(b)))
proofs.append({"old_lambda":"enter_phase","member":"EnterPhase with EnterSurfaceRepair, PreparePhaseDirection, and CompletePhaseEntry inlined","token_equal":True})

manifest={
 "unapplied":True,
 "base_commit":BASE_COMMIT,
 "base_sha256":{"CNativeEngine3D.cpp":EXPECTED["main"],"EngineControl.hpp":EXPECTED["header"],"EngineControl.cpp":EXPECTED["cpp"]},
 "proposed_sha256":{"CNativeEngine3D.cpp":sha(main_without.encode()),"EngineControl.hpp":sha(new_header.encode()),"EngineControl.cpp":sha(new_cpp.encode())},
 "call_routing":calls,
 "method_lines":methods_audit,
 "capture_aliases":{name:{key:{"declaration":BIND[key][0],"expression":BIND[key][1]} for key in caps(body)} for name,body in [
   ("BeginLengthGeneration",old_map["begin_length_generation"]),("SurfaceReady",old_map["surface_ready"]),
   ("EnterSurfaceRepair",branch),("PreparePhaseDirection",direction),("CompletePhaseEntry",tail),
   ("EnterPhase",main_enter),("DiscardCheckpoint",old_map["discard_checkpoint"]),
   ("CaptureCheckpoint",old_map["capture_checkpoint"]),("RollbackCheckpoint",rollback)]},
 "helper_inlining_actual_removed_lambda_token_proofs":proofs,
 "main_call_lambda_reversal_byte_equal":True,
 "observer_binding":{"current":"Existing Adapt-local observe_progress is passed as a template reference; no allocation/storage/copy.",
   "future_B5":"Rebind observer use to EngineControl::ObserveProgress(context, curved, stage)."},
 "future_B6_binding":"PhaseOwnership(context, quality, curved) is a provisional target and must be rebound to the reviewed B6 API.",
 "comparison":"C++ lexical token equality after removing only method-local borrowed aliases/type aliases and inlining the extracted EnterPhase helpers.",
 "no_live_source_written":True,
}
for filename,text in [("CNativeEngine3D.cpp.proposed",main_without),("EngineControl.hpp.proposed",new_header),("EngineControl.cpp.proposed",new_cpp)]:
    (OUT/filename).write_text(text)
(OUT/"binding_manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
print(json.dumps({"proposed_sha256":manifest["proposed_sha256"],"method_lines":methods_audit,"calls":calls},indent=2))
