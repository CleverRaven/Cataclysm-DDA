{
  description = "Cataclysm-DDA development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = { self, nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      commonInputs = with pkgs; [
          ccache
          cmake
          freetype
          gcc
          gettext
          glslang
          gnumake
          libx11
          ncurses
          pkg-config
          python3
      ];
    in {
      devShells.${system} = {
        default = pkgs.mkShell {
          buildInputs = commonInputs ++ (with pkgs; [
            sdl3
            sdl3-image
            sdl3-mixer
            sdl3-ttf
          ]);

          shellHook = ''
            echo "Cataclysm-DDA build environment ready (SDL3)"
          '';
        };

        sdl2 = pkgs.mkShell {
          buildInputs = commonInputs ++ (with pkgs; [
            SDL2
            SDL2_image
            SDL2_mixer
            SDL2_ttf
          ]);

          shellHook = ''
            echo "Cataclysm-DDA build environment ready (SDL2)"
          '';
        };
      };
    };
}
