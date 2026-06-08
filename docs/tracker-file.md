# Tracker File Timeline Plan

This plan covers tracker files only.  The output is a neutral
par-beatdown JSON timeline.  It is not a ParAnimator config file.

Later, par-beatdown may emit ParAnimator config JSON directly, or a
separate command-line tool may convert par-beatdown JSON into a
ParAnimator timeline.  Keep that later step out of this first pipeline.

Use libopenmpt for tracker parsing and playback.  Use nlohmann-json for
all JSON creation, parsing, and tests.  Do not hand-build JSON strings.

## Goals

* Load tracker files through libopenmpt.
* Capture stable module metadata.
* Capture order, pattern, row, channel, and command structure.
* Capture useful playback timing in seconds and frames.
* Optionally capture simple rendered PCM features.
* Write deterministic neutral JSON.
* Keep the schema documented and versioned.

## Non-goals

* No ParAnimator config output in this slice set.
* No music theory inference.
* No beat detection from tracker rows.
* No audio-file backend work.
* No Sonic Annotator or Vamp plugin work.

## Output

Default output name:

```
song.music.json
```

Current command form:

```
par-beatdown song.xm -o song.music.json
    [--include source|module|timeline|events|features]
    [--fps 30] [--offset 0] [--feature-hop 0.0333333333]
```

The file should contain one selected subsong.  The first implementation
may use libopenmpt's default subsong selection.  Later CLI options can
select a specific subsong or emit one file per subsong.

## JSON Rules

Use `nlohmann::ordered_json` for deterministic key order.

Rules:

* Root object keys appear in schema order.
* Arrays preserve source order.
* Missing optional event fields are omitted, not set to null.
* Floating point values are rounded before insertion.
* Output uses `dump(2)` plus one final newline.
* Tests parse output with nlohmann-json before comparing fields.

## Integration Tests

Tool integration tests live under `tests/par-beatdown`.

Rules:

* keep global fields present in every output
* use `--include <section>` to choose non-global JSON sections
* add one `gold-<test-case>.json` per integration use case
* do not grow old gold files when a later slice adds a new section
* run field checks before comparing generated output to gold JSON
* normalize line endings before comparing generated output to gold JSON
* label all tool integration tests `par-beatdown`

Global fields:

* `schema`
* `version`
* `generator`
* `source`
* `render`
* `diagnostics`

Current include sections:

* `source` writes only global fields
* `module` also writes `module`
* `timeline` also writes timeline clock data and timing events
* `events` also writes tracker pattern command events
* `features` also writes rendered PCM feature frames

## Schema V1

Human-readable schema:

```
TrackerTimeline {
  schema: string
  version: integer
  generator: Generator
  source: Source
  render: Render
  module: Module
  timeline: Timeline
  events: Event[]
  features: FeatureFrame[]
  diagnostics: Diagnostics
}

Generator {
  name: string
  version: string
  backend: string
  library: string
  library_version: string
}

Source {
  file: string
  size_bytes: integer
  format: string
  title: string
  duration_seconds: number
  selected_subsong: integer
  subsong_count: integer
}

Render {
  fps: number
  sample_rate: integer
  channels: integer
  offset_seconds: number
  feature_hop_seconds: number
}

Module {
  channel_count: integer
  order_count: integer
  pattern_count: integer
  instrument_count: integer
  sample_count: integer
  metadata: MetadataItem[]
  subsongs: Subsong[]
  orders: Order[]
  patterns: Pattern[]
}

MetadataItem {
  key: string
  value: string
}

Subsong {
  index: integer
  name: string
  restart_order: integer
  restart_row: integer
}

Order {
  index: integer
  pattern: integer
  name: string
  kind: string
  time_seconds: number
  frame: integer
}

Pattern {
  index: integer
  name: string
  row_count: integer
  rows_per_beat: integer
  rows_per_measure: integer
}

Timeline {
  duration_seconds: number
  frames: integer
  first_frame: integer
  last_frame: integer
}

Event {
  kind: string
  time_seconds: number
  frame: integer
  order: integer
  pattern: integer
  row: integer
  channel: integer
  note: string
  instrument: integer
  sample: integer
  volume: integer
  effect: string
  parameter: integer
  text: string
}

FeatureFrame {
  time_seconds: number
  frame: integer
  rms: number
  peak: number
  active_channels: integer
  channel_vu_mono: number[]
}

Diagnostics {
  warnings: string[]
  unsupported: string[]
  log: string[]
}
```

## Field Reference

### Root

`schema` identifies the schema name.  Use
`par-beatdown.tracker-timeline`.

`version` is the integer schema version.  Start at `1`.

`generator` describes the tool and backend that produced the file.

`source` describes the input tracker file and selected subsong.

`render` describes frame and PCM settings used during analysis.

`module` describes static module structure.

`timeline` describes the resulting time and frame range.

`events` contains sparse timeline events.  It may be empty.

`features` contains continuous sampled features.  It may be empty.

`diagnostics` contains non-fatal warnings and backend notes.

### Generator

`name` is the generating program name.  Use `par-beatdown`.

`version` is the par-beatdown version string.

`backend` is the backend id.  Use `tracker-file`.

`library` is the backend library name.  Use `libopenmpt`.

`library_version` is the libopenmpt version string.

### Source

`file` is the input path as provided by the user.

`size_bytes` is the input file size in bytes.

`format` is the module format reported by libopenmpt, such as `xm`.

`title` is the module title reported by libopenmpt.

`duration_seconds` is the selected subsong duration in seconds.

`selected_subsong` is the selected libopenmpt subsong index.

`subsong_count` is the number of subsongs reported by libopenmpt.

### Render

`fps` is the requested animation frame rate.

`sample_rate` is the PCM sample rate used for rendered features.

`channels` is the PCM output channel count used for rendered features.

`offset_seconds` is the user sync offset applied to frame conversion.

`feature_hop_seconds` is the spacing between feature samples.

### Module

`channel_count` is the number of pattern channels.

`order_count` is the number of orders in the selected sequence.

`pattern_count` is the number of distinct patterns.

`instrument_count` is the number of instruments.

`sample_count` is the number of samples.

`metadata` contains raw libopenmpt metadata key and value pairs.

`subsongs` contains all known subsong summaries.

`orders` contains order list entries in playback order.

`patterns` contains pattern summaries.

### MetadataItem

`key` is the libopenmpt metadata key.

`value` is the metadata value.

### Subsong

`index` is the libopenmpt subsong index.

`name` is the subsong name.

`restart_order` is the order used when playback restarts.

`restart_row` is the row used when playback restarts.

### Order

`index` is the order index.

`pattern` is the pattern index at this order.

`name` is the order name reported by libopenmpt.

`kind` is `pattern`, `skip`, or `stop`.

`time_seconds` is the first playback time for this order.

`frame` is `time_seconds` converted to an animation frame.

### Pattern

`index` is the pattern index.

`name` is the pattern name reported by libopenmpt.

`row_count` is the number of rows in the pattern.

`rows_per_beat` is libopenmpt's rows-per-beat value.

`rows_per_measure` is libopenmpt's rows-per-measure value.

### Timeline

`duration_seconds` is the output timeline duration.

`frames` is the number of frames in the output timeline.

`first_frame` is the first valid frame, normally `0`.

`last_frame` is the final valid frame.

### Event

`kind` is the event type.  Initial values are `order`, `pattern`,
`row`, `note`, `effect`, and `tempo`.

`time_seconds` is the event time in seconds.

`frame` is the event frame after applying offset and fps.

`order` is the source order index.

`pattern` is the source pattern index.

`row` is the source pattern row index.

`channel` is the source pattern channel index.  Omit when not relevant.

`note` is a formatted tracker note.  Omit when no note exists.

`instrument` is the tracker instrument number.  Omit when absent.

`sample` is the tracker sample number.  Omit when absent.

`volume` is the tracker volume command or value.  Omit when absent.

`effect` is the tracker effect command.  Omit when absent.

`parameter` is the raw effect parameter.  Omit when absent.

`text` is a human-readable event note.  Omit when absent.

### FeatureFrame

`time_seconds` is the feature sample time.

`frame` is the feature sample frame.

`rms` is rendered PCM root-mean-square amplitude.

`peak` is rendered PCM peak absolute amplitude.

`active_channels` is libopenmpt's current active channel count.

`channel_vu_mono` is one mono VU value per tracker channel.

### Diagnostics

`warnings` contains non-fatal warnings.

`unsupported` lists unsupported or skipped tracker details.

`log` contains libopenmpt log lines collected during load or render.

## Implementation Slices

### Slice 1: Diagnostics And Errors

Make failure modes useful.

The slice should:

* report file-open failure
* report libopenmpt load failure
* report unsupported empty modules
* report invalid fps
* report invalid output path
* carry non-fatal libopenmpt log lines into `diagnostics.log`
* keep user-facing errors short

The integration test should:

* add a tool integration case for a missing input file
* add a tool integration case for an invalid tracker file
* add a tool integration case for an invalid output path
* validate non-zero exit codes and short stderr text

Remove this slice when common error paths are tested.

### Slice 2: Future ParAnimator Adapter

Do not implement this in the tracker-file JSON pipeline.

Later work may choose one of two paths:

* add `par-beatdown --emit-paranimator-config`
* add a separate `music2keyframes` or timeline adapter tool

That later output must have its own schema and field documentation.
That later plan must include integration tests for every adapter slice.

Remove this slice when a separate ParAnimator adapter plan exists.

## Test Data

Use:

```
data/my_neighbors_kid_is_an_internet_addict.xm
```

The sidecar metadata is:

```
data/my_neighbors_kid_is_an_internet_addict.md
```

Current tool integration gold files are:

```
tests/par-beatdown/gold-write-tracker-source.json
tests/par-beatdown/gold-write-module-structure.json
tests/par-beatdown/gold-write-timeline-clock.json
tests/par-beatdown/gold-write-pattern-events.json
tests/par-beatdown/gold-write-feature-frames.json
tests/par-beatdown/gold-write-custom-fps.json
tests/par-beatdown/gold-write-timeline-custom-fps.json
tests/par-beatdown/gold-write-sync-offset.json
tests/par-beatdown/gold-write-custom-feature-hop.json
```

The fixture is useful because it is XM, public domain, and large enough to
exercise patterns and samples.
