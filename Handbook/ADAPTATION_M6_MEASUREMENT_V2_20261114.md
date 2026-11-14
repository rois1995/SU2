# M6 measurement v2 — 2026-11-14

**Both prescribed runs reached normal process exit; all flat recorders/parsers exited0 and pins were unchanged. No native source/build/config/gate/budget changes.**
Run A:300splits,304578positive private tets, surface_prepared=false (same bounded stage, not an accepted mesh). Run B:two configured subiterations, both time-budget rejected; solver exit0 retained61330points/303359tets.
B private exit meshes:697183 and685450tets, neither accepted; reference event1 accepted149051points/736323tets, event2 accepted172001points/870422tets. Euler complexity50000 is nominal, not a node-count gate.
R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`; evidence=`R/measurement_20261114_v2`; v1 is preserved. Program/config/input hashes, commands, environments and resources: per-case provenance/status and final_host.json.
Pinned CFD SHA0311b5a2a4f871bd82f48cd7cef3eac49a932cd1d90012bbc7e3c0d887e74ee6; A repeats the original test-driver SHAe6c25520db86bdd212b500249420a7cf3ceddf4af5c17c40e02b1b0b743cd365. Saved485/new492 target and latest492 checkpoints were preserved.

## T0 — Run A clocks and wall time

| Part | Monotonic start → end (s) | Wall seconds |
|---|---|---:|
| Preparation | OS birth[136736.210,136736.220] →136879.191632075 | 142.972–142.982 |
| Native public PrepareSurface call | 136879.191632075→137272.885798218 | 393.694166 |
| Post-call observation/checks | 137272.885798218→137304.092796831 | 31.206999 |
OS birth has10ms granularity; loader stop136736.475593031→entry142.716039s, with zero earlier native CPU samples. Both requested profiles include all observed samples in their intervals.
The existing fixture API timer is393.694989634s; internal exit timer388.843170562s has a narrower scope. Post-call work includes the external30s after-face probe and is excluded from both profiles.

## T1 — exclusive identified leaf samples and top20

User-space CPU samples only; not wall fractions. Shared helpers stay Other; inlined/inclusive costs are unresolved. NR means fewer100samples behind a share. B totals cover both invocations plus CFD/preparation.
| Profile | N | Proof | Predicates | Target/BL | Tree | Memory | MPI | Pack | Cavity | Other |
|---|---:|---|---|---|---|---|---|---|---|---|
| A prep | 13,883 | 10 NR | 734 (5.29%) | 17 NR | 139 (1.00%) | 221 (1.59%) | 0 NR | 9 NR | 11 NR | 12,742 (91.78%) |
| A native | 38,549 | 688 (1.78%) | 3,395 (8.81%) | 11,297 (29.31%) | 579 (1.50%) | 821 (2.13%) | 2 NR | 56 NR | 83 NR | 21,628 (56.11%) |
| B rank0 | 958,860 | 1,074 (0.11%) | 17,946 (1.87%) | 2,424 (0.25%) | 7,047 (0.73%) | 8,753 (0.91%) | 752,249 (78.45%) | 752 (0.08%) | 145 (0.02%) | 168,470 (17.57%) |
| B rank1 | 944,973 | 220 (0.02%) | 9,994 (1.06%) | 2,225 (0.24%) | 5,955 (0.63%) | 5,490 (0.58%) | 851,266 (90.08%) | 787 (0.08%) | 114 (0.01%) | 68,922 (7.29%) |
| B rank2 | 953,488 | 631 (0.07%) | 14,508 (1.52%) | 4,081 (0.43%) | 7,661 (0.80%) | 8,408 (0.88%) | 787,725 (82.62%) | 820 (0.09%) | 166 (0.02%) | 129,488 (13.58%) |
| B rank3 | 960,627 | 1,357 (0.14%) | 41,585 (4.33%) | 1,440 (0.15%) | 6,878 (0.72%) | 9,382 (0.98%) | 638,489 (66.47%) | 824 (0.09%) | 168 (0.02%) | 260,504 (27.12%) |
| B rank4 | 953,647 | 912 (0.10%) | 8,909 (0.93%) | 2,108 (0.22%) | 6,072 (0.64%) | 6,953 (0.73%) | 782,116 (82.01%) | 714 (0.07%) | 138 (0.01%) | 145,725 (15.28%) |
| B rank5 | 954,405 | 1,161 (0.12%) | 19,713 (2.07%) | 1,452 (0.15%) | 7,301 (0.76%) | 9,631 (1.01%) | 722,803 (75.73%) | 857 (0.09%) | 150 (0.02%) | 191,337 (20.05%) |
Top20 entries give symbol,count(share of the corresponding N); shortened namespaces G=geometry anonymous, P=native anonymous, N=native. Full signatures/DSOs and unique label hashes: symbol_legend.json. `?@module` is an unresolved symbol aggregated within that known DSO.
| Profile | Top20 symbols |
|---|---|
| A prep | `G::Multiply(R,R)` 3,552 (25.59%); `__nexttowardl` 3,094 (22.29%); `G::Add` 1,599 (11.52%); `G::Multiply(R,scalar)` 1,115 (8.03%); `G::Adjacent` 955 (6.88%); `G::Blossom` 581 (4.18%); `P::Coordinate` 216 (1.56%); `G::EdgeNormal` 170 (1.22%); `P::Difference<66ul>` 154 (1.11%); `G::CrossBox` 134 (0.97%); `P::Difference<132ul>` 132 (0.95%); `G::Difference` 130 (0.94%); `G::CubicNormalControls` 114 (0.82%); `P::Integer<` 105 (0.76%); `G::TriangleDistance` 100 (0.72%); `G::NormBound` 96 NR; `N::PlanarReference::Get` 94 NR; `P::ParametricBounds` 92 NR; `__memset_avx2_unaligned_erms` 75 NR; `N::BoundaryParameterState::Entity` 58 NR |
| A native | `OptionA::InStrip` 6,292 (16.32%); `BL::BoundaryGuidance lambda` 4,935 (12.80%); `G::Multiply(R,scalar)` 3,618 (9.39%); `__nexttowardl` 3,282 (8.51%); `G::Adjacent` 3,240 (8.40%); `G::Add` 2,707 (7.02%); `G::Multiply(R,R)` 2,652 (6.88%); `G::Blossom` 1,636 (4.24%); `P::Coordinate` 926 (2.40%); `P::Difference<66ul>` 833 (2.16%); `Geom::MappedFacetReference::ClosestCurved` 608 (1.58%); `P::Difference<132ul>` 496 (1.29%); `P::Integer<` 412 (1.07%); `G::NormBound` 361 (0.94%); `G::TriangleDistance` 325 (0.84%); `G::UnionChord` 310 (0.80%); `P::Difference<198ul>` 290 (0.75%); `__memset_avx2_unaligned_erms` 261 (0.68%); `P::Integer<` 231 (0.60%); `G::Difference` 211 (0.55%) |
| B rank0 | `?@mca_btl_vader.so` 396,972 (41.40%); `opal_progress` 128,694 (13.42%); `?@mca_osc_pt2pt.so` 56,648 (5.91%); `?@libopen-pal.so.40.30.3` 53,157 (5.54%); `mca_common_ompio_progress` 47,374 (4.94%); `ompi_coll_libnbc_progress` 45,778 (4.77%); `G::Adjacent` 34,962 (3.65%); `G::Multiply(R,scalar)` 26,460 (2.76%); `G::Add` 18,937 (1.97%); `G::Multiply(R,R)` 18,532 (1.93%); `mca_pml_ob1_recv` 15,168 (1.58%); `__nexttowardl` 15,053 (1.57%); `G::Blossom` 13,671 (1.43%); `opal_progress@plt` 7,614 (0.79%); `P::Coordinate` 5,803 (0.61%); `G::UnionChord` 5,402 (0.56%); `P::Difference<66ul>` 3,556 (0.37%); `void CBlasStructure::tql2<double [3][3], double [3], double [3]>` 2,982 (0.31%); `P::Difference<198ul>` 2,624 (0.27%); `G::NormBound` 2,624 (0.27%) |
| B rank1 | `?@mca_btl_vader.so` 492,870 (52.16%); `opal_progress` 136,937 (14.49%); `?@mca_osc_pt2pt.so` 69,793 (7.39%); `?@libopen-pal.so.40.30.3` 65,672 (6.95%); `ompi_coll_libnbc_progress` 56,845 (6.02%); `ompi_request_default_wait` 14,820 (1.57%); `__nexttowardl` 10,308 (1.09%); `G::Multiply(R,scalar)` 8,685 (0.92%); `G::Adjacent` 7,680 (0.81%); `opal_progress@plt` 7,385 (0.78%); `G::Add` 7,321 (0.77%); `G::Multiply(R,R)` 6,762 (0.72%); `mca_pml_ob1_recv` 3,926 (0.42%); `P::Coordinate` 3,264 (0.35%); `G::Blossom` 3,186 (0.34%); `void CBlasStructure::tql2<double [3][3], double [3], double [3]>` 2,465 (0.26%); `CSolver::ComputeMetric` 2,151 (0.23%); `P::Difference<66ul>` 1,953 (0.21%); `opal_progress@plt` 1,885 (0.20%); `_int_malloc` 1,383 (0.15%) |
| B rank2 | `?@mca_btl_vader.so` 442,063 (46.36%); `opal_progress` 130,481 (13.68%); `?@mca_osc_pt2pt.so` 72,198 (7.57%); `?@libopen-pal.so.40.30.3` 62,340 (6.54%); `ompi_coll_libnbc_progress` 53,319 (5.59%); `G::Adjacent` 21,334 (2.24%); `G::Multiply(R,scalar)` 18,231 (1.91%); `mca_pml_ob1_recv` 17,591 (1.84%); `__nexttowardl` 14,799 (1.55%); `G::Add` 13,262 (1.39%); `G::Multiply(R,R)` 12,946 (1.36%); `opal_progress@plt` 8,824 (0.93%); `G::Blossom` 8,726 (0.92%); `void CBlasStructure::tql2<double [3][3], double [3], double [3]>` 5,757 (0.60%); `P::Coordinate` 4,694 (0.49%); `CSolver::ComputeMetric` 3,984 (0.42%); `G::UnionChord` 3,138 (0.33%); `P::Difference<66ul>` 2,831 (0.30%); `_int_malloc` 2,353 (0.25%); `P::Difference<198ul>` 2,048 (0.21%) |
| B rank3 | `?@mca_btl_vader.so` 359,158 (37.39%); `opal_progress` 106,873 (11.13%); `?@mca_osc_pt2pt.so` 52,974 (5.51%); `?@libopen-pal.so.40.30.3` 50,178 (5.22%); `__nexttowardl` 47,978 (4.99%); `ompi_coll_libnbc_progress` 46,919 (4.88%); `G::Adjacent` 44,546 (4.64%); `G::Multiply(R,scalar)` 39,080 (4.07%); `G::Add` 32,852 (3.42%); `G::Multiply(R,R)` 19,929 (2.07%); `P::Coordinate` 12,775 (1.33%); `ompi_request_default_wait` 11,271 (1.17%); `G::Blossom` 10,342 (1.08%); `P::Difference<66ul>` 8,082 (0.84%); `P::Difference<198ul>` 6,877 (0.72%); `G::TriangleDistance` 6,741 (0.70%); `G::BoxGap` 6,432 (0.67%); `G::NormBound` 5,822 (0.61%); `opal_progress@plt` 5,715 (0.59%); `P::Difference<132ul>` 5,206 (0.54%) |
| B rank4 | `?@mca_btl_vader.so` 442,654 (46.42%); `opal_progress` 132,867 (13.93%); `?@mca_osc_pt2pt.so` 64,836 (6.80%); `?@libopen-pal.so.40.30.3` 61,375 (6.44%); `ompi_coll_libnbc_progress` 52,902 (5.55%); `G::Adjacent` 33,976 (3.56%); `G::Multiply(R,scalar)` 23,309 (2.44%); `ompi_request_default_wait` 17,595 (1.85%); `G::Multiply(R,R)` 17,166 (1.80%); `G::Add` 15,932 (1.67%); `G::Blossom` 13,815 (1.45%); `__nexttowardl` 9,722 (1.02%); `opal_progress@plt` 8,988 (0.94%); `G::UnionChord` 5,265 (0.55%); `P::Coordinate` 2,899 (0.30%); `void CBlasStructure::tql2<double [3][3], double [3], double [3]>` 2,409 (0.25%); `G::NormBound` 2,243 (0.24%); `CSolver::ComputeMetric` 2,043 (0.21%); `P::Difference<66ul>` 1,604 (0.17%); `CADTBaseClass::BuildADT` 1,535 (0.16%) |
| B rank5 | `?@mca_btl_vader.so` 416,982 (43.69%); `opal_progress` 117,285 (12.29%); `?@mca_osc_pt2pt.so` 59,187 (6.20%); `?@libopen-pal.so.40.30.3` 55,546 (5.82%); `ompi_coll_libnbc_progress` 48,869 (5.12%); `G::Adjacent` 40,624 (4.26%); `G::Multiply(R,scalar)` 30,966 (3.24%); `G::Add` 22,278 (2.33%); `G::Multiply(R,R)` 21,428 (2.25%); `__nexttowardl` 17,859 (1.87%); `ompi_request_default_wait` 16,026 (1.68%); `G::Blossom` 15,617 (1.64%); `opal_progress@plt` 7,984 (0.84%); `G::UnionChord` 6,258 (0.66%); `P::Coordinate` 6,173 (0.65%); `P::Difference<66ul>` 3,910 (0.41%); `G::NormBound` 3,206 (0.34%); `P::Difference<198ul>` 2,983 (0.31%); `P::Difference<132ul>` 2,429 (0.25%); `_int_malloc` 2,315 (0.24%) |
A prep interval arithmetic(Multiply/Add/Adjacent/Blossom)+__nexttowardl10896/13883(78.48%); __nexttowardl3094/13883(22.29%). Full native interval+libm17135/38549(44.45%); v1 approximate cuts are not equivalent phase boundaries.

## T2 — Run B phase bins

Cells below: max-rank boundary seconds / global accepted edits / edits per max-rank second; C=tracked collective calls,E=World::elect calls (common per-rank counts, not summed six times). Literal world exchanges were not emitted.
C*=initialization+collapse; S*=surface with generation/setup and any failed return; R*=ownership staging/commit plus phase setup; Q*=quality_entry→failed exit; A*=emitted handoffs/setup. These are disjoint observable bins, not pure operator timers.
| Phase | Invocation1 | Invocation2 | World exchanges |
|---|---|---|---|
| C* collapse | 1423.519s / 2496 / 1.753; C119540 E261115 | 1521.328s / 2510 / 1.650; C113270 E248664 | NR |
| S* surface | 2044.353s / 10995 / 5.378; C320148 E664276 | 2739.857s / 10104 / 3.688; C338759 E702143 | NR |
| I interior | 59.010s / 70537 / 1195.339; C1151211 E2288695 | 1058.662s / 70354 / 66.456; C1119160 E2223403 | NR |
| R* repartition/setup | 65.399s / 0 / 0.000; C12836 E63947 | 4.682s / 0 / 0.000; C40 E92 | NR |
| Q* quality | 1751.015s / 2477 / 1.415; C8392 E24630 | not reached | NR |
| A* alternation/setup | 1.502s / 0 / 0.000; C7 E10 | 0.164s / 0 / 0.000; C0 E0 | NR |
| Strict finish | not reached | not reached | NR |
Named S/I lane counters=2027.815/136.764s and2723.961/1073.726s; they differ from boundary bins because timer scopes span generation/setup and failed returns. Logged ownership timer maxima=20.102s/4.678s; R* includes additional setup. Per-rank intervals/counters: run_B_phase_samples.json and final_tables.json.

## T3 — compute against wait by rank/phase

Each cell W/C(counts),then W%; W=MPI-library samples under the brief’s convention,C=other sampled CPU,N=W+C. NR if either count<100. Off-CPU/kernel wall wait and slowest-rank causality are not measured.
Only timestamps guaranteed within epoch brackets are assigned (width0.356–0.379s for invocation1,0.012–0.014s for invocation2); boundary-ambiguous samples are excluded. GDB variable read failed nonfatally; source+ELF disassembly bound the epoch. Phase definitions match T2.
| Inv/rank | C* | S* | I | R* | Q* | A* | Finish |
|---|---|---|---|---|---|---|---|
| 1/0 | 124,761/13,191 (90.44%) | 190,155/3,655 (98.11%) | 2,534/2,270 (52.75%) | 2,624/3,044 (46.29%) | 147,966/12,479 (92.22%) | 0/66 NR | not reached |
| 1/1 | 128,601/9,580 (93.07%) | 160,704/32,634 (83.12%) | 2,768/2,003 (58.02%) | 2,100/3,202 (39.61%) | 151,349/8,728 (94.55%) | 1/95 NR | not reached |
| 1/2 | 122,442/15,604 (88.70%) | 171,627/20,632 (89.27%) | 2,190/2,634 (45.40%) | 1,979/3,272 (37.69%) | 146,518/12,241 (92.29%) | 0/75 NR | not reached |
| 1/3 | 44,838/93,312 (32.46%) | 153,196/39,573 (79.47%) | 2,057/2,795 (42.39%) | 2,286/3,148 (42.07%) | 102,801/58,148 (63.87%) | 0/80 NR | not reached |
| 1/4 | 122,782/15,372 (88.87%) | 184,086/7,869 (95.90%) | 2,977/1,836 (61.85%) | 3,010/2,677 (52.93%) | 65,361/96,352 (40.42%) | 2/82 NR | not reached |
| 1/5 | 120,353/17,786 (87.12%) | 54,760/137,414 (28.50%) | 2,455/2,346 (51.14%) | 1,985/3,415 (36.76%) | 147,814/13,968 (91.37%) | 0/95 NR | not reached |
| 2/0 | 132,922/9,824 (93.12%) | 85,898/138,775 (38.23%) | 57,805/3,419 (94.42%) | 71/344 NR | not reached | 0/0 NR | not reached |
| 2/1 | 129,465/12,287 (91.33%) | 217,663/3,964 (98.21%) | 50,670/2,226 (95.79%) | 44/357 NR | not reached | 0/0 NR | not reached |
| 2/2 | 127,434/15,354 (89.25%) | 158,118/66,660 (70.34%) | 55,587/3,748 (93.68%) | 68/355 NR | not reached | 0/0 NR | not reached |
| 2/3 | 49,154/92,868 (34.61%) | 214,087/12,190 (94.61%) | 59,693/3,511 (94.44%) | 56/352 NR | not reached | 0/0 NR | not reached |
| 2/4 | 120,493/21,648 (84.77%) | 220,094/4,102 (98.17%) | 54,752/3,085 (94.67%) | 44/358 NR | not reached | 0/0 NR | not reached |
| 2/5 | 126,908/15,595 (89.06%) | 205,524/20,529 (90.92%) | 53,320/3,399 (94.01%) | 59/356 NR | not reached | 0/0 NR | not reached |
Excluded ambiguous samples by rank0–5: 2043,2128,2100,2032,1990,2017. Full assignments and clock bounds are retained; no epoch or CPU seconds estimated from sample frequency.

## T4 — timer coverage

| Rank | Inner exclusive coverage1 / residual seconds | Inner exclusive coverage2 / residual seconds |
|---|---|---|
| 0 | 45.23% / 2927.265 | 69.70% / 1613.204 |
| 1 | 44.58% / 2961.712 | 69.79% / 1608.570 |
| 2 | 45.34% / 2921.162 | 69.78% / 1608.889 |
| 3 | 53.82% / 2467.848 | 69.78% / 1609.002 |
| 4 | 58.20% / 2233.805 | 69.79% / 1608.294 |
| 5 | 45.71% / 2901.437 | 69.74% / 1611.169 |
Coverage=1−reported other/total in NATIVE3D_DETAIL(refine). Outer remesher buckets cover99.98–99.98% by enclosing refinement; that is a different level. Residual is not proof of wholly untimed code: nested counters/scoped failure returns overlap or leave attribution gaps; union coverage is unresolved.
A emits no complete exclusive bucket breakdown. Engine upper173.858064s/388.843171s=44.71% and geometry-admission guidance144.738106s=37.22% overlap. Fixture upper233.722574s includes two external before/after probes; do not mix scopes.
Largest named A target self routines are InStrip and BoundaryGuidance lambda (T1); neither has a dedicated wall timer, and initial-census calls can land in other (CPU-only measurement timer). Exact largest functions wholly outside every timer are not identifiable from these flat profiles; no residual-function cost is invented.

## T5 — peak resident memory

| Rank command | Observed native /proc VmHWM KiB | GNU-time maxRSS KiB |
|---|---:|---:|
| A0 | 3100520 | 3100520 |
| B0 | 2231880 | 2230580 |
| B1 | 2505212 | 2504100 |
| B2 | 2441384 | 2440260 |
| B3 | 2435640 | 2434456 |
| B4 | 2327412 | 2326256 |
| B5 | 2434148 | 2432604 |
/proc polled every0.5s; GNU time wraps debugger/native descendants and kernel accounting differs slightly. No whole-MPI peak is inferred from one rank or from summing individual maxima.

## T6 — audit claims

| # | Finding |
|---:|---|
| 1 | Not verified as wall wait/slowest-rank cause. Guaranteed native-window MPI CPU shares range67.25–91.83% (counts T3), not37–47%. |
| 2 | Literal one edit on every rank in every round is false: first interior3490–8438splits per rank/8492rounds. An at-most-one cap is not measured by aggregate counts. |
| 3 | Recorded (C+E)/reported round is219.872/215.471, amortized including setup. Per-round distribution and literal world synchronization/exchange counts remain unmeasured. |
| 4 | World election means(rank0)=1020.894/963.393µs, not177µs. Directory-family means=75.364/336.017µs; do not substitute/add them. Waiting versus message cost not separated. |
| 5 | Not measured: neither invocation reached strict accepted finish; no new520s-per-rank proof result. |
| 6 | First interior pass1820.37global edits/s; cumulative boundary-bin rates1195.34/66.46. A universal1024 is unsupported; measured surface/quality costs are in T2 (quality2 unreached). |
| 7 | A1897reservation conflicts/2640surface rounds=71.86%; that is not the explicit deferral counter. Proof/admission timer union is unmeasured, so “proof plus guidance takes most” cannot be established by adding them. |
| 8 | Identified self costs are small: A native tree579/38549(1.50%), memory821/38549(2.13%); B per-rank counts/shares T1. Inclusive/inlined tree/allocation cost remains unresolved. |

## T7 — sampled near callers (optional LBR)

Use run_A/nearest_callers.json and caller_review.json; broad caller_summary.json is superseded. Only plausible near prefixes are attributed; far LBR tails may contain stale history. Finite60s windows, not whole-phase inclusive shares or invocation counts.
| Question | Measured evidence |
|---|---|
| Preparation interval callers | Of3178: mobility2461(77.44%), parameter/coverage510(16.05%), unresolved138(4.34%); point/normal69 NR. |
| Native interval callers | Of808: target surface evaluation527(65.22%), parameter/coverage126(15.59%); point/normal83, local face deviation40, union proof6, mobility12, unresolved14 each NR. |
| __nexttowardl / nextafter | Prep near Adjacent1319/1355(97.34%); native90/98 NR. Remaining near symbols cannot distinguish direct nextafter from inlined Adjacent. |
| InStrip | 2301/2301 sampled near parents are BoundaryGuidance lambda; exact calls per guidance evaluation not measured. |
| Guidance lambda | 1634/1634 show method→fixture handler→MeasureSurfacePlanning; seed/scoring/candidate/census outer step unresolved. |

## Difference/overhead check and remaining limits

A matched API393.694989634s vs389.572521653s=+1.058%;300vs305edits=−1.639%; neither exceeds5%. No correction.
B inner native durations1/2: 5344.540316s vs3640.719259s (+46.80%,FLAG>5%); 5324.459194s vs2314.855222s (+130.01%,FLAG>5%). First rank0 accepted edits11586vs18201; both current events rejected instead of accepted. No timing correction.
Second current subiteration starts from retained coarse mesh after rejection; reference second starts from the accepted first adaptation. Executable/build(older test-driver versus494 CFD), external debugger clock probes and host contention also differ; these are not isolated profiler-overhead measurements.
Host load changed from3.10 at B launch to15–17; a2.099s receipt observed unrelated Lean at173.4%CPU and native ranks74.8–94.8%. No whole-run contention cost or slowest-rank cause is estimated.
Conservative retained profiler/support data across v1+v2=1,542,163,121B (<2,000,000,000B); raw perf stays local4TB/out ofgit. All six optional run LBR captures decoded0 with --no-inline; their raw data was removed after summary. No DWARF call graph.
Unmeasured: literal world exchanges/per-round sync distribution, off-CPU wall wait, timer-union/complete residual-function attribution, exact guidance calls/evaluation, outer guidance step, strict accepted finish, inclusive/inlined costs and private mesh point counts. Native failure debts are not accepted mesh quality.
All owned handles terminal; normal exits/flat success/hash receipts retained. No refactoring recommendation or design work. Measurement ends here.
