# Building EasyWiFi

EasyWiFi is a **PlatformIO library**, not an application. The repository root
holds the library (`library.json` + `src/`) and has **no `platformio.ini`** —
so the PlatformIO VS Code extension shows no build/upload buttons when you open
the root folder. That is expected: a library only compiles as part of a project
that consumes it.

The buildable project in this repo is the demo at
[`examples/Lantern/`](examples/Lantern), which depends on the library via a
local `symlink://../..` reference, so edits to `src/` are picked up immediately.

## Building in VS Code

Pick one:

1. **Open the example as the project** (simplest) — `File → Open Folder →`
   `examples/Lantern`. PlatformIO detects its `platformio.ini` and the
   build / upload / monitor buttons appear.
2. **Multi-root workspace** — keep the repo root open and add `examples/Lantern`
   as a second folder so PlatformIO finds the manifest there.

## Building from the command line

```bash
pio run   -d examples/Lantern            # build
pio run   -d examples/Lantern -t upload  # flash
pio device monitor                       # serial monitor (115200 baud)
```

## Task runner: `build.sh`

`build.sh` wraps the common release tasks. Pass one or more actions; they run in
order and the script **stops on the first failure**:

```bash
./build.sh clean build publish
```

| Action    | What it does |
|-----------|--------------|
| `clean`   | Removes build artifacts: `.pio` caches (root and example) and `dist/`. |
| `build`   | Compiles `examples/Lantern` (verifies the library builds), then runs `pio pkg pack` to produce `dist/EasyWiFi-<version>.tar.gz`. |
| `publish` | Publishes the packed tarball to the PlatformIO Registry. Packs first if no tarball exists. |

Examples:

```bash
./build.sh build              # compile + pack only
./build.sh clean build        # fresh compile + pack
./build.sh clean build publish  # full release to the registry
```

The version in the tarball name comes from `library.json`.

### Publishing notes

`publish` requires an authenticated PlatformIO account. If you are not logged
in, the action fails immediately (and the chain stops) with:

```
✗ not logged in to PlatformIO. Run: pio account login
```

Log in once, then re-run:

```bash
pio account login
./build.sh clean build publish
```

PlatformIO shows its own confirmation prompt before the package is uploaded.
Publishing is **irreversible** for a given version — bump `version` in
`library.json` before republishing.

## Consuming the released library

Other PlatformIO projects depend on EasyWiFi by Git URL, pinned to a tag:

```ini
lib_deps =
    https://github.com/brennanMKE/EasyWiFi.git#v0.3.0
```

Once published to the registry, the registry form also works:

```ini
lib_deps =
    brennanmke/EasyWiFi@^0.3.0
```
