# pkg-manager

A package manager for x86_64 Macs (Intel, and AMD hackintoshes), modelled on
Homebrew.

Homebrew stopped building x86_64 macOS binaries in late August 2026, so on
these machines `brew install` now compiles most packages from source. This
project is for people who are still on x86 hardware and want installs that
take seconds, not hours.

## What's here

| Path | What it is | State |
| --- | --- | --- |
| `xbrew/` | A Homebrew fork with x86 support restored | Working |
| `library.cpp`, `main.cpp` | A from-scratch package manager in C++ | Early prototype (search only) |

## xbrew

`xbrew` is Homebrew itself (same commands, same packages) with two changes:

- **Pinned package set.** homebrew-core is pinned to the commit from
  25 Aug 2026, the last point where the x86_64 binary set was complete:
  8,380 of 8,571 formulae have a prebuilt x86 binary.
- **x86_64 treated as supported.** The "we do not support Intel" warnings are
  patched out, and packages are read from the pinned set instead of
  Homebrew's live index, which no longer lists x86 binaries.

Install:

```bash
./xbrew/install.sh
```

This needs the Xcode Command Line Tools and a writable `/usr/local/bin`.
Then use it exactly like `brew`:

```bash
xbrew install wget ffmpeg node
xbrew install --cask iterm2
xbrew update
```

Things to know:

- Formula versions are frozen at 25 Aug 2026, so there are no newer releases
  or security updates. Casks (apps) still follow upstream.
- Packages install into `/usr/local`, the same place as stock Homebrew, so
  the two tools share installed packages. Stock Homebrew is not modified.
- The binaries are downloaded from Homebrew's registry and work as long as
  those old builds stay hosted.

Details, limits and how to change the pinned versions are in
[xbrew/README.md](xbrew/README.md).

## C++ prototype

`library.cpp` and `main.cpp` are the start of a standalone package manager
written in C++20 with libcurl. So far it has one command, which searches
GitHub for C++ repositories matching a name. It does not install anything yet.

```bash
cmake -S . -B build && cmake --build build
./build/pkg search wget
```

## Requirements

- macOS on x86_64
- Xcode Command Line Tools (`xcode-select --install`)

## License

xbrew is built on Homebrew, which is BSD-2-Clause licensed,
© Homebrew contributors.
