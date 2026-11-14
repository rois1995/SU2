"""Render the requested compact report from completed, saved measurement summaries."""
import hashlib,json,re
from pathlib import Path
R=Path(__file__).resolve().parent;REPO=R.parents[4]
load=lambda p:json.loads((R/p).read_text())
a=load('run_A/split_profiles.json');bp=load('run_B/samples_summary.json');b=load('run_B/final_tables.json');storage=load('profiler_storage.json')
assert load('run_B/status.json')['all_owned_handles_terminal'] and not load('run_B/status.json')['profiler_failed']
assert len(bp)==6 and all(len(x['events'])==2 for x in b['ranks'])
profiles=[('A prep',a['preparation']['profile']),('A native',a['native']['profile'])]+[(f'B rank{x["rank"]}',x) for x in bp]
G=('wall_proof','exact_predicates','target_BL','tree','memory','MPI','pack_unpack','cavity_topology','other')
def share(n,d):return f'{n:,} ({100*n/d:.2f}%)' if n>=100 else f'{n:,} NR'
def cell(z):
 if not z:return 'not reached'
 n=z['samples'];w=z['groups']['MPI'];c=n-w
 return f'{w:,}/{c:,} '+(f'({100*w/n:.2f}%)' if min(w,c)>=100 else 'NR')
def short(s,d):
 if s=='[unknown]':return '?@'+Path(d).name
 if 'BoundaryGuidance' in s and 'lambda' in s:return 'BL::BoundaryGuidance lambda'
 if 'OptionA::InStrip' in s:return 'OptionA::InStrip'
 if '_Rb_tree' in s:
  m=re.findall(r'::(find|erase|insert|_M_\w+|_Rb_tree_\w+)\b',s)
  return 'Rb::'+(m[-1] if m else s.split('(')[0].split()[-1])+'#'+hashlib.sha256(s.encode()).hexdigest()[:4]
 if 'SU2NativeGeometry::(anonymous namespace)::Multiply(' in s:return 'G::Multiply(R,R)' if 'Range, SU2NativeGeometry' in s else 'G::Multiply(R,scalar)'
 s=s.replace('SU2NativeGeometry::(anonymous namespace)::','G::').replace('SU2Native3D::(anonymous namespace)::','P::').replace('(anonymous namespace)','anon').replace('SU2NativeGeometry::','Geom::').replace('SU2Native3D::','N::')
 name=s.split('(',1)[0]
 if 'Integer<' in name:
  m=re.search(r'Integer<([^>]+)>',name);f=re.findall(r'::(Difference|Product|Compare|ParametricBounds)\b',name)
  if m and f:name='P::'+f[-1]+'<'+m.group(1)+'>'
 if len(name)>85:name=name[:75]+'#'+hashlib.sha256(s.encode()).hexdigest()[:5]
 return name
legend=[]
lines=['# M6 measurement v2 — 2026-11-14','', '**Both prescribed runs reached normal process exit; all flat recorders/parsers exited0 and pins were unchanged. No native source/build/config/gate/budget changes.**', 'Run A:300splits,304578positive private tets, surface_prepared=false (same bounded stage, not an accepted mesh). Run B:two configured subiterations, both time-budget rejected; solver exit0 retained61330points/303359tets.', 'B private exit meshes:697183 and685450tets, neither accepted; reference event1 accepted149051points/736323tets, event2 accepted172001points/870422tets. Euler complexity50000 is nominal, not a node-count gate.', 'R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`; evidence=`R/measurement_20261114_v2`; v1 is preserved. Program/config/input hashes, commands, environments and resources: per-case provenance/status and final_host.json.', 'Pinned CFD SHA0311b5a2a4f871bd82f48cd7cef3eac49a932cd1d90012bbc7e3c0d887e74ee6; A repeats the original test-driver SHAe6c25520db86bdd212b500249420a7cf3ceddf4af5c17c40e02b1b0b743cd365. Saved485/new492 target and latest492 checkpoints were preserved.', '', '## T0 — Run A clocks and wall time', '', '| Part | Monotonic start → end (s) | Wall seconds |','|---|---|---:|','| Preparation | OS birth[136736.210,136736.220] →136879.191632075 | 142.972–142.982 |','| Native public PrepareSurface call | 136879.191632075→137272.885798218 | 393.694166 |','| Post-call observation/checks | 137272.885798218→137304.092796831 | 31.206999 |','OS birth has10ms granularity; loader stop136736.475593031→entry142.716039s, with zero earlier native CPU samples. Both requested profiles include all observed samples in their intervals.', 'The existing fixture API timer is393.694989634s; internal exit timer388.843170562s has a narrower scope. Post-call work includes the external30s after-face probe and is excluded from both profiles.', '', '## T1 — exclusive identified leaf samples and top20', '', 'User-space CPU samples only; not wall fractions. Shared helpers stay Other; inlined/inclusive costs are unresolved. NR means fewer100samples behind a share. B totals cover both invocations plus CFD/preparation.', '| Profile | N | Proof | Predicates | Target/BL | Tree | Memory | MPI | Pack | Cavity | Other |','|---|---:|---|---|---|---|---|---|---|---|---|']
for name,p in profiles:
 assert sum(v['samples'] for v in p['groups'].values())==p['native_samples']
 lines.append('| '+name+' | '+f'{p["native_samples"]:,}'+' | '+' | '.join(share(p['groups'][g]['samples'],p['native_samples']) for g in G)+' |')
lines+=['Top20 entries give symbol,count(share of the corresponding N); shortened namespaces G=geometry anonymous, P=native anonymous, N=native. Full signatures/DSOs and unique label hashes: symbol_legend.json. `?@module` is an unresolved symbol aggregated within that known DSO.', '| Profile | Top20 symbols |','|---|---|']
for name,p in profiles:
 entries=[]
 for i,x in enumerate(p['top20'],1):
  label=short(x['symbol'],x['dso']).replace('|','/')
  entries.append('`'+label+'` '+share(x['samples'],p['native_samples']))
  legend.append(dict(profile=name,position=i,label=label,**x))
 lines.append('| '+name+' | '+'; '.join(entries)+' |')
(R/'symbol_legend.json').write_text(json.dumps(legend,indent=2)+'\n')
lines+=['A prep interval arithmetic(Multiply/Add/Adjacent/Blossom)+__nexttowardl10896/13883(78.48%); __nexttowardl3094/13883(22.29%). Full native interval+libm17135/38549(44.45%); v1 approximate cuts are not equivalent phase boundaries.', '', '## T2 — Run B phase bins', '', 'Cells below: max-rank boundary seconds / global accepted edits / edits per max-rank second; C=tracked collective calls,E=World::elect calls (common per-rank counts, not summed six times). Literal world exchanges were not emitted.', 'C*=initialization+collapse; S*=surface with generation/setup and any failed return; R*=ownership staging/commit plus phase setup; Q*=quality_entry→failed exit; A*=emitted handoffs/setup. These are disjoint observable bins, not pure operator timers.', '| Phase | Invocation1 | Invocation2 | World exchanges |','|---|---|---|---|']
keys=[('C* collapse','collapse_init'),('S* surface','surface_setup'),('I interior','interior'),('R* repartition/setup','ownership_setup'),('Q* quality','quality_return'),('A* alternation/setup','alternation_setup'),('Strict finish','strict_finish')]
def rang(v):return str(int(v[0])) if v and v[0]==v[1] else (f'{int(v[0])}–{int(v[1])}' if v else 'NR')
for title,k in keys:
 vals=[]
 for e in b['events']:
  z=e['categories'][k];t=z['maximum_rank_boundary_wall_seconds'];v='not reached' if not t else f'{t:.3f}s / {z["global_accepted_edits"]} / {z["global_edits_per_max_rank_boundary_second"]:.3f}; C{rang(z["counter_ranges"]["collective_calls"])} E{rang(z["counter_ranges"]["election_calls"])}';vals.append(v)
 lines.append('| '+title+' | '+' | '.join(vals)+' | NR |')
lanes=[]
for e in b['events']:
 f=e['final_stages'];lanes.append('/'.join(f'{max(float(x[k]) for x in f):.3f}' for k in ('surface_seconds','interior_seconds')))
lines.append(f'Named S/I lane counters={lanes[0]}s and{lanes[1]}s; they differ from boundary bins because timer scopes span generation/setup and failed returns. Logged ownership timer maxima={max(b["events"][0]["ownership_timer_seconds_per_rank"]):.3f}s/{max(b["events"][1]["ownership_timer_seconds_per_rank"]):.3f}s; R* includes additional setup. Per-rank intervals/counters: run_B_phase_samples.json and final_tables.json.')
lines+=['', '## T3 — compute against wait by rank/phase', '', 'Each cell W/C(counts),then W%; W=MPI-library samples under the brief’s convention,C=other sampled CPU,N=W+C. NR if either count<100. Off-CPU/kernel wall wait and slowest-rank causality are not measured.', 'Only timestamps guaranteed within epoch brackets are assigned (width0.356–0.379s for invocation1,0.012–0.014s for invocation2); boundary-ambiguous samples are excluded. GDB variable read failed nonfatally; source+ELF disassembly bound the epoch. Phase definitions match T2.', '| Inv/rank | C* | S* | I | R* | Q* | A* | Finish |','|---|---|---|---|---|---|---|---|']
for e in range(2):
 for row in b['ranks']:
  cats=row['events'][e]['categories'];lines.append(f'| {e+1}/{row["rank"]} | '+' | '.join(cell(cats.get(k)) for _,k in keys)+' |')
lines.append('Excluded ambiguous samples by rank0–5: '+','.join(str(x['sample_assignment']['boundary_ambiguous_samples']) for x in b['ranks'])+'. Full assignments and clock bounds are retained; no epoch or CPU seconds estimated from sample frequency.')
lines+=['', '## T4 — timer coverage', '', '| Rank | Inner exclusive coverage1 / residual seconds | Inner exclusive coverage2 / residual seconds |','|---|---|---|']
for row in b['ranks']:
 lines.append('| '+str(row['rank'])+' | '+' | '.join(f'{x["exclusive_coverage_percent"]:.2f}% / {float(x["detail"]["other"]):.3f}' for x in row['events'])+' |')
outer=[x['outer_coverage_percent'] for row in b['ranks'] for x in row['events']]
lines+= [f'Coverage=1−reported other/total in NATIVE3D_DETAIL(refine). Outer remesher buckets cover{min(outer):.2f}–{max(outer):.2f}% by enclosing refinement; that is a different level. Residual is not proof of wholly untimed code: nested counters/scoped failure returns overlap or leave attribution gaps; union coverage is unresolved.', 'A emits no complete exclusive bucket breakdown. Engine upper173.858064s/388.843171s=44.71% and geometry-admission guidance144.738106s=37.22% overlap. Fixture upper233.722574s includes two external before/after probes; do not mix scopes.', 'Largest named A target self routines are InStrip and BoundaryGuidance lambda (T1); neither has a dedicated wall timer, and initial-census calls can land in other (CPU-only measurement timer). Exact largest functions wholly outside every timer are not identifiable from these flat profiles; no residual-function cost is invented.', '', '## T5 — peak resident memory', '', '| Rank command | Observed native /proc VmHWM KiB | GNU-time maxRSS KiB |','|---|---:|---:|']
ast=load('run_A/profile/rank0.status.json');at=(R/'run_A/profile/rank0.time.txt').read_text();am=int(re.search(r'Maximum resident set size \(kbytes\): (\d+)',at).group(1));lines.append(f'| A0 | {ast["sampled_main_peak_hwm_kib"]} | {am} |')
for row in b['ranks']:lines.append(f'| B{row["rank"]} | {row["memory"]["observed_native_hwm_kib"]} | {row["memory"]["time_max_rss_kib"]} |')
lines+=['/proc polled every0.5s; GNU time wraps debugger/native descendants and kernel accounting differs slightly. No whole-MPI peak is inferred from one rank or from summing individual maxima.', '', '## T6 — audit claims', '', '| # | Finding |','|---:|---|']
native=[]
for row in b['ranks']:
 n=sum(z['samples'] for e in row['events'] for z in e['categories'].values());w=sum(z['groups']['MPI'] for e in row['events'] for z in e['categories'].values());native.append(100*w/n)
lines +=[f'| 1 | Not verified as wall wait/slowest-rank cause. Guaranteed native-window MPI CPU shares range{min(native):.2f}–{max(native):.2f}% (counts T3), not37–47%. |','| 2 | Literal one edit on every rank in every round is false: first interior3490–8438splits per rank/8492rounds. An at-most-one cap is not measured by aggregate counts. |',f'| 3 | Recorded (C+E)/reported round is{b["events"][0]["rank0_amortized_tracked_plus_elections_per_round"]:.3f}/{b["events"][1]["rank0_amortized_tracked_plus_elections_per_round"]:.3f}, amortized including setup. Per-round distribution and literal world synchronization/exchange counts remain unmeasured. |',f'| 4 | World election means(rank0)={b["events"][0]["rank0_world_election_mean_us"]:.3f}/{b["events"][1]["rank0_world_election_mean_us"]:.3f}µs, not177µs. Directory-family means={b["events"][0]["rank0_directory_election_mean_us"]:.3f}/{b["events"][1]["rank0_directory_election_mean_us"]:.3f}µs; do not substitute/add them. Waiting versus message cost not separated. |','| 5 | Not measured: neither invocation reached strict accepted finish; no new520s-per-rank proof result. |',f'| 6 | First interior pass1820.37global edits/s; cumulative boundary-bin rates{b["events"][0]["categories"]["interior"]["global_edits_per_max_rank_boundary_second"]:.2f}/{b["events"][1]["categories"]["interior"]["global_edits_per_max_rank_boundary_second"]:.2f}. A universal1024 is unsupported; measured surface/quality costs are in T2 (quality2 unreached). |','| 7 | A1897reservation conflicts/2640surface rounds=71.86%; that is not the explicit deferral counter. Proof/admission timer union is unmeasured, so “proof plus guidance takes most” cannot be established by adding them. |','| 8 | Identified self costs are small: A native tree579/38549(1.50%), memory821/38549(2.13%); B per-rank counts/shares T1. Inclusive/inlined tree/allocation cost remains unresolved. |', '', '## T7 — sampled near callers (optional LBR)', '', 'Use run_A/nearest_callers.json and caller_review.json; broad caller_summary.json is superseded. Only plausible near prefixes are attributed; far LBR tails may contain stale history. Finite60s windows, not whole-phase inclusive shares or invocation counts.', '| Question | Measured evidence |','|---|---|','| Preparation interval callers | Of3178: mobility2461(77.44%), parameter/coverage510(16.05%), unresolved138(4.34%); point/normal69 NR. |','| Native interval callers | Of808: target surface evaluation527(65.22%), parameter/coverage126(15.59%); point/normal83, local face deviation40, union proof6, mobility12, unresolved14 each NR. |','| __nexttowardl / nextafter | Prep near Adjacent1319/1355(97.34%); native90/98 NR. Remaining near symbols cannot distinguish direct nextafter from inlined Adjacent. |','| InStrip | 2301/2301 sampled near parents are BoundaryGuidance lambda; exact calls per guidance evaluation not measured. |','| Guidance lambda | 1634/1634 show method→fixture handler→MeasureSurfacePlanning; seed/scoring/candidate/census outer step unresolved. |', '', '## Difference/overhead check and remaining limits', '', 'A matched API393.694989634s vs389.572521653s=+1.058%;300vs305edits=−1.639%; neither exceeds5%. No correction.']
refpath=R.parent/'m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k/run.log';ref=[dict(re.findall(r'(\w+)=([^ ]+)',x)) for x in refpath.read_text().splitlines() if x.startswith('NATIVE3D_DETAIL stage=refine rank=0 ')]
cur=[row['events'][e]['detail'] for e in range(2) for row in b['ranks'] if row['rank']==0]
checks=[]
for e in range(2):
 now=float(cur[e]['total']);old=float(ref[e]['total']);checks.append(f'{now:.6f}s vs{old:.6f}s ({100*(now/old-1):+.2f}%,FLAG>5%)')
lines +=['B inner native durations1/2: '+'; '.join(checks)+'. First rank0 accepted edits11586vs18201; both current events rejected instead of accepted. No timing correction.', 'Second current subiteration starts from retained coarse mesh after rejection; reference second starts from the accepted first adaptation. Executable/build(older test-driver versus494 CFD), external debugger clock probes and host contention also differ; these are not isolated profiler-overhead measurements.', 'Host load changed from3.10 at B launch to15–17; a2.099s receipt observed unrelated Lean at173.4%CPU and native ranks74.8–94.8%. No whole-run contention cost or slowest-rank cause is estimated.', f'Conservative retained profiler/support data across v1+v2={storage["retained_bytes"]:,}B (<2,000,000,000B); raw perf stays local4TB/out ofgit. All six optional run LBR captures decoded0 with --no-inline; their raw data was removed after summary. No DWARF call graph.', 'Unmeasured: literal world exchanges/per-round sync distribution, off-CPU wall wait, timer-union/complete residual-function attribution, exact guidance calls/evaluation, outer guidance step, strict accepted finish, inclusive/inlined costs and private mesh point counts. Native failure debts are not accepted mesh quality.', 'All owned handles terminal; normal exits/flat success/hash receipts retained. No refactoring recommendation or design work. Measurement ends here.']
assert len(lines)<=150,(len(lines),'report line cap')
(REPO/'Handbook/ADAPTATION_M6_MEASUREMENT_V2_20261114.md').write_text('\n'.join(lines)+'\n')
print('Final report lines',len(lines))
