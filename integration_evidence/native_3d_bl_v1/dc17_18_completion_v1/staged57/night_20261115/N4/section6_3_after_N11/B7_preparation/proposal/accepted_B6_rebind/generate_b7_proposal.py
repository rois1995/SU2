from pathlib import Path
import hashlib, json, re, subprocess

ROOT = Path.cwd()
PREP = ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/B7_preparation/proposal/accepted_B6_rebind"
OUT = PREP / "generated"
OUT.mkdir(parents=True, exist_ok=True)
MAIN = PREP / "input_CNativeEngine3D.cpp"
HEADER = PREP / "input_EngineControl.hpp"
CPP = PREP / "input_EngineControl.cpp"
EXPECTED = {
    "main": "f82d588aceb84f5a9bbc5258695240743f1df423a78a8fd76f21a975b1620f45",
    "header": "0bc306f23434fc0f84013c026912b0a2f7c9e6d62247ae61445683a61748062e",
    "cpp": "956858a5a2e5a1915d3afac49ea21ce9f8336120b24738f440f8a3bd6d92fb7b",
}
BASE_COMMIT = "de723482c70fbbeff0bb1817e254a487732940d5"
def git_bytes(path):
    return subprocess.check_output(["git", "show", f"{BASE_COMMIT}:{path}"], cwd=ROOT)
def sha(data): return hashlib.sha256(data).hexdigest()
main_b = MAIN.read_bytes()
header_b = HEADER.read_bytes()
cpp_b = CPP.read_bytes()
assert sha(main_b) == EXPECTED["main"], "Candidate B6 main changed; rebind this unapplied proposal."
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
main_enter=pre[:dir_a]+"    PreparePhaseDirection(context, next, refine_geometry);\n"+pre[dir_b:]+"    return CompletePhaseEntry(context, next, refine_geometry, atlas_budget);\n"
main_enter=main_enter[:branch_a]+"    if (surface_target && next == Phase::Improvement) {\n      return EnterSurfaceRepair(context, atlas_budget);\n    }\n"+main_enter[branch_b:]
# Keep current condition's exact body for helper; only nested lambda name changes to member.
branch_mask=mask(branch); branch_open=branch_mask.index("{")
branch=branch[branch_open+1:-1]
branch=branch.replace("surface_ready()", "SurfaceReady(context)")
tail=tail.replace("begin_length_generation(false)", "BeginLengthGeneration(context, false)")
assert "context.ownership.PhaseOwnership(context, true, atlas_budget)" in tail
# Rewire the methods called from moved bodies.
rollback_old=next(x for x in removed if x["name"]=="rollback_checkpoint")["body"]
rollback=rollback_old.replace("discard_checkpoint()", "DiscardCheckpoint(context)")
rollback=rollback.replace("enter_phase(Phase::Improvement)", "EnterPhase(context, Phase::Improvement, false, atlas_budget)")

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
routed("surface_ready",lambda a:"context.control.SurfaceReady(context)")
def enter_route(a):
    args=split_args(a)
    if len(args)==1: args.append("false")
    assert len(args)==2,args
    return "context.control.EnterPhase(context, "+args[0]+", "+args[1]+", atlas_budget)"
routed("enter_phase",enter_route)
routed("capture_checkpoint",lambda a:"context.control.CaptureCheckpoint(context)")
routed("rollback_checkpoint",lambda a:"context.control.RollbackCheckpoint(context, atlas_budget)")
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

# Member definitions borrow the originally selected atlas budget explicitly.
methods = []
def method(sig, name, body, type_phase=False):
    return sig+" {\n"+aliases(body,type_phase)+"\n"+body+"\n}\n\n"
methods.append(method("bool EngineControl::BeginLengthGeneration(EngineContext& context, bool collapse)","BeginLengthGeneration",next(x for x in removed if x["name"]=="begin_length_generation")["body"]))
methods.append(method("bool EngineControl::SurfaceReady(EngineContext& context)","SurfaceReady",next(x for x in removed if x["name"]=="surface_ready")["body"]))
methods.append(method("bool EngineControl::EnterSurfaceRepair(EngineContext& context, SU2NativeGeometry::AtlasBudget& atlas_budget)","EnterSurfaceRepair",branch))
methods.append(method("void EngineControl::PreparePhaseDirection(EngineContext& context, EnginePhase next, bool refine_geometry)","PreparePhaseDirection",direction,True))
methods.append(method("bool EngineControl::CompletePhaseEntry(EngineContext& context, EnginePhase next, bool refine_geometry, SU2NativeGeometry::AtlasBudget& atlas_budget)","CompletePhaseEntry",tail,True))
methods.append(method("bool EngineControl::EnterPhase(EngineContext& context, EnginePhase next, bool refine_geometry, SU2NativeGeometry::AtlasBudget& atlas_budget)","EnterPhase",main_enter,True))
methods.append(method("void EngineControl::DiscardCheckpoint(EngineContext& context)","DiscardCheckpoint",next(x for x in removed if x["name"]=="discard_checkpoint")["body"]))
methods.append(method("bool EngineControl::CaptureCheckpoint(EngineContext& context)","CaptureCheckpoint",next(x for x in removed if x["name"]=="capture_checkpoint")["body"]))
methods.append(method("bool EngineControl::RollbackCheckpoint(EngineContext& context, SU2NativeGeometry::AtlasBudget& atlas_budget)","RollbackCheckpoint",rollback,True))
formatted=subprocess.run(["clang-format-18"],input="".join(methods),text=True,capture_output=True,check=True).stdout

# Header declarations for the extracted members.
decls='''
  bool BeginLengthGeneration(EngineContext&, bool collapse);
  bool SurfaceReady(EngineContext&);
  bool EnterSurfaceRepair(EngineContext&, SU2NativeGeometry::AtlasBudget& atlas_budget);
  void PreparePhaseDirection(EngineContext&, EnginePhase next, bool refine_geometry);
  bool CompletePhaseEntry(EngineContext&, EnginePhase next, bool refine_geometry, SU2NativeGeometry::AtlasBudget& atlas_budget);
  bool EnterPhase(EngineContext&, EnginePhase next, bool refine_geometry, SU2NativeGeometry::AtlasBudget& atlas_budget);
  void DiscardCheckpoint(EngineContext&);
  bool CaptureCheckpoint(EngineContext&);
  bool RollbackCheckpoint(EngineContext&, SU2NativeGeometry::AtlasBudget& atlas_budget);
'''
assert header.count("  size_t SelectionBound(EngineContext& context, bool repairs);")==1
new_header=header.replace("  size_t SelectionBound(EngineContext& context, bool repairs);",decls+"\n  size_t SelectionBound(EngineContext& context, bool repairs);",1)
new_header=subprocess.run(["clang-format-18"],input=new_header,text=True,capture_output=True,check=True).stdout
purpose="// Runs phase and checkpoint transitions, observation and finalization using borrowed canonical state without policy."
cpp=purpose+"\n"+cpp.split("\n",1)[1]
new_cpp=subprocess.run(["clang-format-18"],input=cpp+"\n"+formatted,text=True,capture_output=True,check=True).stdout

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
    body=body.replace("DiscardCheckpoint(context)","discard_checkpoint()")
    body=body.replace("EnterPhase(context, Phase::Improvement, false, atlas_budget)","enter_phase(Phase::Improvement)")
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
enter=enter.replace("return EnterSurfaceRepair(context, atlas_budget);",branch_body,1)
enter=enter.replace("PreparePhaseDirection(context, next, refine_geometry);",direction_body,1)
enter=enter.replace("return CompletePhaseEntry(context, next, refine_geometry, atlas_budget);",tail_body,1)
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
 "observer_binding":"Existing static EngineControl::ObserveProgress(context, working, atlas_budget, stage), unchanged.",
 "B6_binding":"Existing context.ownership.PhaseOwnership(context, true, atlas_budget), unchanged.",
 "comparison":"C++ lexical token equality after removing only method-local borrowed aliases/type aliases and inlining the extracted EnterPhase helpers.",
 "no_live_source_written":True,
}
for filename,text in [("CNativeEngine3D.cpp.proposed",main_without),("EngineControl.hpp.proposed",new_header),("EngineControl.cpp.proposed",new_cpp)]:
    (OUT/filename).write_text(text)
(OUT/"binding_manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
print(json.dumps({"proposed_sha256":manifest["proposed_sha256"],"method_lines":methods_audit,"calls":calls},indent=2))
# Candidate must not depend on any removed local lambda, including nested members.
for old_name in NAMES:
 for label, text in [('main',main_without),('generated members',''.join(methods))]:
  assert not re.search(r'\b'+re.escape(old_name)+r'\b',mask(text)),(label,old_name)
# Verify exactly the alias and type-alias prefixes actually emitted, not arbitrary aliases.
source_bodies={'BeginLengthGeneration':old_map['begin_length_generation'],'SurfaceReady':old_map['surface_ready'],
 'EnterSurfaceRepair':branch,'PreparePhaseDirection':direction,'CompletePhaseEntry':tail,'EnterPhase':main_enter,
 'DiscardCheckpoint':old_map['discard_checkpoint'],'CaptureCheckpoint':old_map['capture_checkpoint'],'RollbackCheckpoint':rollback}
phase_methods={'PreparePhaseDirection','CompletePhaseEntry','EnterPhase','RollbackCheckpoint'}
for name, body in source_bodies.items():
 actual=member_body(name); rows=[]
 for line in actual.splitlines():
  if not line.strip():continue
  if re.match(r'\s*(?:using Phase\s*=\s*EnginePhase;|(?:const auto&|auto&|bool&)\s+\w+\s*=.*;)\s*$',line): rows.append(line.strip())
  else:break
 expected=aliases(body,name in phase_methods).splitlines()
 assert rows==[x.strip() for x in expected],(name,rows,expected)
 # Check captured free names from the actual final body after stripping members from expressions.
 bare=re.sub(r'(?:\.|->)\s*\w+','.',mask(strip_alias(actual)))
 used=set(re.findall(r'\b[A-Za-z_]\w*\b',bare)) & set(BIND)
 assert used<=set(caps(body)),(name,used-set(caps(body)))
# Existing fields, special members, and B5 declarations survive after removing B7 additions.
recovered=new_header
for name in source_bodies:
 recovered,n=re.subn(r'\b(?:bool|void)\s+'+name+r'\s*\([^;]*\)\s*;', '',recovered,count=1,flags=re.S)
 assert n==1,name
assert tok(recovered)==tok(header),'B5 Control header changed outside added declarations'
old_prefix=new_cpp[:new_cpp.index('bool EngineControl::BeginLengthGeneration')]
assert tok(old_prefix)==tok(cpp),'B5 Control implementation tokens changed outside appended methods'
# Full-file format inventory includes pre-existing free functions and lambdas.
def scan_file(filename,text):
 formatted_text=subprocess.run(['clang-format-18'],input=text,text=True,capture_output=True,check=True).stdout
 masked_text=mask(formatted_text); entries=[]
 pat=re.compile(r'(\[[^\]\n]*\]\s*(?:\([^;{}]*?\))?\s*(?:mutable\s*)?(?:->\s*[^{};]+)?|(?:[\w:~]+)\s*\([^;{}]*?\)\s*(?:const\s*)?(?:noexcept\s*)?)\s*\{',re.S)
 for match in pat.finditer(masked_text):
  label=' '.join(match.group(1).split()); head=label.split('(')[0].strip()
  if head in {'if','for','while','switch','catch'}:continue
  op=match.end()-1
  try:cl=close_brace(formatted_text,masked_text,op)
  except AssertionError:continue
  entries.append({'signature':label[:180],'line':formatted_text.count('\n',0,match.start())+1,'formatted_lines':formatted_text[match.start():cl+1].count('\n')+1,'lambda':label.startswith('[')})
 return entries
inventory={n:scan_file(n,t) for n,t in [('CNativeEngine3D.cpp',main_without),('EngineControl.hpp',new_header),('EngineControl.cpp',new_cpp)]}
(OUT/'full_file_formatted_inventory.json').write_text(json.dumps({'tool':'clang-format-18','scope':'entire actual generated files; includes pre-existing code','limitations':'lexical scan may classify initializers as definitions','files':inventory},indent=2)+'\n')
# Measure all new definitions from the formatted FULL final implementation, not an isolated template.
full_cpp=subprocess.run(['clang-format-18'],input=new_cpp,text=True,capture_output=True,check=True).stdout
for item in methods_audit:
 match=re.search(r'(?:bool|void) EngineControl::'+item['name']+r'\s*\([^;]*?\)\s*\{',mask(full_cpp),re.S); assert match
 op=match.end()-1; end=close_brace(full_cpp,mask(full_cpp),op); lines=full_cpp[match.start():end+1].count('\n')+1
 assert lines<=80,(item['name'],lines)
 item['full_file_formatted_lines']=lines
# Every added lambda or free function in a new method is accounted for by this assertion.
for entry in inventory['EngineControl.cpp']:
 if entry['lambda'] and entry['line']>=full_cpp[:full_cpp.index('bool EngineControl::BeginLengthGeneration')].count('\n')+1:
  assert entry['formatted_lines']<=80,entry
for filename, actual in [('EngineControl.hpp',new_header),('EngineControl.cpp',new_cpp)]:
 subprocess.run(['clang-format-18','--dry-run','--Werror','--assume-filename='+filename],input=actual,text=True,capture_output=True,check=True)
manifest['full_control_files_clang_format_dry_run_Werror_passed']=True
manifest.update({'status':'CANDIDATE_NOT_ACCEPTED','all_six_old_names_absent_from_main_and_generated_members':True,
 'actual_alias_prefixes_match_canonical_bindings':True,'header_original_tokens_equal_after_removing_added_declarations':True,
 'B5_cpp_prefix_token_identical_after_full_format_and_comment':True,'method_lines':methods_audit,'full_file_inventory':'full_file_formatted_inventory.json',
 'surface_ready_correction':'One external main-loop call had not been routed by the older unapplied generator; this candidate routes it and includes it in reversal.',
 'compiled_or_tested':False})
(OUT/'binding_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('free-name, actual binding, header identity, B5 prefix and full-file checks passed')
# Input snapshots are checked against their recorded immutable hashes on every generation.
receipt=json.loads((PREP/'INPUT_SHA_RECEIPT.json').read_text())
for name,info in receipt['inputs'].items():
 assert sha((PREP/('input_'+name)).read_bytes())==info['sha256'],name
assert receipt['inputs']['EngineOwnership.hpp']['sha256']=='c8563750fd9880e3b2d7c576cdf6a3e2df609eb0e54938703574badd481c1492'
assert len(re.findall(r'EngineControl\s*::\s*ObserveProgress\s*\(',''.join(methods)))==3
assert len(re.findall(r'context\s*\.\s*ownership\s*\.\s*PhaseOwnership\s*\(',''.join(methods)))==1
assert not re.search(r'\b(?:Observer|observe_progress|curved)\b',mask(''.join(methods)))
files=['generate_b7_proposal.py','INPUT_SHA_RECEIPT.json']+['input_'+name for name in receipt['inputs']]
hashes={name:sha((PREP/name).read_bytes()) for name in files}
for name in ['CNativeEngine3D.cpp.proposed','EngineControl.hpp.proposed','EngineControl.cpp.proposed','binding_manifest.json','full_file_formatted_inventory.json']:
 hashes['generated/'+name]=sha((OUT/name).read_bytes())
(PREP/'CANDIDATE_SHA256.json').write_text(json.dumps({'status':'CANDIDATE_NOT_ACCEPTED','unapplied':True,'sha256':hashes},indent=2)+'\n')
print('candidate input pins, direct observer/ownership calls and frozen SHA map passed')
