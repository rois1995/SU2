# N14 inventory correction record

The earlier `N14_source_binding_inventory.md` mixed current-source and `5488d1f1b5` facts. While reviewing the commit’s restoration symbols, I attributed its `WallFaceEdgeWeights`, `ObserveSource` and `NATIVE3D_WALL_SOURCE` additions to the current source without checking those exact names against the current files. That was an inventory error.

I preserved the earlier note as `N14_source_binding_inventory.md.before`, then checked the current `ffcb292b4639128c26495f7df44a4b63b9c61b0df62222547f4b4ad253ba9020` source and wrote `N14_source_binding_inventory.corrected.md`.

Verified current state: none of those four symbols is present; the current engine has the base `EstimateWallEdge` census helper and still prints its `entry` census at line 1545. The corrected inventory distinguishes this from the symbols/entry-call removal in 5488. It also treats N14 as released by the later explicit user order, superseding the historical archived “prepared, not released” text.

No production/test/policy/pinned files were changed; no build, test, or run was started.
