# Own helper corrections

- Post-only manifest writer used `h.parents[1]` for the phase status, which names historical N4. Restored the clean historical file byte-for-byte from HEAD immediately and wrote the phase status at `h.parent`. No numerical input, test or production code changed.
