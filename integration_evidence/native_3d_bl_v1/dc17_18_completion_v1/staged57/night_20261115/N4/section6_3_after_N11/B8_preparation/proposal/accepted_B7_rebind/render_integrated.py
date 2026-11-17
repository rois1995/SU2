from pathlib import Path
import hashlib,json,re,subprocess,difflib
P=Path(__file__).resolve().parent
read=lambda n:(P/n).read_text()
sha=lambda t:hashlib.sha256(t.encode()).hexdigest()
fmt=lambda t:subprocess.run(['clang-format-18'],input=t,text=True,capture_output=True,check=True).stdout
origin=read('input_CNativeEngine3D.cpp');main=read('CNativeEngine3D.cpp.proposed');hpp=read('EngineSetup.hpp.proposed');cpp=read('EngineSetup.cpp.proposed')
direct='EngineControl::Finish(context, curved, working, atlas_budget, started, started_cpu)'
# All unaffected sites stay direct, including S10/S12/S18/S19.
main=main.replace('finish()',direct)
for method in ('RefreshWorkingSurface(context, atlas_budget, finish)','ConstructRegion(context, global_original, failure, finish)','InitializeFirstRows(context, finish)'):
 assert main.count(method)==1,method
 main=main.replace(method,method.replace(', finish)',', setup_finish)'),1)
anchor='  if (!ready) return '+direct+';\n  if (auto done = EngineSetup::RefreshWorkingSurface'
assert main.count(anchor)==1
wrapper='  auto setup_finish = [&]() {\n    return '+direct+';\n  };\n'
main=main.replace(anchor,'  if (!ready) return '+direct+';\n'+wrapper+'  if (auto done = EngineSetup::RefreshWorkingSurface',1)
old_begin='              if(bidirectional)directory->BeginConstructionPass();'; new_begin='  EngineSetup::BeginConstructionPass(context);';assert main.count(old_begin)==1
main=main.replace(old_begin,new_begin,1)
old_seed='''  {
    CLocalFailure failure;
    try{selection.SeedFrontier(working);}catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!context.exchange.Vote(context, failure))return '''+direct+''';
  }
'''
new_seed='''  {
    CLocalFailure failure;
    if (!EngineSetup::SeedFrontier(context, failure))\n      return '''+direct+''';
  }
'''
assert main.count(old_seed)==1;main=main.replace(old_seed,new_seed,1)
# Format only newly introduced routing lines; inverse records the exact pre-format bytes.
routing_whitespace=[
 ('    auto region_done=EngineSetup::ConstructRegion(context, global_original, failure, setup_finish);\n    if(region_done)return std::move(*region_done);',
  '    auto region_done = EngineSetup::ConstructRegion(context, global_original, failure, setup_finish);\n    if (region_done) return std::move(*region_done);'),
 ('  if (!EngineSetup::CheckRegionApplicability(context, region_applicable, prismatic)) return '+direct+';',
  '  if (!EngineSetup::CheckRegionApplicability(context, region_applicable, prismatic))\n    return '+direct+';'),
]
for old,new in routing_whitespace:
 assert main.count(old)==1;main=main.replace(old,new,1)
# Exact inverse of the integration-only rebindings; each extraction stage has its own inverse.
reversed_main=main
for old,new in reversed(routing_whitespace):
 assert reversed_main.count(new)==1;reversed_main=reversed_main.replace(new,old,1)
reversed_main=reversed_main.replace(new_seed,old_seed,1).replace(new_begin,old_begin,1).replace(wrapper,'',1)
for method in ('RefreshWorkingSurface(context, atlas_budget, setup_finish)','ConstructRegion(context, global_original, failure, setup_finish)','InitializeFirstRows(context, setup_finish)'):
 assert reversed_main.count(method)==1;reversed_main=reversed_main.replace(method,method.replace(', setup_finish)',', finish)'),1)
reversed_main=reversed_main.replace(direct,'finish()'); assert reversed_main==read('CNativeEngine3D.cpp.proposed')
# Undo byte edits using exact stage bytes and reverse order; verify each source slice.
def inverse_stage(current, before, after):
 assert current==after
 prefix=0
 while prefix<min(len(before),len(after)) and before[prefix]==after[prefix]:prefix+=1
 suffix=0
 while suffix<min(len(before)-prefix,len(after)-prefix) and before[-1-suffix]==after[-1-suffix]:suffix+=1
 end_before=len(before)-suffix;end_after=len(after)-suffix
 assert current[prefix:end_after]==after[prefix:end_after]
 current=current[:prefix]+before[prefix:end_before]+current[end_after:]
 assert current==before
 return current
stages=['synthetic_finish_normalized_main.cpp','CNativeEngine3D_S10.cpp.proposed','CNativeEngine3D_S11.cpp.proposed','CNativeEngine3D_S12.cpp.proposed','CNativeEngine3D_S13.cpp.proposed','CNativeEngine3D.cpp.proposed']
for i in range(len(stages)-1,0,-1):reversed_main=inverse_stage(reversed_main,read(stages[i-1]),read(stages[i]))
assert reversed_main.replace('finish()',direct)==origin
# Original setup fields and destructor remain; add only the two small declarations and methods.
decl='  static void BeginConstructionPass(EngineContext&);\n  static bool SeedFrontier(EngineContext&, CLocalFailure&);\n'; assert hpp.count('  EngineSetup();')==1;hpp=hpp.replace('  EngineSetup();',decl+'  EngineSetup();',1)
small=read('../EngineSetup.cpp.proposed').split('\n',1)[1];cpp+='\n'+small
hpp=hpp.replace('// Owns the original setup RAII in declaration order; never performs collectives or decisions.','// Owns setup RAII and executes the original setup stages using borrowed canonical state.')
cpp=cpp.replace('// Implements the original-order setup owners; never starts a deferred owner or a collective.','// Executes original setup stages and collectives while preserving canonical ownership and lifetimes.')
cpp=cpp.replace('#include "EngineSetup.hpp"','#include "EngineSetup.hpp"\n#include "EngineTrial.hpp"',1)
hpp=fmt(hpp);cpp=fmt(cpp)
for n,t in [('CNativeEngine3D.integrated.cpp.proposed',main),('EngineSetup.integrated.hpp.proposed',hpp),('EngineSetup.integrated.cpp.proposed',cpp)]: (P/n).write_text(t)
# Extract actual final definitions, including complete function bodies and balanced nested braces.
lex=re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',re.S)
def mask(s):return lex.sub(lambda m:''.join('\n' if c=='\n' else ' ' for c in m[0]),s)
def close(s,i):
 d=0
 for n,c in enumerate(mask(s)[i:],i):
  if c=='{':d+=1
  if c=='}':
   d-=1
   if d==0:return n
 raise ValueError('unclosed')
token=re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\w+|[^\s]')
def toks(s):return token.findall(re.sub(r'//[^\n]*|/\*.*?\*/','',s,flags=re.S))
def defs(s):
 result={}; pattern=r'\b(?:void|bool|std::optional<EngineResult>)\s+EngineSetup::(\w+)\s*\([^;{}]*?\)\s*\{'
 for m in re.finditer(pattern,mask(s),re.S):
  op=m.end()-1;end=close(s,op);assert m[1] not in result;result[m[1]]=s[m.start():end+1]
 return result
before=defs(read('EngineSetup.cpp.proposed')+'\n'+small);after=defs(cpp);assert set(before)==set(after)
for name in before:assert toks(before[name])==toks(after[name]),name
lengths={name:body.count('\n')+1 for name,body in after.items()};assert max(lengths.values())<=80,lengths
# New closures, including service callbacks, must be <=15 lines.
lambda_lengths=[]
for m in re.finditer(r'\[[^\]\n]*\]\s*(?:\([^;{}]*?\))?\s*(?:->\s*[^{};]+)?\s*\{',mask(cpp),re.S):
 op=m.end()-1;end=close(cpp,op); n=cpp[m.start():end+1].count('\n')+1;lambda_lengths.append(n);assert n<=15,(cpp[m.start():m.start()+70],n)
assert wrapper.count('\n')<=15
# Existing fields and all six special members survive unchanged.
old_hpp=read('input_EngineSetup.hpp');old_prefix=old_hpp.split('struct EngineSetup {',1)[1].split('  EngineSetup();',1)[0]
new_prefix=hpp.split('struct EngineSetup {',1)[1].split('  static ',1)[0]
assert toks(old_prefix)==toks(new_prefix)
old_suffix=old_hpp.split('  EngineSetup();',1)[1];new_suffix=hpp.split('  EngineSetup();',1)[1]
assert toks(old_suffix)==toks(new_suffix)
for filename,text in [('EngineSetup.hpp',hpp),('EngineSetup.cpp',cpp)]:subprocess.run(['clang-format-18','--dry-run','--Werror','--assume-filename='+filename],input=text,text=True,capture_output=True,check=True)
manifest={'status':'CANDIDATE_NOT_ACCEPTED_UNAPPLIED','whole_B7_main_reverse_byte_equal':True,'extraction_stage_byte_reversals':stages,'actual_final_formatted_method_tokens_equal_to_sequential_stage_methods':True,'formatted_method_lines':lengths,'formatted_lambda_lines':lambda_lengths,'original_setup_fields_and_special_members_token_equal':True,'formatter_dry_run_Werror':True,'s11_correction':'Added missing return std::nullopt on successful PublishRefreshedSurface; otherwise control reaches end of non-void helper.','proof_limitation':'Stage recursive checks are inherited from adapted generators; final-body token identity transfers those checks but does not establish semantic equivalence or compile.','no_build_or_numerical_run':True}
(P/'INTEGRATED_PROOF.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('integrated draft proof passes',lengths,'lambda lengths',lambda_lengths)
# Terminal/status audit: an unconditional final return is required in every bool/optional worker.
terminal={}; alias_inventory={}
for name,body in after.items():
 sig=body[:body.index('{')]; funcbody=body[body.index('{')+1:-1]; clean=re.sub(r'//[^\n]*|/\*.*?\*/','',funcbody,flags=re.S).strip()
 aliases=re.findall(r'^\s*((?:const\s+)?auto\s*&\s*\w+\s*=\s*[^;]+;)',funcbody,re.M)
 alias_inventory[name]={'signature':' '.join(sig.split()),'aliases':aliases}
 if sig.startswith(('bool ','std::optional<EngineResult>')):
  last=re.search(r'\breturn\s+([^;{}]+);\s*$',clean,re.S)
  assert last,(name,'missing terminal return')
  expression=' '.join(last.group(1).split())
  if sig.startswith('std::optional'):
   assert expression=='std::nullopt' or expression.startswith('EngineSetup::'),(name,expression)
   direct_finishes=len(re.findall(r'\bfinish\s*\(\s*\)',clean))
   engaged=len(re.findall(r'return\s+std::optional\s*<\s*EngineResult\s*>\s*\(\s*finish\s*\(\s*\)\s*\)\s*;',clean))
   assert direct_finishes==engaged,(name,direct_finishes,engaged)
  else:
   assert expression in ('true','false','context.exchange.Vote(context, failure)'),(name,expression)
   direct_finishes=0
  terminal[name]={'signature':' '.join(sig.split()),'last_unconditional_return':expression,'direct_failure_finish_calls':direct_finishes,'all_direct_finish_calls_return_engaged_optional':True}
# Scan all 31 actual bodies for references to canonical locals lacking a declaration
# or parameter. Member names after dot/arrow are excluded; named type qualifiers stay.
canonical=set('world options geometry composition original accepted result stats working selection initial directory source maximum_node maximum_cell coarsening repairing field_identity last_rejection wall_counters started surface_target region_rebuilt region_active region_started layer_active layer_started atlas_geometry target_geometry_lease atlas_work_initial bidirectional intent accepted_bytes eligible_extensions atlas_budget curved ready'.split())
for name,body in after.items():
 text=re.sub(r'(?:\.|->)\s*\w+','.',mask(body))
 used=set(re.findall(r'\b\w+\b',text)) & canonical
 declared=set(re.findall(r'(?:auto|bool|Id|size_t|Clock::time_point|Owned|PlanarReference|EngineOptions|EngineResult|EngineContext|AtlasBudget|const size_t)\s*(?:\*|&)*\s*(\w+)\s*(?=[=,);{])',text))
 assert used<=declared,(name,sorted(used-declared))
assert 'return std::nullopt;' in after['PublishRefreshedSurface']
assert main.count('auto setup_finish =')==1 and main.count('setup_finish')==4
for symbol in ('plan','construct','publish','needs_layer'):
 # needs_layer legitimately exists as a local argument/closure in S15; all other removed names must not be invoked.
 if symbol!='needs_layer':assert not re.search(r'(?<![.>:])\b'+symbol+r'\s*\(',mask(cpp)),symbol
removed=('finish','begin_length_generation','surface_ready','enter_phase','discard_checkpoint','capture_checkpoint','rollback_checkpoint','plan','construct','publish')
for name in removed:
 assert not re.search(r'(?<![.>:])\b'+name+r'\s*\(',mask(main)),('unbound removed caller',name)
assert '#include "EngineTrial.hpp"' in cpp and 'EngineTrial::HeightCheck' in cpp
assert 'EngineReports::' not in cpp
for name in after:assert re.search(r'\b'+re.escape(name)+r'\s*\(',hpp),name
calls=set(re.findall(r'EngineSetup\s*::\s*(\w+)\s*\(',mask(cpp+'\n'+main)))
assert calls<=set(after)|{'EngineSetup'},sorted(calls-set(after)-{'EngineSetup'})
(P/'TERMINAL_BINDING_AUDIT.json').write_text(json.dumps({'status':'source-level audit; not a compiler or formal control-flow proof','terminal_returns':terminal,'types_and_borrowed_aliases':alias_inventory,'all_EngineSetup_calls_have_candidate_definitions':True,'no_unbound_original_plan_construct_publish_calls':True,'no_removed_local_calls_in_main':True,'EngineTrial_declaration_included_before_use':True,'EngineReports_reference_absent':True,'single_borrowed_synchronous_setup_finish':True,'all_31_methods_canonical_name_declaration_scan_passed':True,'scan_limit':'lexical name audit; not a C++ type checker'},indent=2)+'\n')
manifest['all_optional_and_bool_workers_have_terminal_return_audit']=True
manifest['terminal_binding_audit']='TERMINAL_BINDING_AUDIT.json'
(P/'INTEGRATED_PROOF.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('terminal status, finish transport, call and include audits passed',len(terminal))
