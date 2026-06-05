# Vamp Plug-ins

par-beatdown should use Sonic Annotator as a Vamp host.  Sonic
Annotator does not include useful analysis plug-ins by itself, so the
first pipeline needs a small set of Vamp plug-ins installed through vcpkg
overlay ports.

Keep the plug-in ports host-agnostic.  Do not install them into the Sonic
Annotator tool directory.  Install each plug-in set into its own vcpkg
tool directory, then run Sonic Annotator with `VAMP_PATH` pointing at the
selected plug-in directories.

On Windows, `VAMP_PATH` uses `;` as a separator.  On Unix-like systems it
uses `:`.

## Suggested Plug-ins

### QM Vamp Plug-ins

Start here.

Use for:

* beat events
* tempo estimates
* onset events
* bar events

Important plug-ins:

* `vamp:qm-vamp-plugins:qm-tempotracker`
* `vamp:qm-vamp-plugins:qm-onsetdetector`
* `vamp:qm-vamp-plugins:qm-barbeattracker`

This is the best first package because it proves the beat pipeline with
the fewest moving parts.

### BBC Vamp Plug-ins

Add after QM timing works.

Use for:

* RMS energy
* intensity
* spectral flux
* rhythm features

Important plug-ins:

* `vamp:bbc-vamp-plugins:bbc-energy`
* `vamp:bbc-vamp-plugins:bbc-intensity`
* `vamp:bbc-vamp-plugins:bbc-spectral-flux`
* `vamp:bbc-vamp-plugins:bbc-rhythm`

This package gives useful continuous features for animation without
turning the first implementation into a research survey.

### libxtract Vamp Plug-ins

Add later.

Use for:

* RMS
* spectral centroid
* spectral rolloff
* spectral flatness
* other low-level descriptors

This package is useful for richer feature sets, but it should wait until
the core Sonic Annotator, plug-in discovery, CSV import, and timeline
writer path works.

## Overlay Port Slices

### Slice 1: QM Vamp Plug-ins

Create `vcpkg-ports/qm-vamp-plugins`.

The port should:

* download a pinned binary release
* verify the archive hash
* install the `.dll`, `.so`, or `.dylib` files into
  `tools/qm-vamp-plugins`
* install any `.cat`, `.n3`, or `.ttl` metadata files beside the binary
* install the upstream license file
* add a usage file that shows the `VAMP_PATH` directory

Then:

* add `qm-vamp-plugins` to the root `vcpkg.json`
* run `cmake --workflow rt-default`
* run Sonic Annotator with `VAMP_PATH` set to the installed QM directory
* verify `sonic-annotator -l` lists `qm-tempotracker`
* do not parse audio yet

### Slice 2: BBC Vamp Plug-ins

Create `vcpkg-ports/bbc-vamp-plugins`.

The port should:

* download a pinned binary release
* verify the archive hash
* install the plug-in binary into `tools/bbc-vamp-plugins`
* install `.cat` and `.n3` metadata beside the binary
* install the upstream license file
* add a usage file that shows how to append it to `VAMP_PATH`

Then:

* add `bbc-vamp-plugins` to the root `vcpkg.json`
* run `cmake --workflow rt-default`
* run Sonic Annotator with both QM and BBC directories in `VAMP_PATH`
* verify `sonic-annotator -l` lists `bbc-energy`
* verify `sonic-annotator -l` lists `bbc-spectral-flux`
* still do not parse audio unless the discovery path is clean

### Slice 3: Plug-in Path Helper

Add a small CMake or source-level helper that records the vcpkg tool
directories needed by par-beatdown.

The helper should provide:

* Sonic Annotator executable path
* QM plug-in directory
* BBC plug-in directory
* platform-correct `VAMP_PATH` joining

The tool should still allow explicit command-line overrides.  Discovery
order should be:

* explicit command-line path
* configured vcpkg tool path
* system `PATH` or system Vamp path

### Slice 4: Sonic Annotator Smoke Test

Add an integration smoke test that runs:

```
sonic-annotator -l
```

The test should require the expected plug-in identifiers.  It should not
depend on a real audio file.

Expected identifiers:

* `vamp:qm-vamp-plugins:qm-tempotracker`
* `vamp:qm-vamp-plugins:qm-onsetdetector`
* `vamp:qm-vamp-plugins:qm-barbeattracker`
* `vamp:bbc-vamp-plugins:bbc-energy`
* `vamp:bbc-vamp-plugins:bbc-spectral-flux`

### Slice 5: libxtract Vamp Plug-ins

Create `vcpkg-ports/libxtract-vamp-plugins` only after QM and BBC
feature extraction are working.

The port should:

* download or build a pinned release
* verify the archive hash
* install the plug-in binary into `tools/libxtract-vamp-plugins`
* install metadata beside the binary
* install the upstream license file
* update the plug-in smoke test with selected descriptors

## Notes

Avoid bundling every available Vamp plug-in at the start.  The first
milestone is:

```
audio file
  -> sonic-annotator
  -> QM beat data
  -> par-beatdown import
  -> neutral timeline JSON
```

After that works, add continuous features.

## References

* https://vamp-plugins.org/plugin-doc/qm-vamp-plugins.html
* https://github.com/bbc/bbc-vamp-plugins
* https://vamp-plugins.org/download.html
