# FP2-B correction 3 · Pre-edit sheet placement

Build639 compiled test_driver only in 59.578 s with zero new warnings. Common and SU2_CFD matched `da8088eae2`. The solver remained build638. The test-driver SHA256 was `45850bd6401db92dce1217ff4e9d21f824fc138898fc9d70db4d69fc9553347d`. [Build639 receipt](../build639/build_status.json)

The hidden IN1 fixture passed 11 assertions in 37.288 s. It inspected 72,757 sorted nonfeature edges with two physical faces. There were 69,436 accepted placements and 3,321 Walk refusals. The raw transcript records all refused edge pairs and the first three accepted bindings, coordinates and uint64 bit patterns. [Fixture receipt](../scoped/build639/sheet/fixture_status.json) · [Pin transcript archive](../scoped/build639/sheet/test_outer.log.gz)

| First accepted edge | Binding dimension | Feature | Version | u | v |
|---|---:|---:|---:|---:|---:|
| 0–834 | 1 | 2 | 18 | 0.499999999999999 | 0 |
| 0–1095 | 1 | 3 | 18 | 0.50000000000000056 | 0 |
| 0–13978 | 1 | 4 | 18 | 0.5 | 0 |

R3. The wrapper lacked the output root required by its final status printer. The controller was already terminal with return code 0 and the passing fixture receipt and raw hashes were sealed. The receipt was verified directly. The corrected fixture wrapper supplies the output root. The pre-edit fixture ran once.
