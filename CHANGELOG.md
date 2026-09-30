# Changelog

All notable changes to PlainRun are recorded here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and PlainRun uses [semantic versioning](https://semver.org/).

## [Unreleased]

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

[Unreleased]: https://github.com/pingskills/plainrun/compare/v0.1.1...HEAD
[0.1.1]: https://github.com/pingskills/plainrun/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/pingskills/plainrun/releases/tag/v0.1.0
