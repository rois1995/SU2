# Frozen M6 construction witnesses — diagnostic55

Status: 28 bounded snapshots, 16 independently reproduced shape refusals, no
fully repaired witness. All causal construction classifications remain unresolved.
See [decision](M6_DIAGNOSTIC_DECISION.md).

## Selection and provenance

The existing hidden `NativeFrozenVolume3D` test consumes the original 303,359-cell
M6 and its saved 61,330-node sensor. Exact original node-ID/coordinate correspondence,
SPD tensors and a bitwise metric round trip are asserted by `diagnostic55/prepare.py`.
The original geometry authority sidecar is unchanged. All three physical marker
groups are preserved: FARFIELD 2,060, SYMMETRY 8,094, WING3D 38,800 triangles.

`SU2_NATIVE_FROZEN_WITNESSES=1` enables only a test-local rejected-trial observer.
It retains the first four distinct feature/surface-presence keys in each of three
categories per rank: failed geometric construction, shape refusal, other metric
refusal. Maximum storage is twelve copied patches per rank. It copies records in
memory and writes them after remeshing. There is no production switch or new
hot-path collective. Ordinary hidden replay without the environment variable has
no observer. Existing Refine already performs the relevant diagnostic-failure vote.

The sample is systematic, not a random sample or an estimate of failure frequency.
The prefix covers 2,048 work units and 476 private edits; no CFD steps or complete
BL phase execute. All captured patches have physical surface faces. Ranks 0–1
have no refusals in this prefix. Interior failures, near/far MPI-interface strata
and late validation54 failures are not represented. Their absence is a limitation,
not a claim that they are easy or nonexistent.

## Results

| Recorded category | Snapshots | Independent finding |
|---|---:|---|
| Shape refusal | 16 | All candidate cells have exact positive orientation; production's failed-cell q/J reproduced within 1e-8 relative / 1e-11 absolute |
| Geometric construction refusal | 11 | Reasons mention retained-skin visibility or a specific obstructing interface face; no causal feasibility proof |
| Metric progress refusal | 1 | Candidate passes centroid q/J thresholds but minimum q decreases from 0.214133 to 0.206724; recorded reason is no metric size/quality improvement |

All sampled centroids were located in exact-containing original tetrahedra. The
analysis reuses the existing AABB builder/query in `frozen_field_audit.py`, adds
exact rational 3D orientation/containment for the binary64 points, and interpolates
the saved nodal sensor without retriangulation, extrapolation or averaging changes
to production. Centroid quality is independently recomputed. Self-checks cover a
real donor-centroid interpolation and every captured production shape refusal.

**Counter detail discovered:** streamed production measurement in
`CNativeEngine3D.cpp:1939–1968` stops when the first cell fails shape admission.
Its recorded q/J therefore need not equal the minimum over the whole candidate.
The initial independent check assumed a patch minimum and correctly failed; its
log is preserved. Comparing the specific failing cell named in the production
reason reproduces all sixteen refusals. The analysis also reports complete patch
centroid minima separately. This is a diagnostic interpretation correction, not a
production numerical change.

`witness_rank2_0.json` (edge 23754–24333): two old cells, four proposed cells,
q=0.1839719600100712 and J=0.042789630122176694. The subsequent captured snapshot
for the same edge has five old cells and twelve proposal records, with retained-skin
visibility refusal. These observed sizes do not hit the 64/128 caps; the full
growth history was not captured, so no statement about the final limiting cap follows.

`witness_rank3_0.json` (edge 81–17687) is a second surface example from a different
rank, also with a strictly positive candidate and reproduced shape failure.

For geometric refusals, `fresh` may be partial or retained from an earlier attempt.
Positive orientations in such records do not establish that the intended replacement
was built, is embedded, or covers the cavity. No candidate is promoted to an accepted
mesh by this analysis.

## Reproduction and gaps

Evidence root: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/diagnostic55`.

```sh
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
  python3 integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/diagnostic55/witness_analysis.py
```

`observed/command.json` records the exact six-rank replay; `run.py` deliberately
refuses to overwrite an existing run. `observed/witness_rank*.json` preserve old/new
cell IDs, coordinates, physical facet IDs and sparse geometry bindings.
`witness_analysis.json` preserves all independent results and explicit unresolved
classifications. Full input identity is in `input_manifest.json` and per-run pins.

Not supplied by the callback: complete rank ownership/version records, external
collision context, every attempted growth state, exact source-query traces or
successful repairs. No complete metric-edge audit, surface/contact audit, alternative
topology repair or accepted-witness proof was performed. The next transaction
experiment in the decision document fills this specific gap; valid-intermediate
admission has not been tried here.
