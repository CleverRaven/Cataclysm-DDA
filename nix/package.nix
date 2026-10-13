# Build Cataclysm: Dark Days Ahead from this checkout with the project's
# own Makefile (see doc/c++/COMPILING.md). The Makefile stays the build
# system; Nix just hands it dependencies and install targets.
#
# Shaped after nixpkgs' pkgs/games/cataclysm-dda/common.nix, ported to this
# tree's SDL3-only tiles backend (SDL2 support was removed upstream on
# 2026-08-11; the nixpkgs derivation still links SDL2 and will not build
# against this source).
#
# No patches are required:
#   * PREFIX is baked in at compile time (-DPREFIX -DDATA_DIR_PREFIX ->
#     src/main.cpp init_base_path), so the binary reads data from
#     $out/share/cataclysm-dda and locales from $out/share/locale.
#   * translations.cpp already resolves locale_dir() via base_path() on
#     Linux (the locale-path patch nixpkgs carries is for a 2024 revision).
#   * Without .git in the flake source, the Makefile falls back to the
#     VERSION= value we pass, and skips -DGIT_VERSION.
{
  lib,
  stdenv,
  pkg-config,
  gettext,
  python3,
  glslang,
  ncurses,
  zlib,
  bzip2,
  sdl3,
  sdl3-image,
  sdl3-ttf,
  sdl3-mixer,
  freetype,
  # SDL3 graphical build; ncurses text build when false
  tiles ? true,
  # sound via SDL3_mixer; requires tiles (COMPILING.md: SOUND needs TILES)
  sound ? true,
  # debug build: no RELEASE=1, keep symbols
  debug ? false,
  # user files in $XDG_DATA_HOME instead of ~/.cataclysm-dda
  useXdgDir ? false,
  # build the cata_test Catch2 binary instead of the game
  buildTests ? false,
  # composed tileset derivations (nix/tileset.nix) baked into
  # share/cataclysm-dda/gfx/<tilesetDir>; release-bundle equivalent
  extraTilesets ? [ ],
  version,
  src,
}:

assert lib.assertMsg (tiles || !sound) "cataclysm-dda: sound requires tiles";

let
  inherit (lib)
    optionals
    optionalString
    concatStrings
    ;

  gameDeps =
    if tiles then
      [
        sdl3
        sdl3-image
        sdl3-ttf
        freetype
      ]
      ++ optionals sound [ sdl3-mixer ]
    else
      [ ncurses ];

  targetName = if tiles then "cataclysm-tiles" else "cataclysm";

  variantSuffix = concatStrings (optionals tiles [ "-tiles" ] ++ optionals sound [ "-sound" ]);
in
stdenv.mkDerivation (
  finalAttrs:
  {
    pname = "cataclysm-dda" + optionalString buildTests "-test" + variantSuffix;
    inherit version src;

    # python3: tools/build_shaders.py (tiles); glslang: glslangValidator for
    # GLSL -> SPIR-V; gettext: msgfmt for lang/*.po -> lang/*.mo.
    nativeBuildInputs = [
      pkg-config
      gettext
      python3
    ]
    ++ optionals tiles [ glslang ];

    buildInputs = [
      zlib
      bzip2
      gettext
    ]
    ++ gameDeps;

    strictDeps = true;

    postPatch = ''
      patchShebangs lang/
    '';

    # The Makefile invokes these by bare name; be explicit anyway.
    env.GLSLANG = lib.getExe' glslang "glslangValidator";

    makeFlags = [
      "PREFIX=${placeholder "out"}"
      "VERSION=${finalAttrs.version}"
      "TESTS=${if buildTests then "1" else "0"}"
      "RUNTESTS=0"
      "LOCALIZE=1"
      "LANGUAGES=all"
      (if useXdgDir then "USE_XDG_DIR=1" else "USE_HOME_DIR=1")
    ]
    ++ optionals (!debug) [ "RELEASE=1" ]
    ++ optionals tiles [ "TILES=1" ]
    ++ optionals sound [ "SOUND=1" ];

    # Default goal builds game + locales + shader stamps + zzip; for the test
    # package switch to the `tests` goal (TESTS=1 above wires it up). `prefix`
    # generates src/prefix.h from PREFIX. `all` pulls it in but `tests`
    # does not, so ask for it here.
    buildFlags = optionals buildTests [
      "prefix"
      "tests"
    ];

    postInstall =
      optionalString (tiles && !buildTests) ''
        substituteInPlace $out/share/applications/org.cataclysmdda.CataclysmDDA.desktop \
          --replace-fail "Exec=cataclysm-tiles" "Exec=$out/bin/cataclysm-tiles"
      ''
      + optionalString (extraTilesets != [ ]) ''
        # Merge contents into the target dir (mkdir -p + cp src/.), never
        # nest: make install already ships in-tree gfx/ (e.g.
        # gfx/Larwick_Overmap), and cp -R src dst/ into an existing dir
        # would create dst/<store-path>/ with a duplicate tileset.txt.
        # Merging with overwrite mirrors release.yml ("Fetch tileset
        # artifact into build": download-artifact with merge-multiple
        # into gfx/ on top of the game checkout).
        ${lib.concatMapStrings (t: ''
          mkdir -p $out/share/cataclysm-dda/gfx/${t.tilesetDir}
          cp -R --no-preserve=ownership ${t}/. $out/share/cataclysm-dda/gfx/${t.tilesetDir}/
        '') extraTilesets}
      '';

    dontStrip = debug;
    enableParallelBuilding = true;

    passthru = {
      isTiles = tiles;
      isSound = sound;
      inherit targetName;
    };

    meta = {
      description = "Free, post-apocalyptic, zombie-infested roguelike";
      longDescription = ''
        Cataclysm: Dark Days Ahead is a roguelike set in a post-apocalyptic
        world: scavenge, craft, fight zombies, mutants and rogue machines in a
        persistent procedurally generated world with per-body-part combat,
        vehicles, mutations, bionics, NPCs and mods.
      '';
      homepage = "https://cataclysmdda.org/";
      license = lib.licenses.cc-by-sa-30;
      mainProgram = if buildTests then "cata_test" else targetName;
      platforms = lib.platforms.linux;
    };
  }
  // lib.optionalAttrs buildTests {
    # The test build must not run the Makefile's install target (it would
    # compile the game binary too); just ship the cata_test executable.
    installPhase = ''
      runHook preInstall
      install -Dm755 tests/cata_test "$out/bin/cata_test"
      runHook postInstall
    '';
  }
)
