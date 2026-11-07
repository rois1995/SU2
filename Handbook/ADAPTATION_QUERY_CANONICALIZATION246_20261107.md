# Selected connected change: canonical single-triangle certification queries

2026-11-07. Design selected after the terminal Mach-only M6 and actual-context
analysis; implemented in build247, with serial and MPI qualification passed. ZERO accepted meshes.
Full Euler50k/transfer/continued-CFD/actual disk-restart,then BL/MPI and RANS500k
obligations remain unchanged. CAD stays optional and all gates/budgets stay strict.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`;
A=`R/m6_runs/euler_build244_mach_only_real_cfd50k`.

## Missing capability and evidence

The frozen Mach-only first event exhausted time with2035q/967J/1046geometry-bad
cells and a complete final census. Geometry-query traversal consumed3642.913284
summed rank seconds during quality and overlaps boundary smoothing3622.500379s.
Strict whole-mesh acceptance remains unmet,not merely a timing target.

Diagnostic-only build245passed168.177528s,all1303pins unchanged,both targets current,
all5sampled build handles absent. It adds exact authority/support/tolerance/coordinate
identity plus cache/result/work to the EXISTING geometry work trace,not a new
production configuration. It also measures raw collapse before the retained-star
optimizer in the existing actual-context fixture. No reconstruction/repair decision
or gate was changed. The lower traversal already printed inputs; the enrichment
also observes hits and pairs exact results/work. The parser distinguishes these
record formats explicitly.

Same three captured boundary contexts on245:tool68907terminal0/92.918482s,pins
unchanged,case R/cases/current_compound_20261107T135435_4105949. All5process IDs
sampled during execution are absent on HOST. No pre-exit start-tick snapshot was
obtained for that short job; no signal was sent. A/current_query245_attempt.json.
Verbose timing is not performance evidence. Full trace40,748,599bytes remains local,
hash de78309128b6a9da789f8d45b44049da238b8176a51bcfc27a320aa97ef65771.

R/analyze_mach245_queries.py self-checks exact repetition,permutation,nearby movement,
changed authority/support/tolerance,bypass and incomplete records. All1907traced
queries completed; no unknown queries or identical-answer changes:

-1516identical repeats,all cache hits; no identical-repeat misses in this scope.
-391misses consume603099counted work; hits consume78827.
-64geometrically identical single triangles under vertex permutations miss the
  ordered key and consume73450work:12.178763%of miss work in this bounded corpus.
-252new positions share an exact edge and unchanged authority/support/tolerance.
 127have a sufficient numeric tolerance margin,using exact rational comparison of
 squared displacement against squared margin. Their45181work is7.491473%of misses.

This is a bounded,overlapping local corpus,not a measured whole-run speedup or
population estimate. A/mach245_query_identity_assessment.json preserves counters,
source/log hashes and examples. Expensive prior twelve-query motion-slack results
remain valid in their earlier scope. No production motion cache is selected.

## Scope and proof

Change only the shared BoundPhysicalSurface single-triangle query path. Coarsening,
splitting and quality callers already use it; no phase/operator/scheduler dispatch
or construction-quality policy changes are required.

Canonicalize a single triangle's corner order deterministically from its exact
coordinate bit patterns. The convex triangle domain is identical under all six
permutations. Keep immutable authority identity,the COMPLETE ordered support-ID
list and tolerance bits exact. Do not merge nearby coordinates,supports,bodies,
geometry versions or tolerance/resolution policies. Multiple-triangle queries keep
their current representation and behavior.

Normalize BOTH the memo key and the actual full query on a miss/recomputation.
Key-only aliasing could return a different conservative value than a recomputation
with another corner order; that is not the proposed change. Canonicalization is
numeric distance reuse,not permission to bypass fresh association/containment,
feature/body/interface/height checks or strict final certification.

Account simultaneously for canonical triangle storage,key construction and query
scratch,including active prepared references. Preserve caller-byte restoration,
sticky exhaustion,cumulative work and bounded memo replacement/accounting. If
canonical scratch cannot be admitted,fall back to the existing full computation;
never manufacture a passing result or enlarge a resource allowance. Cache-disabled
recomputation with sufficient admission must use the same canonical query.

Acceptance conditions:all permutations give equal cached/recomputed canonical
results,exact support/tolerance/coordinate invalidation remains effective,and
curved seams,holes,separate bodies,genuine features,requested heights and resource
refusal remain protected. Compare actual-context counted traversal/query work and
complete-context q/J/geometry/size outcomes. Any source or behavior failure is fixed
before continuing; no acceptance or budget relaxation.

Use focused serial affected checks,then ONE connected MPI6 milestone reusing the
existing qualified geometry/height/resource/publication fixtures. Return immediately
to full two-event M6 with the SAME saved real CFD seed,MACH-only,L1/50k and unchanged
native5400/outer14400 budgets. Compare net whole-field q/J-bad removal and geometry
counts at measured matching quality ages,stating gaps; commits/debt alone are not
success. Passing diagnostics do not complete the workflow.

## Related measured gaps kept outside this change

The245raw-collapse control establishes a local quality coupling gap: core211800460,
retaining279303589,raw complete-context11q/9J→13q/13J fails Repair progress; collapse
plus retained-star optimizer gives8q/7J with zero geometry excess and valid metric/
replacement/interface checks. Another collapse already yields7q/7J raw but becomes
9q/8J after optimization,so indiscriminate composition is not justified. The
shape+geometry context still regresses geometry under all tested combinations.

Production's quality fallback validates raw collapse; optimizer repair acts on the
original imported patch. The existing coupled optimizer handles shell+split,not
this collapse+complete retained-star scope. Safe private MPI closure/reservation of
that retained star remains a design requirement,not an implemented facility or
proof the production run would select this core. Preserve this measured repair-power
gap for the next full outcome; do not start it as a competing implementation now.

Candidate ordering and motion-certificate reuse also remain deferred. The numeric
motion argument alone does not establish association,height,body separation,
invalidation,resource admission or final mesh acceptance. Select additional work
only from the next complete M6 outcome or its demonstrated bottleneck.

## Build247 implementation and qualification

The shared single-triangle path now canonicalizes exact coordinate bits for BOTH
cache keys and computation. It charges nine work units and admits canonical scratch
before allocation; temporary buffers die before caller-byte restoration. Multi-face
queries are unchanged; admission failure retains full-query fallback.

Build246 and247 completed successfully with1303source pins unchanged. The first
serial attempt on246 failed two fixture contracts: unsorted support must throw,and
an exact work allowance must include the nine new units. Production budgets were
not enlarged. These fixture corrections are the only246→247source differences.
Build247 affected serial:38cases/1678assertions passed;case
`canonical_serial_20261107T143317_2642`,all five sampled handles absent.

The same actual three-context trace on247 completed successfully;case
`current_compound_20261107T143840_16331`,all five sampled handles absent.
Counted query work681926→631308(−7.4228%),including canonical/key overhead;
64permutation misses become hits. Complete candidate inventory and q/J/geometry
bad counts are identical; no useful candidate is lost or gained. Trace wall times
are not performance evidence. A/canonical247_actual_comparison.json records scope.

A harness provenance bug was corrected before MPI qualification: companion files
follow the requested config directory,even when run.cfg is a symlink. Three branches
now use absolute() instead of resolve(). Earlier subset probes have explicit
supplemental provenance from their unchanged PRELAUNCH preparation hashes in
A/actual_companion_input_provenance.json; their original harness pins did not cover
the subset TSV. Original raw cell/metric payloads were shared and pinned.

ONE connected placement_mpi6 milestone passed on247,case
`placement_mpi6_20261107T145231_48860`,tool session52411terminal0/165.014869s.
90cases on EVERY rank,pins unchanged,all ten owned handles absent. Exact owned startup
identities are in its owned_startup.json. Qualified pt2pt/private4TBscratch/libs1;
geometry trace disabled. Reuses byte-identical historical geometry/ownership
fixture payloads with archived exact original producer source; this is not a new
CFD target or baseline campaign. Checkpoint and return immediately to
full same-seed MACH-only L1/50k TWO-event M6. Full247TWO-event M6 is LIVE,tool14272,after pushed checkpoint f7855ed7ec.
A247=`R/m6_runs/euler_build247_canonical_mach_only_real_cfd50k`. Config bytes match
Mach-only244;exact live handles and source/input freeze are in HANDOFF_Codex.md.

Full247quality_entry is complete at native689.582599 versus244701.671352.
Whole-field counts match EXACTLY:878443cells/106454q/106204J/1381geometry/0size,
all measured/flags complete. Construction upper work282283657→274601233 and
traversal263600528→256967738,with identical120785local accepted operations.
A247/quality_entry_comparison244.json and comparison244_matching_stages.json
preserve the measured scope;this is not quality closure or a full-run speedup.
