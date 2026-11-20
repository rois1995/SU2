# FP1 Resume 6 source check (R4)

Base `cc96ab9e29`; its production and test tree matches `d76fa0d5bc`. The Resume 5 workflow handles are terminal. [Receipt](prior_terminal.json)

- S1. `BeginLengthGeneration` has the specified last reference and admission branch. Admission and election, history refresh, construction pass setup, length direction and selection phase changes emit no checkpoint. The length-generation caller makes the collapse checkpoint before starting the next generation.
- S2. `CompletePhaseEntry` starts the specified try block with selection phase change and initial census-list preparation. The scope ends before the catch, Vote and Measure. The first GQ construction-entry checkpoint comes from EnterPhase before it calls CompletePhaseEntry. The possible checkpoint from CompletePhaseEntry's final ObserveProgress occurs after the try block.
- S3. `GenerationCensus` has the specified last reference. Admission, local sample assembly, MeasureGenerationProgress and its reductions/exchanges have no checkpoint. The lane/length callers invoke ObserveProgress after GenerationCensus returns. Its inner communication scopes retain their own exclusive rows.
- Existing Scheduling already accounts for selection work. The lexical scope uses the open timer phase, adds no operation or member and does no timing work with a disabled root. The changes reach the native solver and its fixtures through the existing engine sources.
- G2 uses the N14 prefix and GQ the N22 capture through the existing Work order 3 comparator. Resume 6 additionally makes only construction/scheduling calls reportable in the two specified remesh boundaries, while requiring the row and the same deltas across ranks and collapse cases. The G5 controller can retain all existing ON standing checks and load the authorized build615 OFF proof and clocks independently of the build616 pins.

No source contradiction requires omitting any scope. The expected scheduling deltas and newly attributed seconds are report values.
