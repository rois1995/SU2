# FP2-0 fixture corrections

- Build618 split stopped at the frozen-sensor header assertion before any prototype edit. The copied FP0 volume-reader count 146,676 was incorrect for the exact B sensor pinned by FP2-0. The pinned file has 61,330 records with IDs 0 through 61,329, exact size for that record count and 61,330 distinct coordinate bit tuples. A read-only scan matched all 24,479 IN1 sidecar coordinates. Build619 asserts the correct file count and retains the per-record and exact-coordinate checks. No input payload changed. [Failed attempt](cases/split/measurement.json)
