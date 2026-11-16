# G2 wrapper provenance correction (R3)

The G2 prefix was active when `run_g2_gq_prefix.py` changed from SHA256 `3852a540e140b8ad79cdec454dbbf9f36eab9556bba30a8132ae61e96928a972` to `78ed0a0e9281171304d32d5a17ab2f1f24b7a7a9db74f7379a6e2399fe44531b`. The added lines were confined to `self_check()` (`publish_aliases()` and a call to `verify_pins()`). The active controller had pinned the earlier wrapper bytes, so its `inputs_unchanged` receipt became false for this helper-only reason.

Restoration: removed only those two `self_check()` additions. The wrapper SHA256 is again `3852a540e140b8ad79cdec454dbbf9f36eab9556bba30a8132ae61e96928a972`. A source check confirmed the restore removed only those self-check additions. No case output, controller status, provenance receipt, source pin, production file, gate, build, or run was changed by this correction.

The parent reports that the G2 production gate itself passed with work `90,809,869` and expected counts, and that all nine owned handles are terminal. The controller's helper provenance is false because of the wrapper hash change; the parent retains qualification/postprocessing review. No launch or case reprocessing was performed here.
