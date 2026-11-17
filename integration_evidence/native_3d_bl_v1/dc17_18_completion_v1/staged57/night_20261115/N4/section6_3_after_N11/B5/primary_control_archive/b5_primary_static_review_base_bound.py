from pathlib import Path
import ast,re,json,hashlib,subprocess
p=Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11');g=p/'B5_preparation/proposal/generated';source=(p/'B4_preparation/proposal/generate_b4_proposal.py').read_text();env={'re':re,'hashlib':hashlib}
for node in ast.parse(source).body:
 if isinstance(node,ast.FunctionDef) and node.name in ['mask_lex','close_brace','body_tokens']:
  exec(compile(ast.Module(body=[node],type_ignores=[]),'<existing cut proof primitives>','exec'),env)
mask,close,tokens=env['mask_lex'],env['close_brace'],env['body_tokens']
def methods(text):
 masked=mask(text);rows={}
 pattern=r'^(?:static\s+)?(?:void|bool|EngineResult|size_t|std::ostream&)\s+([A-Za-z_]\w*(?:::\w+)*)\s*\([^;{}]*?\)\s*\{'
 for match in re.finditer(pattern,masked,re.M|re.S):
  opening=match.end()-1;closing=close(text,opening);name=match[1];assert name not in rows,name
  rows[name]={'body':text[opening+1:closing],'lines':text[match.start():closing+1].count('\n')+1}
 return rows
frozen=json.loads((g/'frozen_proposal_sha_map.json').read_text())['frozen_proposal_sha256'];sha=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest();assert all(sha(g/name)==h for name,h in frozen.items());line_counts={};old_bodies=[]
for unit in ['EngineControl','EngineReports']:
 old=Path('Common/src/adaptation/native_engine3d')/(unit+'.cpp');new=g/(unit+'.cpp.proposed');before=methods(subprocess.check_output(['git','show','c0486d57daf6ac4a1e66733db4a68b3c2dfe133a:'+str(old)],text=True));after=methods(new.read_text());assert before
 for name,row in before.items():assert tokens(row['body'])==tokens(after[name]['body']),name;old_bodies.append(name)
 for name,row in after.items():assert row['lines']<=80,(name,row['lines']);line_counts[name]=row['lines']
 for suffix in ['hpp']:
  a=subprocess.check_output(['git','show','c0486d57daf6ac4a1e66733db4a68b3c2dfe133a:Common/src/adaptation/native_engine3d/'+unit+'.'+suffix],text=True);b=(g/(unit+'.'+suffix+'.proposed')).read_text();fields=lambda t:[re.sub(r'\s+','',x) for x in re.findall(r'std::optional[^;]+;',t)];assert fields(a)==fields(b),unit
assert 'NativeStageSuffixWork' in line_counts and 'NativeStageSuffix' in line_counts
assert not any(name.startswith('EngineControl::Emit') and name!='EngineControl::EmitFinalRowsAndMove' for name in line_counts)
out={'passed':True,'base_commit':'c0486d57daf6ac4a1e66733db4a68b3c2dfe133a','proposal_sha256':frozen,'original_method_bodies_retained':old_bodies,'original_optional_fields_identical':True,'actual_formatted_function_lines':line_counts,'all_actual_functions_le_80':True,'longest':max(line_counts,key=line_counts.get),'note':'Primary additionally read every new Control/quality/stage worker for aliases, original clocks/queries/elections, synchronous callback, same stdout/flush and original finalization position. Actual recursive body proof remains a separate requirement.'};(g/'primary_static_review_base_bound.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({'passed':True,'functions':len(line_counts),'old_methods_retained':len(old_bodies),'longest':out['longest'],'lines':max(line_counts.values())}))
