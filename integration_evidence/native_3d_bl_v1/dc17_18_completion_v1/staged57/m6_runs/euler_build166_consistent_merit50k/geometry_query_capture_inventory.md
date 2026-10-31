# Existing M6 geometry-query capture inputs — read-only inventory

Current166is frozen. No files/jobs/tests changed by the inventory agent. No exact
query-event payload or exact engine continuation exists for166; it has counters,
profiles, source/runtime/input pins and raw rank logs. Trace was not enabled.

Reusable older private geometry input candidates (verify before use):

- staged57/cases/m6_stage_20261027T094843_1705337 (build18) and
  m6_stage_20261027T102502_1804803 (build19): complete6-rank quality_entry and
  observation_stop owned/donor/parameter shards. Terminal exit0, pins verified,
  four windows, donors unchanged in status. stage_output_pins.json hashes outputs.
  Example rank0entry96317owned rows,208-byte records,344-byte donor records;
  parameter payload~4.53MB. Stop rank0has98664rows at19264rounds.
- m6_stage_20261027T193554_3391623 (build48): complete6-rank quality_entry
  (667993global cells) plus rejected_exit shards; lacks observation_stop, terminal
  diagnostic error. private_quality_entry_archive_receipt.json pins/archives shards
  and common cfg/sensor/native_ref. Explicit exact_continuation=false/source_valid=false;
  no final accepted mesh or runtime checkpoint. Do not claim campaign success/restart.
- m6_stage_20261027T145416_2560816 rejected-exit is construction failure only.
  m6_stage_20261027T173120_3053606 has partial quality-entry shards; not complete.

Readers/facilities:

- staged57/run_cases.py::validate_stage_shards checks counts/record sizes/hashes,
  donor identity and parameter payload length/windows.
- staged57/analyze_stage_topology.py decodes208-byte owned rows (IDs, coordinates,
  physical flags/facet IDs) and parameter header; no donor decode/query replay.
- [NativeFrozenVolume3D] diagnostic fixture is the existing C++ capture consumer.
- TransactionView/capture_feature exposes exact borrowed patch/private geometry;
  StageView exposes working/donor/parameter data. Callbacks copy state themselves,
  cannot call collectives. SU2_NATIVE_GEOMETRY_WORK_TRACE prints exact one-triangle/
  <=64facet support/tolerance/direction inputs, plus preparation/bound/refine counters;
  it lacks standalone authority/binding/multi-triangle/resolution replay inputs.
- CNativeAffineReference cache fixtures already call BoundPhysicalSurface/
  BoundSurfaceUnion with known authority/support/coordinates/tolerance and compare
  cached/uncached/refused/exhausted behavior. No serialized M6 query reader exists.

The older shards preserve mathematical mesh/association/parameter state, not the
actual expensive smoothing candidate/query identity or cache warmth. Verify all
hashes, reference authority, record ABI/schema and current reader semantics before
reconstructing representative query inputs. They are not exact166continuation.
Use existing readers/observers plus a narrow payload bridge only where necessary;
no independent framework or another full baseline campaign. CAD remains optional.
