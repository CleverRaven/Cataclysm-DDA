# Nix build

This flake packages Cataclysm: DDA from this checkout by driving the project's own `Makefile` (see `doc/c++/COMPILING.md`). Nix supplies the toolchain and dependencies; the Makefile remains the build system.

## Build & run

```sh
nix build .#tiles        # SDL3 tiles + sound (default)
nix run .#tiles          # ...and run it
nix build .#curses       # ncurses text build
nix run .#curses
./result/bin/cataclysm-tiles --help
```

User files (config, saves) live in `~/.cataclysm-dda` by default; the derivation exposes `useXdgDir = true` for XDG placement.

## Tests

```sh
nix build .#cata-test    # Catch2 test binary (tiles+sound flavor, like upstream CI)
nix flake check          # builds everything + runs the [calendar] smoke suite
```

The smoke check runs `cata_test "[calendar]"` headless: the game data is symlinked as `./data` next to the binary (the layout upstream CI uses) and the run happens in a temp dir, since tests write logs/saves relative to the cwd. To run the full suite after building:

```sh
cd "$(mktemp -d)" && ln -s ../result/share/cataclysm-dda data   # adjust path
# or from the source tree (data/ is already there):
../result/bin/cata_test ~[slow]
```

## How it works

- `nix/package.nix`: one `stdenv.mkDerivation` covering all variants (`tiles`, `sound`, `buildTests` flags). It mirrors nixpkgs' `pkgs/games/cataclysm-dda` shape, ported to this tree's SDL3-only tiles backend, with no source patches:
  - `PREFIX=$out` is baked into the binary (`-DPREFIX -DDATA_DIR_PREFIX`), so data resolves to `$out/share/cataclysm-dda` and locales to `$out/share/locale`.
  - `LANGUAGES=all LOCALIZE=1` compiles every translation via `msgfmt`.
  - Tiles builds compile GLSL shaders to SPIR-V with `glslangValidator` (`tools/build_shaders.py`); macOS/Windows formats are not produced.
- `flake.nix`: flake-parts wrapper exposing `packages.{tiles,curses,cata-test}`, a `devShells.default` with the full dependency set, and the `checks.calendar-test` smoke test.

## Tuning

```sh
nix build .#packages.x86_64-linux.curses.overrideAttrs (_: { dontStrip = false; })
```

or import `nix/package.nix` directly via `pkgs.callPackage` with the flags you want (`debug`, `useXdgDir`, `tiles`, `sound`, `buildTests`, `extraTilesets`).

## Tilesets

All community tilesets are composed from source with `tools/gfx_tools/compose.py`, same as `.github/workflows/compose-tilesets.yml`: same per-set flags, `--format-json --feedback CONCISE --loglevel INFO`, same strictness (compose errors fail the build). Sources are lean sparse checkouts, not a whole-repo mirror.

```sh
nix build .#tileset-retrodays     # one of 17: see flake.nix tilesetMatrix
nix build .#tiles-full            # game with every set baked into
                                  # share/cataclysm-dda/gfx (release equivalent)
nix build .#checks.x86_64-linux.tilesets-validate   # structural check of all sets
```

Use a set standalone by linking it into the user gfx dir. The game
looks in user gfx first, same as installing by hand:

```sh
ln -s ./result ~/.cataclysm-dda/gfx/Retrodays
```

### How it works

- `nix/tileset.nix`: generic builder. Runs compose under `python3 + pyvips`, then assembles `$out` (`tile_config.json` + composited sheets + `tileset.txt`/`fallback.png`/`layering.json` moved alongside, like the workflow). Per-set flags come from the `tilesetMatrix` table in `flake.nix`.
- `packages.json-formatter`: `json_formatter.cgi` built with the exact make target the workflow uses; staged where compose's CWD-relative lookup finds it, so output bytes match upstream releases.
- `checks.tilesets-validate`: every set must parse with non-empty `tiles-new`, and every referenced sheet file must exist. (This gate caught a real unloadable set, see below.)
- `checks.tileset-discovery`: game-state init with a composed pack in `gfx/`, proving discovery without tripping assertions.

### Sets that need work

- **IsoMSX+**: broken at the pinned rev. Its JSON names seasonal sprites nobody ever drew, so compose exits 1. Commented out with a NOTE; filing upstream is tracked work.
- **Mushroom-Dream**: composes fine but ships no `fallback.png`, and the loader opens every `tiles-new` file under an assert. Trialed in-game: fails to load as predicted. Commented out with a NOTE; upstream issue still TODO.

### Updating pins

Revs + hashes live in `flake.nix` (`tilesetsSrc`, `cuteclysmSrc`). Bump
them with the updater app (pinned-by-default, fresh-on-demand):

```sh
nix run .#update-tilesets -- --check   # compare current vs HEAD, no changes
nix run .#update-tilesets              # bump stale pins, verifying hashes
```

The updater reads HEAD with `git ls-remote`, puts the new rev in with
a throwaway hash, and takes the verified `got:` hash back out of a
probe build. The probe dies fetching, so updating never compiles
anything. To put it plainly: evaluation reads expressions, fetching
downloads sources, compilation turns sources into outputs, and the
updater stops after fetching. Sources already at HEAD get skipped.
Then run `nix flake check --no-build`, look over the diff, and commit.

How it stays safe: it resolves every HEAD and works out the whole plan before writing anything. Writes go through a backup (put back on any failure) plus a re-read at the end to confirm. Probe output has to hold exactly one good hash or the run stops and shows you the output. A leftover placeholder hash from a killed run gets repaired, not skipped. `nurl` stays out on purpose: it hashes whole repos and these pins hash sparse subtrees, so its hashes would fail every build.

`nix/test-update-tilesets.sh` tests the updater with mocked git/nix (skip, update, sparse preservation, bad diagnostics, fetch failures, interrupted runs, ambiguous output, surprise success) and runs as `checks.updater-tests` with no network or builds.

Bump when tilesets go stale: new game content without matching sprites falls back to ASCII/unknown glyphs in-game.
