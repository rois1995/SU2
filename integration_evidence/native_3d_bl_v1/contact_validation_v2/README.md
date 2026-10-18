# Exact contact validation — scope and retained evidence

Goal3 remains ACTIVE/incomplete. The existing opt-in BL budget policy and strict
configuration behavior are preserved; this change only reduces repeated geometry
validation work in the shared native3D contact predicate.

## Exact certificate

Both tetrahedra must remain positive/finite, and all common IDs must retain exactly
the same coordinates, before any early acceptance. Coordinate bounds use comparisons
only. A strict box gap proves separation. When opposite boxes meet at a coordinate
plane, all possible contact lies in that plane. If either tetrahedron's complete
extremal simplex consists of shared vertices, it is contained in the other extremal
simplex. Their intersection is precisely the common mesh simplex. Otherwise the
original exact separating-axis and rational clipping tests run unchanged.

No tolerance, approximate coplanarity, positive-volume-only acceptance, global
embedding assumption, profile change, public ABI, cache or MPI collective is added.
The existing geometry/height/quality/composed-field/transfer gates stay in place.

## Comparisons

The previous `contact_validation_v1` record measures the unmodified predicate on
all51040 pairs of the accepted320-cell sphere release:44.874014 seconds and3482000
exact orientations. The new record checks the identical pairs in12.220260 seconds
with853706 exact orientations; all pairs pass. This is a contended local comparison,
not scaling or an affordability claim.

The5440-cell mesh has14794080 pairs:14522677 strict box gaps,117665 eligible common-
support candidates and153738 unresolved candidates. Baseline classification does
not run exact predicates. The new complete contact-only replay reaches its explicit
90-second ceiling; its failed log/receipt and empty incomplete output are preserved.
No invalid pair was reported, but partial work cannot establish a full pass. The
ceiling was not increased. The complete native comparison uses the preexisting
6144-work/300-second construction limits instead.

Focused controls cover axis face/edge/vertex and one-sided support, tiny/subnormal/
large scales, rotated non-axis fallback, unshared/partial contacts, one-ULP gaps and
overlaps, inconsistent common coordinates and invalid input. Existing primitive
N/M Euler lifecycle checks and independent oracles are reused, with current source,
build/binary/config/input/helper provenance.

## Reproduction and preservation

- `run.py`: bounded saved-mesh component comparison; intentionally returns1 at the
  recorded BL work ceiling. Preserve the receipt before any changed attempt.
- `qualify.py serial|mpi4`: resumable focused tests and actual small primitive
  lifecycles. These are not the5440-cell BL sphere CFD/transfer lifecycle.
- `construction_v1/run.py`, `release_v1/run.py`: identical frozen-field native
  construction/removal comparisons, no CFD advance/Hessian/normalization rerun.
- `audit_primitive_lifecycles.py`: existing independent lifecycle oracle.
- `verify.py`: provenance and byte-identical scientific-output checks; reuses the
  original exact geometry/height/shape audit and complete release-field oracle
  only after every relevant actual input matches. No231-second duplicate audit.

One owned heavy job, nice19, threads1, build-j1, local MPI<=4. Compiler temporaries
stay on4TB. Binary executables are retained compressed locally and excluded from
Git; source snapshots, receipts and logs preserve the executed versions. All input
symlinks stay inside this repository. No unchanged frozen M6 construction retry.

Complete native results are byte-identical to the previous accepted scientific
outputs: construction103.576070seconds versus202.612478, release20.120511versus
57.383686, total123.696581versus259.996164 (observed52.42% reduction). Work6087 is
unchanged. Serial4cases/2511assertions and MPI4four cases/rank pass. The retained
independent oracle passes15actual primitive lifecycles/30events. These remain
small Euler controls, separate from the serial big-BL mesh stages.

Complete-result clocks and remaining limits are recorded in `summary.json` and
[the current handbook](../../../Handbook/GOAL3_CONTACT_VALIDATION.md).
