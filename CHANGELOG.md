# Changelog

All notable changes to PlainRun are recorded here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and PlainRun uses [semantic versioning](https://semver.org/).

## [Unreleased]

## [0.2.0] - 2026-10-01

### Added
- Optional average heart rate per run, entered in the run editor (30–250 bpm).
  The selected run shows it with its **beats per km** (heart rate × minutes per km).
- A beats-per-km trend: the monthly median over the last 12 months, for months
  with at least three runs with a heart rate. It stays hidden until a heart
  rate has been recorded.
- CSV export and import carry heart rate in an optional `avg_hr_bpm` column.
  Files without it still import.
- In narrow windows, a Runs | Trends switch (View → Show Trends, Ctrl+T) keeps
  the charts and personal bests reachable instead of hiding them.

### Changed
- The side panel now shows only trends: charts and personal bests. The selected
  run's note, heart rate and Edit/Delete buttons sit in a strip under the list
  at every window width.
- In short windows the summary bar shrinks to one line, and the period stats line
  is hidden, so the run list keeps the space.
- In narrow windows, note search collapses to a button; Ctrl+F still opens it.
- Database schema version 2. Existing databases and backups are upgraded
  automatically when opened or restored. PlainRun 0.1 refuses to open an
  upgraded database rather than damage it.

## [0.1.1] - 2026-09-30

### Changed
- The data directory is created private to the user (0700), and the database,
  backups and pre-restore copies are written as 0600. Existing files keep their
  permissions.
- `docs/PLAIN_APPS_GUIDELINES.md` is now the canonical family guidelines, with
  conventions learned from PlainWeight.

## [0.1.0] - 2026-09-30

First release.

### Added
- Record runs with date, distance, time and an optional note; pace is calculated.
- Summary of the last run and this week / month / year, with last week and last month for comparison.
- Run history with Week / Month / Year / All views, note search and sortable columns.
- View, edit and delete runs (deletion asks for confirmation).
- Weekly distance (12 weeks) and monthly distance (12 months) charts.
- Personal bests for 1 km, 5 km, 10 km, half marathon and marathon.
- Backup and restore of the SQLite database, with validation and a safety copy of current data.
- CSV export and import (validated, transactional, skips runs already recorded).
- Read-only command line: `--week-distance`, `--month-distance`, `--year-distance`, `--last-run`.
- Follows the active Omarchy theme when present, including live theme changes.
- Desktop entry, icon and AppStream metadata; Arch Linux PKGBUILD.

[Unreleased]: https://github.com/pingskills/plainrun/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/pingskills/plainrun/compare/v0.1.1...v0.2.0
[0.1.1]: https://github.com/pingskills/plainrun/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/pingskills/plainrun/releases/tag/v0.1.0
