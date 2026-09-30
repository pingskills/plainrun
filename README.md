# PlainRun

A simple native running log for Linux.

PlainRun records your runs and makes it easy to see how much you have been
running. It is the first of the **Plain Apps**: small, fast, native
applications that do one thing well.

<!-- Screenshot: run PlainRun, then save a PNG to docs/screenshot.png and
     uncomment the line below. On Omarchy: Super+Shift+S (or `grim -g "$(slurp)"`). -->
<!-- ![PlainRun](docs/screenshot.png) -->

## Features

- **Fast entry**: date (defaults to today), distance, time and an optional note.
  Pace is calculated as you type.
- **At a glance**: your last run, plus this week, month and year, with last
  week and last month for comparison.
- **History**: Week / Month / Year / All views, note search and sortable columns.
  View, edit or delete any run.
- **Two quiet charts**: weekly distance (12 weeks) and monthly distance (12 months).
- **Personal bests** for 1 km, 5 km, 10 km, half marathon and marathon.
- **Backup and restore**, plus **CSV export and import**.
- **Keyboard friendly** throughout.
- **Local only**: no accounts, cloud, telemetry or network access.
- Follows the **Omarchy** theme when available; otherwise uses your system colours.

## Philosophy

PlainRun is not a fitness platform. It has no GPS tracking, training plans,
social features, badges or coaching. It stores four things per run (date,
distance, time and note) and derives everything else. When a choice comes up
between another feature and a faster, clearer, more reliable workflow, it
takes the second.

## Supported platforms

Developed and tested on Arch Linux with Omarchy (Hyprland, Wayland) and Qt 6.11.
It is a standard Qt 6 application and should build on any modern Linux
distribution with Qt 6.5 or newer; other distributions have not been tested yet.

## Building

### Dependencies

| Purpose | Arch / Omarchy | Debian / Ubuntu (24.04+) | Fedora |
|---|---|---|---|
| Compiler | `base-devel` | `build-essential` | `gcc-c++` |
| Build | `cmake ninja` | `cmake ninja-build` | `cmake ninja-build` |
| Qt 6 | `qt6-base qt6-declarative` | `qt6-base-dev qt6-declarative-dev qml6-module-qtquick-controls qml6-module-qtquick-dialogs qml6-module-qtquick-layouts qml6-module-qtcore libqt6sql6-sqlite` | `qt6-qtbase-devel qt6-qtdeclarative-devel` |

A C++20 compiler (GCC 11+ or Clang 14+), CMake 3.21+ and Qt 6.5+ are required. The Debian/Ubuntu and Fedora package names are a best guide; those
distributions have not been tested.

### Arch Linux / Omarchy

```sh
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative
git clone https://github.com/pingskills/plainrun.git
cd plainrun
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
./build/plainrun
```

To install as an Arch package instead (recommended on Arch), see
[Installing](#installing).

### Other Linux distributions

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
ctest --test-dir build
sudo cmake --install build
```

## Installing

**Arch Linux / Omarchy**, from this repository:

```sh
cd packaging/arch
makepkg -si
```

Once published, `plainrun` will also be available from the AUR
(`yay -S plainrun` or `paru -S plainrun`).

**Anywhere else**: `sudo cmake --install build` as above. The install
puts these files under the prefix:

```
bin/plainrun
share/applications/io.github.pingskills.plainrun.desktop
share/metainfo/io.github.pingskills.plainrun.metainfo.xml
share/icons/hicolor/{scalable,16x16,…,256x256}/apps/io.github.pingskills.plainrun.{svg,png}
share/licenses/plainrun/LICENSE
```

PlainRun has no built-in updater; update it through your package manager.

## Your data

Each Linux user has their own database, created the first time they run PlainRun:

```
~/.local/share/plainrun/plainrun.db
```

More precisely, `$XDG_DATA_HOME/plainrun/plainrun.db`
(Qt's `QStandardPaths::AppDataLocation`). **Help → About** shows the exact
path and has a button to open the folder. The only other file is a small
settings file at `~/.config/plainrun/plainrun.conf`, which remembers the
selected history view.

The data folder is created readable only by you (0700), and the database
and any backups PlainRun writes are 0600.

The database is plain SQLite; see [docs/DATABASE.md](docs/DATABASE.md) for
the schema. Nothing is ever sent anywhere.

## Backup and restore

- **File → Backup Data…** (Ctrl+B) writes a complete, consistent copy of your
  data to a file you choose, by default `plainrun-backup-YYYY-MM-DD.db`.
  Copy that file to another computer to move your running log.
- **File → Restore Data…** checks that the file is a genuine PlainRun backup,
  shows how many runs it contains and asks for confirmation. Before replacing
  anything, your current data is saved in the data folder as
  `plainrun-pre-restore-YYYY-MM-DD-HHMMSS.db`. If the restore fails, your
  current data is left as it was.

A backup is a standard SQLite database file; the format is documented in
[docs/DATABASE.md](docs/DATABASE.md#backup-format).

## CSV export and import

**File → Export CSV…** writes every run as UTF-8 CSV:

```csv
date,distance_km,duration,pace,note
2026-09-27,10.000,58:00,5:48,Long run
2026-09-30,5.000,28:15,5:39,"Easy run, around Karkarook"
```

- `date`: ISO format (`YYYY-MM-DD`).
- `distance_km`: three decimals, i.e. exact metres.
- `duration`: `m:ss` or `h:mm:ss`.
- `pace`: for reference only, and ignored on import.
- `note`: quoted when it contains commas, quotes or line breaks (RFC 4180).

**File → Import CSV…** reads the same format. Columns are matched by header
name, so their order doesn't matter and extra columns are ignored; `note` and
`pace` are optional. Every row is checked first. If any row is invalid,
nothing is imported and the problem rows are listed with line numbers. Runs
that already exist with the same date, distance, time and note are skipped,
so importing the same file twice doesn't duplicate anything. The import runs
in a single transaction.

Backup/restore moves PlainRun between computers. CSV is for spreadsheets and
other tools.

## Keyboard shortcuts

| Shortcut | Action |
|---|---|
| Ctrl+N | Add run |
| Ctrl+E / Enter | Edit selected run |
| Delete | Delete selected run (asks first) |
| Ctrl+S | Save, in the run editor |
| Esc | Cancel the editor or dialog; clear search |
| Ctrl+F | Search notes |
| Ctrl+1 … Ctrl+4 | This week / month / year / all runs |
| Ctrl+B | Backup data |
| Ctrl+Q | Quit |
| ↑ / ↓ | Move through runs; in the date field, change the day |
| Page Up / Page Down | In the date field, change by a week |

## Command line

The same `plainrun` executable answers a few read-only questions without
opening a window. This is intended for status bars and scripts, such as a
future Omarchy widget.

```console
$ plainrun --week-distance
15.20
$ plainrun --month-distance
42.10
$ plainrun --year-distance
512.34
$ plainrun --last-run
2026-09-27	10.00	58:00	5:48
$ plainrun --version
plainrun 0.1.0
```

- Distances are kilometres with two decimals and a `.` decimal point,
  whatever the locale.
- Weeks start on Monday.
- `--last-run` prints `date<TAB>km<TAB>duration<TAB>pace`.
- The CLI opens the database read-only and never creates it.

| Exit code | Meaning |
|---|---|
| 0 | Success (no database yet counts as zero distance) |
| 1 | The database could not be read |
| 2 | Usage error |
| 3 | `--last-run`: no runs recorded |

## Omarchy integration

Omarchy is optional; PlainRun is an ordinary Qt application. When Omarchy is
present:

- **Colours** are read from the active theme's `colors.toml`
  (`~/.local/state/omarchy/current/theme/`, or
  `~/.config/omarchy/current/theme/` on older releases). PlainRun uses
  `background`, `foreground`, `accent`, `selection`, `red` and `mode`.
- **Theme changes** apply immediately. PlainRun watches those files, so
  `omarchy theme set …` recolours an open window.
- **Text size and scaling** come from the same places as every other Qt
  application: fontconfig/GTK font settings and Hyprland's monitor scale.
  Omarchy has no separate text-size setting to follow.

If the files are missing or unreadable, PlainRun uses the system (Qt/GTK)
colours, or its own restrained light/dark palette. The integration is
isolated in `src/omarchy/` (file discovery and parsing) and `src/ui/theme.*`.

Two environment variables override this:

| Variable | Effect |
|---|---|
| `PLAINRUN_NO_OMARCHY=1` | Ignore Omarchy |
| `PLAINRUN_COLOR_SCHEME=light` or `dark` | Use PlainRun's built-in palette |

## Uninstalling

Removing the package (`sudo pacman -R plainrun`, or deleting the installed
files) **does not delete your running history**. Your data stays in
`~/.local/share/plainrun/`, and reinstalling PlainRun picks it up again.

To remove your data permanently, back it up first if you might want it later,
then delete the folders:

```sh
plainrun   # File → Backup Data…, save somewhere safe
rm -r ~/.local/share/plainrun ~/.config/plainrun
```

## Development

See [CONTRIBUTING.md](CONTRIBUTING.md). In short:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DPLAINRUN_BUILD_DEVTOOLS=ON
cmake --build build && ctest --test-dir build --output-on-failure
```

Diagnostics are quiet by default. Enable them with
`QT_LOGGING_RULES="plainrun.*=true" plainrun`; notes are never logged.

Project documents:

- [docs/DATABASE.md](docs/DATABASE.md): schema, migrations, backup format
- [docs/RELEASE.md](docs/RELEASE.md): making a release
- [docs/AUR.md](docs/AUR.md): publishing to the AUR
- [docs/PLAIN_APPS_GUIDELINES.md](docs/PLAIN_APPS_GUIDELINES.md): conventions for the Plain Apps family
- [docs/FUTURE.md](docs/FUTURE.md): ideas deliberately left out of version 1

## Licence

MIT. See [LICENSE](LICENSE).
