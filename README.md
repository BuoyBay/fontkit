# Discover fonts and register process-local font files

Font discovery and process-local font-file registration for Buoy 0.72.0.
This package owns discovery/loading only; it does not choose fallback fonts,
measure text, shape text, or install fonts for other applications.

## Install

From a Buoy consumer project with BPM installed:

```sh
bpm add fontkit --git https://github.com/BuoyBay/fontkit.git --rev refs/heads/main
```

BPM records the resolved commit in its lockfile. Keep that lockfile with the
consumer project for reproducible installs.

## API at a glance

| API | Behavior |
| --- | --- |
| `FontKitFaces()` | An array of `FontFaceInfo` objects copied from a native snapshot. |
| `FontFaceInfo.FamilyName` | Native family name, suitable for exact discovery checks. |
| `FontFaceInfo.FaceName` | Native full face name, not a synthesized style or stable identifier. |
| `FontFaceInfo.PostScriptName` | Native PostScript name where exposed; empty when unavailable. |
| `FontKitHasFamily(name)` | Exact, case-sensitive family presence in a fresh snapshot. No aliases or fallback matching. |
| `FontKitBackend()` | Identifies the registration/discovery backend. |

## Build and API reference

Source is in `modules/`, with native/runtime inputs where required. Use the
[package guide](docs/API.md) for backend-specific build prerequisites, ownership
rules and supported targets. Rebuilding a native backend requires that target's
compiler and SDK; sealing an archive alone does not rebuild the backend.

## Local Buoy setup

On your own machine, install Buoy and run:

```sh
buoy ai init --both
```

The generated agent files and editor settings stay local and are ignored by Git.

## Contributing

[Contributing](CONTRIBUTING.md) · [License](LICENSE)
