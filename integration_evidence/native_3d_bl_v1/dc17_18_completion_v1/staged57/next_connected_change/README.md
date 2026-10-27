# Next connected implementation — drafts, not validation

These patches are unapplied and unbuilt. The running M6 build61 sources remain unchanged.

- `UNAPPLIED_support_index.patch`: current two-face incidence and original-support/current-face lookup, incremental publication and load/rebind reconstruction. Intended caller: planned local shared-edge normalization; integrate and test them together. Preserve all current archive fixes when applying.
- `UNAPPLIED_empty_rank_fixture.patch`: elect and broadcast common RANS initialization from a populated rank. The broad MPI61 campaign aborted at the fixture's unguarded point-zero read before reaching the full selection; this patch still needs validation.

The proposed global normalizer was rejected. See `Handbook/WORKING_SURFACE_RECONSTRUCTION_DECISION.md` for the connected local closure, mandatory certificates and routing limits. No accepted M6/BL adaptation, complete transfer or solution restart is established by these drafts.
