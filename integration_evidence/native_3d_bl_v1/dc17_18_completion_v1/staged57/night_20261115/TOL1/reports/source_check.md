# TOL1 whole-item source check

Base a94f2f0980. The copied order was checked against the user’s SHA256.
This audit uses base line numbers and precedes production edits.

## Configuration and reference setup

- `CConfig.cpp:3352-3355,6528-6536`: the DEF1 option already parses a
  canonical name-ordered map and resets the scalar on inheritance. Native
  postprocessing sets the strict value from that map and validates the
  native upper bound and the raw BL marker count (1379 in CConfig.hpp).
  MMG keeps its scalar and map path. `SetMarkers` (6881–7470, called
  after postprocessing at 155, 191, 235, 277) checks declared names.
- The 2D rejection belongs in `CNativeRemesher.cpp:34`. In 3D,
  `CNativeRemesher3D.cpp:108-145` may first load a sidecar and resolve the
  inventory through ImportGeometry. Reject unsupported authority after
  load/import and before mobility. The event's second preparation site
  is `NativeRemeshRun.inc:285-299` before faceted promotion (304–326).
  Reject before that promotion, once the authority exists. On already
  loaded mapped state also validate mobility at the first preparation
  site (245). Tolerance floor reads original reference coordinates,
  including cubic controls, in original geometry units.
- Parent agreement belongs in TryRemesh at `NativeRemeshRun.inc:167-204`,
  before any reference preparation or worker dispatch. One MIN reduction
  of the word and its complement yields both extrema without a custom
  collective. The map iteration order and binary64 values supply the
  config-only identity. Existing engine agreement on the strict scalar
  remains `EngineSetup.cpp:21-24`.
- Remesh owns one projected table outside charged scalar data. Charge it
  through the caller/retained/admission chain, including provisional
  allocation. `CNativeReferenceIO3D.cpp:126,291` keeps scalar s in the
  header and policies in the existing encoded roles. `CSolver.cpp:2640,
  2653,4113`, the BL ramp signature and target checkpoint keep their
  layouts and the strict scalar. Native BL rejects list mode before use.

## Frozen seam

- Role producers and exact identity consumers occur in
  `CNativeRemesher3D.cpp:75-94`,
  `CNativeBoundaryParameters3D.cpp:66-68,144,250-285,378-383,732-747`,
  `CNativeAffineReference.cpp:2091,2127,2145`,
  `CNativeDistributed3D.cpp:1544`, and
  `CNativeLocalSurfaceGeometry3D.cpp:427,939`. These keep the full encoded
  role. Rebuild and agreement use the same values through reader
  rebind (`CNativeImport3D.cpp:2076)) and sidecar decode.
- Role-kind validation sites are `CNativeAffineReference.cpp:2093` and
  `CNativeBoundaryParameters3D.cpp:218` (also reject a negative code).
  Separate BL roles at `CNativeBLTarget3D.hpp:171,481,524,547-556`,
  `CNativeBLPipeline3D.hpp:382,418`, `BLFirstRow9.inl:191-192` and
  `CNativeTargetCheckpoint3D.hpp:146,198-199,222,228` are BL-only.
- Existing protected marker interfaces are classified at
  `CNativeAffineReference.cpp:2098-2117`. The corner and mobility sites
  are 2126–2134 and 2167–2180. Identify the seam by classes of the
  original edge’s two incident marker facets, including a mesh vertex
  bound to the interior of that edge.
- Engine split paths `CNativeBoundary3D.cpp:213,1562,1677,2016,2208,2288,2313`
  funnel through BindNewPoint (BoundaryParameters 1678–1702).
  Movement at 264,3266 and optimizer sites 1191,1339 reach mobility or
  BindMovedPoint (1704–1731); private atlas moves at 299,433–434 use
  mobility. Collapse at 3076,3164 and `EngineSelectionQueues.cpp:424`
  use the backend ContractionAllowed (distributed use at
  `CNativeDistributed3D.cpp:857)). Optimizer boundary flip at 2087–2106
  requires the same facet; mixed-marker seams already fail it. Interior
  flips preserve physical skin (`CNativeBoundary3D.cpp:2960)).
- Core surface L/D go through `CNativeLocalCore3DSurface.cpp:359` and
  `StateSurfaceGeometry::Place` (699–708). Step C refusal sites are
  `CNativeLocalCore3DCollapse.cpp:184-195` and
  `CNativeLocalCore3DSurface.cpp:988`. Core free-point movement and
  interior swaps/splits at `CNativeLocalCore3DChanges.cpp:275,476,548,612`
  preserve the seam through their existing surface exclusions and
  checks. Count actual refusals, excluding reporting probes.
- Shared publication guards are `CNativeBoundaryParameters3D.cpp:907,
  948-951,1199-1240`. The mesh writer must check staged coordinates
  even without a parameter delta in `CNativeDistributed3D.cpp:1478,
  1493,1591,1608`. Core's replacement goes through BuildSurfaceState at
  `CNativeLocalSurfaceGeometry3D.cpp:925-945` and installation at
  `NativeRemeshRun.inc:1004`. Check oriented side entities and markers
  by original current mesh IDs before reader renumbering, ignoring the
  incident face’s third vertex.

## Gate readers

- Selection face comparison and debt bins
  (`EngineSelectionCensus.cpp:521-562`), trial GeometryCounts and guidance
  (`EngineTrial.cpp:303-326,476,524-559`), prepared-surface gate
  (`EngineControl.cpp:64-73`), already-valid test
  (`EngineSetup.cpp:695`), optimizer smoothing comparisons and guidance
  (`MeshOptimizer.cpp:1435-1454,2526-2550,2909-2912`).
  GeometryGuidanceExcess is shared with BL, so opt in explicitly for
  the engine (`CNativeBoundary3D.cpp:3466-3527`). Apply L to both
  excess and early rejection thresholds within the face loop.
- Entity gate query (`CNativeBoundaryParameters3D.cpp:498-540,1584-1850`):
  L controls support expansion, cutoff/classification and early pass,
  s the scope and tessellation accuracy.
  `CNativeAffineReference.cpp:859-915,1206-1219,2707-2802,2863-2888,
  2970-3026,3080-3191` covers the witness crop, forest early bound,
  exact answer and M27 keys, initial resolution, bounding boxes, inner
  witness and magnitude convergence. Keep the existing L=s path.
- Final proof bounds and eligibility (`CNativeBoundaryParameters3D.cpp:
  542-590,599-655,683-729`, `CNativeGeometryBackend3D.cpp:249-278`,
  `CNativeEngine3D.cpp:104-141,370-416`, `EngineControl.cpp:30-44`).
  Both directions use the entity marker and s accuracy. The incomplete
  source certificate cache (658–680, import at
  `CNativeImport3D.cpp:479`) stays strict and distinct in list mode.
- SC1 key and all qualifying writes
  (`CNativeGeometryCertificates3D.hpp:56`, local WithinTolerance,
  committed trials via ParameterError, owned proof).
  The census reader uses its face marker and the same key. A proof or
  bound at a looser value cannot qualify a stricter key.
- Engine collapse comparison `CNativeEngine3D.cpp:769-796` gets L
  while retaining its early-return and direct association checks.
  Core forward samples `CNativeLocalCore3DCollapse.cpp:471-478` and
  step D midpoint and face triggers `CNativeLocalCore3DSurface.cpp:
  798-812,860,944` use 0.7 L. Chord selector/trigger (744,833) use
  the minimum of both incident marker limits. Core WithinTolerance
  (`CNativeLocalSurfaceGeometry3D.cpp:777-786`) checks L.

## Strict readers and readings used

1. The signed parent-site line range now falls in AdaptWorker. Use
   TryRemesh, the actual parent admission before the worker split.
   The signed affine keeper line instead contains ComponentPointError.
   The live keeper check is the mapped backend in CNativeGeometryBackend3D.
2. The early remesher preparation can see no original authority yet.
   Validate a known authority before preparing mobility, and validate
   after import when creating the original authority. The isolated
   ADAP_LOOP=NO restart does not prepare a native reference.
3. Final proof currently emits entity bounds after its gate; list
   mode emits available calculated bounds before voting on the gate.
   The scalar emission order remains the same.
4. The common guidance default, general SurfaceError, source donor
   envelope and split prediction remain strict. The BL feedback
   (`BLEvent15.inl:251-258`) keeps its zero guidance threshold. Frozen
   BL, transfer and symmetry adoption readers stay on their existing
   scalar calls. The two cached query inputs are distinct only where
   L differs from s.
5. Step D geometric resolution/length floors remain strict even when
   the trigger uses the marker value. Core thinness and reverse checks
   (`CNativeLocalCore3DCollapse.cpp:244-266`), walking accuracy
   (`CNativeLocalSurfaceGeometry3D.cpp:625`), outward step
   (`CNativeLocalCore3DSurface.cpp:381-393`) and contact margin/pass
   proof (`NativeRemeshRun.inc:1087`,
   `CNativeLocalContact3D.cpp:366`) stay strict.
6. Strict work estimates `EngineOwnership.cpp:183-203`,
   `NativeRemeshRun.inc:551`, `CNativeImport3D.cpp:1662`,
   and predicted split parent and child bounds
   (`EngineSelectionQueues.cpp:30`, BoundaryParameters 1645) keep s.
   Lift and outside walk vetoes (`EngineTrial.cpp:1879,2044-2050`),
   wall-region and coupled wall gates (`EngineSetup.cpp:780`,
   `EngineLegacyWall.cpp:949`, `CNativeRegion3D.cpp:243`,
   `CNativeBoundary3D.cpp:2276-2300`), transfer locator bounds, and
   symmetry (`CNativeImport3D.cpp:1490`) keep s.
   Analytic-domain and affine declared mapping contracts elsewhere
   in CNativeBoundary3D are distinct from ADAP_HAUSD.
7. The native encoded policy is distinct from BL roles. The shared
   policy keeps exact full values for persistence and identity. Only
   actual native role-kind readers decode the kind.
