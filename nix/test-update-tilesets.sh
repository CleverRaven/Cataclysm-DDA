#!/usr/bin/env bash
# Tests for update-tilesets.sh with mocked git/nix. No network, no builds.
#
# Usage: test-update-tilesets.sh <updater-under-test>
#
# Each case builds a temp dir with mock git/nix on PATH and a fixture
# flake.nix, runs the updater, and asserts exit codes, file contents and
# which external commands ran. Cases: unchanged-skip, revision-update,
# sparse-preservation, malformed-diagnostics, fetch-failure,
# interrupted-repair, multi-hash ambiguity, unexpected-success.
set -u

UPDATER="$1"
PASS=0
FAIL=0
TMPDIRS=""

ok() {
  PASS=$((PASS + 1))
  echo "ok: $1"
}

bad() {
  FAIL=$((FAIL + 1))
  echo "NOT OK: $1"
  [ -n "${2-}" ] && printf '%s\n' "$2" | head -20
}

cleanup() {
  for d in $TMPDIRS; do rm -rf "$d"; done
}
trap cleanup EXIT

# 40-hex shas and valid SRI hashes for fixtures/mocks.
SHA_OLD_A="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
SHA_OLD_B="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
SHA_NEW_A="cccccccccccccccccccccccccccccccccccccccc"
SHA_NEW_B="dddddddddddddddddddddddddddddddddddddddd"
FAKE="sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA="
HASH_A="sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB="
HASH_B="sha256-BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB="
HASH_NEW="sha256-CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC="
HASH_NEW2="sha256-DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD="

# $1 erk_rev $2 erk_hash $3 cute_rev $4 cute_hash $5 outfile; $6 extra sparse entries
fixture_flake() {
  cat >"$5" <<EOF
{
  outputs = { ... }: {
    unrelated-top = 1;
    a = pkgs.fetchFromGitHub {
      owner = "I-am-Erk";
      repo = "CDDA-Tilesets";
      rev = "$1";
      sha256 = "$2";
      sparseCheckout = [ "gfx/Altica" $6 ];
    };
    b = pkgs.fetchFromGitHub {
      owner = "pixel-32";
      repo = "CDDA-tileset";
      rev = "$3";
      sha256 = "$4";
    };
    unrelated-bottom = [ 1 2 3 ];
  };
}
EOF
}

# Sets T, MOCKBIN; writes mock git/nix; caller exports MOCK_* first.
# Mock shebangs use an absolute bash path resolved here: valid both on a
# normal system and in the Nix sandbox (no /usr/bin/env there), with no
# patchShebangs dependency.
new_case() {
  T="$(mktemp -d)"
  TMPDIRS="$TMPDIRS $T"
  MOCKBIN="$T/bin"
  mkdir -p "$MOCKBIN" "$T/wt"
  BASH_BIN="$(command -v bash)"
  printf '#!%s\n' "$BASH_BIN" >"$MOCKBIN/git"
  cat >>"$MOCKBIN/git" <<'EOF'
if [ "$1" = "rev-parse" ]; then printf '%s\n' "$MOCK_ROOT"; exit 0; fi
if [ "$1" = "ls-remote" ]; then
  case "$2" in
    *CDDA-Tilesets) h="$MOCK_HEAD_ERK" ;;
    *CDDA-tileset) h="$MOCK_HEAD_CUTE" ;;
    *) echo "mock git: unexpected url $2" >&2; exit 99 ;;
  esac
  printf '%s\tHEAD\n' "$h"; exit 0
fi
echo "mock git: unexpected args: $*" >&2; exit 99
EOF
  printf '#!%s\n' "$BASH_BIN" >"$MOCKBIN/nix"
  cat >>"$MOCKBIN/nix" <<'EOF'
echo "$*" >> "$MOCK_NIX_LOG"
probe="$2"
case "$probe" in
  *retrodays*) mode="$MOCK_NIX_ERK"; got="$MOCK_GOT_ERK" ;;
  *cuteclysm*) mode="$MOCK_NIX_CUTE"; got="$MOCK_GOT_CUTE" ;;
  *) echo "mock nix: unexpected probe $probe" >&2; exit 99 ;;
esac
case "$mode" in
  got) printf 'error: hash mismatch:\n specified: fake\n got:    %s\n' "$got" >&2; exit 1 ;;
  multi) printf 'got:    %s\ngot:    %s\n' "$MOCK_GOT_ERK" "$MOCK_GOT2" >&2; exit 1 ;;
  garbage) echo "some unrelated noise" >&2; exit 1 ;;
  netfail) echo "error: unable to download: network unreachable" >&2; exit 1 ;;
  ok) exit 0 ;;
  *) echo "mock nix: bad mode $mode" >&2; exit 99 ;;
esac
EOF
  chmod +x "$MOCKBIN/git" "$MOCKBIN/nix"
  export MOCK_ROOT="$T/wt" MOCK_NIX_LOG="$T/nix.log" PATH="$MOCKBIN:$PATH"
  : >"$MOCK_NIX_LOG"
}

run_updater() {
  bash "$UPDATER" "$@" >"$T/out.txt" 2>&1
  echo "$?"
}

nix_calls() {
  grep -c . "$MOCK_NIX_LOG" || true
}

# 1. unchanged: pins current and valid -> no writes, no fetches.
t_unchanged() {
  new_case
  export MOCK_HEAD_ERK="$SHA_OLD_A" MOCK_HEAD_CUTE="$SHA_OLD_B"
  fixture_flake "$SHA_OLD_A" "$HASH_A" "$SHA_OLD_B" "$HASH_B" "$T/wt/flake.nix" '"gfx/MShockXotto+"'
  cp "$T/wt/flake.nix" "$T/before.nix"
  rc="$(run_updater)"
  [ "$rc" -eq 0 ] || {
    bad "unchanged: exit $rc"
    return
  }
  cmp -s "$T/before.nix" "$T/wt/flake.nix" || {
    bad "unchanged: file modified"
    return
  }
  [ "$(nix_calls)" -eq 0 ] || {
    bad "unchanged: nix invoked"
    return
  }
  grep -q "plan: skip" "$T/out.txt" || {
    bad "unchanged: no skip message"
    return
  }
  ok "unchanged-skip"
}

# 2. update: erk stale -> rev+hash rewritten; cute already current.
t_update() {
  new_case
  export MOCK_HEAD_ERK="$SHA_NEW_A" MOCK_HEAD_CUTE="$SHA_OLD_B"
  export MOCK_NIX_ERK=got MOCK_NIX_CUTE=got MOCK_GOT_ERK="$HASH_NEW" MOCK_GOT_CUTE="$HASH_B"
  fixture_flake "$SHA_OLD_A" "$HASH_A" "$SHA_OLD_B" "$HASH_B" "$T/wt/flake.nix" '"gfx/Altica"'
  fixture_flake "$SHA_NEW_A" "$HASH_NEW" "$SHA_OLD_B" "$HASH_B" "$T/expected.nix" '"gfx/Altica"'
  rc="$(run_updater)"
  [ "$rc" -eq 0 ] || {
    bad "update: exit $rc"
    cat "$T/out.txt"
    return
  }
  cmp -s "$T/expected.nix" "$T/wt/flake.nix" || {
    bad "update: content mismatch"
    diff "$T/expected.nix" "$T/wt/flake.nix"
    return
  }
  ok "revision-update"
}

# 3. sparse preservation: tricky entries + unrelated content survive byte-identical.
t_sparse() {
  new_case
  export MOCK_HEAD_ERK="$SHA_NEW_A" MOCK_HEAD_CUTE="$SHA_OLD_B"
  export MOCK_NIX_ERK=got MOCK_NIX_CUTE=got MOCK_GOT_ERK="$HASH_NEW" MOCK_GOT_CUTE="$HASH_B"
  fixture_flake "$SHA_OLD_A" "$HASH_A" "$SHA_OLD_B" "$HASH_B" "$T/wt/flake.nix" '"gfx/MShockXotto+" "gfx/HitButton_iso"'
  fixture_flake "$SHA_NEW_A" "$HASH_NEW" "$SHA_OLD_B" "$HASH_B" "$T/expected.nix" '"gfx/MShockXotto+" "gfx/HitButton_iso"'
  rc="$(run_updater)"
  [ "$rc" -eq 0 ] || {
    bad "sparse: exit $rc"
    return
  }
  cmp -s "$T/expected.nix" "$T/wt/flake.nix" || {
    bad "sparse: content mismatch"
    diff "$T/expected.nix" "$T/wt/flake.nix"
    return
  }
  ok "sparse-preservation"
}

# 4. malformed diagnostics: garbage probe output -> clear failure, file untouched.
t_malformed() {
  new_case
  export MOCK_HEAD_ERK="$SHA_NEW_A" MOCK_HEAD_CUTE="$SHA_OLD_B"
  export MOCK_NIX_ERK=garbage MOCK_NIX_CUTE=got MOCK_GOT_CUTE="$HASH_B"
  fixture_flake "$SHA_OLD_A" "$HASH_A" "$SHA_OLD_B" "$HASH_B" "$T/wt/flake.nix" '"gfx/Altica"'
  cp "$T/wt/flake.nix" "$T/before.nix"
  rc="$(run_updater)"
  [ "$rc" -ne 0 ] || {
    bad "malformed: exit 0"
    return
  }
  cmp -s "$T/before.nix" "$T/wt/flake.nix" || {
    bad "malformed: file modified"
    return
  }
  grep -q "expected exactly 1 hash" "$T/out.txt" || {
    bad "malformed: no clear error"
    cat "$T/out.txt"
    return
  }
  ok "malformed-diagnostics"
}

# 5. fetch failure: network error, no hash -> clear failure, file untouched.
t_fetchfail() {
  new_case
  export MOCK_HEAD_ERK="$SHA_NEW_A" MOCK_HEAD_CUTE="$SHA_OLD_B"
  export MOCK_NIX_ERK=netfail MOCK_NIX_CUTE=got MOCK_GOT_CUTE="$HASH_B"
  fixture_flake "$SHA_OLD_A" "$HASH_A" "$SHA_OLD_B" "$HASH_B" "$T/wt/flake.nix" '"gfx/Altica"'
  cp "$T/wt/flake.nix" "$T/before.nix"
  rc="$(run_updater)"
  [ "$rc" -ne 0 ] || {
    bad "fetchfail: exit 0"
    return
  }
  cmp -s "$T/before.nix" "$T/wt/flake.nix" || {
    bad "fetchfail: file modified"
    return
  }
  grep -q "network unreachable" "$T/out.txt" || {
    bad "fetchfail: no probe context"
    return
  }
  ok "fetch-failure"
}

# 6. interrupted repair: rev==HEAD but placeholder hash -> hash repaired, rev kept.
t_repair() {
  new_case
  export MOCK_HEAD_ERK="$SHA_OLD_A" MOCK_HEAD_CUTE="$SHA_OLD_B"
  export MOCK_NIX_ERK=got MOCK_NIX_CUTE=got MOCK_GOT_ERK="$HASH_NEW" MOCK_GOT_CUTE="$HASH_B"
  fixture_flake "$SHA_OLD_A" "$FAKE" "$SHA_OLD_B" "$HASH_B" "$T/wt/flake.nix" '"gfx/Altica"'
  fixture_flake "$SHA_OLD_A" "$HASH_NEW" "$SHA_OLD_B" "$HASH_B" "$T/expected.nix" '"gfx/Altica"'
  rc="$(run_updater)"
  [ "$rc" -eq 0 ] || {
    bad "repair: exit $rc"
    cat "$T/out.txt"
    return
  }
  cmp -s "$T/expected.nix" "$T/wt/flake.nix" || {
    bad "repair: content mismatch"
    diff "$T/expected.nix" "$T/wt/flake.nix"
    return
  }
  grep -q "plan: repair" "$T/out.txt" || {
    bad "repair: not detected as repair"
    return
  }
  ok "interrupted-repair"
}

# 7. ambiguous multi-hash output -> failure, file untouched.
t_multihash() {
  new_case
  export MOCK_HEAD_ERK="$SHA_NEW_A" MOCK_HEAD_CUTE="$SHA_OLD_B"
  export MOCK_NIX_ERK=multi MOCK_NIX_CUTE=got MOCK_GOT_ERK="$HASH_NEW" MOCK_GOT2="$HASH_NEW2" MOCK_GOT_CUTE="$HASH_B"
  fixture_flake "$SHA_OLD_A" "$HASH_A" "$SHA_OLD_B" "$HASH_B" "$T/wt/flake.nix" '"gfx/Altica"'
  cp "$T/wt/flake.nix" "$T/before.nix"
  rc="$(run_updater)"
  [ "$rc" -ne 0 ] || {
    bad "multihash: exit 0"
    return
  }
  cmp -s "$T/before.nix" "$T/wt/flake.nix" || {
    bad "multihash: file modified"
    return
  }
  grep -q "found 2" "$T/out.txt" || {
    bad "multihash: no clear error"
    return
  }
  ok "multi-hash-ambiguity"
}

# 8. atomic rollback: erk probe fails while cute has a valid pending
# update -> failure, and cute's update must roll back too.
t_atomicity() {
  new_case
  export MOCK_HEAD_ERK="$SHA_NEW_A" MOCK_HEAD_CUTE="$SHA_NEW_B"
  export MOCK_NIX_ERK=garbage MOCK_NIX_CUTE=got MOCK_GOT_ERK="$HASH_NEW" MOCK_GOT_CUTE="$HASH_NEW2"
  fixture_flake "$SHA_OLD_A" "$HASH_A" "$SHA_OLD_B" "$HASH_B" "$T/wt/flake.nix" '"gfx/Altica"'
  cp "$T/wt/flake.nix" "$T/before.nix"
  rc="$(run_updater)"
  [ "$rc" -ne 0 ] || {
    bad "atomicity: exit 0"
    return
  }
  cmp -s "$T/before.nix" "$T/wt/flake.nix" || {
    bad "atomicity: partial write survived"
    diff "$T/before.nix" "$T/wt/flake.nix"
    return
  }
  ok "atomic-rollback"
}

# 9. unexpected probe success -> failure, file untouched.
t_unexpected_ok() {
  new_case
  export MOCK_HEAD_ERK="$SHA_NEW_A" MOCK_HEAD_CUTE="$SHA_OLD_B"
  export MOCK_NIX_ERK=ok MOCK_NIX_CUTE=got MOCK_GOT_CUTE="$HASH_B"
  fixture_flake "$SHA_OLD_A" "$HASH_A" "$SHA_OLD_B" "$HASH_B" "$T/wt/flake.nix" '"gfx/Altica"'
  cp "$T/wt/flake.nix" "$T/before.nix"
  rc="$(run_updater)"
  [ "$rc" -ne 0 ] || {
    bad "unexpected-ok: exit 0"
    return
  }
  cmp -s "$T/before.nix" "$T/wt/flake.nix" || {
    bad "unexpected-ok: file modified"
    return
  }
  grep -q "unexpectedly succeeded" "$T/out.txt" || {
    bad "unexpected-ok: no clear error"
    return
  }
  ok "unexpected-success"
}

t_unchanged
t_update
t_sparse
t_malformed
t_fetchfail
t_repair
t_multihash
t_atomicity
t_unexpected_ok

echo "pass=$PASS fail=$FAIL"
[ "$FAIL" -eq 0 ]
