# PlainRun database

## Location

One SQLite file per Linux user:

```
$XDG_DATA_HOME/plainrun/plainrun.db      (normally ~/.local/share/plainrun/plainrun.db)
```

This is resolved with `QStandardPaths::AppDataLocation`, with the Qt
application name `plainrun` and no organisation name. The directory is created
on first launch. The package never creates or removes it.

Other files that may appear in that directory:

| File | Purpose |
|---|---|
| `plainrun-pre-restore-YYYY-MM-DD-HHMMSS.db` | Your data as it was just before a restore. Safe to delete once you are happy. |
| `plainrun.db.restoring` | Temporary file during a restore. Removed automatically. |

## Schema (version 2)

```sql
PRAGMA application_id = 1349276270;  -- 0x506C526E, ASCII "PlRn"
PRAGMA user_version = 2;             -- schema version

CREATE TABLE runs (
  id               INTEGER PRIMARY KEY,
  run_date         TEXT    NOT NULL
                   CHECK (run_date GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]'),
  distance_metres  INTEGER NOT NULL CHECK (distance_metres > 0),
  duration_seconds INTEGER NOT NULL CHECK (duration_seconds > 0),
  note             TEXT    NOT NULL DEFAULT '',
  created_at       TEXT    NOT NULL,   -- ISO 8601 UTC, e.g. 2026-09-30T05:12:33Z
  updated_at       TEXT    NOT NULL,
  avg_heart_rate_bpm INTEGER           -- added in version 2; NULL = not recorded
                   CHECK (avg_heart_rate_bpm IS NULL OR avg_heart_rate_bpm BETWEEN 30 AND 250)
);
CREATE INDEX runs_run_date_idx ON runs (run_date);
```

| Version | PlainRun | Change |
|---|---|---|
| 1 | 0.1.0 | `runs` table |
| 2 | 0.2.0 | optional `avg_heart_rate_bpm` (`ALTER TABLE … ADD COLUMN`) |

Design notes:

- **Distance is whole metres and duration is whole seconds**, stored as
  integers. There is no floating-point drift, and totals are exact. Kilometres
  typed in the form are converted exactly from the decimal text, rounding half
  up to the nearest metre.
- **The run date is a calendar date** (`YYYY-MM-DD`) with no time zone. A run
  on the 30th is always on the 30th, wherever the computer is.
- **Heart rate is optional** and stored as `NULL` when not recorded, never 0.
- **Nothing derived is stored.** Pace, beats per km, weekly, monthly and yearly
  totals, averages, charts and personal bests are calculated from `runs` each time the
  data changes. There are few rows, so this is instantaneous, and derived
  values can never disagree with the data.

## Migrations

`PRAGMA user_version` holds the schema version, and
`src/database/migrations.cpp` holds an append-only list of migrations. Each
migration is `(version, [SQL statements])`.

When PlainRun opens the database:

1. It reads `application_id`, `user_version` and whether any tables exist.
2. It **refuses to open** (and shows an error without changing anything) if:
   - the file is not a SQLite database;
   - it has tables but isn't marked as PlainRun (`application_id`);
   - its `user_version` is higher than this build supports (it was written by
     a newer PlainRun).
3. It applies each migration newer than `user_version` **in its own
   transaction**, together with the `user_version` update. If a statement
   fails, that transaction is rolled back and the database stays at the
   previous version with all data intact.

PlainRun never deletes, resets or recreates a database, whether to recover
from an error or because the schema changed.

### Adding a migration (for developers)

1. Append `{N, {...statements...}}` to `migrations()`. **Never edit a
   migration that has been released.**
2. Update the code that reads and writes runs.
3. Add a test to `tests/tst_database.cpp` that creates a database at version
   N−1 containing data, opens it with the new build, and checks that the data
   survived.
4. Document the new version in this file.

`tests/tst_database.cpp` already covers upgrading with data present, a
failing migration rolling back, and refusal of newer or foreign databases.

## Backup format

A PlainRun backup is **a complete, standalone SQLite database file** with the
same schema as `plainrun.db`. It can be opened with the `sqlite3` tool, and
future PlainRun versions will migrate it forward when it is restored.

- It is written with SQLite's `VACUUM INTO`, which produces a transactionally
  consistent, compacted snapshot even while PlainRun has the database open.
  PlainRun never copies a live database file byte-for-byte.
- The snapshot is written to `<target>.partial`, verified (header,
  `PRAGMA quick_check`, readable `runs` table), then atomically renamed to the
  chosen file name.

A file is accepted for restore only if:

1. it is a readable SQLite database;
2. `application_id` is PlainRun's (`0x506C526E`);
3. `user_version` is between 1 and the version this build supports;
4. `PRAGMA quick_check` reports `ok`;
5. the `runs` table can be read.

The restore itself:

1. Copy the backup into the data directory as `plainrun.db.restoring`
   (again with `VACUUM INTO`), and migrate that copy if it came from an older
   version.
2. Save the current data as `plainrun-pre-restore-<timestamp>.db`.
3. Close the database and atomically rename the prepared copy over
   `plainrun.db`.
4. Reopen it. If that fails, put the pre-restore copy back.

If any step up to 3 fails, `plainrun.db` has not been touched.

## Inspecting your data

```sh
sqlite3 -readonly ~/.local/share/plainrun/plainrun.db \
  "SELECT run_date, distance_metres/1000.0, duration_seconds, note FROM runs ORDER BY run_date"
```

Scripts should use `plainrun --week-distance` and the other CLI options rather
than depending on the schema.
