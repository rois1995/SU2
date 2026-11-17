# B8 validation adapters for build 558

These are new adapters retargeted from the executed B6 build556 helpers. They remain preparation drafts. No helper in this draft folder has started a test or numerical job.

## Path and gate bindings

- Build: `P/build558`.
- Candidate controls and receipts: `P/B8/validation`.
- G1 reference tables: accepted N14/552 tables via the unchanged B3 reference aliases. G1 wall-exception sets remain empty; successful assertion totals and launcher return codes remain observations.
- G2: the accepted 552 G2 reference, exact P6c collapse work/counts and old integer rows, with the same 6.8/6.9 timer-census comparator.
- Expected build pin count from the parent request: 1781.

## Preparation flow after the build source checkpoint

1. Run `python3 B8_preparation/validation_drafts/install_validation_aliases.py` from the repository root. This installs the drafted wrappers in `B8/validation`, creates links to adopted N14/G1 inputs, installs `run_g2_gq_prefix.py`, and copies the build alias to `P/build558_aliases.py`.
2. After the build is sealed, run all prelaunch stubs: `prelaunch_housepin_stub.py --cohort serial|mpi2|mpi4` and `prelaunch_prefix_stub.py --gate G2`. They stop immediately before the test/solver launch after checking setup receipts.
3. Refresh actual-host preflight receipts immediately before launches.
4. Launch build with `/tmp/b8_launch_build558.py`; seal only after parent confirms the observed session is terminal with `/tmp/b8_seal_build558.py SESSION`.
5. Launch G1 with `/tmp/b8_launch_g1_558.py serial|mpi2|mpi4`; use matching `seal_g1` helper with the observed session.
6. Launch G2 with `/tmp/b8_launch_prefix_558.py G2`; seal with the matching `seal_prefix` helper and observed session.

The launch and seal helpers are Python syntax-compiled. The build alias path self-check passed without reading receipts. No tests or build were run by this preparation.
