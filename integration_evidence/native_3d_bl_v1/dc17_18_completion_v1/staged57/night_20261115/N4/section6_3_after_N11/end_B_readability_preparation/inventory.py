#!/usr/bin/env python3
"""Read-only lexical inventory of formatted native-engine definitions; no build or source writes."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess
ROOT=Path.cwd()
P=ROOT/'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11'
OUT=P/'end_B_readability_preparation'
PRIVATE=ROOT/'Common/src/adaptation/native_engine3d'
MAIN=ROOT/'Common/src/adaptation/CNativeEngine3D.cpp'
parser=argparse.ArgumentParser();parser.add_argument('--draft-overlay',action='store_true',help='use unapplied B6/B7/B8 candidate files for review only');args=parser.parse_args()
OVER={}
if args.draft_overlay:
 b6=P/'B6_preparation/proposal/accepted_B5_rebind';b7=P/'B7_preparation/proposal/candidate_after_B6_build555/generated';b8=P/'B8_preparation/proposal/integrated_candidate_after_B7'
 OVER={str(MAIN):b8/'CNativeEngine3D.integrated.cpp.proposed',str(PRIVATE/'EngineSetup.hpp'):b8/'EngineSetup.integrated.hpp.proposed',str(PRIVATE/'EngineSetup.cpp'):b8/'EngineSetup.integrated.cpp.proposed',str(PRIVATE/'EngineControl.hpp'):b7/'EngineControl.hpp.proposed',str(PRIVATE/'EngineControl.cpp'):b7/'EngineControl.cpp.proposed',str(PRIVATE/'EngineOwnership.hpp'):b6/'EngineOwnership.hpp.proposed',str(PRIVATE/'EngineOwnership.cpp'):b6/'EngineOwnership.cpp.proposed'}
expected={'CNativeEngine3D.integrated.cpp.proposed':'0afc267191212edb36b6dc57c5921ad155bdc1a4d256fe9eb8c2bb7756965083','EngineSetup.integrated.hpp.proposed':'9c13988eedb1727220ea4e07f9df160027080359cc24c5780211975917c3d953','EngineSetup.integrated.cpp.proposed':'3bebe80761c1d1005984b07193fc67fe66736cd475281645bd7ae1db0e495b27'}
LEX=re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',re.S)
def mask(text):return LEX.sub(lambda m:''.join('\n' if c=='\n' else ' ' for c in m[0]),text)
def pairs(text):
 stacks={'(':[], '{':[], '[':[]}; rev={')':'(', '}':'{', ']':'['};out={}
 for i,c in enumerate(text):
  if c in stacks:stacks[c].append(i)
  elif c in rev and stacks[rev[c]]:
   j=stacks[rev[c]].pop();out[j]=i;out[i]=j
 return out
CONTROL={'if','for','while','switch','catch','sizeof','alignof','decltype','static_assert','noexcept','requires','return','throw','assert'}
def signature_start(text,start):
 pos=text.rfind('\n',0,start)+1
 # Include adjacent template or isolated return-type lines, preserving formatted indentation.
 while pos:
  prev_end=pos-1;prev_start=text.rfind('\n',0,prev_end)+1;line=text[prev_start:prev_end].strip()
  if line.startswith('template ') or line.startswith('template<') or re.fullmatch(r'(?:static\s+)?(?:inline\s+)?(?:void|bool|auto|int|double|size_t|[\w:]+(?:<[^;{}]+>)?[&*]?)',line):pos=prev_start
  else:break
 return pos
def scan(source,label):
 text=mask(source); links=pairs(text);functions=[];lambdas=[];uncertain=[]
 # Lambda syntax, including parameterless [] { ... } and trailing return clauses.
 lp=re.compile(r'\[(?!\[)[^\]\n]*\]\s*(?:\([^;{}]*?\))?\s*(?:(?:mutable|constexpr|consteval|noexcept)\s*)*(?:->\s*[^;{}]+)?\{',re.S)
 lambda_open=set()
 for m in lp.finditer(text):
  op=m.end()-1
  if op not in links:continue
  cl=links[op];lambda_open.add(op);lambdas.append({'file':label,'line':source.count('\n',0,m.start())+1,'formatted_lines':source[m.start():cl+1].count('\n')+1,'signature':' '.join(source[m.start():op].split())[:180]})
 # A definition requires an identifier/declarator and a body immediately after its matched parameter list.
 for m in re.finditer(r'\(',text):
  op=m.start();cl=links.get(op)
  if cl is None:continue
  left=max(0,op-256);before=text[left:op];nm=re.search(r'(?P<name>(?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*|operator\s*(?:\(\)|\[\]|[^\s(]+))\s*$',before)
  if not nm:continue
  name=nm.group('name');short=name.split('::')[-1]
  if short in CONTROL:continue
  start=left+nm.start();line_start=text.rfind('\n',0,start)+1;prefix=text[line_start:start]
  if re.search(r'(?:\.|->)\s*$',prefix):continue
  if re.search(r'\b(?:return|throw|if|for|while|switch|catch|new|delete)\b|[=;{}]',prefix):continue
  tail=text[cl+1:];q=re.match(r'\s*(?:(?:const|volatile|override|final|noexcept|constexpr)\s*)*(?:->\s*[^;{}]+)?\{',tail)
  if not q:
   # Constructor initializer clauses: name(args) or name{args}, comma separated,
   # followed by the actual function body. Other colon constructs remain a manual audit.
   colon=re.match(r'\s*:',tail)
   parts=name.split('::');is_ctor=(len(parts)>1 and parts[-1]==parts[-2]) or (len(parts)==1 and re.fullmatch(r'[A-Z]\w*',name) and not prefix.strip())
   body=None
   if colon and is_ctor:
    cursor=cl+1+colon.end()
    while cursor<len(text):
     item=re.match(r'\s*(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*\s*',text[cursor:])
     if not item:break
     cursor+=item.end()
     if cursor>=len(text) or text[cursor] not in '({':break
     end=links.get(cursor)
     if end is None:break
     cursor=end+1
     while cursor<len(text) and text[cursor].isspace():cursor+=1
     if cursor<len(text) and text[cursor]==',':cursor+=1;continue
     if cursor<len(text) and text[cursor]=='{':body=cursor
     break
   if body is None:
    if colon and is_ctor:uncertain.append({'file':label,'line':source.count('\n',0,start)+1,'name':name,'reason':'constructor initializer list could not be parsed'})
    continue
  else:body=cl+q.end()
  if body not in links or body in lambda_open:continue
  # Bare unqualified names with no same-line type are accepted only for constructors/destructors.
  if not prefix.strip() and '::' not in name and not re.match(r'~?[A-Z]',name):continue
  end=links[body];begin=signature_start(source,start)
  functions.append({'file':label,'name':name,'line':source.count('\n',0,begin)+1,'formatted_lines':source[begin:end+1].count('\n')+1,'signature':' '.join(source[begin:body].split())[:240]})
 # Defaulted/deleted special members are definitions too, even without bodies.
 special=re.compile(r'(?m)^[ \t]*(?:(?:explicit|virtual|constexpr|inline)\s+)*(?P<name>(?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*|operator\s*[^\s(]+)\s*\([^;{}]*?\)\s*(?:const\s*)?(?:noexcept\s*)?=\s*(?P<kind>default|delete)\s*;',re.S)
 for match in special.finditer(text):
  begin=signature_start(source,match.start()); end=match.end()
  functions.append({'file':label,'name':match.group('name'),'line':source.count('\n',0,begin)+1,'formatted_lines':source[begin:end].count('\n')+1,'signature':' '.join(source[begin:end].split())[:240],'definition_kind':match.group('kind')})
 return functions,lambdas,uncertain
files=[MAIN]+sorted(p for p in PRIVATE.iterdir() if p.suffix in ('.hpp','.cpp'))
functions=[];lambdas=[];uncertain=[];inputs=[]
for path in files:
 target=OVER.get(str(path),path);data=target.read_bytes(); digest=hashlib.sha256(data).hexdigest()
 if target.name in expected:assert digest==expected[target.name],(target,digest)
 formatted=subprocess.run(['clang-format-18','--assume-filename='+str(path)],input=data,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True).stdout.decode()
 label=str(path.relative_to(ROOT)); fs,ls,us=scan(formatted,label);functions+=fs;lambdas+=ls;uncertain+=us
 inputs.append({'logical_file':label,'read_from':str(target),'sha256':digest,'candidate':str(path) in OVER,'formatted_sha256':hashlib.sha256(formatted.encode()).hexdigest()})
# The function inventories are lexical candidates; ownership is restricted to the new units.
new=[x for x in functions if '/native_engine3d/' in x['file']]
new_long=[x for x in new if x['formatted_lines']>80]
new_lambdas=[x for x in lambdas if '/native_engine3d/' in x['file']]
long_lambdas=[x for x in new_lambdas if x['formatted_lines']>15]
wrapper=[x for x in lambdas if x['file'].endswith('CNativeEngine3D.cpp') and 'return' not in x['signature'] and x['signature'].startswith('[&]')]
result={'status':'DRAFT_SYNTHETIC_B8_OVERLAY_NOT_ACCEPTED' if args.draft_overlay else 'LIVE_READ_ONLY','inputs':inputs,'functions_count':len(functions),'lambdas_count':len(lambdas),'ten_longest_functions':sorted(functions,key=lambda x:x['formatted_lines'],reverse=True)[:10],'private_units_functions_over_80':new_long,'private_units_lambdas_over_15':long_lambdas,'all_function_candidates':functions,'all_lambda_candidates':lambdas,'constructor_initializer_candidates_manual_review':uncertain,'written_exceptions':['EngineLegacyWall: explicit >80-line exception in CutPlan section 3 and source_check.md; no presumption that 8.4 has retired it.','Adapt: <=80 lines is a phase-D completion requirement, not an end-of-B gate.','Pre-existing Selection and MeshOptimizer bodies remain outside this cut; old main-file functions are baseline, not new-file exemptions.'],'detection_limitations':['Lexical scanner, not a compiler or full C++ parser; templates, requires clauses, macros, function-try-blocks, operator forms, raw strings, conditional preprocessing and brace-initialized constructor members may be missed or misclassified.','Reports potential constructor initializer lists separately for manual inspection; call/definition distinction is heuristic.','Formatted lengths count from detected signature line (and contiguous template/return-type line) through its closing brace; actual compiler scope is not proved.','Lambdas in unmodified main are inventoried but are not automatically classified as new; review candidate B8 setup_finish against the 15-line rule separately.','Private-unit overlay is synthetic until B6/B7/B8 are accepted; do not treat it as production or gate evidence.']}
if args.draft_overlay:
 b6lines=json.loads((P/'B6_preparation/proposal/accepted_B5_rebind/DRAFT_BINDINGS.json').read_text())['formatted_method_lines']
 b7rows=json.loads((P/'B7_preparation/proposal/candidate_after_B6_build555/generated/binding_manifest.json').read_text())['method_lines']
 b8lines=json.loads((P/'B8_preparation/proposal/integrated_candidate_after_B7/INTEGRATED_PROOF.json').read_text())['formatted_method_lines']
 stage_checks={}
 for stage,unit,names in [('B6','EngineOwnership.cpp',set(b6lines)),('B7','EngineControl.cpp',{x['name'] for x in b7rows}),('B8','EngineSetup.cpp',set(b8lines))]:
  rows=[x for x in functions if x['file'].endswith('/'+unit) and x['name'].split('::')[-1] in names]
  found={x['name'].split('::')[-1] for x in rows};assert found==names,(stage,sorted(names-found))
  stage_checks[stage]={'expected_methods':len(names),'detected_methods':len(rows),'methods_over_80':[x for x in rows if x['formatted_lines']>80]}
  assert not stage_checks[stage]['methods_over_80'],stage
 # The one new main lambda is identified by its initializer, not by matching every [&].
 formatted_main=subprocess.run(['clang-format-18','--assume-filename='+str(MAIN)],input=OVER[str(MAIN)].read_bytes(),stdout=subprocess.PIPE,check=True).stdout.decode()
 sm=re.search(r'auto\s+setup_finish\s*=\s*(\[&\]\s*\([^)]*\)\s*\{)',formatted_main);assert sm
 opening=sm.end()-1;closing=pairs(mask(formatted_main))[opening]
 stage_checks['B8_main_setup_finish']={'formatted_lines':formatted_main[sm.start(1):closing+1].count('\n')+1}
 assert stage_checks['B8_main_setup_finish']['formatted_lines']<=15
 result['known_B6_B7_B8_candidate_checks']=stage_checks
name='draft_synthetic_inventory.json' if args.draft_overlay else 'live_inventory.json';(OUT/name).write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'report':str(OUT/name),'functions':len(functions),'lambdas':len(lambdas),'new_functions_over_80':len(new_long),'new_lambdas_over_15':len(long_lambdas),'constructor_candidates':len(uncertain),'top_ten':result['ten_longest_functions']},indent=2))
