
## 2026-11-17T22:07:06+01:00 · N4 phase C · C1

Can B02 use the same entry point as B07–B29 while leaving B03–B06 at their original scope and lifetime boundaries? The options were one wrapper spanning selection and lane setup, or a pre-selection entry for B02 plus the ordered decision function for B07–B29. Used the pre-selection entry. B02 can finalize before the lane timer, while later finalization must run with that timer alive. Keeping B03–B06 at the call site preserves their lifetimes and leaves acquisition code in its planned step. C1 base commit `ad64feb822`.

## 2026-11-17T22:32:54+01:00 · N4 phase C · C2

Where should the Topology timer shared by B35e–g live when those blocks become named functions? Separate stage timers or one scope in the Exchange unit were possible. Used one Exchange scope with the original features and retained-cell state kept alive through expansion and receipt service. This preserves the original timer call and nesting. C2 base commit `f7017771e8`.
