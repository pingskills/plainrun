# Publishing PlainRun to the AUR

There are two separate repositories:

| Repository | Contains | Who uses it |
|---|---|---|
| **Upstream**: `github.com/pingskills/plainrun` | source code, docs, reference `packaging/arch/PKGBUILD` | developers; GitHub releases |
| **AUR**: `ssh://aur@aur.archlinux.org/plainrun.git` | **only** `PKGBUILD` and `.SRCINFO` | Arch users (`yay -S plainrun`) |

The AUR repository never contains source code. Its PKGBUILD downloads the
tagged GitHub source archive.

`.SRCINFO` is **generated** from the PKGBUILD with
`makepkg --printsrcinfo > .SRCINFO`. Never edit it by hand. **Regenerate it
every time the PKGBUILD changes** (version, pkgrel, dependencies, sources,
checksums). The AUR rejects pushes where `.SRCINFO` is missing.

## Before you start (once)

1. Create an account at <https://aur.archlinux.org/register>.
2. Add an SSH public key to the account (*My Account → SSH Public Key*). For example:
   ```sh
   ssh-keygen -t ed25519 -f ~/.ssh/aur -C "aur"
   cat ~/.ssh/aur.pub          # paste this into the AUR account page
   ```
3. Tell SSH to use it, in `~/.ssh/config`:
   ```
   Host aur.archlinux.org
     IdentityFile ~/.ssh/aur
     User aur
   ```
4. Set the `# Maintainer:` line in `packaging/arch/PKGBUILD` to your name and
   the email you want shown publicly.

## 1. Create the AUR package repository

The first release (v0.1.0) must already be published on GitHub; see
[RELEASE.md](RELEASE.md).

```sh
git clone ssh://aur@aur.archlinux.org/plainrun.git ~/aur/plainrun
```

Cloning a package name that doesn't exist yet gives you an empty repository.
It becomes the package on your first push.

## 2. Add the PKGBUILD

```sh
cp ~/Work/plainrun/packaging/arch/PKGBUILD ~/aur/plainrun/
cd ~/aur/plainrun
updpkgsums      # downloads v0.1.0.tar.gz from GitHub and writes its sha256
```

Check that `sha256sums` now contains a real checksum rather than `SKIP`.

## 3. Generate .SRCINFO

```sh
makepkg --printsrcinfo > .SRCINFO
```

## 4. Test with makepkg

```sh
makepkg -Cfsi      # clean build from the GitHub tarball, run tests, install
namcap PKGBUILD
namcap plainrun-0.1.0-1-*.pkg.tar.zst
plainrun --version
```

For an even cleaner test that catches missing dependencies, build in a
chroot. `extra-x86_64-build` is in the `devtools` package.

## 5. Commit

Only `PKGBUILD` and `.SRCINFO` belong in the AUR repository:

```sh
printf '*\n!PKGBUILD\n!.SRCINFO\n!.gitignore\n' > .gitignore
git add PKGBUILD .SRCINFO .gitignore
git commit -m "Initial import: plainrun 0.1.0"
```

## 6. Push to the AUR

```sh
git push origin master
```

The AUR only accepts the `master` branch. The package appears at
<https://aur.archlinux.org/packages/plainrun> within a minute.

Copy the pinned checksum back into the upstream `packaging/arch/PKGBUILD` and
commit it there too, so the two stay identical.

## 7. Updating for later releases

After a new version (e.g. v0.2.0) has been tagged and released on GitHub:

```sh
cd ~/aur/plainrun
git pull
# Edit PKGBUILD: pkgver=0.2.0, pkgrel=1 (or copy the upstream packaging/arch/PKGBUILD)
updpkgsums
makepkg --printsrcinfo > .SRCINFO
makepkg -Cfsi && namcap PKGBUILD *.pkg.tar.zst
git commit -am "Update to 0.2.0"
git push
```

For a packaging-only fix with the same upstream version (for example, a
missing dependency), increase `pkgrel` instead, regenerate `.SRCINFO`, commit
and push.

Nothing is published to the AUR automatically. These steps are always run by
hand by the maintainer.
