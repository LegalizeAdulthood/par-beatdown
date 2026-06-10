# par-beatdown

Extract beats, abuse keyframes, drive ParAnimator timelines.

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
* Keep audio-analysis tools out of ParAnimator itself.
* Keep the audio-analysis backend replaceable.

## Non-goals

* Real-time audio analysis.
* Live audio playback.
* Audio/video muxing.
* Tight coupling to any particular audio-analysis framework.
* Musicological accuracy.
* Pretending FFT bins are emotions.

## System Overview

```
song.xm
  -> par-beatdown
  -> song.music.json
  -> beat-keys
  -> animation.music.json
  -> paranimator
  -> generated PAR files / rendered frames
  -> external video/audio muxing
```

par-beatdown is a separate command-line tool.  It reads tracker files
directly or invokes external audio-analysis tools, then converts their
output into a neutral timeline file.

`beat-keys` is the current bridge between the neutral music timeline and
ParAnimator.  It reads a base animation config, the `song.music.json`
timeline, and a small adapter config.  It then writes ordinary generated
keyframes or appends generated tracks to a ParAnimator-style config.

ParAnimator can then consume normal animation data while native music
timeline support is still being proven.

The important boundary is:

```
music-analysis backend  ->  neutral JSON data  ->  beat-keys
beat-keys               ->  generated animation data  ->  ParAnimator
```

ParAnimator should not know whether the analysis came from libopenmpt,
Sonic Annotator, Marsyas, aubio, Essentia, a hand-authored file, or
something found under a desk with a 2007 timestamp.

## Recommended Backend Strategy

The default first backend should be tracker files through libopenmpt.

Tracker files such as XM, MOD, S3M, IT, and MPTM contain musical structure
instead of only rendered audio.  They expose orders, patterns, rows,
channels, tempo changes, instruments, samples, and playback timing.  That
is useful for animation because it can provide beat-like and section-like
timing without starting with blind audio analysis.

Conceptually:

```
tracker file
  -> libopenmpt
  -> pattern / timing / PCM data
  -> par-beatdown conversion
  -> song.music.json
```

This keeps the first implementation inside the program and avoids Vamp
plugin discovery while the timeline format is still young.

The optional audio-file backend should be Sonic Annotator.

Sonic Annotator is a batch feature-extraction tool.  It runs Vamp audio
analysis plugins on audio files and writes feature results in selectable
formats.

That is almost exactly the shape needed here:

```
audio file
  -> Sonic Annotator
  -> feature output
  -> par-beatdown conversion
  -> song.music.json
```

This keeps par-beatdown small at first.  It does not need to become an
audio decoder, a DSP framework, or a museum for research code.

## Backend Candidates

Initial backend:

```
tracker-file: libopenmpt
```

Possible later backends:

```
audio-file: Sonic Annotator
Marsyas
aubio
Essentia
direct Vamp plugin hosting
hand-authored JSON
some regrettable script from 2007
```

libopenmpt should be treated as the default first implementation.

Sonic Annotator remains the preferred first audio-file backend, but it
should be optional because it depends on external Vamp plugins for useful
analysis.

Marsyas remains useful as an experimental or alternate backend, but should
not be the foundation unless it provides a specific feature that cannot be
obtained more conveniently from Vamp plugins.

## Licensing Boundary

par-beatdown may depend on GPL tools.  That is acceptable because
par-beatdown is a separate tool and ParAnimator consumes only the generated
JSON timeline.

The architecture should remain:

```
par-beatdown       GPL-compatible audio analysis and timeline generator
beat-keys          JSON adapter from music timeline to animation keyframes
paranimator        no direct dependency on audio-analysis backends
*.music.json       data interchange
```

The JSON output is data, not code from the analysis backend.

ParAnimator should not link against libopenmpt, Sonic Annotator, Marsyas,
aubio, Essentia, or any other music-analysis backend.  It should read
timeline files.

## Input Files

par-beatdown should accept tracker files, compressed audio files, and
uncompressed audio files.

Expected initial tracker formats depend on libopenmpt, but the design
should assume common module files such as:

```
MOD
XM
S3M
IT
MPTM
```

Expected audio-file formats depend on the selected audio backend, but the
design should assume common audio files such as:

```
MP3
Ogg
Opus
WAV
AIFF
FLAC, if supported by the selected backend
```

Tracker-file analysis can use musical structure directly and may also
decode PCM samples through libopenmpt when continuous energy features are
useful.

Internally, audio analysis operates on decoded PCM samples.  The public
design should not care whether the original file was MP3 or WAV.

Conceptually:

```
input audio file
  -> decode / resample / downmix
  -> PCM analysis stream
  -> feature extraction
  -> song.music.json
```

If a backend does not support a file type directly, par-beatdown may use an
external conversion step.

For example:

```
ffmpeg -i song.mp3 -ar 44100 -ac 1 temporary-analysis.wav
```

Then:

```
temporary-analysis.wav
  -> analysis backend
  -> song.music.json
```

The timeline should still record the original audio file, not just the
temporary analysis file.

## Command Line

Initial command:

```
par-beatdown song.xm -o song.music.json --fps 30
```

Equivalent audio-file example:

```
par-beatdown song.wav -o song.music.json --fps 30
```

With optional sync offset:

```
par-beatdown song.mp3 -o song.music.json --fps 30 --offset 0.000
```

Possible later options:

```
--backend tracker-file
--backend sonic-annotator
--backend marsyas
--backend aubio
--normalize track
--normalize none
--hop-size 512
--window-size 2048
--analysis-rate 44100
--analysis-channels mono
--dump-debug-csv song.features.csv
--keep-temporary-files
```

## Components

### 1. Input Stage

Responsible for locating, validating, and reading the source file.

Responsibilities:

* Accept the source filename.
* Determine file type where practical.
* Preserve original filename in the timeline metadata.
* Read tracker structure when the selected backend supports it.
* Decode, downmix, or resample as needed by the selected backend.
* Account for manual audio offset.

This stage exists because MP3 files exist, and civilization has not
recovered.

### 2. Backend Runner

Responsible for invoking the selected analysis backend.

For the initial tracker-file backend, this means:

* Load the module with libopenmpt.
* Read song duration, channel count, and format metadata.
* Map order, pattern, and row timing into timeline events.
* Decode PCM when continuous energy features are needed.

For the optional Sonic Annotator backend, this means:

* Locate sonic-annotator.
* Locate required Vamp plugins.
* Run the configured transforms.
* Capture CSV or other feature output.
* Report useful errors when transforms or plugins are missing.

The backend runner should be isolated from the rest of the code.

The rest of par-beatdown should see backend output as feature data, not as
libopenmpt state or Sonic Annotator trivia.

### 3. Feature Importer

Responsible for converting backend-specific output into internal feature
streams.

For Sonic Annotator, this probably means parsing CSV output.

For tracker files, this means converting module metadata, playback timing,
patterns, rows, channels, and optional PCM analysis into the same internal
feature streams.

Internal representation should distinguish:

* sparse events
* continuous features
* metadata
* confidence values, if available

### 4. Timeline Writer

Responsible for writing normalized JSON timeline data.

Responsibilities:

* Convert analysis times to animation frames.
* Normalize feature values.
* Emit sparse events.
* Emit continuous per-frame or per-hop features.
* Preserve metadata for synchronization and debugging.

### 5. ParAnimator Timeline Reader

A future ParAnimator feature.

Responsibilities:

* Read external timeline files.
* Sample feature values at frame F.
* Find nearby events at frame F.
* Convert events into pulses/envelopes.
* Apply bindings to current animation parameters.

ParAnimator should not know about Sonic Annotator.

It should not know about libopenmpt.

It should not know about Marsyas.

It should know only about external timeline data.

### 6. beat-keys Adapter

The current bridge to ParAnimator.

Responsibilities:

* Read a base animation config.
* Read `song.music.json`.
* Read `adapter.beat-keys.json`.
* Map timeline features or events onto animation parameters.
* Write a generated keyframe overlay or merged animation config.

`beat-keys` exists so the mapping model can be tested before ParAnimator
learns native music timeline evaluation.

## Timeline File Format

The primary output is JSON.

Example:

```
{
  "version": 1,
  "generator": {
    "name": "par-beatdown",
    "backend": "tracker-file"
  },
  "audio": {
    "file": "song.xm",
    "codec": "xm",
    "duration": 184.2,
    "source_channels": 4,
    "source_sample_rate": 44100,
    "analysis_channels": 2,
    "analysis_sample_rate": 44100,
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

The offset exists because audio/video sync always finds a way to be
annoying.

MP3 encoder delay and padding may create small sync disagreements between
tools.  The offset field exists so this can be corrected without lying to
the timeline.

## Sonic Annotator Integration

The audio-file implementation should treat Sonic Annotator as an external
command.

Conceptual flow:

```
par-beatdown
  -> run sonic-annotator with selected transforms
  -> collect feature output
  -> normalize and frame-align features
  -> write song.music.json
```

The user should not have to read Sonic Annotator output directly unless
something has already gone wrong.

par-beatdown should eventually provide named presets such as:

```
beats
energy
spectrum
full
```

A preset should expand to one or more Sonic Annotator transforms.

For example:

```
par-beatdown song.mp3 --preset beats -o song.music.json
```

or:

```
par-beatdown song.mp3 --preset full -o song.music.json
```

The exact transform files and plugin choices can evolve without changing
the timeline format.

## Tracker File Integration

The tracker-file implementation should use libopenmpt directly.

Conceptual flow:

```
par-beatdown
  -> load module with libopenmpt
  -> read structural timing and metadata
  -> decode PCM only where needed
  -> normalize and frame-align features
  -> write song.music.json
```

The first tracker-file backend should focus on predictable structure:

```
duration
format
channels
orders
patterns
rows
tempo changes
```

This is enough to emit useful timing markers before trying to infer musical
meaning from rendered audio.

Possible tracker-derived events:

```
row
pattern
order
tempo
loop
```

Possible tracker-derived continuous features:

```
channel_activity
sample_activity
pcm_rms
pcm_peak
```

The tracker backend should not pretend every row is a beat.  It should
expose honest structural events first.  Later code can derive pulses or
downsample rows into animation-friendly signals.

## ParAnimator Integration

ParAnimator should gain support for external timelines.

Example animation configuration:

```
{
  "music": {
    "file": "song.mp3",
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

### Phase 1: libopenmpt Tracker Reader

Implement par-beatdown as a libopenmpt-based tracker-file reader.

Input:

```
song.xm
```

Output:

```
song.music.json
```

Extract only:

```
duration
format
channel count
row or pattern events
optional PCM RMS envelope
```

This is enough to prove the pipeline.

The first version should be boring.  Boring is how software earns the right
to become weird later.

### Phase 2: Timeline Writer

Write tracker-derived data to the neutral JSON timeline.

At this phase, the important work is:

* stable parsing
* stable frame conversion
* stable metadata
* clear error messages
* no attempt to solve music theory

### Phase 3: Sonic Annotator Runner

Implement the optional audio-file feature as a wrapper around Sonic
Annotator.

Input:

```
song.mp3
```

Output:

```
song.music.json
```

Extract only:

```
tempo
beat events
rms or amplitude envelope
```

### Phase 4: beat-keys Adapter

Before modifying ParAnimator, add a small converter:

```
beat-keys base-animation.json song.music.json adapter.beat-keys.json \
  -o music-animation.json
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

It also avoids adding abstractions before knowing which ones are not
stupid.

### Phase 5: Native External Timeline Support

Add ParAnimator support for:

```
music.analysis
bindings
feature sampling
event pulses
smoothing
clamping
```

At this point, `beat-keys` becomes optional.

### Phase 6: More Features

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
backend selection
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
par-beatdown song.mp3 --fps 30 -o song.music.json

beat-keys animation.json song.music.json adapter.beat-keys.json \
  -o animation.music.json

paranimator animation.music.json

ffmpeg -r 30 -i frames/frame%05d.png -i song.mp3 \
  -shortest -c:v libx264 -pix_fmt yuv420p final.mp4
```

## Error Handling

par-beatdown should produce useful errors for common failures:

* Tracker file cannot be loaded.
* libopenmpt reports an unsupported module format.
* Sonic Annotator is not installed.
* Required Vamp plugin is missing.
* Requested preset cannot be resolved.
* Audio file cannot be decoded.
* Backend output cannot be parsed.
* Feature stream is empty.
* Timeline FPS is missing or invalid.

Errors should explain the missing thing and the command or setting likely
needed to fix it.

No one needs a stack trace because a plugin named something like
vamp:qm-vamp-plugins:qm-tempotracker:beats was not installed.  The name is
punishment enough.

## Architecture Rule

ParAnimator should consume neutral timeline data.

It should not consume libopenmpt.

It should not consume Sonic Annotator.

It should not consume Marsyas.

It should not know what a Vamp plugin is.

It should not care.

That is how software avoids becoming archaeology.
