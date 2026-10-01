# Future ideas (not in version 1)

These are recorded so they are not forgotten, and so they don't complicate
version 1. Each needs a clear case that it makes PlainRun better *as a small
running log* before it is built.

## Plausible, small

- **Omarchy shell widget** showing this week's distance, built on the
  existing read-only CLI (`plainrun --week-distance`). This would live outside
  PlainRun itself.
- **Miles.** Everything is stored in metres, so this is a display and parsing
  preference only. It needs a settings entry and CSV header handling
  (`distance_mi`).
- **A date picker** alongside the ISO field, if the keyboard-first field
  proves awkward.
- **More PB distances** (e.g. 15 km, 10 miles), with the same tolerance rule.
- **Translations.** Strings already go through `qsTr`/`tr`.
- **Flatpak** packaging for non-Arch users.

## Larger; only with care

- **Import from watch exports** (FIT/GPX/TCX) as *summary only*
  (date, distance, time), mapped into the existing CSV-like import path.
  No routes or splits.
- **Strava import** from a Strava bulk-export archive (`activities.csv`), again
  summary only. No API or account connection.
- **Shoe tracking**: a `shoes` table and an optional `shoe_id` on runs,
  added by a migration.
- **Simple goals** (e.g. a weekly distance target shown beside "This week").
  Must stay quiet: no badges, streaks or notifications.

## Deliberately out of scope

- GPS routes and maps
- Automatic cloud syncing (backup files work with any sync tool the user
  already has)
- Training plans, AI coaching
- Heart-rate zones, streams or analysis beyond the single average and beats
  per km (PlainRun 0.2 added an optional average heart rate per run)
- Interval or split tracking
- Notifications
- Social features, accounts
- A mobile app
