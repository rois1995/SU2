# Existing recovery guards during frozen253 — source facts

2026-11-07. Read-only supporting survey of known blocked boundary demand;NO
implementation,fixture,test,configuration,budget or runtime-input change. The full
M6 attempt stays frozen. A bounded routine agent survey was independently checked
by the primary agent against the current pinned source. This is source evidence,
not an operator capability or runtime transition result.

Common/src/adaptation/CNativeEngine3D.cpp SHA256: `5ba75ce1c6ac985c4ed5ea87e4e95d745a71f74885d81e6d40dd9a90bf3c8d68`.

- Best(true),388–393,returns inactive when repair_queue is empty. Global any is
  world.maximum(choice.active),2910. Service(false),848–853,removes a rejected
  feature from both queues and marks it blocked. TouchRepairSupport,505–514,
  unblocks matching features after their complete support changes;publication
  invokes it at5477–5482. Unchanged rejected supports remain absent from the queue.
- clean_quality,2957–2958,requires globally zero quality_invalid,zero unmeasured
  shape and a complete cached row set. Dedicated size/geometry-repair modes at
  2960–2973 are gated on that condition. They do require closed shape/height.
- A SEPARATE return to Construction,3002–3011,does not universally require q/Jzero.
  It requires globally published improvement progress (or a clean-quality
  construction handoff),remaining construction demand (or standalone coarsening),
  and either clean demand with no dedicated repair mode OR global any=false.
  Thus after prior productive improvement,an exhausted/blocked q/J queue plus
  remaining construction-invalid demand can reach this existing return path.
- In bidirectional mode refine_geometry additionally requires progress.complete,
  zero size/height bad or unresolved counts,and nonzero geometry-bad counts.
  Progress.complete,346–363,means measurement/census coverage,not zero q/Jbad.
  Geometry invalidity contributes to construction_invalid at731/822–827.
- enter_phase(Construction,true),2778–2805,calls begin_length_generation(false).
  That calls length_direction(false) and ChangePhase(false),2763–2772,seeding the
  existing validity-first construction pass. It does not bypass final q/Jgates;
  shape must subsequently be measured/reclosed. Successful quality publication
  sets improvement_changed=true at5455–5458.

Known251context209724804 had all14features blocked and geometry debt. Existing
single-star,whole-context and collapse+retained-star controls found no useful
quality candidate there. Current253has a small blocked set while global quality
work remains productive. These facts DO NOT establish a hard global shape-zero
barrier:source already permits recovery after queue exhaustion under the guards
above. A growing blocked count also does not prove every star is permanently
blocked:neighbor publications can reopen support-dependent demand.

Wait for the frozen run's terminal outcome and complete counts/flags,queue/stuck
set and geometry-return records. Do not introduce new phase scheduling,a larger
cavity/operator campaign,or loosen budgets/gates from an assumed phase deadlock.
No accepted mesh,geometry closure,transfer,adapted CFD or disk restart is proven
by this source review. Current shared witness253optimization remains the ONE
active implementation/validation cycle.
