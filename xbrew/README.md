# xbrew — Homebrew for x86_64 Macs

Homebrew stopped building x86_64 macOS bottles in late August 2026, so on an
Intel or AMD Mac `brew install` now compiles most things from source.

xbrew is a fork of Homebrew that keeps prebuilt x86 installs working. It is
the Homebrew code itself with the same commands and the same packages, with
two changes:

1. **Pinned package set.** homebrew-core is pinned to commit `2881ed801408`
   (25 Aug 2026), the last point where the x86_64 bottle set was complete:
   8,380 of 8,571 formulae have an x86 bottle (the rest are Linux-only or
   never had one). Homebrew's servers still host those bottles.
2. **x86_64 is a supported platform.** The "we do not support Intel" warning
   and Tier 3 banner are patched out, and formulae are read from the pinned
   tap instead of the live API (which no longer lists x86 bottles).

## Install

```bash
./install.sh
```

Needs the Xcode Command Line Tools and a writable `/usr/local/bin`. Takes
about 15 seconds. Remove with `./install.sh uninstall` (installed packages
are left alone).

## Use

Everything is `brew`, spelled `xbrew`:

```bash
xbrew install wget ffmpeg node
xbrew install --cask iterm2
xbrew search / info / list / uninstall / services / doctor ...
xbrew update        # refreshes casks; formulae stay on the pinned snapshot
```

## How it fits with Homebrew

- xbrew's code lives in `~/.xbrew/Homebrew` (Homebrew 7.0.14 + patches);
  `/usr/local/bin/xbrew` is a symlink to it.
- Packages go into the standard Intel prefix, `/usr/local`, because that is
  where x86 bottles are built to live. If stock Homebrew is installed, both
  tools see the same packages. Stock Homebrew itself is not modified.
- Casks (apps) track upstream homebrew-cask, since they are vendor binaries.

## Limits

- **Versions are frozen at 25 Aug 2026.** No newer releases and no security
  updates for formulae. For a newer version of something, stock
  `brew install <name>` still builds the current one from source.
- If you use both tools, `brew upgrade` will try to rebuild xbrew-installed
  packages from source to the latest version. Upgrade selectively.
- Bottles are downloaded from Homebrew's registry (ghcr.io). xbrew works as
  long as those old bottles stay hosted.
- A formula can only be installed from source if it has no x86 bottle in the
  snapshot (about 190, mostly Linux-only).

## Changing the pins

`BREW_TAG` and `CORE_SHA` are at the top of `install.sh`. Edit and re-run.

Homebrew is BSD-2-Clause licensed, © Homebrew contributors.
