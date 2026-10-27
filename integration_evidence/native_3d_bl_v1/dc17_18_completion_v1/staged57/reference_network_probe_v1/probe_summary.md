# Geometry Central representation probe: build 4

Build4, planar4 and m64 receipts report terminal exit 0 and verified unchanged
pins. Both event files contain exactly events 0–15; their run receipts also report
complete 16-event sequences. See the corresponding `*_status.json`, `*_pins.json`,
`*_events.json`, logs and resource receipts in this directory.

| Measure across 16 events | Planar disk | Faceted M6 star |
|---|---:|---:|
| Input vertices / faces | 5 / 4 | 7 / 6 |
| Current vertices / faces | 7 / 8 | 9 / 10 |
| Vertex / edge / face capacity | 7 / 14 / 8 | 9 / 18 / 10 |
| Common-subdivision vertices | 7–9 | 9–11 |
| Transverse intersections | 0–2 | 0–2 |
| Maximum / sum normal coordinates | 1 / 8–9 | 1 / 12–13 |
| Traced pieces | 14–16 | 18–20 |
| Per-event elapsed seconds | 0.000037047–0.000106721 | 0.000046299–0.000278425 |

For each of the four returning phases, events 4–15 have identical reported
counts: planar subdivision vertices/intersections/max-normal/sum-normal/pieces
are 7/0/1/8/14; M6 values are 9/0/1/12/18. Initial events have the higher
ranges above. These aggregate counts do not establish identical correspondence
or bounded behavior for arbitrary sequences. The probe emits no arithmetic-work
counter.

Runner wall times were 0.1824 s planar and 0.1923 s M6; these include launcher
and receipt overhead. `/usr/bin/time` reports maximum RSS 5,160 KiB and 5,260 KiB
respectively for the timed process. The sparse sampled process-tree receipts peak
at only 745,472 and 12,288 bytes, so they miss the short-lived probe and cannot
establish whole-job peak memory. Build4 took 7.125 s with sampled process-tree RSS
531,095,552 bytes; that sampled value is not a whole-job peak.

This is a fixed-input, disposable-overlay representation/API experiment. The M6
input is a small faceted star, not smooth cubic authority. It does not qualify a
joint network, native remeshing, volume quality, BL, MPI, transfer or restart.
