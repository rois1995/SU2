# B2/B3 source review — scratch only

No source edits or execution. Full P5 workflow owns frozen source.

The policy is mutable through Update and public full/base metric handles. Invalidation cannot rely on Update alone.
Use existing full/base GetSampleVersion, GetGeometrySampleVersion and GetSupportVersion, plus identities, core/h0, A/radius/strip generation, roles and marker names. Existing getter versions change on profile mutation, not on ordinary sampling.

For a parameterized context, BoundaryGuidance and IntrinsicSurfaceGuidance use the current node record/binding and immutable geometry; they do not inspect current cell-star/surface topology. Validate authority, node identity/coordinates and sensor before lookup. Key by node id, exact coordinates, binding feature/version/dimension/u/v and packed sensor. Do not invalidate all unchanged nodes for an unrelated ParameterState revision.
For a context without a stable parameter association, retain the original uncached path initially.

Strip membership uses only exact point coordinates and actual fixed strip policy; it can survive sensor/binding changes. Cache full returned sided sample vectors without changing sample/strip ordering or the old ambiguity checks.

Reserve bounded capacities before caller-byte snapshots; count the reservation, not just populated entries. TargetPolicy::StorageBytes is the existing ownership hook. Driver CSinglezoneDriver.cpp ~394-409 adds it to control.caller_bytes after policy Update. RunEvent computes stateCeiling/query scratch and caller bytes ~341-369, checks StateBytes ~404-428 and snapshots preparation.caller_bytes ~441 before guidance ~449. Direct saved-state PrepareSurface fixture also snapshots family/StateBytes before the call and needs the same reservation protocol.
Avoid double counting when control.caller_bytes already owns policy storage. Preserve old behavior when there is no admitted cache capacity. Output-vector copy scratch remains necessary even on a cache hit.

Raw policy members are public; inspect actual writer sites and rely only on proved version coverage. A direct mutex member would make TargetPolicy noncopyable; current construction sites make fresh shared policies, but verify all users before changing layout/copy properties.

Meaningful checks: exact sample-vector equality cached versus uncached; changed sensor/binding/coordinates/profile/authority/marker names cannot hit stale entries; unrelated mesh revision can reuse; rejected invalid inputs still throw before a hit; reserved/populated bytes stay within the unchanged event ceiling. Then the existing difficult saved-state serial BL run and full no-BL/2D regression gates.
