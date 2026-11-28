# TOL1 build683 fixture correction

Build683 at 3428991fed ended with compiler exit 1 after 1104.207 seconds. The new frozen-seam fixture supplied the `Owned` map to the generic row serializer, which requires a `Fields` member. The diagnostic is preserved in [build_outer.log](../build683/build_outer.log). The fixture now uses the existing `OwnedBytes` snapshot helper for the before/after comparison. This R3 correction touches only test code.
