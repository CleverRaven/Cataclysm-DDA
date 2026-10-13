{
  description = ''
    Cataclysm: Dark Days Ahead, a post-apocalyptic survival roguelike.
    SDL3 tiles + ncurses builds packaged from the project's own Makefile,
    plus all community tilesets composed from source.
  '';

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    inputs@{ flake-parts, self, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      perSystem =
        { pkgs, ... }:
        let
          inherit (pkgs) lib;

          # VERSION for the binary banner; flake sources have no .git, so
          # the Makefile would otherwise fall back to the bare 0.J default.
          version = "0.J" + (if self ? rev then "+${self.shortRev}" else "+dirty");

          mkCdda =
            attrs:
            pkgs.callPackage ./nix/package.nix (
              attrs
              // {
                inherit version;
                src = self;
              }
            );

          tiles = mkCdda {
            tiles = true;
            sound = true;
          };
          curses = mkCdda {
            tiles = false;
            sound = false;
          };
          # Test binary with tiles + sound, same flags upstream CI uses.
          # Keeps the tileset and soundpack lists non-empty so game-state
          # init works inside tests.
          cata-test = mkCdda {
            tiles = true;
            sound = true;
            buildTests = true;
          };

          # json_formatter.cgi, built with the same make target the
          # tileset workflow uses. compose.py looks it up relative to CWD
          # (tools/format/json_formatter.cgi), so the tileset builder drops
          # it under its work root.
          jsonFormatter = pkgs.stdenv.mkDerivation {
            pname = "cdda-json-formatter";
            inherit version;
            src = self;
            # The Makefile's global LDFLAGS drag -lncursesw -lz into this
            # link, so those go on buildInputs and the make target runs as-is.
            buildInputs = with pkgs; [
              ncurses
              zlib
            ];
            buildPhase = ''
              runHook preBuild
              make tools/format/json_formatter.cgi -j$NIX_BUILD_CORES
              runHook postBuild
            '';
            installPhase = ''
              runHook preInstall
              install -Dm755 tools/format/json_formatter.cgi $out/bin/json_formatter.cgi
              runHook postInstall
            '';
            enableParallelBuilding = true;
            meta = {
              description = "CDDA canonical JSON formatter (tileset compose step)";
              license = lib.licenses.cc-by-sa-30;
              platforms = lib.platforms.unix;
            };
          };

          composePy = self + "/tools/gfx_tools/compose.py";

          # Sparse checkouts of just the gfx/<set> dirs, same as the
          # workflow's sparse-checkout of "gfx".
          tilesetsSrc = pkgs.fetchFromGitHub {
            owner = "I-am-Erk";
            repo = "CDDA-Tilesets";
            rev = "dfd8d69f388e6b9c2719b250845e51d8d30eb2cb";
            sha256 = "sha256-lx0q3CZTpfPr+LiQeIBtZahQJifpCjxN7AnVQn2EeZU=";
            sparseCheckout = [
              "gfx/Altica"
              "gfx/ASCII_Overmap"
              "gfx/BrownLikeBears"
              "gfx/Chibi_Ultica"
              "gfx/GiantDays"
              "gfx/HitButton_iso"
              "gfx/HollowMoon"
              # TODO: re-enable once upstream draws the missing sprites
              # (see NOTE at the commented-out matrix entry below).
              # "gfx/IsoMSX+"
              "gfx/Larwick_Overmap"
              "gfx/MShockXotto+"
              "gfx/MShockXotto+_Portraits"
              # TODO: re-enable with the matrix entry below once upstream
              # ships the file (see NOTE there).
              # "gfx/Mushroom-Dream"
              "gfx/NeoDays"
              "gfx/PenAndPaper"
              "gfx/Retrodays"
              "gfx/SurveyorsMap"
              "gfx/Ultica_iso"
              "gfx/UltimateCataclysm"
            ];
          };
          cuteclysmSrc = pkgs.fetchFromGitHub {
            owner = "pixel-32";
            repo = "CDDA-tileset";
            rev = "3f06254658f48f3c01e5a29994c971f2a6e5fc8a";
            sha256 = "sha256-ZAZ3uAuav2H7CTI6CLHnnQIFMxNHQJV9HpgUcFhp5bc=";
          };

          # Per-set table copied from .github/workflows/compose-tilesets.yml:
          # --use-all everywhere; --obsolete-fillers on Altica,
          # BrownLikeBears, HollowMoon and UltimateCataclysm. The result
          # is exactly the 17-set release matrix: IsoMSX+ and
          # Mushroom-Dream stay commented out with NOTEs (broken and
          # unloadable at the pinned rev, respectively).
          tilesetMatrix = [
            {
              attr = "tileset-altica";
              name = "Altica";
              dir = "gfx/Altica";
              args = [
                "--use-all"
                "--obsolete-fillers"
              ];
            }
            {
              attr = "tileset-ascii-overmap";
              name = "ASCII_Overmap";
              dir = "gfx/ASCII_Overmap";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-brownlikebears";
              name = "BrownLikeBears";
              dir = "gfx/BrownLikeBears";
              args = [
                "--use-all"
                "--obsolete-fillers"
              ];
            }
            {
              attr = "tileset-chibi-ultica";
              name = "Chibi_Ultica";
              dir = "gfx/Chibi_Ultica";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-hollowmoon";
              name = "HollowMoon";
              dir = "gfx/HollowMoon";
              args = [
                "--use-all"
                "--obsolete-fillers"
              ];
            }
            {
              attr = "tileset-larwick-overmap";
              name = "Larwick_Overmap";
              dir = "gfx/Larwick_Overmap";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-mshockxotto-plus";
              name = "MShockXotto+";
              dir = "gfx/MShockXotto+";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-mshockxotto-plus-portraits";
              name = "MShockXotto+_Portraits";
              dir = "gfx/MShockXotto+_Portraits";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-neodays";
              name = "NeoDays";
              dir = "gfx/NeoDays";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-retrodays";
              name = "Retrodays";
              dir = "gfx/Retrodays";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-giantdays";
              name = "GiantDays";
              dir = "gfx/GiantDays";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-smashbutton-iso";
              name = "SmashButton_iso";
              dir = "gfx/HitButton_iso";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-surveyorsmap";
              name = "SurveyorsMap";
              dir = "gfx/SurveyorsMap";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-ultimatecataclysm";
              name = "UltimateCataclysm";
              dir = "gfx/UltimateCataclysm";
              args = [
                "--use-all"
                "--obsolete-fillers"
              ];
            }
            {
              attr = "tileset-ultica-iso";
              name = "Ultica_iso";
              dir = "gfx/Ultica_iso";
              args = [ "--use-all" ];
            }
            {
              attr = "tileset-penandpaper";
              name = "PenAndPaper";
              dir = "gfx/PenAndPaper";
              args = [ "--use-all" ];
            }
            # NOTE: IsoMSX+ commented out. At the pinned rev its JSON
            # references seasonal sprites that were never drawn
            # (t_grass_summer/autumn, ...), so compose.py exits 1 on
            # missing-sprite errors and upstream releases can't carry it
            # either. Filed upstream as
            # https://github.com/I-am-Erk/CDDA-Tilesets/issues/2938.
            # Re-enable the entry + sparse path above once fixed.
            # {
            #   attr = "tileset-isomsx-plus";
            #   name = "IsoMSX+";
            #   dir = "gfx/IsoMSX+";
            #   args = [ "--use-all" ];
            # }
            # NOTE: Mushroom-Dream stays commented out. It composes fine
            # but ships no fallback.png, and the loader opens every
            # tiles-new file under an assert, so picking it breaks loading.
            # Confirmed in-game (debug error "Could not load image ...
            # fallback.png" from main_menu opening_screen). The stock
            # fallback sheet does not fit (16px cells on a 32x32 set).
            # TODO: upstream issue filed as
            # https://github.com/I-am-Erk/CDDA-Tilesets/issues/2939.
            # Re-enable the entry + sparse path above once fixed.
            # {
            #   attr = "tileset-mushroom-dream";
            #   name = "Mushroom-Dream";
            #   dir = "gfx/Mushroom-Dream";
            #   args = [ "--use-all" ];
            # }
            {
              attr = "tileset-cuteclysm";
              name = "Cuteclysm";
              dir = ".";
              args = [ "--use-all" ];
              src = cuteclysmSrc;
              tilesVersion = "3f06254";
            }
          ];

          mkTileset =
            {
              attr,
              name,
              dir,
              args,
              src ? tilesetsSrc,
              tilesVersion ? "dfd8d69",
            }:
            pkgs.callPackage ./nix/tileset.nix {
              inherit
                name
                src
                composePy
                jsonFormatter
                ;
              sourceDir = dir;
              composeArgs = args;
              installName = name;
              version = tilesVersion;
            };

          tilesets = lib.listToAttrs (
            map (entry: lib.nameValuePair entry.attr (mkTileset entry)) tilesetMatrix
          );

          # Same idea as a release bundle: the game with every composed
          # set baked into share/cataclysm-dda/gfx.
          tiles-full = tiles.override {
            extraTilesets = builtins.attrValues tilesets;
          };
        in
        {
          packages = {
            inherit
              tiles
              curses
              cata-test
              tiles-full
              ;
            json-formatter = jsonFormatter;
            default = tiles;
          }
          // tilesets;

          devShells.default = pkgs.mkShell {
            inputsFrom = [
              tiles
              curses
            ];

            packages = with pkgs; [
              ccache
              git
              nixfmt-rfc-style
              python3Packages.pyvips
              vips
            ];

            # The Makefile stamps git versions when git is available;
            # dummy drivers let cata_test run headless in the shell.
            env.GLSLANG = "${pkgs.glslang}/bin/glslangValidator";
            env.SDL_VIDEODRIVER = "dummy";
            env.SDL_AUDIODRIVER = "dummy";

            shellHook = ''
              echo 'CDDA dev shell. Try: nix build .#tiles && ./result/bin/cataclysm-tiles' 1>&2
            '';
          };

          checks = {
            # Smoke test: cata_test with the calendar suite. The test
            # binary always starts with an empty base path, so all resource
            # dirs are relative to cwd (see set_standard_filenames in
            # src/path_info.cpp): datadir is ./data, but gfxdir is ./gfx
            # (not ./data/gfx) and langdir is ./lang/mo. The symlinks below
            # rebuild the post-make repo layout upstream CI runs in. Data
            # comes from the tiles package: it holds everything the curses
            # package has plus gfx/, shaders/ and sound/. ./gfx has to be
            # there or build_tilesets_list() asserts and the check aborts.
            # (Soundpacks fall back to a built-in instead, so ./data/sound
            # is belt-and-suspenders.) Everything runs in a temp dir because
            # non-[nogame] tests boot a full game state (loads all JSON)
            # and write logs and saves under cwd.
            calendar-test = pkgs.runCommand "cdda-calendar-test" { } ''
              cd "$(mktemp -d)"
              ln -s ${tiles}/share/cataclysm-dda data
              ln -s ${tiles}/share/cataclysm-dda/gfx gfx
              mkdir -p lang
              ln -s ${tiles}/share/locale lang/mo
              export HOME="$PWD"
              export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
              ${cata-test}/bin/cata_test "[calendar]"
              touch $out
            '';

            # Every composed set must be structurally sound: parseable
            # tile_config.json with a non-empty tiles-new, plus tileset.txt
            # and at least one composited sheet.
            tilesets-validate =
              let
                allSets = builtins.attrValues tilesets;
              in
              pkgs.runCommand "cdda-tilesets-validate"
                {
                  nativeBuildInputs = [ pkgs.python3 ];
                }
                ''
                  fail=0
                  ${lib.concatMapStrings (t: ''
                    echo "== ${t.tilesetDir}"
                    python3 -c "
                    import json, pathlib, sys
                    t = pathlib.Path(sys.argv[1])
                    d = json.loads((t / 'tile_config.json').read_text())
                    sheets = d.get('tiles-new')
                    assert sheets, 'empty tiles-new'
                    missing = [s['file'] for s in sheets if not (t / s['file']).is_file()]
                    assert not missing, 'missing sheets: %s' % (missing,)
                    assert (t / 'tileset.txt').is_file(), 'missing tileset.txt'
                    print(' ', len(sheets), 'sheets, all present')
                    " '${t}' || fail=1
                  '') allSets}
                  test "$fail" -eq 0
                  touch $out
                '';

            # Boot a game state with a real composed pack in gfx. Proves a
            # composed tileset gets discovered without tripping assertions.
            tileset-discovery = pkgs.runCommand "cdda-tileset-discovery" { } ''
              cd "$(mktemp -d)"
              ln -s ${tiles}/share/cataclysm-dda data
              mkdir gfx
              ln -s ${tiles}/share/cataclysm-dda/gfx/* gfx/
              ln -s ${tilesets.tileset-retrodays} gfx/Retrodays
              mkdir -p lang
              ln -s ${tiles}/share/locale lang/mo
              export HOME="$PWD"
              export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
              ${cata-test}/bin/cata_test "[calendar]"
              touch $out
            '';

            # Unit tests for the updater with mocked git/nix. No network,
            # no builds. Mock programs get absolute bash shebangs when
            # created, so nothing needs patching; the scripts run
            # through bash directly.
            updater-tests =
              pkgs.runCommand "cdda-updater-tests"
                {
                  nativeBuildInputs = with pkgs; [
                    bash
                    coreutils
                    gawk
                    gnugrep
                    gnused
                  ];
                }
                ''
                  bash ${self + /nix/test-update-tilesets.sh} ${self + /nix/update-tilesets.sh}
                  touch $out
                '';
          };

          apps.update-tilesets =
            let
              updater = pkgs.writeShellApplication {
                name = "update-tilesets";
                runtimeInputs = with pkgs; [
                  coreutils
                  gawk
                  git
                  gnugrep
                  gnused
                  nix
                ];
                text = builtins.readFile ./nix/update-tilesets.sh;
              };
            in
            {
              type = "app";
              program = lib.getExe updater;
            };
        };
    };
}
