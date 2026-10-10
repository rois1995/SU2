The focused test run passed27cases/2370assertions, but CNativePredicates3D.cpp
was edited while compilation/testing was in flight. The original runner captured
hashes at the end only, so validation.json cannot authenticate the compiled
predicate source. CNativePredicates3D_original.cpp is the pre-edit source; all
original logs/receipt remain untouched. Do not reuse these objects as validated
current-source artifacts. edge_controls_v3 uses pre/post source pins and is the
final-source control. This receipt is retained as an intermediate observation.
