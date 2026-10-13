# Turn a tileset source tree into a game-ready tileset with
# tools/gfx_tools/compose.py. Same flags as
# .github/workflows/compose-tilesets.yml, including --format-json
# --feedback CONCISE --loglevel INFO.
#
# compose.py turns symbolic sprite names ("fg": "t_grass") into numbers
# and stitches the loose PNGs into sheet images, writing out
# tile_config.json + sheets. Any ERROR it logs (missing sprite,
# duplicate id, wrong size) fails the build, same as upstream CI.
# Single-level function (like nix/package.nix) so callers use
# pkgs.callPackage with all non-nixpkgs arguments passed explicitly.
{
  lib,
  stdenv,
  python3,
  vips,
  # Matrix/display name, e.g. "UltimateCataclysm" (used for $out layout docs)
  name,
  # Directory name the set installs as under gfx/, e.g. "UltimateCataclysm".
  # Matches the release layout (workflow DEST gfx/<matrix name>).
  installName ? name,
  # Source tree containing the set (sparse checkout); the set lives at
  # <src>/<sourceDir> with tile_info.json + tileset.txt + pngs_* dirs.
  src,
  sourceDir,
  # Per-set compose flags from the upstream matrix, e.g. [ "--use-all" ].
  composeArgs ? [ "--use-all" ],
  # tools/gfx_tools/compose.py from the game repo.
  composePy,
  # json_formatter.cgi from the game repo (packages.json-formatter).
  # compose.py looks for it under CWD, so the build drops it in
  # ./tools/format/json_formatter.cgi and runs from there, like the
  # workflow does from the repo root.
  jsonFormatter,
  version,
}:

stdenv.mkDerivation {
  pname = "cdda-tileset-${lib.toLower (builtins.replaceStrings [ "+" "_" ] [ "-plus" "-" ] name)}";
  inherit version src;

  nativeBuildInputs = [
    (python3.withPackages (ps: [ ps.pyvips ]))
    vips
  ];

  buildPhase = ''
    runHook preBuild
    mkdir -p $NIX_BUILD_TOP/work/tools/format
    cp ${composePy} $NIX_BUILD_TOP/work/compose.py
    install -m755 ${jsonFormatter}/bin/json_formatter.cgi $NIX_BUILD_TOP/work/tools/format/json_formatter.cgi
    cd $NIX_BUILD_TOP/work
    python ./compose.py \
      ${lib.escapeShellArgs composeArgs} \
      --format-json --feedback CONCISE --loglevel INFO \
      "$src/${sourceDir}" ./composed
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p $out
    cp -r $NIX_BUILD_TOP/work/composed/. $out/
    # The workflow moves these alongside the composed output afterwards.
    for f in tileset.txt fallback.png layering.json; do
      if [ -f "$src/${sourceDir}/$f" ]; then
        cp "$src/${sourceDir}/$f" $out/
      fi
    done
    runHook postInstall
  '';

  passthru = {
    # Directory name for installation under gfx/.
    tilesetDir = installName;
  };

  meta = {
    description = "Cataclysm: DDA tileset ${name}, composed from source";
    homepage = "https://github.com/I-am-Erk/CDDA-Tilesets";
    license = lib.licenses.cc-by-sa-30;
    platforms = lib.platforms.all;
  };
}
