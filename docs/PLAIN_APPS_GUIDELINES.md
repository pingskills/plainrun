# Plain Apps guidelines

Conventions for the Plain Apps family (PlainRun, and later PlainWeight,
PlainTasks, …).

> Small, fast, native applications that do one thing well.

Each app is **independent**: its own repository, package and database. None
requires another to be installed. There is deliberately no shared library yet.
When three apps have copied the same code and it has stopped changing, that is
the time to consider extracting it. These conventions keep the apps consistent
without coupling them.

## Naming and identity

| Item | Convention | PlainRun |
|---|---|---|
| Display name | `Plain` + noun, CamelCase | PlainRun |
| Executable | lowercase, no separators | `plainrun` |
| Repository | lowercase | `github.com/pingskills/plainrun` |
| Arch/AUR package | same as executable | `plainrun` |
| Application ID | `io.github.pingskills.<executable>` | `io.github.pingskills.plainrun` |
| Qt application name | same as executable | `plainrun` |
| Qt organisation name | **unset** (keeps data at `~/.local/share/<app>`) | |

The application ID names the `.desktop` file, the AppStream metainfo, the
icon, and the Wayland `app_id` (`QGuiApplication::setDesktopFileName`).

## Architecture

- **C++20, Qt 6, Qt Quick/QML, CMake.** No Electron, web views, local servers,
  Python runtimes or cloud services.
- Keep dependencies to Qt plus the standard library. Draw simple charts with
  Qt Quick items.
- Layout:
  - `src/core/`: pure logic (formatting, validation, calculations), Qt Core only.
  - `src/database/`: SQLite connection, migrations, repository, backup.
  - `src/services/`: controller exposed to QML, CLI, data location.
  - `src/ui/`: GUI-only C++ (theme, QML registration).
  - `src/omarchy/`: optional desktop-specific integration, isolated.
  - `qml/`: declarative UI; `components/` and `views/`.
  - `tests/`: Qt Test, run by `ctest`.
- Core logic, database and controller link only Qt Core and Qt SQL, so tests
  run headless. The QML module is a static library so a smoke test can load the
  real UI offscreen and fail on any QML warning.
- **The version is declared once**, in `project(... VERSION x.y.z)`, and
  compiled in through a generated header.

## Data

- **Local-first.** No accounts, telemetry, analytics, crash reporting, remote
  fonts or network requests of any kind.
- Per-user data in `QStandardPaths::AppDataLocation`
  (`$XDG_DATA_HOME/<app>/<app>.db`). Never beside the executable, in `/usr`,
  in the source tree or in the working directory. Never hardcode home paths.
- Settings, if any, go in `$XDG_CONFIG_HOME/<app>/<app>.conf`. Keep them
  minimal.
- Packages **never** create, modify or delete user data. Uninstalling leaves
  data in place.
- No sample data in production. Development seeding is a separate tool that
  refuses to touch the real database.

## SQLite conventions

- One database file per app.
- `PRAGMA application_id` set to a unique four-character tag
  (PlainRun: `PlRn`) to identify the app's databases and backups.
- `PRAGMA user_version` as the schema version, with an append-only list of
  migrations, each in its own transaction with the version bump.
- Refuse (don't "fix") databases that are foreign, corrupt or from a newer
  version. Never reset or recreate a database to recover from an error.
- Store base units as integers (metres, seconds, grams, …). Store calendar
  dates as `YYYY-MM-DD` text and timestamps as ISO 8601 UTC.
- Derive totals and statistics; don't store them.
- Name columns `snake_case` with units in the name (`distance_metres`).

## Backup conventions

- **File → Backup Data…** (Ctrl+B) writes a standalone SQLite copy using
  `VACUUM INTO`, verified before being renamed into place. Default name:
  `<app>-backup-YYYY-MM-DD.db`.
- **File → Restore Data…** validates (application_id, version, quick_check),
  confirms with counts, saves the current data as
  `<app>-pre-restore-<timestamp>.db`, swaps atomically and reopens. A failed
  restore must leave current data untouched.
- CSV export/import, where it makes sense, uses a documented header, RFC 4180
  quoting, UTF-8 and all-or-nothing import that skips exact duplicates.

## CLI conventions

- Same executable; a handful of **read-only** options for scripts and status
  bars (`--version`, `--help`, and a few app-specific queries).
- Machine-friendly output: C-locale numbers, ISO dates, tab-separated fields.
- Exit codes: 0 success, 1 data error, 2 usage error, 3 "nothing to report".
- Never start the GUI and never create the database from the CLI.

## Visual philosophy

- Calm, lightweight, native and restrained. Typography and spacing do the work.
- No gradients, dashboard cards, heavy rounding, gamification, badges,
  motivational text or decorative animation.
- One accent colour, used for the primary action and current values.
- Tabular figures for numbers (`font.features: { "tnum": 1 }`).
- Follow the system/Omarchy theme; support light and dark; check contrast
  (secondary text at least WCAG AA).
- Visible keyboard focus, accessible names on controls, sensible shortcuts
  (Ctrl+N, Ctrl+S, Ctrl+F, Ctrl+Q, Esc) shown in menus and tooltips.
- No fixed window size. Choose a sensible minimum, and check at 1× and
  fractional scales.
- No onboarding wizard. An empty state with one clear action is enough.

## Desktop integration

Install with CMake's `GNUInstallDirs` (respecting `CMAKE_INSTALL_PREFIX`):

- `bin/<app>`
- `share/applications/<app-id>.desktop`
- `share/metainfo/<app-id>.metainfo.xml` (validate with `appstreamcli validate`)
- `share/icons/hicolor/scalable/apps/<app-id>.svg`, plus rendered PNGs at
  16–256 px (SVG source kept in the repo)
- `share/licenses/<app>/LICENSE`

Application code must not assume `/usr` or `/usr/local`.

## Omarchy

Optional and isolated (`src/omarchy/`). Read the active theme's
`colors.toml`, watch it for changes, and fall back silently to Qt/system
styling when it is absent. Never depend on Omarchy in packaging. Document
exactly what is integrated.

## Releases (GitHub)

- Semantic versioning; tags `vX.Y.Z`.
- Steps: update `project(VERSION)`, the `CHANGELOG.md` entry and the metainfo
  `<release>`; commit; tag; push the tag. CI verifies the tag matches the
  CMake version and creates the GitHub release.
- The GitHub source archive for the tag is what distribution packages build from.

## Arch packaging

- `packaging/arch/PKGBUILD` in the upstream repo is the reference.
- Build from the tagged GitHub source archive (never a working directory),
  with a real `sha256sums` (`updpkgsums`).
- `makedepends`: `cmake ninja`. `depends`: only the Qt modules actually used.
- Run tests in `check()`, and check the package with `namcap`.
- The **AUR repository is separate** from the upstream repo. It contains only
  `PKGBUILD` and `.SRCINFO` (generated with `makepkg --printsrcinfo`, never
  edited by hand). See [AUR.md](AUR.md).
