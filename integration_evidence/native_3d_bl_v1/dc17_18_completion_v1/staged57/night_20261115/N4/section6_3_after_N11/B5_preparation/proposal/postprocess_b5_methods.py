from pathlib import Path
import re, json, hashlib, subprocess
root = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
out = root/'integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/B5_preparation/proposal/generated'

def method_span(text, marker):
    start = text.index(marker)
    opening = text.index('{', start)
    depth = 0
    for i in range(opening, len(text)):
        if text[i] == '{': depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0: return start, i+1
    raise RuntimeError(marker)

# Keep the long exact-output tails in same-stream helpers; do not query or flush.
f = out/'EngineReports.cpp.proposed'
s = f.read_text()
# PrintQualityCensus details are moved mechanically from its original timings block.
a,b = method_span(s, 'void EngineReports::PrintQualityCensus(')
m = s[a:b]
q0 = m.index('    auto histogram =')
q1 = m.rfind('\n  }')
details = m[q0:q1]
assert 'std::cout' in details and 'histogram(' in details
print_helper = '''\n\n// Continues the original quality output on the current stdout stream.\nstatic void PrintQualityDetails(EngineContext& context, uint64_t quality_census_serial,\n    const QualityRepairStats& q, const QualityRepairStats& old,\n    const std::array<uint64_t, 2>& q_location, const std::array<uint64_t, 2>& j_location,\n    const std::array<uint64_t, 3>& q_features, const std::array<uint64_t, 3>& j_features,\n    const std::array<uint64_t, 6>& q_hist, const std::array<uint64_t, 6>& j_hist,\n    const std::array<uint64_t, 6>& geometry_hist, const std::array<uint64_t, 8>& geometry_types,\n    uint64_t bad_total, uint64_t bad_short09, uint64_t bad_short05, uint64_t stuck_shape,\n    uint64_t stuck_required, uint64_t missing_features, uint64_t blocked_features) {\n  auto& world = context.world;\n'''+details+'\n}\n'
call='''    PrintQualityDetails(context, quality_census_serial, q, old, q_location, j_location, q_features, j_features,\n                       q_hist, j_hist, geometry_hist, geometry_types, bad_total, bad_short09, bad_short05,\n                       stuck_shape, stuck_required, missing_features, blocked_features);'''
m = m[:q0]+call+m[q1:]
if 'auto& options = context.options;' not in m: m=m.replace('  auto& world = context.world;\n','  auto& world = context.world;\n  auto& options = context.options;\n',1)
s=s[:a]+print_helper+m+s[b:]
# Split NativeStageSuffix at a complete field boundary, preserving stream adjacency.
a,b = method_span(s, 'static void NativeStageSuffix(')
m = s[a:b]
marker='      << " smooth_boundary_stars="'
k = m.index(marker)
end = len(m)-1
chunk=m[k:end]
assert chunk.rstrip().endswith(';')
chunk=chunk.replace(marker, '  out << " smooth_boundary_stars="', 1)
# The existing prefix chain ends immediately before the moved field chain.
first=m[:k].rstrip()+';\n  NativeStageSuffixWork(out, context, started, edge_census_round);\n'
first=first.replace('  const auto& started = *context.setup.control_prefix.started;\n','')
first=first.replace('  auto& edge_census_round = *context.census.edge_census_round;\n','')
first=first.rstrip()+'\n}\n'
work='''\n\n// Continues the original stage row on the same stream without a flush or new query.\nstatic void NativeStageSuffixWork(std::ostream& out, EngineContext& context, const Clock::time_point& started,\n                                  size_t edge_census_round) {\n  auto& stats = context.control.result->stats;\n'''+chunk+'}\n'
s=s[:a]+first+work+s[b:]
# Give the integrated helper files role-based comments, not draft-stage labels.
s=s.replace('// Provisional B5 print helpers. They retain the original stream and row order.','// Stage report helpers retain the original stream and row order.')
s=s.replace('// PROVISIONAL B5 additions; preserve SelectionBound above.','// Terminal control methods borrow the live Adapt state.')
f.write_text(s)
# Fix the generated rejected-state alias omitted by the extraction alias list.
c = out/'EngineControl.cpp.proposed'; cs=c.read_text().replace('// Applies the original control budget leaf; never changes a bound or owns a second state.','// Owns observer and finalization flow; borrows canonical state and adds no policy.',1); needle='void EngineControl::ObserveRejectedState(EngineContext& context, Owned& working) {\n'
if 'auto& observe_state = *context.control.observe_state;' not in cs[cs.index(needle):cs.index(needle)+300]:
 cs=cs.replace(needle,needle+'  auto& observe_state = *context.control.observe_state;\n',1)
cs=cs.replace('const const auto& started =','const auto& started =')
cs=cs.replace('// PROVISIONAL B5 additions; preserve SelectionBound above.','// Terminal control methods borrow the live Adapt state.')
c.write_text(cs)
# Format complete proposal translation units.
for name in ['EngineControl.cpp.proposed','EngineReports.cpp.proposed']:
 subprocess.run(['clang-format-18','-i',str(out/name)], check=True)
# Rebuild full header proposals from accepted source headers plus current declaration snippets.
for name,frag in [('EngineControl.hpp','EngineControl.proposed.declarations.inc'),('EngineReports.hpp','EngineReports.quality.declarations.proposed.inc')]:
 base=subprocess.run(['git','show',f'c0486d57daf6ac4a1e66733db4a68b3c2dfe133a:Common/src/adaptation/native_engine3d/{name}'],cwd=root,capture_output=True,text=True,check=True).stdout
 decl=(out/frag).read_text(); idx=base.rfind('};'); (out/(name+'.proposed')).write_text(base[:idx]+decl+base[idx:])
 subprocess.run(['clang-format-18','-i',str(out/(name+'.proposed'))],check=True)
