# DEF1 reverse-AD build recovery

The standard test_driver_AD build ended at compile time before linking or running an AD fixture. CBoundaryLayerMetric.cpp:2619 subtracts an active component into a long-double array. Line 3299 passes a passive coordinate pointer to EvaluateImpl's active pointer parameter. The file is byte-identical to the CL1-close base. No DEF1 production edit enters either expression.

The affected DEF1 checks construct CConfig and never call the boundary-layer CFD module. The focused test_driver_AD uses the exact same reverse-AD Meson flags, Catch main, DEF1 fixture object, existing CommonAD library and required dependencies. Its link omits unrelated solver and boundary-layer modules and unrelated test objects. This recovered the required test build without modifying pinned source or changing the normal binary. The standard target failure is preserved in build681/ad_full_setup.
