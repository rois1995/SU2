#!/usr/bin/env python3
"""Unapplied S15 plan-worker fragment; no production files are written."""
import hashlib
import json
import re
import subprocess
from pathlib import Path

here = Path(__file__).resolve().parent
rev = 'c0486d57daf6ac4a1e66733db4a68b3c2dfe133a'
path = 'Common/src/adaptation/CNativeEngine3D.cpp'
source = subprocess.check_output(['git', 'show', f'{rev}:{path}']).decode()
assert hashlib.sha256(source.encode()).hexdigest() == '6149e2cb2de3af25feefefdf056ce1c2b4aa75aa6170029348d4eedbffde5b0a'
start = '      auto plan=[&]() {\n'
end = '      };\n      if(!plan())return finish();'
assert source.count(start) == 1
body = source.split(start, 1)[1].split(end, 1)[0]
alias = '''  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& working = *context.ownership.working;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
'''
method = '''bool EngineSetup::PlanFirstRows(EngineContext& context, std::set<Id>& pending,
    CLocalFailure& failure, const size_t& layerScratch, std::vector<Feature>& features,
    std::vector<Id>& queued, DependencyBatch& dependencies, ImportedBatch& imported,
    ReservationBatch& reserved) {
''' + alias + '  // BEGIN_MOVED_BODY\n' + body + '  // END_MOVED_BODY\n}\n'
formatted = subprocess.check_output(['clang-format-18', '--style=file'], input=method.encode()).decode()
actual = formatted.split('// BEGIN_MOVED_BODY', 1)[1].split('// END_MOVED_BODY', 1)[0]
token = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_][A-Za-z_0-9]*|\d+(?:\.\d*)?|::|->|&&|\|\||==|!=|<=|>=|\+\+|--|\+=|-=|\*=|/=|<<|>>|[^\s]')
def tokens(text):
    return [item for item in token.findall(text) if not item.startswith(('//', '/*'))]
assert tokens(actual) == tokens(body)
(here / 'plan_worker.cpp.fragment').write_text(formatted)
(here / 'plan_worker_proof.json').write_text(json.dumps({'source_revision': rev, 'source_sha256': hashlib.sha256(source.encode()).hexdigest(), 'worker_formatted_lines': formatted.count('\n'), 'formatted_worker_body_token_equal_to_original_plan_body': True, 'scope': 'plan operation only; not an integrated S15 proposal', 'unapplied': True}, indent=2) + '\n')

# Publication retains its timer and all routing/edit owners in its caller.
publish_start = '      auto publish=[&]() {\n'
publish_end = '      };\n      if(!publish())return finish();'
assert source.count(publish_start) == 1
pub = source.split(publish_start, 1)[1].split(publish_end, 1)[0]
part_start = '        try {for(size_t k=0;k<proposals.size();++k)if(reserved.chosen[k]) {'
part_end = '        if(!context.exchange.Vote(context, failure))return false;'
assert pub.count(part_start) == 1
ix, iy = pub.index(part_start), pub.index(part_end)
part = pub[ix:iy]
alias_pub = '''  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& working = *context.ownership.working;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
'''
part_alias = '  auto& geometry = context.geometry;\n'
part_sig = '''void EngineSetup::PrepareFirstRowPublication(EngineContext& context, CLocalFailure& failure,
    std::vector<Patch>& proposals, ReservationBatch& reserved, ImportedBatch& imported,
    DependencyBatch& dependencies, std::vector<std::vector<Dependency>>& toRemove,
    std::vector<std::vector<OwnedCell>>& toAdd, size_t& published,
    std::vector<BoundaryParameterEdit>& layer_parameter_edits,
    SU2NativeGeometry::BoundaryCoverage& layer_coverage) {
'''
part_fmt = subprocess.check_output(['clang-format-18', '--style=file'], input=(part_sig + part_alias + '// BEGIN_MOVED_BODY\n' + part + '// END_MOVED_BODY\n}\n').encode()).decode()
actual_part = part_fmt.split('// BEGIN_MOVED_BODY', 1)[1].split('// END_MOVED_BODY', 1)[0]
assert tokens(actual_part) == tokens(part)
call = '''        EngineSetup::PrepareFirstRowPublication(context, failure, proposals, reserved, imported,
            dependencies, toRemove, toAdd, published, layer_parameter_edits, layer_coverage);
'''
pub_sig = '''template<class NeedsLayer>
bool EngineSetup::PublishFirstRows(EngineContext& context, CLocalFailure& failure,
    std::set<Id>& pending, const size_t& layerScratch, uint64_t& epoch,
    std::vector<Patch>& proposals, ReservationBatch& reserved, ImportedBatch& imported,
    DependencyBatch& dependencies, NeedsLayer& needs_layer) {
'''
pub_body = pub[:ix] + call + pub[iy:]
pub_fmt = subprocess.check_output(['clang-format-18', '--style=file'], input=(pub_sig + alias_pub + '// BEGIN_MOVED_BODY\n' + pub_body + '// END_MOVED_BODY\n}\n').encode()).decode()
actual_pub = pub_fmt.split('// BEGIN_MOVED_BODY', 1)[1].split('// END_MOVED_BODY', 1)[0]
# Inline actual formatted worker body, not original text, at its actual call site.
call_pattern = r'EngineSetup\s*::\s*PrepareFirstRowPublication\s*\([^;]*\)\s*;'
assert len(re.findall(call_pattern, actual_pub, re.S)) == 1
rebuilt = re.sub(call_pattern, lambda _: actual_part, actual_pub, count=1, flags=re.S)
assert tokens(rebuilt) == tokens(pub)
(here / 'publish_workers.cpp.fragment').write_text(part_fmt + '\n' + pub_fmt)
(here / 'publish_workers_proof.json').write_text(json.dumps({'source_revision': rev, 'part_formatted_lines': part_fmt.count('\n'), 'owner_formatted_lines': pub_fmt.count('\n'), 'recursive_actual_worker_token_equal': True, 'scope': 'publish operation only; not an integrated S15 proposal', 'unapplied': True}, indent=2) + '\n')

# Construct operation: extract donor retry as a boolean worker. The original
# group owner still owns donors, group, span and the field/accounting objects.
construct_start = '      auto construct=[&]() {\n'
construct_end = '      };\n      if(!construct())return finish();'
assert source.count(construct_start) == 1
construct = source.split(construct_start, 1)[1].split(construct_end, 1)[0]
donor_start = '            Timer timer(stats.layer_donor_seconds,options.timings, SU2NativeTiming3D::Operation::Import);\n'
donor_end = '          }\n          std::unique_ptr<FrozenField> target;'
assert construct.count(donor_start) == 1
start_donor = construct.index(donor_start)
end_donor = construct.index(donor_end, start_donor)
donor_body = construct[start_donor:end_donor]
donor_alias = '''  auto& world = context.world;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& source = *context.exchange.source;
'''
donor_sig = '''template<class JointCandidate>
bool EngineSetup::ImportFirstRowDonors(EngineContext& context, CLocalFailure& failure,
    const size_t& layerScratch, size_t at, const size_t& slots, size_t& group, size_t& span,
    DonorPatch& donors, ImportedBatch& imported, ReservationBatch& reserved,
    JointCandidate& joint_candidate) {
'''
def fmt_method(sig, aliases, body, tail=''):
    return subprocess.check_output(['clang-format-18', '--style=file'],
        input=(sig + aliases + '// BEGIN_MOVED_BODY\n' + body + '// END_MOVED_BODY\n' + tail + '}\n').encode()).decode()
def body_of(method):
    return method.split('// BEGIN_MOVED_BODY', 1)[1].split('// END_MOVED_BODY', 1)[0]
donor_fmt = fmt_method(donor_sig, donor_alias, donor_body, 'return true;\n')
assert tokens(body_of(donor_fmt)) == tokens(donor_body)
(here / 'construct_donor_worker.cpp.fragment').write_text(donor_fmt)
(here / 'construct_donor_proof.json').write_text(json.dumps({'source_revision': rev, 'worker_formatted_lines': donor_fmt.count('\n'), 'actual_formatted_body_token_equal': True, 'scope': 'donor retry only; remaining construct is not extracted', 'unapplied': True}, indent=2) + '\n')

# Extract only the joint branch, preserving background/reuse owners in its owner.
joint_start = '                std::vector<Point> backgrounds{Centroid(old.cells.front())};\n'
joint_end = '              }\n              if(!approved)'
assert construct.count(joint_start) == 1
jstart = construct.index(joint_start)
jend = construct.index(joint_end, jstart)
joint = construct[jstart:jend]
background_start = '                std::map<Id,Point> coordinates;'
score_start = '                for(const auto& background:backgrounds) {'
assert joint.count(background_start) == 1 and joint.count(score_start) == 1
bi, si = joint.index(background_start), joint.index(score_start)
# coordinates remains owned by the joint branch for its original lifetime.
background_raw = joint[bi:si]
reuseline = '                long double bestLength=std::numeric_limits<long double>::infinity();std::map<Edge,EdgeMeasure> reuse;\n'
assert background_raw.endswith(reuseline)
background_raw = background_raw[:-len(reuseline)]
background_body = background_raw.replace('std::map<Id,Point> coordinates;', '', 1)
score_body = joint[si:]
bg_sig = '''void EngineSetup::FirstRowBackgrounds(EngineContext& context, const Patch& old,
    std::vector<Point>& backgrounds, std::map<Id, Point>& coordinates) {
'''
bg_fmt = fmt_method(bg_sig, '  auto& geometry = context.geometry;\n  auto& options = context.options;\n', background_body)
assert tokens(body_of(bg_fmt)) == tokens(background_body)
score_sig = '''void EngineSetup::ScoreFirstRowBackgrounds(EngineContext& context, const Patch& old,
    Patch& fresh, Id node, Id cell, std::unique_ptr<FrozenField>& target,
    std::vector<Point>& backgrounds, long double& bestLength,
    std::map<Edge, EdgeMeasure>& reuse, bool& approved) {
'''
score_alias = '''  auto& geometry = context.geometry;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
'''
score_fmt = fmt_method(score_sig, score_alias, score_body)
assert tokens(body_of(score_fmt)) == tokens(score_body)
bg_call = 'EngineSetup::FirstRowBackgrounds(context, old, backgrounds, coordinates);'
score_call = 'EngineSetup::ScoreFirstRowBackgrounds(context, old, fresh, node, cell, target, backgrounds, bestLength, reuse, approved);'
joint_transformed = joint[:bi] + 'std::map<Id,Point> coordinates;\n' + bg_call + '\n' + reuseline + score_call + '\n'
joint_sig = '''void EngineSetup::ConstructFirstRowJoint(EngineContext& context, const Patch& old,
    Patch& fresh, Id node, Id cell, std::unique_ptr<FrozenField>& target, bool& approved) {
'''
joint_fmt = fmt_method(joint_sig, '', joint_transformed)
rebuilt_joint = body_of(joint_fmt)
for call_name, actual in [('FirstRowBackgrounds', body_of(bg_fmt)), ('ScoreFirstRowBackgrounds', body_of(score_fmt))]:
    pat = r'EngineSetup\s*::\s*' + call_name + r'\s*\([^;]*\)\s*;'
    assert len(re.findall(pat, rebuilt_joint, re.S)) == 1
    rebuilt_joint = re.sub(pat, lambda _: actual, rebuilt_joint, count=1, flags=re.S)
# Undo the audited coordinate declaration split.
assert tokens(rebuilt_joint) == tokens(joint)
(here / 'construct_joint_workers.cpp.fragment').write_text(bg_fmt + '\n' + score_fmt + '\n' + joint_fmt)
(here / 'construct_joint_proof.json').write_text(json.dumps({'source_revision': rev, 'formatted_lines': [bg_fmt.count('\n'), score_fmt.count('\n'), joint_fmt.count('\n')], 'recursive_actual_worker_token_equal': True, 'scope': 'joint candidate branch only; remaining construct is not extracted', 'unapplied': True}, indent=2) + '\n')

# Keep the caller's try/catch and rejected flag; only the try body is moved.
candidate_start = '              const auto old=GeometryPatch(imported.candidates[k], &geometry);'
candidate_end = '            }catch(const std::exception& e){failure.Set(2,k<queued.size()?queued[k]:0,e.what());rejected=true;}'
ci, ce = construct.index(candidate_start), construct.index(candidate_end)
candidate_raw = construct[ci:ce]
relative_jstart, relative_jend = jstart-ci, jend-ci
assert candidate_raw[relative_jstart:relative_jend] == joint
candidate_transformed = candidate_raw[:relative_jstart] + 'EngineSetup::ConstructFirstRowJoint(context, old, fresh, node, cell, target, approved);\n' + candidate_raw[relative_jend:]
candidate_sig = '''template<class JointCandidate>
void EngineSetup::ConstructFirstRowCandidate(EngineContext& context, CLocalFailure& failure,
    uint64_t& epoch, size_t k, ImportedBatch& imported, std::vector<Patch>& proposals,
    std::unique_ptr<FrozenField>& target, JointCandidate& joint_candidate, bool& rejected) {
'''
candidate_alias = '''  auto& world = context.world;
  auto& geometry = context.geometry;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
'''
candidate_fmt = fmt_method(candidate_sig, candidate_alias, candidate_transformed)
candidate_actual = body_of(candidate_fmt)
pat_joint = r'EngineSetup\s*::\s*ConstructFirstRowJoint\s*\([^;]*\)\s*;'
assert len(re.findall(pat_joint, candidate_actual, re.S)) == 1
candidate_rebuilt = re.sub(pat_joint, lambda _: rebuilt_joint, candidate_actual, count=1, flags=re.S)
assert tokens(candidate_rebuilt) == tokens(candidate_raw)
# Replace donor retry and candidate body in the original, from the right.
construct_transformed = construct[:ci] + '              EngineSetup::ConstructFirstRowCandidate(context, failure, epoch, k, imported, proposals, target, joint_candidate, rejected);\n' + construct[ce:]
construct_transformed = construct_transformed[:start_donor] + '            if (!EngineSetup::ImportFirstRowDonors(context, failure, layerScratch, at, slots, group, span, donors, imported, reserved, joint_candidate)) return false;\n' + construct_transformed[end_donor:]
# A worker owns one group and updates at after the same final vote.
outer_open = '        for(size_t at=0;at<slots;) {\n'
outer_close = '        }\n        return true;\n'
assert construct_transformed.startswith(outer_open) and construct_transformed.endswith(outer_close)
group_body = construct_transformed[len(outer_open):-len(outer_close)]
group_sig = '''template<class TimedOut, class JointCandidate>
bool EngineSetup::ConstructFirstRowGroup(EngineContext& context, CLocalFailure& failure,
    const size_t& layerScratch, uint64_t& epoch, const size_t& slots, size_t& at, size_t& span,
    const bool& ranked_background, ImportedBatch& imported, ReservationBatch& reserved,
    std::vector<Id>& queued, std::vector<Patch>& proposals, TimedOut& timed_out,
    JointCandidate& joint_candidate) {
'''
group_alias = '''  auto& world = context.world;
  auto& geometry = context.geometry;
  auto& options = context.options;
  auto& composition = context.composition;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& field_identity = *context.census.field_identity;
  const auto& started = *context.setup.control_prefix.started;
'''
group_fmt = fmt_method(group_sig, group_alias, group_body, 'return true;\n')
owner_sig = '''template<class TimedOut, class JointCandidate>
bool EngineSetup::ConstructFirstRows(EngineContext& context, CLocalFailure& failure,
    const size_t& layerScratch, uint64_t& epoch, const size_t& slots, size_t& span, const bool& ranked_background,
    ImportedBatch& imported, ReservationBatch& reserved, std::vector<Id>& queued,
    std::vector<Patch>& proposals, TimedOut& timed_out, JointCandidate& joint_candidate) {
'''
owner_body = '''  for (size_t at=0; at<slots;) {
    if (!EngineSetup::ConstructFirstRowGroup(context, failure, layerScratch, epoch, slots,
        at, span, ranked_background, imported, reserved, queued, proposals, timed_out,
        joint_candidate)) return false;
  }
  return true;
'''
owner_fmt = fmt_method(owner_sig, '', owner_body)
# Inverse routing removes only the group call's status propagation and its added success return.
actual_group = body_of(group_fmt)
for name, worker in [('ImportFirstRowDonors', body_of(donor_fmt)), ('ConstructFirstRowCandidate', candidate_rebuilt)]:
    if name == 'ImportFirstRowDonors':
        pat = r'if\s*\(\s*!\s*EngineSetup\s*::\s*ImportFirstRowDonors\s*\([^;]*?\)\s*\)\s*return\s+false\s*;'
    else:
        pat = r'EngineSetup\s*::\s*ConstructFirstRowCandidate\s*\([^;]*\)\s*;'
    assert len(re.findall(pat, actual_group, re.S)) == 1
    actual_group = re.sub(pat, lambda _: worker, actual_group, count=1, flags=re.S)
owner_actual = body_of(owner_fmt)
pat_group = r'if\s*\(\s*!\s*EngineSetup\s*::\s*ConstructFirstRowGroup\s*\([^;]*?\)\s*\)\s*return\s+false\s*;'
assert len(re.findall(pat_group, owner_actual, re.S)) == 1
construct_rebuilt = re.sub(pat_group, lambda _: actual_group, owner_actual, count=1, flags=re.S)
a, b = tokens(construct_rebuilt), tokens(construct)
if a != b:
    z = next((i for i, (x,y) in enumerate(zip(a,b)) if x != y), min(len(a),len(b)))
    raise AssertionError((z, a[z:z+20], b[z:z+20], len(a), len(b)))
(here / 'construct_workers.cpp.fragment').write_text('\n'.join([donor_fmt, bg_fmt, score_fmt, joint_fmt, candidate_fmt, group_fmt, owner_fmt]))
lengths = [x.count('\n') for x in [donor_fmt, bg_fmt, score_fmt, joint_fmt, candidate_fmt, group_fmt, owner_fmt]]
(here / 'construct_workers_proof.json').write_text(json.dumps({'source_revision': rev, 'formatted_lines': lengths, 'recursive_actual_worker_token_equal': True, 'scope': 'construct operation; no integrated S15 proposal yet', 'unapplied': True}, indent=2) + '\n')

# Build the full layer owner and the while-iteration owner from the pinned block.
block_start = '  if(!surface_target&&!options.wall_heights.empty()) {\n'
block_end = '  constexpr size_t MeasurementBatch=16;\n'
assert source.count(block_start) == 1 and source.count(block_end) == 1
bs = source.index(block_start)
be = source.index(block_end, bs)
block = source[bs:be]
loop_start = '    while(world.maximum(!pending.empty())) {\n'
loop_end = '    try {\n      initial.clear();initial.reserve(working.size());'
assert block.count(loop_start) == 1 and block.count(loop_end) == 1
li, le = block.index(loop_start), block.index(loop_end)
loop_raw = block[li:le]
plan_original = start + body + end
construct_original = construct_start + construct + construct_end
publish_original = publish_start + pub + publish_end
for original in (plan_original, construct_original, publish_original):
    assert loop_raw.count(original) == 1
loop_new = loop_raw.replace(plan_original, '''      if (!EngineSetup::PlanFirstRows(context, pending, failure, layerScratch, features, queued,
          dependencies, imported, reserved)) return std::optional<EngineResult>(finish());''')
loop_new = loop_new.replace(construct_original, '''      if (!EngineSetup::ConstructFirstRows(context, failure, layerScratch, epoch, slots, span,
          ranked_background, imported, reserved, queued, proposals, timed_out, joint_candidate))
        return std::optional<EngineResult>(finish());''')
loop_new = loop_new.replace(publish_original, '''      if (!EngineSetup::PublishFirstRows(context, failure, pending, layerScratch, epoch,
          proposals, reserved, imported, dependencies, needs_layer)) return std::optional<EngineResult>(finish());''')
loop_new = loop_new.replace('return finish();', 'return std::optional<EngineResult>(finish());')
loop_sig = '''template<class NeedsLayer, class Finish>
std::optional<EngineResult> EngineSetup::IterateFirstRows(EngineContext& context,
    CLocalFailure& failure, std::set<Id>& pending, const size_t& layerScratch, uint64_t& epoch,
    NeedsLayer& needs_layer, Finish& finish) {
'''
loop_alias = '''  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  const auto& started = *context.setup.control_prefix.started;
'''
loop_fmt = fmt_method(loop_sig, loop_alias, loop_new, 'return std::nullopt;\n')
owner_new = block[:li] + '''    auto layer_done = EngineSetup::IterateFirstRows(context, failure, pending, layerScratch, epoch, needs_layer, finish);
    if (layer_done) return layer_done;
''' + block[le:]
owner_new = owner_new.replace('return finish();', 'return std::optional<EngineResult>(finish());')
layer_sig = '''template<class Finish>
std::optional<EngineResult> EngineSetup::InitializeFirstRows(EngineContext& context, Finish& finish) {
'''
layer_alias = '''  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& surface_target = context.surface_target;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  auto& initial = *context.census.initial;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& layer_active = *context.control.layer_active;
  auto& layer_started = *context.control.layer_started;
'''
layer_fmt = fmt_method(layer_sig, layer_alias, owner_new, 'return std::nullopt;\n')
all_methods = [formatted, part_fmt, pub_fmt, donor_fmt, bg_fmt, score_fmt, joint_fmt, candidate_fmt, group_fmt, owner_fmt, loop_fmt, layer_fmt]
lengths = {re.search(r'EngineSetup::(\w+)', x).group(1): x.count('\n') for x in all_methods}
print('lengths', lengths)
assert max(lengths.values()) <= 80
# Invert actual formatted operation calls, including their status and finish routes.
loop_rebuilt = body_of(loop_fmt)
for name, local, actual in [('PlanFirstRows', 'plan', body_of(formatted)),
                            ('ConstructFirstRows', 'construct', construct_rebuilt),
                            ('PublishFirstRows', 'publish', rebuilt)]:
    pat = (r'if\s*\(\s*!\s*EngineSetup\s*::\s*' + name +
           r'\s*\([^;]*?\)\s*\)\s*return\s+std\s*::\s*optional\s*<\s*EngineResult\s*>\s*\(\s*finish\s*\(\s*\)\s*\)\s*;')
    assert len(re.findall(pat, loop_rebuilt, re.S)) == 1, name
    inverse = 'auto ' + local + '=[&]() {\n' + actual + '\n};\nif(!' + local + '())return finish();'
    loop_rebuilt = re.sub(pat, lambda _: inverse, loop_rebuilt, count=1, flags=re.S)
loop_rebuilt = re.sub(r'return\s+std\s*::\s*optional\s*<\s*EngineResult\s*>\s*\(\s*finish\s*\(\s*\)\s*\)\s*;', 'return finish();', loop_rebuilt)
assert tokens(loop_rebuilt) == tokens(loop_raw)
layer_rebuilt = body_of(layer_fmt)
pat_iter = r'auto\s+layer_done\s*=\s*EngineSetup\s*::\s*IterateFirstRows\s*\([^;]*\)\s*;\s*if\s*\(\s*layer_done\s*\)\s*return\s+layer_done\s*;'
assert len(re.findall(pat_iter, layer_rebuilt, re.S)) == 1
layer_rebuilt = re.sub(pat_iter, lambda _: loop_rebuilt, layer_rebuilt, count=1, flags=re.S)
layer_rebuilt = re.sub(r'return\s+std\s*::\s*optional\s*<\s*EngineResult\s*>\s*\(\s*finish\s*\(\s*\)\s*\)\s*;', 'return finish();', layer_rebuilt)
assert tokens(layer_rebuilt) == tokens(block)
# The original Adapt closure is kept in this pinned draft. The future B5 merge
# binds this Finish& to a synchronous wrapper of EngineControl::Finish instead.
main_call = '''  auto first_rows_done = EngineSetup::InitializeFirstRows(context, finish);
  if (first_rows_done) return std::move(*first_rows_done);
'''
main_proposed = source[:bs] + main_call + source[be:]
main_reversed = main_proposed.replace(main_call, block, 1)
assert main_reversed == source
hpp_path = 'Common/src/adaptation/native_engine3d/EngineSetup.hpp'
impl_path = 'Common/src/adaptation/native_engine3d/EngineSetup.cpp'
hpp = subprocess.check_output(['git', 'show', f'{rev}:{hpp_path}']).decode()
impl = subprocess.check_output(['git', 'show', f'{rev}:{impl_path}']).decode()
sigs = [method[:method.index('{')].rstrip() for method in all_methods]
decls = []
for sig in sigs:
    sig = sig.replace('EngineSetup::', '')
    if sig.startswith('template'):
        template, definition = sig.split('\n', 1)
        sig = template + '\nstatic ' + definition
    else:
        sig = 'static ' + sig
    decls.append(sig + ';\n')
assert hpp.count('  EngineSetup();\n') == 1
hpp_proposed = hpp.replace('  EngineSetup();\n', '\n'.join(decls) + '\n  EngineSetup();\n', 1)
impl_proposed = impl + '\n' + '\n'.join(all_methods)
(here / 'CNativeEngine3D.cpp.proposed').write_text(main_proposed)
(here / 'EngineSetup.hpp.proposed').write_text(hpp_proposed)
(here / 'EngineSetup.cpp.proposed').write_text(impl_proposed)
(here / 'full_reversal_proof.json').write_text(json.dumps({
    'status': 'UNAPPLIED; no compile or runtime test', 'source_revision': rev,
    'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
    'actual_formatted_recursive_layer_reconstruction_token_equal': True,
    'actual_operation_status_and_optional_continuation_inverse_routes': True,
    'main_byte_equal_after_restoring_source_formatting': main_reversed == source,
    'main_sha256_after_restoring_source_formatting': hashlib.sha256(main_reversed.encode()).hexdigest(),
    'formatted_function_lines': lengths,
    'original_nested_operation_boolean_boundaries_preserved': True,
    'future_b5_finish_binding': 'rebind Finish& synchronously to EngineControl::Finish(context,curved,working,atlas_budget,conststarted,conststarted_cpu)',
    'qualification': 'Token proof excludes comments and normalizes formatting. It is not a C++ compiler or semantic lifetime proof.'
}, indent=2) + '\n')
# Inventory every lambda in the formatted proposal methods, not just named ones.
lambda_lengths = []
for method in all_methods:
    for match in re.finditer(r'\[[^\]]*\]\s*\([^)]*\)\s*\{', method):
        begin = match.end() - 1
        depth = 0
        for offset in range(begin, len(method)):
            if method[offset] == '{': depth += 1
            if method[offset] == '}':
                depth -= 1
                if not depth:
                    end = offset
                    break
        else:
            raise AssertionError('unclosed lambda')
        lambda_lengths.append({'function': re.search(r'EngineSetup::(\w+)', method).group(1),
            'capture_and_parameters': match.group(0)[:-1].strip(),
            'formatted_lines': method.count('\n', 0, end) - method.count('\n', 0, match.start()) + 1})
assert len(lambda_lengths) == 5 and max(x['formatted_lines'] for x in lambda_lengths) <= 15
proof_path = here / 'full_reversal_proof.json'
proof = json.loads(proof_path.read_text())
proof['formatted_lambda_inventory'] = lambda_lengths
proof_path.write_text(json.dumps(proof, indent=2) + '\n')
