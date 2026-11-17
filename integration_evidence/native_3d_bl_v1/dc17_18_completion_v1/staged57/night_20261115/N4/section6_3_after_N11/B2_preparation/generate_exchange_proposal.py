from pathlib import Path
import re,json,hashlib,subprocess,time
root=Path.cwd();p=root/'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11';out=p/'B2_preparation/generated';out.mkdir(exist_ok=True)
base=root/'Common/src/adaptation/CNativeEngine3D.cpp';source=base.read_text();assert hashlib.sha256(source.encode()).hexdigest()=='4dd7a5bc894a510a55a537d8e52484f8e8e3fcd23c79b8247ee835d08f022ff0'
lex=re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',re.S)
def mask(s):return lex.sub(lambda m: ''.join('\n' if c=='\n' else ' ' for c in m[0]),s)
def close_at(s,opening):
 m=mask(s);depth=0
 for i in range(opening,len(s)):
  if m[i]=='{':depth+=1
  elif m[i]=='}':
   depth-=1
   if depth==0:return i
 raise AssertionError('unclosed body')
def span(s):
 a=s.index('EngineResult Adapt(');setup=s.index('  EngineSetup setup;',a);opening=s.rfind('{',a,setup);return opening,close_at(s,opening)
start,end=span(source);assert 'R"' not in source[start:end]
names={'elect_trial_failure':'ElectTrialFailure','vote':'Vote','bytes':'Bytes','real_bytes':'RealBytes','admit':'Admit','import_source':'ImportSource'}
removed=[]
for name in names:
 match=re.search(r'^  auto '+name+r'=\[&\].*?\{',source[start:end],re.M|re.S);assert match,name
 a=start+match.start();opening=start+match.end()-1;closing=close_at(source,opening);assert source[closing:closing+3]=='};\n'
 removed.append({'name':name,'start':a,'end':closing+3,'old':source[a:closing+3],'body':source[opening+1:closing]})
cpp=source
for r in sorted(removed,key=lambda r:r['start'],reverse=True):cpp=cpp[:r['start']]+cpp[r['end']:]
old_callback='EngineReports::EmitFixedMetricDrift(context, stage, bytes)';new_callback='EngineReports::EmitFixedMetricDrift(context, stage)';assert cpp.count(old_callback)==1;cpp=cpp.replace(old_callback,new_callback)
pre_route=cpp;a,b=span(cpp);masked=mask(cpp);changes=[]
for match in re.finditer(r'(?<![\w:.])\b('+ '|'.join(names)+r')\s*\(',masked[a:b]):
 lo=a+match.start();hi=a+match.end();name=match[1];j=hi
 while masked[j].isspace():j+=1
 if masked[j]==')':hi=j+1;new=f'context.exchange.{names[name]}(context)'
 else:new=f'context.exchange.{names[name]}(context, '
 changes.append({'name':name,'start':lo,'end':hi,'old':cpp[lo:hi],'new':new})
pieces=[];offset=0
for change in changes:
 pieces.append(cpp[offset:change['start']]);change['new_start']=sum(len(x) for x in pieces);pieces.append(change['new']);offset=change['end']
pieces.append(cpp[offset:]);cpp=''.join(pieces)
reverse=cpp
for change in reversed(changes):
 pos=change['new_start'];assert reverse[pos:pos+len(change['new'])]==change['new'];reverse=reverse[:pos]+change['old']+reverse[pos+len(change['new']):]
assert reverse==pre_route
reverse=reverse.replace(new_callback,old_callback,1)
for r in sorted(removed,key=lambda r:r['start']):reverse=reverse[:r['start']]+r['old']+reverse[r['start']:]
assert reverse==source
# Use the typed draft only as a method spelling template; literal body proofs below are required.
draft=(p/'B2_preparation/draft/B2_EngineExchange_methods.proposed.md').read_text();fences=re.findall(r'```cpp\n(.*?)```',draft,re.S);assert len(fences)==2;defs=fences[1]
defs=defs.replace('const auto& private_repair = *context.exchange.private_repair;','auto& private_repair = *context.exchange.private_repair;')
defs=defs.replace('const bool atlas_geometry = bool(*context.setup.atlas_geometry);','auto& atlas_geometry = *context.setup.atlas_geometry;')
defs=defs.replace('  (void)requests;  // Preserves the existing ignored requests argument.\n','')
va=defs.index('bool EngineExchange::Vote(');vb=defs.index('size_t EngineExchange::Bytes(',va);defs=defs[:va]+defs[va:vb].replace('  auto& world = context.world;\n','')+defs[vb:]
ba=defs.index('size_t EngineExchange::Bytes(');bb=defs.index('size_t EngineExchange::RealBytes(',ba);part=defs[ba:bb]
for name in ['options','accepted_bytes','batch_bytes','checkpoint','result','working','batch_changed','batch_touched','batch_supports','initial','eligible_extensions','selection','retired_edges','campaign_edges','field_identity','inactive_blocked_edges','inactive_blocked_links']:
 part=part.replace('const auto& '+name+' =','auto& '+name+' =')
defs=defs[:ba]+part+defs[bb:]
files={str(base.relative_to(root)):cpp};directory=root/'Common/src/adaptation/native_engine3d';header=(directory/'EngineExchange.hpp').read_text();header=header.replace('struct EngineExchange {','struct EngineContext;\n\nstruct EngineExchange {',1)
declarations='''  CElectedFailure ElectTrialFailure(EngineContext&, const CLocalFailure&);
  bool Vote(EngineContext&, const CLocalFailure&);
  size_t Bytes(EngineContext&);
  size_t RealBytes(EngineContext&);
  bool Admit(EngineContext&, size_t extra);
  DonorPatch ImportSource(EngineContext&, const std::vector<Cell>& cells, size_t extra, bool layer,
                         const std::vector<unsigned char>* requests = nullptr);
'''
header=header.replace('  void RetireDirectory(const DirectoryStats& old);','  void RetireDirectory(const DirectoryStats& old);\n\n'+declarations)
exchange=(directory/'EngineExchange.cpp').read_text().replace('#include "EngineExchange.hpp"','#include "EngineExchange.hpp"\n#include "EngineContext.hpp"',1)+'\n'+defs
files[str((directory/'EngineExchange.hpp').relative_to(root))]=header;files[str((directory/'EngineExchange.cpp').relative_to(root))]=exchange
for name in ['EngineReports.hpp','EngineReports.cpp']:
 text=(directory/name).read_text();assert text.count('template <class Bytes>\n')==3;text=text.replace('template <class Bytes>\n','')
 if name.endswith('.hpp'):
  assert text.count(', Bytes&')==3;text=text.replace(', Bytes&','')
 else:
  assert text.count(', Bytes& bytes')==3;text=text.replace(', Bytes& bytes','');assert text.count('bytes()')==2;text=text.replace('bytes()','context.exchange.Bytes(context)');text=text.replace('InitializeFixedMetricDrift(context, available, bytes)','InitializeFixedMetricDrift(context, available)').replace('ReadFixedMetricDrift(context, selected, have, available, bytes)','ReadFixedMetricDrift(context, selected, have, available)')
 files[str((directory/name).relative_to(root))]=text
for path,text in files.items():
 dest=out/(Path(path).name+'.proposed');dest.write_text(text)
 if Path(path).name!='CNativeEngine3D.cpp':subprocess.run(['clang-format-18','-i',str(dest)],check=True)
# Prove method operation tokens after removing only reference aliases and direct-call binding prefixes.
def tokens(s):
 s=re.sub(r'//[^\n]*|/\*.*?\*/','',s,flags=re.S);return re.findall(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\w+|[^\s]',s)
formatted=(out/'EngineExchange.cpp.proposed').read_text();proof=[]
for r in removed:
 member=names[r['name']];a=formatted.index('EngineExchange::'+member+'(');opening=formatted.index('{',a);closing=close_at(formatted,opening);body=formatted[opening+1:closing]
 aliases=re.findall(r'\s*(?:const\s+)?auto\s*&\s*\w+\s*=.*?;',body,re.S)
 body_without=body
 for alias in aliases:body_without=body_without.replace(alias,'',1)
 for old,new in [('elect_trial_failure','ElectTrialFailure'),('bytes','Bytes'),('vote','Vote')]:
  body_without=re.sub(r'\b'+new+r'\(context,\s*',old+'(',body_without);body_without=re.sub(r'\b'+new+r'\(context\)',old+'()',body_without)
 assert tokens(body_without)==tokens(r['body']),(r['name'],tokens(body_without),tokens(r['body']))
 proof.append({'lambda':r['name'],'member':member,'reference_aliases':len(aliases),'operation_tokens_identical_after_routing_reversal':True,'formatted_lines':formatted[a:closing+1].count('\n')+1})
 assert proof[-1]['formatted_lines']<=80,proof[-1]
newhash={path:hashlib.sha256((out/(Path(path).name+'.proposed')).read_bytes()).hexdigest() for path in files};oldhash={path:hashlib.sha256((root/path).read_bytes()).hexdigest() for path in files}
manifest={'unapplied':True,'compiled_or_launched':False,'parent_passed_pushed_commit':'c8fd7ff400','base_sha256':oldhash,'proposed_sha256':newhash,'whole_CPP_byte_reversal':True,'removed_lambdas':removed,'call_replacements':changes,'main_call_counts':{name:sum(x['name']==name for x in changes) for name in names},'callback_binding':{'old':old_callback,'new':new_callback,'Reports_two_Bytes_calls_routed_directly':True,'three_Reports_templates_removed_without_new_callback':True},'member_body_proofs':proof,'live_private_repair_and_atlas_geometry_references':True,'all_original_constness_preserved':True,'atlas_budget_selector_unchanged_and_option_pointer_never_assigned':True,'requests_ignored_unchanged':True,'no_live_files_changed':True}
(out/'binding_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps({'prepared_files':len(files),'CPP_reversal':True,'body_proofs':proof,'main_call_counts':manifest['main_call_counts']},indent=2))
