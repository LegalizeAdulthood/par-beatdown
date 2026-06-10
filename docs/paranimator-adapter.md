# ParAnimator Adapter

This document covers the `beat-keys` adapter.  It converts neutral
par-beatdown timeline JSON into ParAnimator-friendly keyframes.

`beat-keys` is a separate command-line tool:

```
beat-keys base-animation.json song.music.json \
    adapter.beat-keys.json -o music-animation.json
```

Native ParAnimator music timeline support is tracked separately.  Keep
`beat-keys` inspectable: it reads a base animation config and a neutral
timeline, then writes ordinary keyframes or a generated overlay.

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
adapter.beat-keys.json
```

Output:

```
music-animation.json
```

The output is a generated overlay or a ParAnimator-style config with
generated tracks appended.

## Adapter Config Schema V1

The formal JSON Schema is:

```
schemas/beat-keys.schema.json
```

Human-readable schema:

```
BeatKeysConfig {
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

### BeatKeysConfig

`schema` identifies the adapter config schema.  Use
`par-beatdown.beat-keys`.

`version` is the integer schema version.  Start at `1`.

`source` describes the par-beatdown timeline input and sync settings.

`output` describes how generated keyframes are written.

`bindings` maps timeline fields to animation parameters.

### Source

`timeline` is the path to the par-beatdown timeline JSON.

`fps` is the animation frame rate used for generated keyframes.

`offset_seconds` shifts the music timeline before frame conversion.

### Output

`mode` controls output shape.  Use `overlay` to write the neutral
adapter overlay, or `merge` to append generated tracks to a base
ParAnimator-style `tracks` array.

`namespace` prefixes generated track ids in merge output.

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

The formal JSON Schema is:

```
schemas/beat-keys-overlay.schema.json
```

Merge mode writes a ParAnimator-style config with generated tracks.  The
schema for the beat-keys-owned parts of that output is:

```
schemas/beat-keys-merged-animation.schema.json
```

Human-readable schema:

```
BeatKeysOverlay {
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
`par-beatdown.beat-keys-overlay`.

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

Adapter integration tests live under `tests/beat-keys`.

Rules:

* add one `gold-<test-case>.json` per use case
* compare generated output after normalizing line endings
* parse generated output and check schema fields before gold comparison
* validate failure paths with non-zero exit codes and short stderr text
* label all adapter integration tests `beat-keys`

## Implementation Slices

All adapter slices are complete.
