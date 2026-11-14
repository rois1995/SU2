# B1 draft: x87 adjacent stepping

Draft only; not applied to production or tests. No build/test was run.

The current `Adjacent` in `Common/src/adaptation/CNativeAffineReference.cpp:31-50` accelerates ordinary canonical normals. The measured gap is exponent-0 zero/subnormal and exponent-1 normal values; the draft factors the primitive into a private internal header and accelerates exactly these encodings so the unit test can call the production helper directly after review.

Fast path is gated to x86_64, 64-bit long-double significand and little endian. It retains the existing canonical ordinary-normal fast range (exponent 2..0x7ffd) and adds exponent-0 zeros/subnormals plus exponent-1 normals. It handles directed signed-zero results, min-subnormal toward zero retaining the input sign, max-subnormal↔min-normal, and exponent-1 significand carry into exponent 2. It retains `std::nextafter` for pseudo-denormals (exp 0/integer bit set), unnormals (nonzero exponent/integer bit clear), and exponent >=0x7ffe or other exceptional payloads. The 10-byte payload is copied explicitly to avoid reading/writing long-double padding.

The direct test compares both directions to `std::nextafter` for ±zero, ±min-subnormal, ±max-subnormal, ±min-normal, the exponent-1 upper edge and its transition to exponent 2, ordinary normals, finite extremes, infinities and NaN. Additional explicit checks cover sign and transition behavior. Non-x87 hosts compile the library fallback. No arithmetic, interval policy, thresholds or rounding schedule is changed.
