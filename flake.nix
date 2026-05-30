{
  description = "Cataclysm-DDA development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = { self, nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in {
      devShells.${system}.default = pkgs.mkShell {
        buildInputs = with pkgs; [
          ccache
          cmake
          freetype
          gcc
          gettext
          gnumake
          libx11
          ncurses
          pkg-config
          SDL2
          SDL2_image
          SDL2_mixer
          SDL2_ttf
          sdl3-image
          sdl3-mixer
          sdl3-ttf
        ];

        shellHook = ''
          echo "Cataclysm-DDA build environment ready"
          echo "  Curses:  make -j$(nproc) RELEASE=1"
          echo "  SDL2:    make -j$(nproc) TILES=1 SOUND=1 RELEASE=1"
          echo "  SDL3:    make -j$(nproc) TILES=1 SOUND=1 SDL3=1 RELEASE=1"
        '';
      };
    };
}
