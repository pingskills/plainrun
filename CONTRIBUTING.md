# Contributing to PlainRun

Thanks for your interest. PlainRun is deliberately small. Before proposing a
feature, please read the product philosophy in the [README](README.md) and
[docs/PLAIN_APPS_GUIDELINES.md](docs/PLAIN_APPS_GUIDELINES.md).

> When choosing between adding another feature and making the existing
> workflow faster, clearer and more reliable, choose the latter.

## Bug reports

Please include:

- PlainRun version (`plainrun --version`)
- distribution and desktop (for example Arch + Omarchy, Fedora + GNOME)
- what you did, what you expected and what happened
- terminal output from `QT_LOGGING_RULES="plainrun.*=true" plainrun`

Diagnostic output never includes your run notes. Please don't attach your
database unless you are happy to share its contents.

## Development

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DPLAINRUN_BUILD_DEVTOOLS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Run against a throwaway database instead of your own:

```sh
build/plainrun-devseed .sandbox/plainrun/plainrun.db   # a year of synthetic runs
XDG_DATA_HOME=$PWD/.sandbox build/plainrun
```

Render the interface offscreen, for example to check a layout change:

```sh
QT_QPA_PLATFORM=offscreen build/plainrun-uishot .sandbox/plainrun/plainrun.db shot.png 1080 720 select
```

## Code guidelines

- C++20 and Qt 6; no new third-party dependencies without discussion.
- Non-visual logic lives in `src/core`, `src/database` and `src/services`
  (Qt Core + Qt SQL only) and has tests in `tests/`.
- QML stays declarative; formatting and calculations happen in C++.
- Database changes are new, append-only migrations (see [docs/DATABASE.md](docs/DATABASE.md)).
- Keep the UI keyboard-accessible and check it in light and dark themes.
- `ctest` must pass.
