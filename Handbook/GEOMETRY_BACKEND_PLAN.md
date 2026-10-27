# Geometry backend plan

Status: native interface/adapter implemented; build32 and focused45-case suite
pass, including backend cases and existing cubic differential checks. Broader
M6/transfer/restart qualification remains open. Open CASCADE/importers are planned. Current
atlas implementation and its outstanding work are described in
[CURVED_ATLAS_IMPLEMENTATION.md](CURVED_ATLAS_IMPLEMENTATION.md).

The intended backend interface is small and represents the prescribed geometry,
its associations and its genuine features. It should provide:

- Surface and curve evaluation at associated parameters.
- Projection constrained to the appropriate surface, curve or feature.
- Normals and curvature where defined.
- Stable feature and body identity, including separation between bodies.
- Geometry error checks against the requested geometry tolerance.
- Persistent associations that survive mesh edits, transfer and restart.

Exercise that interface first with the existing cubic geometry, including moves
across chart boundaries and the preservation of feature and body constraints.
A cubic implementation and its tests do not establish CAD support or resolve
historical correspondence growth.

An optional OpenCASCADE backend can follow once the interface and cubic path are
qualified. The intended input sequence is STEP, then STL, then IGES. STL provides
faceted geometry only: do not infer smooth CAD surfaces from it. Actual support
for each format requires its own implementation and validation. Format-specific infrastructure and import code remain deferred until the
interface is exercised.

The interface must connect to native patch construction and remeshing operators,
boundary parameter publication and persistence, source-envelope construction,
and reference I/O. Geometry associations and identities must remain coherent
through distributed publication and reload; solver compatibility and solution
transfer remain separately validated obligations.

Construction remains validity-first: positive nondegenerate elements, topology,
geometry authority and body separation are mandatory. Shape measurements and
improvement belong to the subsequent quality stage. Final acceptance retains all
agreed quality, Jacobian, metric size, requested first-height, geometry, transfer
and restart gates. No backend proposal changes those gates.

The current implementation keeps `SurfaceBinding` and native v5-v8 archive
semantics. Backend identity exposes an in-process authority token, model version
and association schema; the token is never persisted and does not prove model
equality. Full immutable authority declarations remain the persistence/agreement
source of truth. The native adapter reuses the existing sided differential and
certified component search; an unsupported or out-of-scope projection refuses.
It does not claim unique nearest points or CAD certification. Fresh patch planning
uses the interface for evaluation, association admission and physical support;
whole-PN embedding and source separation retain their specialized certificates.
