# par-beatdown Architecture Plan

par-beatdown analyzes an audio track and emits timeline data that can be
used to drive ParAnimator effects.

It does not render audio.
It does not render animation.
It does not teach fractals to dance.
It extracts beats and other gross audio features, then leaves the damage to
ParAnimator.

## Goals

* Analyze a music track offline.
* Extract beat, tempo, energy, and broad spectral information.
* Emit a neutral timeline file.
* Allow ParAnimator to use that timeline to drive animation parameters.
* Keep Marsyas out of ParAnimator itself.
* Keep the audio-analysis backend replaceable.

## Non-goals

* Real-time audio analysis.
* Live audio playback.
* Audio/video muxing.
* Tight coupling to Marsyas APIs inside ParAnimator.
* Musicological accuracy.
* Pretending FFT bins are emotions.

## Repository

Suggested repository name:

```
LegalizeAdulthood/par-beatdown
```

Suggested GitHub description:

```
Extract beats, abuse keyframes, drive ParAnimator timelines.
```

Primary executable:

```
par-beatdown
```

## System Overview

```
song.wav
  -> par-beatdown
  -> song.music.json
  -> paranimator
  -> generated PAR files / rendered frames
  -> external video/audio muxing
```

par-beatdown is a separate command-line tool.  It may use Marsyas internally,
but ParAnimator should only consume the generated timeline data.

The important boundary is:

```
Marsyas-linked GPL tool  ->  neutral JSON data  ->  ParAnimator
```

## Licensing Boundary

Marsyas is GPL-2.0-or-later.  A tool that links against Marsyas should be
treated as GPL-2.0-or-later.

ParAnimator should not link against Marsyas.  Instead, ParAnimator consumes
the JSON timeline emitted by par-beatdown.

This keeps the architecture simple:

```
par-beatdown       GPL-2.0-or-later, because it uses Marsyas
paranimator        GPL-3.0, no Marsyas dependency
*.music.json       data interchange
```

The JSON output is data, not Marsyas code.

## Components

### 1. Audio Analyzer

Responsible for reading an audio file and extracting coarse timeline features.

Initial backend:

```
Marsyas
```

Future possible backends:

```
Aubio
Essentia
Sonic Annotator
hand-authored JSON
some regrettable script from 2007
```

The rest of the architecture should not care which analyzer produced the file.

### 2. Timeline Writer

Converts extracted audio features into a normalized JSON timeline.

Responsibilities:

* Convert analysis times to animation frames.
* Normalize feature values.
* Emit sparse events.
* Emit continuous per-frame or per-hop features.
* Preserve enough metadata for synchronization.

### 3. ParAnimator Timeline Reader

A future ParAnimator feature.

Responsibilities:

* Read external timeline files.
* Sample feature values at frame F.
* Find nearby events at frame F.
* Convert events into pulses/envelopes.
* Apply bindings to current animation parameters.

ParAnimator should not know about Marsyas.

It should know only about external timeline data.

## Command Line

Initial command:

```
par-beatdown song.wav -o song.music.json --fps 30
```

With optional offset:

```
par-beatdown song.wav -o song.music.json --fps 30 --offset 0.000
```

Possible later options:

```
--normalize track
--normalize none
--hop-size 512
--window-size 2048
--backend marsyas
--dump-debug-csv song.features.csv
```

## Timeline File Format

The primary output is JSON.

Example:

```
{
  "version": 1,
  "generator": {
    "name": "par-beatdown",
    "backend": "marsyas"
  },
  "audio": {
    "file": "song.wav",
    "duration": 184.2,
    "offset": 0.0
  },
  "timeline": {
    "fps": 30,
    "frames": 5526
  },
  "tempo": {
    "bpm": 122.4,
    "confidence": 0.81
  },
  "features": [
    {
      "frame": 0,
      "time": 0.000,
      "rms": 0.11,
      "low": 0.42,
      "mid": 0.27,
      "high": 0.18,
      "centroid": 0.32,
      "flux": 0.04,
      "onset": 0.00,
      "beat_phase": 0.12
    }
  ],
  "events": [
    {
      "kind": "beat",
      "frame": 14,
      "time": 0.483,
      "strength": 0.78
    },
    {
      "kind": "bar",
      "frame": 14,
      "time": 0.483,
      "index": 1
    }
  ]
}
```

## Extracted Data

### Sparse Events

Sparse events are discrete musical moments.

Initial event types:

```
beat
bar
section
```

Minimum useful event:

```
{
  "kind": "beat",
  "frame": 14,
  "time": 0.483,
  "strength": 0.78
}
```

### Continuous Features

Continuous features are sampled over time.

Initial features:

```
rms              loudness / energy
low              low-frequency band energy
mid              mid-frequency band energy
high             high-frequency band energy
centroid         spectral brightness
flux             spectral change / instability
onset            transient strength
beat_phase       position between beats, 0.0 to 1.0
```

All feature values should be normalized to a practical 0.0..1.0 range.

Normalization should initially be per-track.

## Frame Conversion

Given:

```
time_seconds
fps
audio_offset_seconds
```

Use:

```
frame = round((time_seconds + audio_offset_seconds) * fps)
```

The offset exists because audio/video sync always finds a way to be annoying.

## ParAnimator Integration

ParAnimator should gain support for external timelines.

Example animation configuration:

```
{
  "music": {
    "file": "song.wav",
    "analysis": "song.music.json",
    "offset": 0.000
  }
}
```

Then add parameter bindings:

```
{
  "bindings": [
    {
      "source": "music.beat_pulse",
      "target": "palette_offset",
      "op": "add",
      "scale": 12.0,
      "decay": 0.120
    },
    {
      "source": "music.rms",
      "target": "maxiter",
      "op": "add",
      "scale": 40.0,
      "smooth": 0.250
    },
    {
      "source": "music.low",
      "target": "zoom",
      "op": "multiply",
      "scale": 0.015,
      "smooth": 0.200
    }
  ]
}
```

## Binding Model

A binding maps a timeline source to an animation parameter.

Fields:

```
source     feature, event, or derived signal
target     animation parameter
op         add, multiply, replace
scale      amount of influence
offset     optional additive bias
smooth     smoothing time in seconds
decay      event pulse decay time in seconds
clamp      optional min/max output bounds
```

Example:

```
{
  "source": "music.rms",
  "target": "maxiter",
  "op": "add",
  "scale": 40.0,
  "smooth": 0.250,
  "clamp": {
    "min": 64,
    "max": 2048
  }
}
```

## Event Pulses

Beat events should not directly slam parameters and leave them there.

A beat should produce a short decaying pulse:

```
pulse(t) = strength * exp(-(t - beat_time) / decay)
```

The pulse is zero before the beat.

Example use:

```
palette_offset = base_palette_offset + 12.0 * beat_pulse
```

## Evaluation Order in ParAnimator

For each frame:

```
1. Evaluate authored keyframes.
2. Read music features at the current frame.
3. Convert nearby events into pulses.
4. Evaluate music bindings.
5. Apply clamps.
6. Emit the frame/PAR state.
```

The authored animation remains the base signal.

Music is an overlay.

## Recommended First Implementation

### Phase 1: Standalone Analyzer

Implement par-beatdown.

Input:

```
song.wav
```

Output:

```
song.music.json
```

Extract only:

```
tempo
beat events
rms
```

This is enough to prove the pipeline.

### Phase 2: Keyframe Preprocessor

Before modifying ParAnimator, add a small converter:

```
music2keyframes base-animation.json song.music.json -o music-animation.json
```

It inserts ordinary keyframes based on beat events.

Example behavior:

```
for each beat:
    at beat frame:
        palette_offset += strength * 12
        zoom *= 1.0 + strength * 0.015

    three frames later:
        return toward authored value
```

This keeps everything inspectable.

It also avoids adding abstractions before knowing which ones are not stupid.

### Phase 3: Native External Timeline Support

Add ParAnimator support for:

```
music.analysis
bindings
feature sampling
event pulses
smoothing
clamping
```

At this point, the keyframe preprocessor becomes optional.

### Phase 4: More Features

Add:

```
low/mid/high band energy
spectral centroid
spectral flux
onset strength
beat phase
bar events
section markers
debug CSV output
```

## Suggested Effects

Useful first mappings:

```
beat             palette offset pulse
bar              stronger palette or camera transition
rms              max iteration count / brightness / glow
low              zoom pressure
mid              rotation / pan drift
high             palette cycling speed
centroid         palette family / color temperature
flux             perturbation amount / turbulence
section          switch to a new authored region
```

Avoid directly binding raw features to highly sensitive fractal parameters.

Use smoothing, scaling, and clamps.

The goal is animation, not a seizure-triggering oscilloscope.

## File Naming

Recommended output:

```
song.music.json
```

Alternative, more branded:

```
song.beatdown.json
```

Prefer song.music.json for interchange.

Prefer song.beatdown.json if being cute is temporarily allowed.

## Example Workflow

```
par-beatdown song.wav --fps 30 -o song.music.json

music2keyframes animation.json song.music.json -o animation.music.json

paranimator animation.music.json

ffmpeg -r 30 -i frames/frame%05d.png -i song.wav \
  -shortest -c:v libx264 -pix_fmt yuv420p final.mp4
```

## Architecture Rule

ParAnimator should consume neutral timeline data.

It should not consume Marsyas.

It should not know what Marsyas is.

It should not care.

That is how software avoids becoming archaeology.

