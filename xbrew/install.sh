#!/bin/bash
# xbrew installer — Homebrew for x86_64 Macs.
#
# xbrew is a fork of Homebrew (https://brew.sh, BSD-2-Clause) that keeps
# Intel/AMD x86_64 macOS working after Homebrew stopped building x86 bottles
# in September 2026. It is the Homebrew code, pinned and lightly patched, with
# homebrew-core pinned to the last commit where x86_64 bottles were complete.
#
#   ./install.sh            install or repair
#   ./install.sh uninstall  remove xbrew (installed packages are left alone)
set -euo pipefail

# ---- pins -------------------------------------------------------------------
BREW_TAG="7.0.14"                                        # Homebrew/brew release
CORE_SHA="2881ed801408e20236ebd2d883ac7eaca3769485"      # homebrew-core, 2026-08-25
# -----------------------------------------------------------------------------

PREFIX="/usr/local"                       # must match Homebrew's Intel prefix: bottles are built for it
XBREW_HOME="${XBREW_HOME:-$HOME/.xbrew}"
REPO="$XBREW_HOME/Homebrew"
TAPS="$REPO/Library/Taps/homebrew"
BIN="$PREFIX/bin/xbrew"

say()  { printf '\033[1;34m==>\033[0m \033[1m%s\033[0m\n' "$*"; }
die()  { printf '\033[1;31mError:\033[0m %s\n' "$*" >&2; exit 1; }

if [[ "${1:-}" == "uninstall" ]]; then
  [[ -L "$BIN" ]] && rm "$BIN"
  rm -rf "$XBREW_HOME"
  say "xbrew removed. Packages in $PREFIX/Cellar were not touched."
  exit 0
fi

[[ "$(uname -s)" == "Darwin" ]] || die "xbrew is for macOS."
[[ "$(uname -m)" == "x86_64" ]] || die "xbrew is for x86_64 Macs. On Apple silicon, use Homebrew."
command -v git >/dev/null      || die "git not found. Run: xcode-select --install"
xcode-select -p >/dev/null 2>&1 || die "Command Line Tools not found. Run: xcode-select --install"
if [[ ! -d "$PREFIX/bin" || ! -w "$PREFIX/bin" ]]; then
  die "$PREFIX/bin is not writable. Run once:
  sudo mkdir -p $PREFIX/bin $PREFIX/Cellar $PREFIX/opt $PREFIX/etc $PREFIX/var/homebrew && sudo chown -R \"\$(whoami)\":admin $PREFIX/bin $PREFIX/Cellar $PREFIX/opt $PREFIX/etc $PREFIX/var"
fi

# ---- 1. the Homebrew code ---------------------------------------------------
if [[ ! -d "$REPO/.git" ]]; then
  say "Fetching Homebrew $BREW_TAG"
  mkdir -p "$XBREW_HOME"
  git clone --quiet --depth 1 --branch "$BREW_TAG" https://github.com/Homebrew/brew "$REPO"
fi

# ---- 2. x86 patches ---------------------------------------------------------
if ! grep -q "xbrew defaults" "$REPO/bin/brew"; then
  say "Applying x86 patches"

  # (a) Defaults: read formulae from the pinned taps instead of the live API
  #     (which no longer lists x86 bottles), never auto-update away from the
  #     pin, and route `xbrew update` to xbrew's own updater.
  /usr/bin/perl -0pi -e 's{\Aset -u\n|(?<=\n)set -u\n}{set -u\n\n# --- xbrew defaults ---\nexport HOMEBREW_NO_INSTALL_FROM_API=1\nexport HOMEBREW_NO_AUTO_UPDATE=1\nexport HOMEBREW_NO_ENV_HINTS=1\nexport HOMEBREW_NO_ANALYTICS=1\nexport HOMEBREW_NO_AUTOREMOVE=1\nif [ "\${1:-}" = "update" ]; then shift; exec "__XBREW_HOME__/update.sh" "\$@"; fi\n# --- end xbrew defaults ---\n}' "$REPO/bin/brew"
  /usr/bin/perl -pi -e "s{__XBREW_HOME__}{$XBREW_HOME}g" "$REPO/bin/brew"
  grep -q "xbrew defaults" "$REPO/bin/brew" || die "could not patch bin/brew"

  # (b) x86_64 is a supported platform here: drop the "we do not support
  #     Intel" finding (and the Tier 3 banner that follows from it).
  DIAG="$REPO/Library/Homebrew/extend/os/mac/diagnostic.rb"
  /usr/bin/perl -pi -e 's{elsif ::Hardware::CPU\.intel\?$}{elsif false \&\& ::Hardware::CPU.intel? # xbrew: x86_64 is supported}' "$DIAG"
  grep -q "xbrew: x86_64 is supported" "$DIAG" || die "could not patch diagnostic.rb"

  git -C "$REPO" -c user.name=xbrew -c user.email=xbrew@localhost commit --quiet -am "xbrew: x86_64 support patches"
  # Keep the version string honest: Homebrew derives it from the nearest tag.
  git -C "$REPO" tag -f "$BREW_TAG-xbrew" >/dev/null
fi

# ---- 3. the package snapshot ------------------------------------------------
mkdir -p "$TAPS"
CORE="$TAPS/homebrew-core"
if [[ "$(git -C "$CORE" rev-parse HEAD 2>/dev/null || true)" != "$CORE_SHA" ]]; then
  say "Fetching homebrew-core at ${CORE_SHA:0:12} (last full x86_64 bottle set)"
  mkdir -p "$CORE"
  git -C "$CORE" init --quiet -b snapshot 2>/dev/null || true
  git -C "$CORE" remote get-url origin >/dev/null 2>&1 || git -C "$CORE" remote add origin https://github.com/Homebrew/homebrew-core
  git -C "$CORE" fetch --quiet --depth 1 origin "$CORE_SHA"
  git -C "$CORE" checkout --quiet -B snapshot FETCH_HEAD
fi

CASK="$TAPS/homebrew-cask"
if [[ ! -d "$CASK/.git" ]]; then
  say "Fetching homebrew-cask (apps)"
  git clone --quiet --depth 1 https://github.com/Homebrew/homebrew-cask "$CASK"
fi

# ---- 4. `xbrew update` ------------------------------------------------------
cat > "$XBREW_HOME/update.sh" <<EOF
#!/bin/bash
# xbrew update: formulae stay on the pinned x86 snapshot; casks follow upstream.
set -euo pipefail
CORE="$CORE"; CASK="$CASK"
echo "==> Formulae: pinned to homebrew-core \$(git -C "\$CORE" rev-parse --short=12 HEAD) (\$(git -C "\$CORE" log -1 --format=%cs)) — last full x86_64 bottle set"
echo "==> Casks: updating"
git -C "\$CASK" fetch --quiet --depth 1 origin HEAD
git -C "\$CASK" reset --quiet --hard FETCH_HEAD
echo "Casks now at \$(git -C "\$CASK" log -1 --format='%h (%cs)')"
EOF
chmod +x "$XBREW_HOME/update.sh"

# ---- 5. the command ---------------------------------------------------------
# Homebrew derives its prefix from where the command lives, so a symlink in
# /usr/local/bin gives xbrew the standard Intel prefix that bottles expect.
ln -sfn "$REPO/bin/brew" "$BIN"

say "Warming up (first run downloads Homebrew's Ruby)"
"$BIN" --version

cat <<EOF

xbrew is installed. It takes the same commands as brew:

  xbrew install wget        xbrew search foo        xbrew list
  xbrew install --cask iterm2                       xbrew update

Code:     $REPO  (Homebrew $BREW_TAG + x86 patches)
Packages: homebrew-core @ ${CORE_SHA:0:12}, installed into $PREFIX
EOF
