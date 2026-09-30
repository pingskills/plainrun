# Releasing PlainRun

PlainRun uses semantic versioning and tags named `vX.Y.Z`.

## Where the version lives

The authoritative version is `project(plainrun VERSION x.y.z)` in
`CMakeLists.txt`. The binary (About dialog, `plainrun --version`) gets it
from there through the generated `plainrun_config.h`.

Three other files record *release history* rather than the current version.
The release workflow refuses to publish unless they agree with the tag:

| File | What to add |
|---|---|
| `CHANGELOG.md` | a `## [x.y.z] - YYYY-MM-DD` section |
| `packaging/io.github.pingskills.plainrun.metainfo.xml` | a `<release version="x.y.z" date="YYYY-MM-DD">` entry at the top of `<releases>` |
| `packaging/arch/PKGBUILD` | `pkgver=x.y.z`, `pkgrel=1` (the checksum is updated *after* tagging) |

## Checklist

```sh
# 1. Update the version and the history files listed above, then check everything
cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build
appstreamcli validate --no-net packaging/io.github.pingskills.plainrun.metainfo.xml
desktop-file-validate packaging/io.github.pingskills.plainrun.desktop

# 2. Commit and tag
git commit -am "Release 0.1.0"
git tag -a v0.1.0 -m "PlainRun 0.1.0"
git push origin main v0.1.0
```

Pushing the tag starts `.github/workflows/release.yml`. It checks that the
tag, CMake version, changelog and metainfo agree, builds and tests on Arch,
and creates the GitHub release with the changelog section as its notes.
GitHub attaches the source archive
`https://github.com/pingskills/plainrun/archive/refs/tags/v0.1.0.tar.gz`
automatically, and the Arch package builds from that archive.

If you prefer not to rely on the workflow, create the release by hand:

```sh
gh release create v0.1.0 --title "PlainRun 0.1.0" --notes-file <(sed -n '/## \[0.1.0\]/,/## \[/p' CHANGELOG.md | sed '1d;$d')
```

## After the release

```sh
# 3. Pin the checksum of the published source archive in the upstream PKGBUILD
cd packaging/arch
updpkgsums            # from pacman-contrib; replaces 'SKIP' with the real sha256
makepkg -Cf           # clean build from the GitHub tarball, including check()
namcap PKGBUILD *.pkg.tar.zst
git commit -am "packaging: checksum for v0.1.0" && git push
```

4. Update the AUR package; see [AUR.md](AUR.md).
5. Add a new empty `## [Unreleased]` section to `CHANGELOG.md` if it is missing.

## Fixing a bad release

Don't move or re-use a published tag, because packagers have already pinned
its checksum. Release `x.y.(z+1)` instead.
