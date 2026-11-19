# N21 whole-item source check

Base f968325a69 and restored build586. The pinned I8T input and the current work
order were read before production edits. The host preflight found no owned heavy job.

| Scope | Source and reading used |
| --- | --- |
| P1 | There is one backend interface and one override. The backend already scans the ordered reference map for body, sheet and marker, reserves the full facet count and charges that capacity. Filter the selected facets after that charge. The model-only query builds the complete triangle union from the supplied physical faces and does not build the model union. Its only model use is root initialization and traversal. Retained sorted facets therefore leave the target construction and caller bytes unchanged. |
| P2 | The boundary state and backend use the same body/marker and sheet lookup: policy sheets when active, canonical facet otherwise. The entity face loop, certification admission, direction selection and query can be shared. Count entity roots in the reference facet pass, then compute the kept count by stride. Adopt traverses keys and counts physical faces without a query or timer; the two directed values and existing tolerance expression suffice. |
| P3 | The existing reverse proof's reserve, collection, election, SUM ownership check and MAX per-entity are unchanged. The existing typed World reduce and election add the instrumented 2E reductions and one vote; they use body/sheet/marker map order across identical replicated states. A local standard exception goes through CLocalFailure before any result reduction. The outer control vote still handles final rejection. |
| P4 | EngineControl.cpp is included in the engine translation unit. Keep report state local to ProveOwnedSurface, clock reads conditional, and print from the control boundary without a new timer scope. The certificate scope is retained for the forward query, and the existing outer scope and timers stay in place. |
| T1–T4 | Use new cases next to the existing entity and missing-face fixtures. Use distinct serial/MPI tags. The MPI case calls the extracted forward function with World and an independent state for the old query. |
| W | The exact three source paths from historical 6b56b42f16 are available. P does not change those paths. I8R2 section 5 and section 6 items 5–7 govern W along with I8T's new run locations and time rule. The factor is 0.95 by user order. |

## Contradictions and readings used

1. The legacy backend converts invalid arguments inside its try into false/reason. P1 and T2 explicitly require invalid shares to throw. Validate the new argument before the try, leaving the old direction and body error path intact.
2. P1 says to return zero for an empty *kept* list. The old unshared query rejects an empty full selection. Only apply the empty shortcut in the shared model-only branch after the full list has been built and the scope checked. A selected entity with fewer roots than ranks is allowed.
3. The spec names the kept count but the backend result's fields are shared with existing cached certificates. Keep the count in the separate local share result and derive it from the canonical entity facet counts. No certificate record layout or budget charge changes.
4. P5.7 prescribes one full two-event run. AGENTS §2 asks for an isolated restart where C asks for it, and C's G5 gate does not ask for one here. Perform the explicitly required isolated event-two restart in W, after run 1.
5. Two-sided flags and the passing maximum follow the existing per-root algorithm. In a resource or representability exception, the existing failure protocol still applies; no bound is accepted from a locally failed query.

No unresolved contradiction blocks P or W.
