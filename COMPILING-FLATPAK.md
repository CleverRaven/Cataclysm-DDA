# COMPILING GUIDE FOR LINUX (USING FLATPAK)

## PREREQUISITES

- Install `flatpak` and `flatpak-builder` from your distribution's package manager.
- Install `freedesktop-platform` & `freedesktop-sdk` using `flatpak install flathub org.freedesktop.Platform//18.08 org.freedesktop.Sdk//18.08`

## BUILD

- Clone the source code and run `flatpak-builder --repo=repo --ccache --force-clean build-dir org.Cataclysm.DDA.json` or `flatpak-builder --repo=repo --ccache --force-clean build-dir org.Cataclysm.DDA.Tiles.json` to build the ncurses or tiles version of the application.

## INSTALLATION

Run `flatpak --user install test-repo org.Cataclysm.DDA` (or) `flatpak --user install test-repo org.Cataclysm.DDA.Tiles` to install the game.
Note: If you are installing the game for first time you will have to set up local repo. Run this `flatpak --user remote-add --no-gpg-verify test-repo repo`

## STARTING THE GAME

`flatpak run org.Cataclysm.DDA` (or) `flatpak run org.Cataclysm.DDA.Tiles`
**or**
Launch the Game from Applications -> Games -> Cataclysm: Dark Days Ahead (or) Cataclysm: Dark Days Ahead - Tiles.
**Note** : For the ncurses version, terminal will just show a blank screen on launch. Just press any button to start the game. 

## PERMISSIONS

- **Ncurses version**
  - Home Directory
- **Tiles Version**
  - Home Directory
  - Wayland
  - Fallback-X11 - (incase Wayland doesn't work)
  - IPC - (Required for X11)
  - PulseAudio
  - DRI
