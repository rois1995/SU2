# Corrected end-step history auditor self-check

PASS: serialized positive box/P1 sensor/state/SU2/HDF5 fixtures, plus six rejected
corruptions (current, time_n, tensor, missing donor, BC range, external HDF5 link).
Retained HDF5 fixture is restored and closed. Current and time_n are identical as
required by actual SU2 end-step transfer; time_n1 is distinct. The v3 fixture's
independent time_n scaling was synthetic and incompatible with actual transfer.
These fixtures qualify the checker, not native output, CFD iterations or scaling.
Source/checker/input hashes and runner snapshot are in validation.json.
