# B5 extraction draft status

This is an **unapplied, provisional draft** for root review. It is bound to the immutable accepted B4 source snapshot at commit `c0486d57daf6ac4a1e66733db4a68b3c2dfe133a`, SHA256 `6149e2cb2de3af25feefefdf056ce1c2b4aa75aa6170029348d4eedbffde5b0a`. The accepted immutable commit, not the currently changing worktree, was used as the generation input. No production file was edited, and no compile, test, build, or numerical job was run.

## Draft contents

`generated/` contains full proposed `EngineControl.hpp/.cpp`, `EngineReports.hpp/.cpp`, and `CNativeEngine3D.control.proposed.cpp`, plus generator inputs, exact removed lambda snapshots, manifests, and proof receipts. Existing Control and Reports methods are retained in their full proposals. The full Adapt main is the original source with the three lambda declarations removed and calls rewritten to explicit members. The generator's reverse reconstruction restores the three original lambda spans and call sites byte-for-byte.

The moved code remains synchronous. `ObserveProgress` still sequences RSS/election, optional checkpoint, census refresh/election, Improvement progress and quality reporting, output, then stage callback/election. `Finish` retains the original terminal returns and ordering, including the final wall and lane counters, timing reread, swap, and moved result. `EmitQualityCensus` calls `PrintQualityCensus` unconditionally; the original `options.timings` guard stays inside that reporting path. `curved` is passed only to `Finish`, where the original lambda uses it.

The quality print body exceeded the 80-line method limit in its initial form. Its histogram/location/ladder output is now extracted as file-local `PrintQualityDetails`; the original wrapper guard and output position are retained. `final_output_expansion_proof.json` compares the final formatted generated method bodies after recursive helper inlining with the three actual lambda bodies from the accepted source. It also checks stage-row prefix/suffix/edge output from the final formatted helpers and verifies main reversal/restoration.

## Binding review

All moved state is passed through explicit parameters or borrowed from the same context slots as the old lambda:

- `world`, `options`, `geometry`, `composition`, `original`, `surface_target`: corresponding `EngineContext` members.
- `result`, `stats`: `*context.control.result` and `result.stats`.
- `selection`: `*context.census.selection`.
- `phase`, `started`, `started_cpu`, `region_rebuilt`, atlas bounds: `context.setup.control_prefix` optionals, dereferenced at the old live call point.
- observer and quality-window state: their `context.control` optionals; directory/source/private parameters and geometry: `context.exchange` / `context.setup` slots.
- `working`, `curved`, `atlas_budget`, `started`, and `started_cpu` remain explicit borrowed arguments where they are Adapt-local or preserve the original RAII lifetime.

The draft generation exposed and fixed two concrete binding omissions before handoff: `result` in donor capture and `observe_state` in rejected-state observation. `NotifyStageObserver`'s start clock is const-correct. `EngineTrial.hpp` is included for the moved final-height proof. Static/full header proposals now include the new declarations, with the file-local quality detail helper intentionally kept out of the class API. This remains a source review only; no compile was run. The final helper expansion receipt records all methods and output comparisons.

## Equivalence and readability evidence

- `final_output_expansion_proof.json`: final formatted-method recursive expansion, stream-output checks, and accepted-source token equality for quality, observer, stage reports, and finish.
- `main_reversal_proof.json`: byte-equal main reversal and restoration against the accepted source snapshot.
- `member_draft_status.json`: current anchored function-definition and lambda body line counts. Longest member is `ObserveProgress` at 74 lines; generated helper `NativeStageSuffixWork` is 64 lines.
- `format_and_line_counts.json`: proposal SHA256s and `clang-format-18 --dry-run --Werror` result for the full member/header proposals. The full main proposal was not reformatted, avoiding unrelated whole-file churn.
- `source_dependency_provenance.json`: source snapshot and proposal generator hashes.

The longest current methods are: ObserveProgress 74; CollectQualityCensus 71; NotifyStageObserver 65; NativeStageSuffixWork 64; NativeStageSuffix 57; PrintQualityCensus 52; CaptureDirectoryDonorStatistics 51; InitializeFixedMetricDrift 51; NativeStagePrefix 51; PrintQualityDetails 48.

Nothing here is accepted or applied. Root owns acceptance and any later application. This proposal is already rebound to accepted B4; no application or compile is implied.
