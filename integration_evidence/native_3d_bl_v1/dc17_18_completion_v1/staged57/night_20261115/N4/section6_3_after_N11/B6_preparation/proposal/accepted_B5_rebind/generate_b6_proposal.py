from pathlib import Path
import hashlib, json, re, subprocess

ROOT=Path.cwd()
OUT=ROOT/'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/B6_preparation/proposal/accepted_B5_rebind'
COMMIT='2a05da05ca1f9642fc1847c5dff57e78d2eb50df'
def read(p): return (OUT/('input_'+Path(p).name)).read_text()
def sha(s): return hashlib.sha256(s.encode()).hexdigest()
for name, expected in {
 'input_CNativeEngine3D.cpp':'18117d033beba96d9d78bc513486352449caf9dc499c241c6b5aad9978801a86',
 'input_EngineOwnership.hpp':'474b4ef2d06f079202bd9ece9e01165ed48dab93c4cc23b72ef64a570e8a9e81',
 'input_EngineOwnership.cpp':'064e1b42d50255852194f4eb7bfa54e8036d5f4c1f2bc21d09744c8b8ea266d5',
}.items(): assert hashlib.sha256((OUT/name).read_bytes()).hexdigest()==expected, name
def mask(s): return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',lambda m: ''.join('\n' if c=='\n' else ' ' for c in m[0]),s,flags=re.S)
def close(s,i):
 d=0
 for n,c in enumerate(mask(s)[i:],i):
  if c=='{': d+=1
  if c=='}':
   d-=1
   if not d:return n
 raise ValueError('unclosed')
def toks(s): return re.findall(r'"(?:\\.|[^"\\])*"|\w+|[^\s]',re.sub(r'//[^\n]*|/\*.*?\*/','',s,flags=re.S))
def fmt(s):return subprocess.run(['clang-format-18'],input=s,text=True,capture_output=True,check=True).stdout
main=read('Common/src/adaptation/CNativeEngine3D.cpp')
assert sha(main)=='18117d033beba96d9d78bc513486352449caf9dc499c241c6b5aad9978801a86'
hpp=read('Common/src/adaptation/native_engine3d/EngineOwnership.hpp')
cpp=read('Common/src/adaptation/native_engine3d/EngineOwnership.cpp')
m=re.search(r'^  auto phase_ownership=\[&\]\(bool quality\) \{',main,re.M); start=m.start(); op=m.end()-1; end=close(main,op); original=main[op+1:end]; assert main[end:end+3]=='};\n'
assert original== (OUT/'input_phase_ownership_frozen_body.cpp.txt').read_text()
# Partition exactly at statement boundaries; each chunk is sourced from the removed lambda.
def cut(s,marker): i=s.index(marker);return s[:i],s[i:]
clock,body=cut(original,'    if(!within_time())return false;')
phase,body=cut(body,'    const auto scratch=')
estimate,body=cut(body,'    try {\n      costs.reserve')
fill,body=cut(body,'    if(!context.exchange.Vote(context, failure)||!within_time())')
vote,body=cut(body,'    const auto max_cost=')
normalize,body=cut(body,'    const double demand_sum=')
should,body=cut(body,'    if (options.timings) ReportOwnershipMemory(world, "before");')
migrate,body=cut(body,'    // Stage exact per-cell census certificates')
cert,body=cut(body,'    try {\n      pending_initial.reserve')
restore,final=cut(body,'    // No private transaction survives')
# A loop body is extracted without changing traversal, catch or reserve order.
loop_start=fill.index('      for(const auto& owned:working) {'); loop_open=fill.index('{',loop_start); loop_end=close(fill,loop_open)
cell=fill[loop_open+1:loop_end]
fill=fill[:loop_open+1]+'\n        AccumulateOwnershipCell(context, quality, owned, complete_stars, costs, geometry_costs, quality_debts, local_demand, local_geometry, local_quality_debt);\n      '+fill[loop_end:]
assert cell.count('continue;')>=2
cell=cell.replace('local_demand+=1;continue;','local_demand+=1;return;',1)
geo_start=cell.index('        double geometry_cost=0;'); geo_end=cell.index('        if(!(std::isfinite(demand)',geo_start)
geo=cell[geo_start:geo_end]+'        return geometry_cost;\n'
cell=cell[:geo_start]+'        const double geometry_cost=OwnershipGeometryCost(context, quality, owned, demand);\n'+cell[geo_end:]
# Function signatures and call sites pass all live parent objects by reference.
V='std::vector<uint32_t>& weights, std::vector<uint32_t>& geometric, std::vector<uint32_t>& quality_weights'
D='std::vector<double>& costs, std::vector<double>& geometry_costs, std::vector<double>& quality_debts'
L='double& local_demand, double& local_geometry, double& local_quality_debt'
A='weights, geometric, quality_weights, costs, geometry_costs, quality_debts, local_demand, local_geometry, local_quality_debt'
M='Owned& pending, std::vector<Id>& pending_initial, std::unique_ptr<Directory>& pending_directory, Selection& pending_selection, PartitionStats& partition'
MA='pending, pending_initial, pending_directory, pending_selection, partition'
C='std::vector<std::vector<Selection::CachedRow>>& outgoing, const std::vector<Selection::CachedRow>& records, size_t pending_bytes'
CA='outgoing, records, pending_bytes'
P='EngineContext& context, bool quality, SU2NativeGeometry::AtlasBudget& atlas_budget'
methods={}
def add(name,sig,body): methods[name]=(sig,body)
add('WithinTime','EngineContext& context',clock[clock.index('{')+1:clock.rindex('}')] )
phase=phase.replace('within_time()','WithinTime(context)').replace('PhaseOwnership::Disabled','SU2Native3D::PhaseOwnership::Disabled')
add('PhaseOwnership',P,phase+'    return EstimateOwnership(context, quality, atlas_budget, global_cells, transition_started);\n')
estimate=estimate.replace('within_time()','WithinTime(context)')
add('EstimateOwnership',P+', uint64_t global_cells, double transition_started',estimate+f'    FillOwnershipCosts(context, quality, complete_stars, failure, {A});\n'+vote.replace('within_time()','WithinTime(context)')+f'    NormalizeOwnershipWeights(context, quality, weights, geometric, quality_weights, costs, geometry_costs, quality_debts);\n    if(!ShouldMigrate(context, quality, global_cells, transition_started, local_demand, local_geometry, local_quality_debt))return true;\n    return MigrateOwnership(context, quality, atlas_budget, transition_started, scratch, failure, weights, geometric, quality_weights);\n')
add('FillOwnershipCosts','EngineContext& context, bool quality, const std::map<Id,uint64_t>& complete_stars, CLocalFailure& failure, '+V+', '+D+', '+L,fill)
add('AccumulateOwnershipCell','EngineContext& context, bool quality, const Owned::value_type& owned, const std::map<Id,uint64_t>& complete_stars, '+D+', '+L,cell)
add('OwnershipGeometryCost','EngineContext& context, bool quality, const Owned::value_type& owned, double demand',geo)
add('NormalizeOwnershipWeights','EngineContext& context, bool quality, '+V+', '+D,normalize)
should=should.replace('return true;}','return false;}').replace('PhaseOwnership::DiagnosticAlways','SU2Native3D::PhaseOwnership::DiagnosticAlways')+'    return true;\n'
add('ShouldMigrate','EngineContext& context, bool quality, uint64_t global_cells, double transition_started, double local_demand, double local_geometry, double local_quality_debt',should)
add('MigrateOwnership',P+', double transition_started, size_t scratch, CLocalFailure& failure, '+V,migrate.replace('within_time()','WithinTime(context)')+f'    return MigrateCertificates(context, quality, atlas_budget, transition_started, scratch, failure, baseline, {MA});\n')
add('MigrateCertificates',P+', double transition_started, size_t scratch, CLocalFailure& failure, size_t baseline, '+M,cert.replace('within_time()','WithinTime(context)')+f'    return RestoreOwnership(context, quality, atlas_budget, transition_started, failure, {MA}, {CA});\n')
add('RestoreOwnership',P+', double transition_started, CLocalFailure& failure, '+M+', '+C,restore.replace('within_time()','WithinTime(context)').replace('observe_progress(', 'context.control.ObserveProgress(context, working, atlas_budget, ')+f'    return FinalizeOwnership(context, quality, atlas_budget, transition_started, {MA}, {CA});\n')
add('FinalizeOwnership',P+', double transition_started, '+M+', '+C,final.replace('observe_progress(', 'context.control.ObserveProgress(context, working, atlas_budget, '))
alias={
'world':'context.world','options':'context.options','surface_target':'context.surface_target','geometry':'context.geometry','bidirectional':'context.bidirectional',
'working':'*context.ownership.working','ownership_evaluated':'*context.ownership.ownership_evaluated','quality_ownership_round':'*context.ownership.quality_ownership_round',
'started':'*context.setup.control_prefix.started','stats':'context.control.result->stats','result':'*context.control.result','checkpoint':'*context.control.checkpoint',
'selection':'*context.census.selection','source':'*context.exchange.source','directory':'*context.exchange.directory',
'geometry_authority':'*context.control.geometry_authority','coarsening':'*context.control.coarsening','repairing':'*context.control.repairing',
'wall_counters':'*context.setup.wall_counters','field_identity':'*context.census.field_identity','initial':'*context.census.initial','seed_slot':'*context.census.seed_slot',
'inactive_blocked_edges':'*context.queues.inactive_blocked_edges','inactive_blocked_links':'*context.queues.inactive_blocked_links','length_blocks_dirty':'*context.queues.length_blocks_dirty',
'retired_edges':'*context.census.retired_edges','campaign_edges':'*context.census.campaign_edges','construction_claims':'*context.exchange.construction_claims'}
ret={'WithinTime':'bool','PhaseOwnership':'bool','EstimateOwnership':'bool','FillOwnershipCosts':'void','AccumulateOwnershipCell':'void','OwnershipGeometryCost':'double','NormalizeOwnershipWeights':'void','ShouldMigrate':'bool','MigrateOwnership':'bool','MigrateCertificates':'bool','RestoreOwnership':'bool','FinalizeOwnership':'bool'}
headers=[]; defs=[]; aliases={}
for name,(sig,body) in methods.items():
 # Only identifiers actually referenced outside string literals/comments need a borrowed alias.
 words=set(re.findall(r'\b\w+\b',mask(body))); used={k:v for k,v in alias.items() if k in words}
 aliases[name]=used
 headers.append('  '+ret[name]+' '+name+'('+sig+');')
 defs.append(ret[name]+' EngineOwnership::'+name+'('+sig+') {\n'+''.join(('  const auto& ' if k == 'started' else '  auto& ')+k+' = '+v+';\n' for k,v in used.items())+body+'}\n')
hpp=hpp.replace('struct EngineOwnership {','struct EngineContext;\n\nstruct EngineOwnership {')
hpp=hpp.replace('  EngineOwnership();','\n'.join(headers)+'\n\n  EngineOwnership();')
cpp=cpp.replace('// Owns only empty optional slots; Adapt-local ContextValues govern value lifetimes.', '// Executes ownership transitions using canonical borrowed state and original transaction lifetimes.')
cpp=fmt(cpp.replace('#include "EngineOwnership.hpp"','#include "EngineOwnership.hpp"\n#include "EngineReports.hpp"')+'\n'+'\n'.join(defs)); hpp=fmt(hpp)
newmain=main[:start]+main[end+3:]
assert newmain.count('phase_ownership(true)')==2 and newmain.count('phase_ownership(false)')==1
newmain=newmain.replace('phase_ownership(true)','context.ownership.PhaseOwnership(context, true, atlas_budget)').replace('phase_ownership(false)','context.ownership.PhaseOwnership(context, false, atlas_budget)')
reverse=newmain.replace('context.ownership.PhaseOwnership(context, true, atlas_budget)','phase_ownership(true)').replace('context.ownership.PhaseOwnership(context, false, atlas_budget)','phase_ownership(false)')
assert reverse[:start]+main[start:end+3]+reverse[start:]==main
for n,s in [('CNativeEngine3D.cpp',newmain),('EngineOwnership.hpp',hpp),('EngineOwnership.cpp',cpp)]: (OUT/(n+'.proposed')).write_text(s)
(OUT/'DRAFT_BINDINGS.json').write_text(json.dumps({'UNAPPLIED':True,'base':COMMIT,'source_sha256':sha(main),'main_byte_reversal':True,'aliases':aliases,'observer_binding':'existing build555 EngineControl::ObserveProgress(context, working, atlas_budget, stage)','compiled':False},indent=2)+'\n')
print('wrote proposal; proof and line audit still required')
# Proof works from the FINAL formatted output: strip exactly the audited generated aliases,
# then reverse each explicit extraction and compare to the removed source tokens.
def extract(name):
 match=re.search(r'\b(?:bool|void|double) EngineOwnership::'+name+r'\s*\([^;]*?\)\s*\{',mask(cpp),re.S)
 assert match,name
 i=match.end()-1; j=close(cpp,i); text=cpp[i+1:j]
 expected=aliases[name]
 for k,v in expected.items():
  pat=r'^\s*(?:const\s+)?auto\s*&\s*'+k+r'\s*=\s*'+re.escape(v).replace(r'\ ',r'\s*')+r'\s*;'
  text,n=re.subn(pat,'',text,count=1);assert n,(name,k,text[:200])
 return text,cpp[match.start():j+1].count('\n')+1
actual={}; counts={}
for name in methods: actual[name],counts[name]=extract(name)
assert all(n<=80 for n in counts.values()),counts
def replace_once(s,pat,repl):
 s,n=re.subn(pat,lambda m:repl,s,count=1,flags=re.S);assert n,pat;return s
# Recursively inline final generated tail bodies.
for parent,child in [('RestoreOwnership','FinalizeOwnership'),('MigrateCertificates','RestoreOwnership'),('MigrateOwnership','MigrateCertificates'),('EstimateOwnership','MigrateOwnership'),('PhaseOwnership','EstimateOwnership')]:
 actual[parent]=replace_once(actual[parent],r'return\s+'+child+r'\s*\([^;]*\)\s*;',actual[child])
# Reverse geometry-value extraction and per-cell early-return mapping.
g=actual['OwnershipGeometryCost']; g=replace_once(g,r'return\s+geometry_cost\s*;','')
c=replace_once(actual['AccumulateOwnershipCell'],r'const\s+double\s+geometry_cost\s*=\s*OwnershipGeometryCost\s*\([^;]*\)\s*;',g)
c=replace_once(c,r'(local_demand\s*\+=\s*1\s*;)\s*return\s*;',r'\1 continue;') if False else re.sub(r'(local_demand\s*\+=\s*1\s*;)\s*return\s*;',r'\1 continue;',c,count=1)
f=replace_once(actual['FillOwnershipCosts'],r'AccumulateOwnershipCell\s*\([^;]*\)\s*;',c)
p=replace_once(actual['PhaseOwnership'],r'FillOwnershipCosts\s*\([^;]*\)\s*;',f)
p=replace_once(p,r'NormalizeOwnershipWeights\s*\([^;]*\)\s*;',actual['NormalizeOwnershipWeights'])
s=actual['ShouldMigrate'];s=replace_once(s,r'return\s+true\s*;\s*$','');s=replace_once(s,r'return\s+false\s*;', 'return true;')
p=replace_once(p,r'if\s*\(\s*!ShouldMigrate\s*\([^;]*?\)\s*\)\s*return\s+true\s*;',s)
p=re.sub(r'WithinTime\s*\(\s*context\s*\)', 'within_time()',p)
p=re.sub(r'SU2Native3D\s*::\s*PhaseOwnership\s*::\s*(Disabled|DiagnosticAlways)', r'PhaseOwnership::\1',p)
p='auto within_time=[&]() {'+actual['WithinTime']+'};'+p
actual_tokens=toks(p); expected_tokens=toks(original)
if actual_tokens!=expected_tokens:
 i=next((i for i,(a,b) in enumerate(zip(actual_tokens,expected_tokens)) if a!=b),min(len(actual_tokens),len(expected_tokens)))
 raise AssertionError((i,actual_tokens[i:i+20],expected_tokens[i:i+20],len(actual_tokens),len(expected_tokens)))
manifest=json.loads((OUT/'DRAFT_BINDINGS.json').read_text()); manifest.update({'actual_generated_body_tokens_equal_after_recursive_inverse':True,'body_token_sha256':sha(' '.join(expected_tokens)),'formatted_method_lines':counts,'whole_main_byte_reverse':True,'proof_scope':'actual final generated member bodies and explicit inverse mappings','risk':'B5 accepted; B6 remains unapplied and not compiled or numerically qualified'})
(OUT/'DRAFT_BINDINGS.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('actual body token proof and method line limits passed',counts)
# clang-format the entire three final files in memory and inventory function/lambda bodies.
# The lexical inventory deliberately over-reports bracket/brace candidates; it is not a C++ parser.
inventory={}
for filename,source_text in [('CNativeEngine3D.cpp',newmain),('EngineOwnership.hpp',hpp),('EngineOwnership.cpp',cpp)]:
 text=fmt(source_text); masked=mask(text); entries=[]
 pattern=re.compile(r'(\[[^\]\n]*\]\s*(?:\([^;{}]*?\))?\s*(?:mutable\s*)?(?:->\s*[^{};]+)?|(?:[\w:~]+)\s*\([^;{}]*?\)\s*(?:const\s*)?(?:noexcept\s*)?)\s*\{',re.S)
 for item in pattern.finditer(masked):
  label=' '.join(item.group(1).split()); head=label.split('(')[0].strip()
  if head in {'if','for','while','switch','catch'}: continue
  opening=item.end()-1
  try: ending=close(text,opening)
  except ValueError: continue
  entries.append({'signature':label[:180], 'line':text.count('\n',0,item.start())+1,'formatted_lines':text[item.start():ending+1].count('\n')+1,'lambda':label.startswith('[')})
 inventory[filename]=entries
(OUT/'FULL_FILE_FORMATTED_INVENTORY.json').write_text(json.dumps({'tool':'clang-format-18','scope':'final generated files, full file, lexical function and lambda inventory; includes unchanged pre-existing code','limitations':'lexical scan can misclassify initializer syntax; inspect >80 entries manually','files':inventory},indent=2)+'\n')
print('full-file inventory entries', {k:len(v) for k,v in inventory.items()})
# Check every originally captured context name against each actual body and explicit signature.
for name,(sig,_) in methods.items():
 body,_=extract(name)
 unqualified=re.sub(r'(?:\.|->)\s*\w+', '.', mask(body))
 used=set(re.findall(r'\b\w+\b',unqualified)) & set(alias)
 params=set(re.findall(r'\b\w+\b',sig))
 missing=used-set(aliases[name])-params
 unused=set(aliases[name])-used
 assert not missing and not unused,(name,missing,unused)
print('actual-body capture binding checks passed')
# Compare the final formatted header with the immutable original after removing only
# the added forward declaration and generated method declarations.
base_hpp=read('Common/src/adaptation/native_engine3d/EngineOwnership.hpp')
recovered=hpp.replace('struct EngineContext;','',1)
for name in methods:
 recovered,n=re.subn(r'\b(?:bool|void|double)\s+'+name+r'\s*\([^;]*\)\s*;', '',recovered,count=1,flags=re.S)
 assert n==1,name
assert toks(recovered)==toks(base_hpp),'original ownership field/special-member tokens changed'
for name in ('WithinTime','ShouldMigrate'):
 match=re.search(r'(?:bool|void|double) EngineOwnership::'+name+r'\s*\([^;]*?\)\s*\{',mask(cpp),re.S)
 actual_body=cpp[match.end():close(cpp,match.end()-1)]
 assert re.search(r'\bconst\s+auto\s*&\s*started\s*=\s*\*context\.setup\.control_prefix\.started\s*;',actual_body)
manifest=json.loads((OUT/'DRAFT_BINDINGS.json').read_text())
manifest['header_original_fields_and_special_members_token_equal']=True
manifest['started_borrow_const_as_original']=True
(OUT/'DRAFT_BINDINGS.json').write_text(json.dumps(manifest,indent=2)+'\n')
artifacts=['generate_b6_proposal.py','CNativeEngine3D.cpp.proposed','EngineOwnership.hpp.proposed','EngineOwnership.cpp.proposed','DRAFT_BINDINGS.json','FULL_FILE_FORMATTED_INVENTORY.json','INPUT_SHA_RECEIPT.json','input_CNativeEngine3D.cpp','input_EngineOwnership.hpp','input_EngineOwnership.cpp','input_phase_ownership_frozen_body.cpp.txt']
(OUT/'FROZEN_SHA256.json').write_text(json.dumps({'base_commit':COMMIT,'UNAPPLIED':True,'sha256':{name:hashlib.sha256((OUT/name).read_bytes()).hexdigest() for name in artifacts}},indent=2)+'\n')
print('header field and special-member identity, const borrowed clocks, SHA map passed')
# Accepted B5 commit is the authority; candidate must be byte-identical, not merely equivalent.
source_paths=['Common/src/adaptation/CNativeEngine3D.cpp','Common/src/adaptation/native_engine3d/EngineOwnership.hpp','Common/src/adaptation/native_engine3d/EngineOwnership.cpp']
for path in source_paths:
 data=subprocess.check_output(['git','show',COMMIT+':'+path],cwd=ROOT)
 assert data==(OUT/('input_'+Path(path).name)).read_bytes(),path
expected={'CNativeEngine3D.cpp.proposed':'f82d588aceb84f5a9bbc5258695240743f1df423a78a8fd76f21a975b1620f45','EngineOwnership.hpp.proposed':'c8563750fd9880e3b2d7c576cdf6a3e2df609eb0e54938703574badd481c1492','EngineOwnership.cpp.proposed':'db0b1494a686beb3f26e2173b177d5794a1e1f8da29252769334969e2f1053b8'}
candidate=OUT.parent/'candidate_build555_rebind'
for name,digest in expected.items():
 data=(OUT/name).read_bytes();assert hashlib.sha256(data).hexdigest()==digest and data==(candidate/name).read_bytes(),name
(OUT/'OUTPUT_IDENTITY_RECEIPT.json').write_text(json.dumps({'base_commit':COMMIT,'UNAPPLIED':True,'all_three_candidate_files_byte_identical':True,'sha256':expected},indent=2)+'\n')
frozen=json.loads((OUT/'FROZEN_SHA256.json').read_text())
for name in ('generate_b6_proposal.py','OUTPUT_IDENTITY_RECEIPT.json'):
 frozen['sha256'][name]=hashlib.sha256((OUT/name).read_bytes()).hexdigest()
(OUT/'FROZEN_SHA256.json').write_text(json.dumps(frozen,indent=2)+'\n')
print('accepted commit inputs and all three candidate output bytes verified')
