#!/usr/bin/env bash
# Bump the tileset source pins in flake.nix to upstream HEAD.
#
# Usage: update-tilesets.sh [--check]
#   (no args)  resolve HEAD revs, prefetch verified hashes, rewrite pins
#   --check    print current vs HEAD revs without changing anything
#
# Cost model (evaluation vs fetching vs compilation):
# * Evaluation only reads expressions (e.g. nix flake check --no-build).
# * Fetching downloads sources into the store. The probe builds below do
#   this and nothing more.
# * Compilation turns sources into outputs (game C++ builds). This script
#   never gets that far: the probe packages pin a throwaway hash, so they
#   die fetching inputs, and Nix prints the verified `got:` hash for the
#   exact sparse tree in the unchanged fetchFromGitHub { sparseCheckout }
#   setup. Nothing here compiles any project code.
#
# Safety: resolve all HEADs and work out the whole plan before writing
# anything. Hashing has to stage rev+placeholder in the working file
# (probes read pins out of flake.nix), so take a backup first, put it
# back on any failure, and re-read everything at the end to confirm.
# Placeholder goes in first so a killed run always lands somewhere
# recoverable: rev==HEAD with a leftover placeholder gets repaired,
# not skipped.
set -euo pipefail

FAKE_HASH="sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA="

die() {
  printf 'ERROR: %s\n' "$*" >&2
  exit 1
}

cd "$(git rev-parse --show-toplevel)"
FLAKE="flake.nix"
CHECK_ONLY=0
if [ "${1-}" = "--check" ]; then
  CHECK_ONLY=1
fi

# owner, repo, and the probe package whose fetch proves the hash.
SOURCES="I-am-Erk CDDA-Tilesets .#tileset-retrodays
pixel-32 CDDA-tileset .#tileset-cuteclysm"

is_sri_hash() {
  [[ "$1" =~ ^sha256-[A-Za-z0-9+/]{43}=$ ]]
}

current_rev() {
  grep -A2 "repo = \"$1\";" "$FLAKE" \
    | grep -m1 -o 'rev = "[0-9a-f]\+"' \
    | cut -d'"' -f2
}

current_hash() {
  grep -A3 "repo = \"$1\";" "$FLAKE" \
    | grep -m1 -o 'sha256 = "sha256-[A-Za-z0-9+/=]\+"' \
    | cut -d'"' -f2
}

resolve_head() {
  git ls-remote "https://github.com/$1/$2" HEAD | cut -f1
}

set_rev() {
  sed -i "/repo = \"$1\";/,/sha256 = / s/rev = \"[0-9a-f]*\"/rev = \"$2\"/" "$FLAKE"
}

set_hash() {
  sed -i "/repo = \"$1\";/,/sha256 = / s|sha256 = \"sha256-[A-Za-z0-9+/=]*\"|sha256 = \"$2\"|" "$FLAKE"
}

# Print the single verified hash for a probe build's fetch, or fail with
# the probe output as context. Never guesses: unexpected success, zero or
# multiple hashes, and malformed hashes are all hard errors.
probe_hash() {
  local probe="$1" out matches n hash
  if out="$(nix build "$probe" 2>&1)"; then
    printf 'ERROR: probe build %s unexpectedly succeeded; refusing to guess a hash\n' "$probe" >&2
    printf 'probe output:\n%s\n' "$out" >&2
    return 1
  fi
  matches="$(printf '%s\n' "$out" | grep -o 'got:[[:space:]]*sha256-[A-Za-z0-9+/=]\+' || true)"
  n="$(printf '%s' "$matches" | grep -c . || true)"
  if [ "$n" -ne 1 ]; then
    printf 'ERROR: expected exactly 1 hash in probe output for %s, found %s\n' "$probe" "$n" >&2
    printf 'probe output:\n%s\n' "$out" >&2
    return 1
  fi
  hash="$(printf '%s\n' "$matches" | awk '{ print $2; }')"
  if ! is_sri_hash "$hash"; then
    printf 'ERROR: malformed hash %q from probe %s\n' "$hash" "$probe" >&2
    return 1
  fi
  printf '%s\n' "$hash"
}

# Phase 1: resolve every HEAD and compute the plan. No writes, no fetches.
p_owner=()
p_repo=()
p_probe=()
p_head=()
p_action=() # update | repair | skip
while read -r owner repo probe; do
  if [ -z "${owner-}" ]; then
    continue
  fi
  echo "== $owner/$repo"
  head_rev="$(resolve_head "$owner" "$repo")" \
    || die "git ls-remote failed for $owner/$repo"
  [[ "$head_rev" =~ ^[0-9a-f]{40}$ ]] \
    || die "ls-remote returned non-SHA for $owner/$repo: $head_rev"
  rev="$(current_rev "$repo")" || die "cannot read current rev for $repo in $FLAKE"
  [[ "$rev" =~ ^[0-9a-f]{40}$ ]] || die "unparseable current rev for $repo in $FLAKE: $rev"
  hash="$(current_hash "$repo")" || die "cannot read current hash for $repo in $FLAKE"
  echo "current: $rev"
  echo "head:    $head_rev"
  action="update"
  if [ "$head_rev" = "$rev" ]; then
    # Skip only internally consistent pins; a leftover placeholder from
    # an interrupted run must be repaired, not skipped.
    if is_sri_hash "$hash" && [ "$hash" != "$FAKE_HASH" ]; then
      action="skip"
    else
      action="repair"
    fi
  fi
  echo "plan: $action"
  p_owner+=("$owner")
  p_repo+=("$repo")
  p_probe+=("$probe")
  p_head+=("$head_rev")
  p_action+=("$action")
done <<<"$SOURCES"

if [ "$CHECK_ONLY" -eq 1 ]; then
  echo "check only; flake.nix untouched."
  exit 0
fi

n_src="${#p_owner[@]}"
all_skip=1
for ((i = 0; i < n_src; i++)); do
  if [ "${p_action[$i]}" != "skip" ]; then
    all_skip=0
    break
  fi
done
if [ "$all_skip" -eq 1 ]; then
  echo "all sources current; flake.nix untouched."
  exit 0
fi

# Phase 2: apply. Placeholder-hash-first ordering keeps every abort state
# self-healing (see header); the backup still guards the whole phase and
# a verification pass confirms every planned pin afterwards.
backup="$(mktemp)"
cp "$FLAKE" "$backup"
apply_ok=1
for ((i = 0; i < n_src; i++)); do
  if [ "${p_action[$i]}" = "skip" ]; then
    continue
  fi
  if [ "$apply_ok" -ne 1 ]; then
    break # fail fast: restore below reverts everything staged so far
  fi
  repo="${p_repo[$i]}"
  probe="${p_probe[$i]}"
  set_hash "$repo" "$FAKE_HASH" || apply_ok=0
  if [ "${p_action[$i]}" = "update" ]; then
    set_rev "$repo" "${p_head[$i]}" || apply_ok=0
  fi
  if [ "$apply_ok" -eq 1 ]; then
    if got="$(probe_hash "$probe")"; then
      set_hash "$repo" "$got" || apply_ok=0
      echo "pinned $repo $got"
    else
      apply_ok=0
    fi
  fi
done

if [ "$apply_ok" -ne 1 ]; then
  cp "$backup" "$FLAKE"
  rm -f "$backup"
  die "update failed; restored $FLAKE from backup"
fi

# Post-write verification: every planned pin reads back as decided and no
# placeholder survives anywhere in the file.
for ((i = 0; i < n_src; i++)); do
  if [ "${p_action[$i]}" = "skip" ]; then
    continue
  fi
  repo="${p_repo[$i]}"
  now_rev="$(current_rev "$repo")" || {
    cp "$backup" "$FLAKE"
    rm -f "$backup"
    die "post-write readback failed for $repo; restored from backup"
  }
  now_hash="$(current_hash "$repo")" || {
    cp "$backup" "$FLAKE"
    rm -f "$backup"
    die "post-write readback failed for $repo; restored from backup"
  }
  if [ "$now_rev" != "${p_head[$i]}" ] || ! is_sri_hash "$now_hash" || [ "$now_hash" = "$FAKE_HASH" ]; then
    cp "$backup" "$FLAKE"
    rm -f "$backup"
    die "post-write verification failed for $repo; restored from backup"
  fi
done
if grep -qF "$FAKE_HASH" "$FLAKE"; then
  cp "$backup" "$FLAKE"
  rm -f "$backup"
  die "placeholder hash survives in $FLAKE; restored from backup"
fi
rm -f "$backup"

echo "done; run nix flake check --no-build, review the diff, and commit."
