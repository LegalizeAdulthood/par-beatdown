# Music Test Plan

This plan proves the current tracker-file backend and `beat-keys`
adapter against a real music file.  Automated tests prove deterministic
JSON.  This plan proves whether the generated music signals are useful
for animation.

Run all commands from the source root:

```
C:\code\par-beatdown\par-beatdown
```

## Test Subject

Use the public-domain XM fixture:

```
data\my_neighbors_kid_is_an_internet_addict.xm
```

The sidecar metadata is:

```
data\my_neighbors_kid_is_an_internet_addict.md
```

## Goals

* Generate a full neutral music timeline from an actual tracker file.
* Inspect the generated tracker metadata, timing, events, and features.
* Convert selected music signals into `beat-keys` output.
* Try the generated animation data in ParAnimator.
* Decide which signals are useful enough for native ParAnimator support.
* Identify default values for fps, offset, and feature hop.

## Non-goals

* Do not add new automated gold files during manual proving.
* Do not tune ParAnimator native timeline behavior here.
* Do not infer musical intent that is not present in the generated data.
* Do not treat tracker rows as beats unless the visual result proves out.

## Baseline Build

Build and run the normal workflow first:

```powershell
cmake --workflow rt-default
```

Expected result:

* configure succeeds
* build succeeds
* unit tests pass
* `par-beatdown` integration tests pass
* `beat-keys` integration tests pass

Stop manual testing if the baseline workflow fails.

Set helper paths for the rest of the plan:

```powershell
$build = '..\build-par-beatdown-default'
$parBeatdown = "$build\tools\par-beatdown\Release\par-beatdown.exe"
$beatKeys = "$build\tools\beat-keys\Release\beat-keys.exe"
```

## Generate A Full Timeline

Generate one small timeline window with every optional section enabled:

```powershell
& $parBeatdown `
  data\my_neighbors_kid_is_an_internet_addict.xm `
  -o C:\tmp\song.music.json `
  --include module `
  --include timeline `
  --include events `
  --include features `
  --start 0 `
  --duration 20
```

Expected result:

* command exits with status `0`
* `C:\tmp\song.music.json` exists
* JSON parses cleanly
* `schema` is `par-beatdown.tracker-timeline`
* `version` is `1`
* `generator.backend` is `tracker-file`
* `source.format` is `xm`
* `module`, `timeline`, `events`, and `features` are present
* `render.start_seconds` is `0.0`
* `render.duration_seconds` is `20.0`

## Inspect Timeline Data

Open `C:\tmp\song.music.json` and check:

* `source.title` matches the module title
* `source.duration_seconds` looks plausible for the full module
* `timeline.duration_seconds` is `20.0`
* `render.fps` is `30.0`
* `render.feature_hop_seconds` is about `0.033333`
* `module.order_count` is greater than zero
* `module.pattern_count` is greater than zero
* `timeline.frames` is greater than zero
* `events` contains `row` events
* `events` contains at least some musical event type, such as `note`
* `features` contains `rms`, `peak`, and `active_channels`

Look for obvious problems:

* negative frame numbers when no offset was requested
* huge gaps in feature frames
* impossible amplitude values
* empty diagnostics hiding load problems
* event storms that would make pulse bindings unusable

If the first 20 seconds are not visually useful, move the window:

```powershell
& $parBeatdown `
  data\my_neighbors_kid_is_an_internet_addict.xm `
  -o C:\tmp\song-later.music.json `
  --include timeline `
  --include events `
  --include features `
  --start 45 `
  --duration 20
```

## Generate An RMS Merge

Generate ParAnimator-style merged tracks from RMS:

```powershell
& $beatKeys `
  tests\beat-keys\base-merge-animation.json `
  C:\tmp\song.music.json `
  tests\beat-keys\merge-rms.beat-keys.json `
  -o C:\tmp\music-animation-rms.json
```

Expected result:

* command exits with status `0`
* output JSON parses cleanly
* authored base tracks are still present
* generated tracks are appended
* generated track ids use the configured namespace
* generated RMS keys are in ascending frame order

Manual judgement:

* RMS should feel like overall loudness
* RMS should not produce constant zero
* RMS should not pin at the clamp maximum for most frames
* the key count should be high enough for motion but not absurdly noisy

## Generate A Peak Overlay

Generate an overlay that uses peak amplitude:

```powershell
& $beatKeys `
  tests\beat-keys\base-animation.json `
  C:\tmp\song.music.json `
  tests\beat-keys\peak.beat-keys.json `
  -o C:\tmp\music-overlay-peak.json
```

Expected result:

* command exits with status `0`
* output schema is `par-beatdown.beat-keys-overlay`
* keyframes use `music.peak`

Manual judgement:

* peak should catch sharp transients better than RMS
* peak should not be too spiky for direct visual use
* peak may need clamp or smoothing before native support

## Generate Note Pulses

Generate pulse keyframes from note events:

```powershell
& $beatKeys `
  tests\beat-keys\base-animation.json `
  C:\tmp\song.music.json `
  tests\beat-keys\note-pulses.beat-keys.json `
  -o C:\tmp\music-overlay-note-pulses.json
```

Expected result:

* command exits with status `0`
* keyframes use `music.note_pulse`
* repeated notes on the same frame merge deterministically
* return-to-zero keys appear after pulse windows

Manual judgement:

* note pulses should read as musical hits
* dense tracker passages may be too busy
* decay may need tuning before this is useful in native support

## Generate Row Pulses

Generate pulse keyframes from row events:

```powershell
& $beatKeys `
  tests\beat-keys\base-animation.json `
  C:\tmp\song.music.json `
  tests\beat-keys\row-pulses.beat-keys.json `
  -o C:\tmp\music-overlay-row-pulses.json
```

Expected result:

* command exits with status `0`
* keyframes use `music.row_pulse`
* row pulse output is deterministic

Manual judgement:

* row pulses may be useful for tight, mechanical motion
* row pulses may be too dense to read as beats
* if row pulses are noisy, prefer note pulses or feature signals

## Try Custom Feature Hop

Generate a lower-density feature timeline:

```powershell
& $parBeatdown `
  data\my_neighbors_kid_is_an_internet_addict.xm `
  -o C:\tmp\song-hop-0_5.music.json `
  --include timeline `
  --include features `
  --start 0 `
  --duration 20 `
  --feature-hop 0.5
```

Then run RMS again:

```powershell
& $beatKeys `
  tests\beat-keys\base-merge-animation.json `
  C:\tmp\song-hop-0_5.music.json `
  tests\beat-keys\merge-rms.beat-keys.json `
  -o C:\tmp\music-animation-rms-hop-0_5.json
```

Manual judgement:

* `0.5` seconds should be visibly coarse
* default hop should feel smoother than the coarse version
* native support likely needs configurable smoothing, not only hop size

## Try Sync Offset

Generate a shifted source timeline:

```powershell
& $parBeatdown `
  data\my_neighbors_kid_is_an_internet_addict.xm `
  -o C:\tmp\song-offset.music.json `
  --include timeline `
  --include events `
  --include features `
  --start 0 `
  --duration 20 `
  --offset 1.25
```

Expected result:

* generated event and feature frames shift by the requested offset
* timing in seconds remains source-relative
* frame conversion reflects the offset

Manual judgement:

* positive offset should make the visual response occur later
* this option is important enough to keep in native ParAnimator support

## ParAnimator Check

Use one merged output, preferably:

```
C:\tmp\music-animation-rms.json
```

Run or load it through ParAnimator using the normal ParAnimator workflow.
Compare it with the same base animation without generated music tracks.

Check:

* ParAnimator accepts the generated JSON shape
* generated track ids do not collide with authored track ids
* authored tracks still render correctly
* generated values affect the intended parameter
* output is deterministic across repeated runs
* the animation feels connected to the music file

Record any mismatch between the generated JSON and ParAnimator's native
config expectations.  Those findings should feed the native
ParAnimator timeline plan.

## Decision Checklist

After manual testing, answer:

* Is `music.rms` useful as a direct animation signal?
* Is `music.peak` useful, or only useful after smoothing?
* Are note pulses useful enough for native support?
* Are row pulses too dense for default use?
* Should effect pulses remain experimental?
* Is the default `30` fps acceptable?
* Is the default feature hop acceptable?
* Is offset handling understandable and correct?
* Should `beat-keys` gain smoothing before native support proceeds?
* Are any schema fields missing for ParAnimator integration?

## Pass Criteria

The current par-beatdown pipeline is manually proven when:

* the full timeline is generated from the XM fixture
* RMS or peak produces useful visual motion in ParAnimator
* at least one pulse source produces an understandable visual accent
* offset changes timing in the expected direction
* feature hop changes output density in the expected way
* no generated JSON needs hand editing before ParAnimator can use it

If these criteria do not pass, tune `par-beatdown` or `beat-keys`
before implementing native ParAnimator timeline support.
