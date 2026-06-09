# Native ParAnimator Timeline Plan

This plan covers native ParAnimator support for external music timelines
after `beat-keys` has proven the mapping model.

The native path should read the same neutral par-beatdown timeline JSON
used by `beat-keys`, but evaluate the music data inside ParAnimator
during frame generation.

## Goals

* Load external par-beatdown timeline JSON from a ParAnimator config.
* Sample continuous music features during frame evaluation.
* Convert sparse timeline events into frame-local pulse values.
* Apply smoothing, decay, scale, offset, and clamp controls.
* Compose music-derived values with authored tracks predictably.
* Cover every slice with ParAnimator integration tests.

## Non-goals

* No tracker or audio decoding inside ParAnimator.
* No direct libopenmpt, Vamp, Sonic Annotator, or plugin dependency.
* No automatic visual mapping.
* No music theory inference.
* No replacement for authored ParAnimator tracks.

## Proposed Config Shape

Human-readable schema extension:

```
MusicTimeline {
  id: string
  file: string
  fps: number
  offset_seconds: number
}

MusicBinding {
  timeline: string
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

The config should keep music data separate from authored tracks:

```
{
  "music-timelines": [
    {
      "id": "song",
      "file": "song.music.json",
      "fps": 30.0,
      "offset_seconds": 0.0
    }
  ],
  "music-bindings": [
    {
      "timeline": "song",
      "source": "music.rms",
      "target": "color.brightness",
      "op": "add",
      "scale": 1.0,
      "offset": 0.0
    }
  ]
}
```

## Field Reference

`music-timelines` lists external par-beatdown timeline JSON files.

`id` is the local timeline identifier used by bindings.

`file` is the path to the neutral timeline JSON file.

`fps` is the animation frame rate used when converting timeline time to
frames if the timeline source needs resampling.

`offset_seconds` shifts the music timeline before evaluation.

`music-bindings` maps timeline values to ParAnimator parameters.

`timeline` names one entry in `music-timelines`.

`source` names a timeline feature, event, or derived signal.

Initial source names:

```
music.rms
music.peak
music.note_pulse
music.effect_pulse
music.row_pulse
```

`target` is the ParAnimator parameter affected by the binding.

`op` is `add`, `multiply`, or `replace`.

`scale` multiplies the sampled source value.

`offset` adds a constant bias after scaling.

`smooth_seconds` smooths continuous feature values.

`decay_seconds` controls sparse event pulse decay.

`clamp` optionally limits generated values.

## Composition Rules

Authored tracks remain the base signal.

Music bindings should be evaluated after authored tracks for the same
frame.  `replace` overwrites the authored value.  `add` adds to the
authored value.  `multiply` multiplies the authored value.

If a music binding targets a parameter with no authored track, the
parameter's base value comes from the source par entry or ParAnimator's
existing defaults.

If multiple music bindings target the same parameter, evaluate them in
config order.  Reject bindings that cannot be applied to the target
parameter type.

## Implementation Slices

### Slice 1: Schema And Loader

Add ParAnimator config schema support for `music-timelines` and
`music-bindings`.

The slice should:

* validate timeline ids are unique
* validate binding timeline references
* load timeline JSON files through the existing config path rules
* reject unsupported timeline schema or version
* keep decoded timeline data out of generated par output

The integration tests should:

* load a minimal timeline JSON file
* reject duplicate timeline ids
* reject a missing timeline file
* reject an unsupported timeline schema

### Slice 2: Continuous Feature Sampling

Sample timeline feature frames during ParAnimator frame evaluation.

The slice should:

* support `music.rms` and `music.peak`
* pick the nearest available feature frame for exact frame output
* apply `scale`, `offset`, and `clamp`
* support `add`, `multiply`, and `replace`
* preserve authored tracks for unrelated parameters

The integration tests should:

* apply an RMS binding to a numeric parameter
* apply a peak binding to a different numeric parameter
* reject an invalid continuous source
* verify authored unrelated tracks are unchanged

### Slice 3: Sparse Event Pulses

Convert timeline events into frame-local pulse values.

The slice should:

* support `music.note_pulse`
* support `music.effect_pulse`
* support `music.row_pulse`
* merge repeated events at the same frame deterministically
* emit zero values after each pulse window

The integration tests should:

* apply a note pulse binding
* apply an effect pulse binding
* apply a row pulse binding
* reject an invalid event source
* verify repeated same-frame events merge

### Slice 4: Smoothing And Decay

Apply temporal shaping to sampled music values.

The slice should:

* apply `smooth_seconds` to continuous feature bindings
* apply exponential `decay_seconds` to event pulse bindings
* keep smoothing local to each binding
* clamp after scale, offset, smoothing, and decay

The integration tests should:

* validate smoothed RMS output
* validate decayed note pulse output
* validate clamp after decay
* reject negative smoothing or decay values

### Slice 5: Authored Track Interaction

Define and test how music bindings compose with authored tracks.

The slice should:

* evaluate authored tracks first
* apply music bindings in config order
* reject unsupported target parameter types
* reject ambiguous multi-binding writes where ordering is not explicit
* keep layer-local tracks and top-level tracks scoped correctly

The integration tests should:

* add to an authored numeric track
* multiply an authored numeric track
* replace an authored numeric track
* verify layer-local music bindings affect only that layer
* reject a binding against a non-numeric target

### Slice 6: End-To-End Music Timeline Render

Add an end-to-end ParAnimator render test that uses a small timeline.

The slice should:

* render a short animation with authored and music-derived tracks
* compare generated par output against gold output
* keep the fixture timeline small and hand-readable
* label tests with both `paranimator` and `music-timeline`

The integration tests should:

* validate generated par assignments for every frame
* validate deterministic output across repeated runs
* verify no music timeline data is written into generated par files
