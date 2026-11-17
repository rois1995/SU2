from pathlib import Path
import re, json, hashlib, subprocess
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
base=root/'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/B5_preparation/proposal'
out=base/'generated'
commit='c0486d57daf6ac4a1e66733db4a68b3c2dfe133a'
source=subprocess.run(['git','show',f'{commit}:Common/src/adaptation/CNativeEngine3D.cpp'],cwd=root,capture_output=True,text=True,check=True).stdout
assert hashlib.sha256(source.encode()).hexdigest()=='6149e2cb2de3af25feefefdf056ce1c2b4aa75aa6170029348d4eedbffde5b0a'
control=(out/'EngineControl.cpp.proposed').read_text(); reports=(out/'EngineReports.cpp.proposed').read_text()

def lambda_body(name):
 m=re.search(r'^  auto '+re.escape(name)+r'=\[&\].*?\{',source,re.M|re.S); assert m,name
 op=m.end()-1; d=0
 for i in range(op,len(source)):
  if source[i]=='{':d+=1
  elif source[i]=='}':
   d-=1
   if d==0:return source[op+1:i]
 raise AssertionError(name)
def function_body(text, marker):
 for match in re.finditer(re.escape(marker), text):
  a=match.start(); op=text.find('{',a); semi=text.find(';',a)
  if op < 0 or (semi >= 0 and semi < op): continue
  d=0
  for i in range(op,len(text)):
   if text[i]=='{':d+=1
   elif text[i]=='}':
    d-=1
    if d==0:return text[op+1:i]
 raise AssertionError(marker)
def tok(s):
 s=re.sub(r'//[^\n]*|/\*.*?\*/','',s,flags=re.S)
 return re.findall(r'"(?:\\.|[^"\\])*"|\w+|[^\s]',s)
def strip_aliases(body, keep=()):
 lines=body.splitlines(keepends=True); k=0
 while k<len(lines):
  line=lines[k]
  if (not line.strip() or line.lstrip().startswith('using Phase =') or
      re.match(r'\s*(?:(?:const )?auto& (\w+)\s*=|const bool surface_target\s*=)',line) and (not re.match(r'\s*(?:const )?auto& (\w+)\s*=',line) or re.match(r'\s*(?:const )?auto& (\w+)\s*=',line).group(1) not in keep)):
   k+=1; continue
  break
 return ''.join(lines[k:])
def replace_call(tokens, name, replacement, whole_return=False):
 # Replace one full call statement by its final emitted helper body tokens.
 i=0
 while i<len(tokens):
  if tokens[i]==name and i+1<len(tokens) and tokens[i+1]=='(':
   begin=i
   if whole_return and i and tokens[i-1]=='return': begin=i-1
   j=i+1; depth=0
   while j<len(tokens):
    if tokens[j]=='(':depth+=1
    elif tokens[j]==')':
     depth-=1
     if depth==0:break
    j+=1
   end=j+1
   if end<len(tokens) and tokens[end]==';':end+=1
   tokens=tokens[:begin]+replacement+tokens[end:]
   return tokens
  i+=1
 raise AssertionError('call not found: '+name)
# Source segmentation retained from the accepted lambda body.
quality=lambda_body('emit_quality_census')
collect0=quality.index('    std::array<uint64_t,2> q_location')
collect1=quality.index('    stats.quality.diagnostic_seconds+=')
print0=collect1+len('    stats.quality.diagnostic_seconds+=std::chrono::duration<double>(Clock::now()-began).count();\n')
advance=quality.index('    previous_quality=stats.quality;',print0)
collect_src=quality[collect0:collect1]
for declaration in [
 '    std::array<uint64_t,2> q_location{},j_location{};\n',
 '    std::array<uint64_t,3> q_features{},j_features{};\n',
 '    std::array<uint64_t,6> q_hist{},j_hist{},geometry_hist{};\n',
 '    std::array<uint64_t,8> geometry_types{};\n',
 '    uint64_t bad_total=0,bad_short09=0,bad_short05=0,stuck_shape=0,stuck_required=0,missing_features=0,blocked_features=0;\n']:
 collect_src=collect_src.replace(declaration,'')
collect_actual=strip_aliases(function_body(reports,'void EngineReports::CollectQualityCensus('))
assert tok(collect_src)==tok(collect_actual),'final CollectQualityCensus body differs from accepted source slice'
# Print: expand the actual file-local helper into the actual final member body.
print_member=strip_aliases(function_body(reports,'void EngineReports::PrintQualityCensus('),keep=('q','old','seconds'))
detail_body=strip_aliases(function_body(reports,'static void PrintQualityDetails('))
pt=tok(print_member); pt=replace_call(pt,'PrintQualityDetails',tok(detail_body))
print_src=quality[print0:advance]
assert tok(print_src)==pt,'final PrintQualityCensus + PrintQualityDetails expansion differs from accepted source slice'
# Quality orchestration: inline the final generated collection and printing functions.
emit_actual=strip_aliases(function_body(reports,'void EngineReports::EmitQualityCensus('))
et=tok(emit_actual)
et=replace_call(et,'CollectQualityCensus',tok(collect_actual))
et=replace_call(et,'PrintQualityCensus',pt)
assert tok(quality)==et,'final EmitQualityCensus recursive expansion differs from accepted lambda body'
# Stage report output: independently compare exact coverage/counting body and stream insertion chain.
observer=lambda_body('observe_progress')
rows_start=observer.index('      size_t face_pieces=0,trace_pieces=0,limbs=0;')
rows_edge=observer.index('      if(std::string(stage)=="length_split_exit")',rows_start)
rows_end=observer.index("<<'\\n';",rows_edge)+len("<<'\\n';")
rows=observer[rows_start:rows_end]
count_end=rows.index('      const auto g=')
coverage_source=rows[:count_end]
g_snapshot_source=rows[count_end:rows.index('      std::cout<<std::setprecision(17)<<"NATIVE3D_STAGE')]
stage_start=rows.index('      std::cout<<std::setprecision(17)<<"NATIVE3D_STAGE')
field_marker='          <<" guidance_seconds="<<stats.geometry_guidance_seconds'
stage_p0=rows.index(field_marker,stage_start)+len(field_marker)
stage_end=rows.index('<<std::endl;',stage_p0)+len('<<std::endl;')
source_prefix=rows[stage_start:stage_p0]
source_suffix=rows[stage_p0:stage_end]
source_edges=rows[stage_end:]
stage_method=strip_aliases(function_body(reports,'void EngineReports::EmitStageRows('))
# Actual face/trace scan and geometry snapshot must equal source code.
stage_tok=tok(stage_method)
source_cov_tok=tok(coverage_source+g_snapshot_source)
# Keep the method's exact initial code through geometry snapshot.
method_prefix=stage_method[:stage_method.index('  auto& out = NativeStagePrefix')]
assert tok(method_prefix)==source_cov_tok,'final EmitStageRows coverage and snapshot code differs from source'
# Stream fragments come from the final formatted helpers. Remove only the shared stream receiver and helper boundary.
prefix_body=function_body(reports,'static std::ostream& NativeStagePrefix(')
prefix_output=prefix_body[prefix_body.index('  std::cout <<'):prefix_body.index('  return std::cout;')].strip()
suffix_body=function_body(reports,'static void NativeStageSuffix(')
suffix_out=suffix_body[suffix_body.index('  out <<'):suffix_body.index('  NativeStageSuffixWork(')].strip()
work_body=function_body(reports,'static void NativeStageSuffixWork(')
work_out=work_body[work_body.index('  out <<'):].strip()
# Chain receiver and split terminators are the sole normalization; no flush/query operation is removed.
def stream_norm(text):
 text=re.sub(r'\bout\b','std::cout',text)
 text=re.sub(r';\s*std::cout\s*<<',' <<',text)
 return tok(text)
assert tok(prefix_output.rstrip('; '))==tok(source_prefix.rstrip('; ')), 'final stage prefix stream differs'
actual_suffix=stream_norm(suffix_out+work_out); actual_suffix=actual_suffix[4:]
source_suffix_tokens=tok(source_suffix)
assert actual_suffix==source_suffix_tokens, 'final stage suffix stream differs from source suffix'
# Edge rows remain directly in final member and must match the accepted source slice.
actual_edge=stage_method[stage_method.index('  if (std::string(stage) == "length_split_exit")'):]
assert tok(actual_edge)==tok(source_edges),'final edge rows differ from source slice'
stage_receipt={'coverage_count_and_geometry_snapshot_token_equal':True,'prefix_stream_exact':True,'suffix_stream_exact':True,'edge_rows_exact':True,'normalization':'only out/std::cout receiver and split `; out <<` continuation boundaries','no_new_flush_or_geometry_snapshot':True}
# Observer callback tail expands from the actual final helper.
obs_actual=strip_aliases(function_body(control,'bool EngineControl::ObserveProgress('))
notify_actual=strip_aliases(function_body(control,'bool EngineControl::NotifyStageObserver('))
notify_src=observer[observer.index('    if(!observe_stage || !stage_storage_admitted || stage_observer_failed)'):]
nt=tok(obs_actual)
nt=replace_call(nt,'NotifyStageObserver',tok(notify_actual),whole_return=True)
# Normalize only the member qualification for calls replacing local closure calls.
for member in ('EmitStageRows','EmitQualityCensus'):
 ns=['EngineReports',':',':',member]
 for i in range(len(nt)-3):
  if nt[i:i+4]==ns:
   nt=nt[:i]+nt[i+3:]
   break
# Stage row helper is separately proven above; use its verified expansion.
nt=replace_call(nt,'EmitStageRows',tok(rows))
# Quality census implementation is independently proven against the source lambda.
nt=replace_call(nt,'EmitQualityCensus',tok('emit_quality_census(stage,census);'))
assert nt==tok(observer),'final ObserveProgress + NotifyStageObserver expansion differs from accepted lambda body'
# Finish helper expansion from actual final member files.
finish_src=lambda_body('finish')
ft=tok(strip_aliases(function_body(control,'EngineResult EngineControl::Finish(')))
helper_specs=[
 ('ProveEntitySurface', 'void EngineControl::ProveEntitySurface('),
 ('ProvePreparedSurface','void EngineControl::ProvePreparedSurface('),
 ('ProveFinalHeights','void EngineControl::ProveFinalHeights('),
 ('ObserveRejectedState','void EngineControl::ObserveRejectedState('),
 ('CaptureFinalCounts','void EngineControl::CaptureFinalCounts('),
 ('CaptureDirectoryDonorStatistics','void EngineControl::CaptureDirectoryDonorStatistics('),
 ('EnrichFinalReasonAndTiming','void EngineControl::EnrichFinalReasonAndTiming('),
 ('EmitFinalRowsAndMove','EngineResult EngineControl::EmitFinalRowsAndMove(')]
for name,marker in helper_specs:
 hb=strip_aliases(function_body(control,marker))
 ft=replace_call(ft,name,tok(hb),whole_return=(name=='EmitFinalRowsAndMove'))
# Restore the original closure call for the terminal exit progress observation.
ft=replace_call(ft,'ObserveProgress',tok('observe_progress("exit");'))
assert ft==tok(finish_src),'final Finish recursive expansion differs from accepted lambda body'
# Refresh final proof receipt only after every actual-file comparison passed.
receipt={'source_commit':commit,'source_sha256':hashlib.sha256(source.encode()).hexdigest(),'final_generated_file_sha256':{f:hashlib.sha256((out/f).read_bytes()).hexdigest() for f in ['EngineControl.cpp.proposed','EngineReports.cpp.proposed','EngineControl.hpp.proposed','EngineReports.hpp.proposed','CNativeEngine3D.control.proposed.cpp']},'actual_formatted_helper_expansion':{'quality_collect_print_emit':True,'observer_plus_notify_callback':True,'stage_rows_prefix_suffix_and_edges':stage_receipt,'finish_all_helpers':True},'main_reversal_byte_equal':json.loads((out/'main_reversal_proof.json').read_text())['full_main_reversal_byte_equal'],'aliases_removed':'only leading `using Phase`, `auto&` and `const auto&` bindings reviewed to direct context slots or explicit Adapt-local parameters','status':'UNAPPLIED; source frozen for root review; no compile or tests'}
(out/'final_output_expansion_proof.json').write_text(json.dumps(receipt,indent=2)+'\n')
