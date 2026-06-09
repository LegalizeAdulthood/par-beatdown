# ParAnimator Adapter Plan

This plan covers a later adapter that converts neutral par-beatdown
timeline JSON into ParAnimator-friendly keyframes.

The first adapter should be a separate command-line tool:

```
music2keyframes base-animation.json song.music.json -o music-animation.json
```

Do not add native ParAnimator music timeline support in this plan.  Keep
the first adapter inspectable: it reads a base animation config and a
neutral timeline, then writes ordinary keyframes or a generated overlay.

## Goals

* Read par-beatdown tracker timeline JSON.
* Read a small adapter config that maps timeline fields to animation
  parameters.
* Emit deterministic generated keyframes.
* Preserve the original authored animation as the base signal.
* Keep every adapter slice covered by an integration test.

## Non-goals

* No libopenmpt, Vamp, Sonic Annotator, or audio decoding.
* No native ParAnimator runtime changes.
* No music theory inference.
* No attempt to guess good visuals without explicit bindings.

## Files

Inputs:

```
base-animation.json
song.music.json
adapter.music2keyframes.json
```

Output:

```
music-animation.json
```

The output is a generated ParAnimator config or overlay.  The exact merge
format should be confirmed against ParAnimator before implementation.

## Adapter Config Schema V1

Human-readable schema:

```
Music2KeyframesConfig {
  schema: string
  version: integer
  source: Source
  output: Output
  bindings: Binding[]
}

Source {
  timeline: string
  fps: number
  offset_seconds: number
}

Output {
  mode: string
  namespace: string
}

Binding {
  source: string
  target: string
  op: string
  scale: number
  offset: number
  smooth_seconds: number
  decay_seconds: number
  clamp: Clamp
}

Clamp {
  min: number
  max: number
}
```

## Field Reference

### Music2KeyframesConfig

`schema` identifies the adapter config schema.  Use
`par-beatdown.music2keyframes`.

`version` is the integer schema version.  Start at `1`.

`source` describes the par-beatdown timeline input and sync settings.

`output` describes how generated keyframes are written.

`bindings` maps timeline fields to animation parameters.

### Source

`timeline` is the path to the par-beatdown timeline JSON.

`fps` is the animation frame rate used for generated keyframes.

`offset_seconds` shifts the music timeline before frame conversion.

### Output

`mode` controls output shape.  Initial value: `overlay`.

`namespace` prefixes generated keyframe ids or comments.

### Binding

`source` names a timeline feature, event, or derived signal.

Initial source names:

```
music.rms
music.peak
music.note_pulse
music.effect_pulse
music.row_pulse
```

`target` is the ParAnimator parameter to write.

`op` is `add`, `multiply`, or `replace`.

`scale` multiplies the sampled source value.

`offset` adds a constant bias after scaling.

`smooth_seconds` smooths continuous feature values.

`decay_seconds` controls sparse event pulse decay.

`clamp` optionally limits generated values.

### Clamp

`min` is the minimum generated value.

`max` is the maximum generated value.

## Generated Overlay Schema V1

Human-readable schema:

```
Music2KeyframesOverlay {
  schema: string
  version: integer
  generator: Generator
  source: OverlaySource
  keyframes: GeneratedKeyframe[]
  diagnostics: Diagnostics
}

Generator {
  name: string
  version: string
}

OverlaySource {
  base_animation: string
  timeline: string
  adapter_config: string
}

GeneratedKeyframe {
  frame: integer
  target: string
  op: string
  value: number
  source: string
}

Diagnostics {
  warnings: string[]
}
```

## Output Field Reference

`schema` identifies generated overlay output.  Use
`par-beatdown.music2keyframes-overlay`.

`version` is the integer schema version.  Start at `1`.

`generator` describes the tool that produced the overlay.

`source` records input paths for debugging.

`keyframes` contains generated keyframe operations in frame order.

`diagnostics` contains non-fatal warnings.

`frame` is the target animation frame.

`target` is the ParAnimator parameter name.

`op` is `add`, `multiply`, or `replace`.

`value` is the generated value after scale, offset, smoothing, pulse
decay, and clamp.

`source` is the binding source that generated the keyframe.

## Integration Tests

Adapter integration tests should live under `tests/music2keyframes`.

Rules:

* add one `gold-<test-case>.json` per use case
* compare generated output after normalizing line endings
* parse generated output and check schema fields before gold comparison
* validate failure paths with non-zero exit codes and short stderr text
* label all adapter integration tests `music2keyframes`

## Implementation Slices

### Slice 1: Tool Skeleton And Schemas

Create `tools/music2keyframes`.

The slice should:

* parse base animation, timeline, adapter config, and output paths
* validate adapter config schema and version
* write an empty generated overlay with generator and source metadata
* reject missing and unreadable inputs with short errors

The integration tests should:

* add `gold-write-empty-overlay.json`
* add missing base animation, timeline, and config error cases
* validate schema, version, generator, source, and empty keyframes

### Slice 2: Continuous Feature Bindings

Convert timeline feature frames into generated keyframes.

The slice should:

* support `music.rms` and `music.peak`
* support `add`, `multiply`, and `replace`
* apply `scale`, `offset`, and `clamp`
* preserve timeline frame order

The integration tests should:

* add `gold-write-rms-keyframes.json`
* add `gold-write-peak-keyframes.json`
* add an invalid continuous source error case
* validate generated frame, target, op, value, and source fields

### Slice 3: Sparse Event Pulses

Convert timeline events into pulse keyframes.

The slice should:

* support `music.note_pulse`, `music.effect_pulse`, and
  `music.row_pulse`
* apply exponential decay over `decay_seconds`
* generate zero-return keyframes after each pulse
* merge repeated pulses deterministically

The integration tests should:

* add `gold-write-note-pulses.json`
* add `gold-write-row-pulses.json`
* add an invalid event source error case
* validate pulse frame, return frame, and generated values

### Slice 4: Base Animation Merge

Merge generated overlay data with a base animation config.

The slice should:

* preserve untouched base animation fields
* append generated keyframes under the confirmed ParAnimator field
* keep generated entries namespaced
* reject conflicting generated ids or targets

The integration tests should:

* add `gold-merge-rms-overlay.json`
* add `gold-merge-event-overlay.json`
* add a generated id conflict error case
* validate preserved base fields and appended generated keyframes

### Slice 5: Native ParAnimator Plan

Do not implement native ParAnimator timeline support here.

Create a separate plan only after `music2keyframes` proves the mapping
model.

That plan should cover:

* loading external timeline files
* feature sampling during frame evaluation
* sparse event pulses
* smoothing and clamping
* interaction with authored keyframes
* ParAnimator integration tests for every slice
