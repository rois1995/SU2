
## 2026-11-17T22:07:06+01:00 · N4 phase C · C1

Can B02 use the same entry point as B07–B29 while leaving B03–B06 at their original scope and lifetime boundaries? The options were one wrapper spanning selection and lane setup, or a pre-selection entry for B02 plus the ordered decision function for B07–B29. Used the pre-selection entry. B02 can finalize before the lane timer, while later finalization must run with that timer alive. Keeping B03–B06 at the call site preserves their lifetimes and leaves acquisition code in its planned step. C1 base commit `ad64feb822`.
