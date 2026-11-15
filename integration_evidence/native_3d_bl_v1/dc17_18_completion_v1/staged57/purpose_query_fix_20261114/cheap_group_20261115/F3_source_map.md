# F3 source map — read-only, source0b56ad8f67

Bounded routine Luna source review; primary retains implementation/acceptance. No source edits or jobs for this map. Numerical profiles remain pinned.

- Engine Timer at Common/src/adaptation/CNativeEngine3D.cpp:62–71 already supports exclusive nested wall accounting by subtracting elapsed from a supplied parent. CpuTimer:73–80 is process CPU, separate from wall.
- Common/include/adaptation/CNativeRuntimeProfile3D.hpp:34–81 is a sequential flat Mark/Report row; other=total−sum(phases). Its ProfileRow allgatherv:26–31 must be called collectively and reports local rank values, not global max.
- CSinglezoneDriver.cpp:252–269 and CSolver.cpp:2735–2737,2986–4116 have enclosing metric and metric_build spans: nested, not independent additive totals.
- CNativeBLPipeline3D.hpp:447–465 already has exact public PrepareSurface start/end. It is nested inside CNativeRemesher3D.cpp:564–630 worker timings. Remesher phase8 accumulates both construction:611 and AdaptMetric:630; phase12 hides target/policy composition:358–386 and766–773.
- Engine actual census is seed:2644–2759, census_started/collective count:2663–2664, return:2757; seed_next:2760–2777. Donor timer:2689–2690 already receives enclosing_seconds; seed timer:2761–2763/2776 can parent-subtract. emit_quality_census:1691–1782 is diagnostic, not the actual census.
- observe_progress also refreshes quality/census and uses collectives. Keep its reporting overhead distinct from numerical construction census.
- Old cavity q/J: MeshOptimizer::OldMerit:6239–6244 uses MinScore:6240 or RowQuality per old id:6242. It is called at6404,6454,6469,6505,6699,6832,6879,6896,6959,7111. Timing only this function misses explicit old shape_debt scans:6506–6507,6696/6701, and counts(original.cells):7113–7116. Include all these costs or label narrowly. Timing broad optimizer entries also captures fresh/geometry work and overlaps MeasureScope:6484–6487/6687–6692. RowQuality may hit the shared quality cache; time is actual cost, not count of new evaluations. No B4 value-reuse change authorized.
- Native stage lines:1815–1904 are cumulative overlapping snapshots, not an exclusive tree. Remesher parent/worker communicators and waits:651ff have different rank scopes; keep them explicit when comparing coverage.

F3 acceptance remains actual named exclusive coverage>=95%, with nested timers, contention and residual gaps explicit. This map is source evidence, not a new numerical design or implementation.
