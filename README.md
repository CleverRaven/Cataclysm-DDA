# Cataclysm: Dark Days Ahead

Cataclysm: Dark Days Ahead is a turn-based survival game set in a post-apocalyptic world. While some have described it as a "zombie game", there is far more to Cataclysm than that. Struggle to survive in a harsh, persistent, procedurally generated world. Scavenge the remnants of a dead civilization for food, equipment, or, if you are lucky, a vehicle with a full tank of gas to get you the hell out of Dodge. Fight to defeat or escape from a wide variety of powerful monstrosities, from zombies to giant insects to killer robots and things far stranger and deadlier, and against the others like yourself, who want what you have...

<p align="center">
    <img src="./data/screenshots/ultica-showcase-sep-2021.png" alt="Tileset: Ultica">
</p>

## Downloads

**Releases** - [Stable](https://cataclysmdda.org/releases/) | [Experimental](https://cataclysmdda.org/experimental/)

**Source** - The source can be downloaded as a [.zip archive](https://github.com/CleverRaven/Cataclysm-DDA/archive/master.zip), or cloned from our [GitHub repo](https://github.com/CleverRaven/Cataclysm-DDA/).

<a href="https://repology.org/project/cataclysm-dda/versions">
    <img src="https://repology.org/badge/vertical-allrepos/cataclysm-dda.svg" alt="Packaging Status" align="right">
</a>

[![General build matrix](https://github.com/CleverRaven/Cataclysm-DDA/actions/workflows/matrix.yml/badge.svg)](https://github.com/CleverRaven/Cataclysm-DDA/actions/workflows/matrix.yml)
[![Coverage Status](https://coveralls.io/repos/github/CleverRaven/Cataclysm-DDA/badge.svg?branch=master)](https://coveralls.io/github/CleverRaven/Cataclysm-DDA?branch=master)
[![Open Source Helpers](https://www.codetriage.com/cleverraven/cataclysm-dda/badges/users.svg)](https://www.codetriage.com/cleverraven/cataclysm-dda)
[![Commit Activity](https://img.shields.io/github/commit-activity/m/CleverRaven/Cataclysm-DDA)](https://github.com/CleverRaven/Cataclysm-DDA/graphs/contributors)
[![Lines of Code](https://tokei.rs/b1/github/CleverRaven/Cataclysm-DDA?category=code)](https://github.com/XAMPPRocky/tokei)
[![TODOs](https://badgen.net/https/api.tickgit.com/badgen/github.com/CleverRaven/Cataclysm-DDA)](https://www.tickgit.com/browse?repo=github.com/CleverRaven/Cataclysm-DDA)

### Packaging status

#### Arch Linux

Ncurses and tiles versions are available in the [community repo](https://www.archlinux.org/packages/?q=cataclysm-dda).

`sudo pacman -S cataclysm-dda`

#### Fedora

Ncurses and tiles versions are available in the [official repos](https://src.fedoraproject.org/rpms/cataclysm-dda).

`sudo dnf install cataclysm-dda`

#### Debian / Ubuntu

Ncurses and tiles versions are available in the [official repos](https://tracker.debian.org/pkg/cataclysm-dda).

`sudo apt install cataclysm-dda-curses cataclysm-dda-sdl`

## Compile

Please read [COMPILING.md](doc/COMPILING/COMPILING.md) - it covers general information and more specific recipes for Linux, OS X, Windows and BSD. See [COMPILER_SUPPORT.md](doc/COMPILING/COMPILER_SUPPORT.md) for details on which compilers we support. And you can always dig for more information in [doc/](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/doc).

We also have the following build guides:
* Building on Windows with `MSYS2` at [COMPILING-MSYS.md](doc/COMPILING/COMPILING-MSYS.md)
* Building on Windows with `vcpkg` at [COMPILING-VS-VCPKG.md](doc/COMPILING/COMPILING-VS-VCPKG.md)
* Building with `cmake` at [COMPILING-CMAKE.md](doc/COMPILING/COMPILING-CMAKE.md)  (*unofficial guide*)

## Contribute

Cataclysm: Dark Days Ahead is the result of contributions from over 1000 volunteers under the Creative Commons Attribution ShareAlike 3.0 license. The code and content of the game is free to use, modify, and redistribute for any purpose whatsoever. See https://creativecommons.org/licenses/by-sa/3.0/ for details.
Some code distributed with the project is not part of the project and is released under different software licenses; the files covered by different software licenses have their own license notices.

Please see [CONTRIBUTING.md](doc/CONTRIBUTING.md) for details.

Special thanks to the contributors, including but not limited to, people below:
<a href="https://github.com/cleverraven/cataclysm-dda/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=cleverraven/cataclysm-dda" />
</a>

Made with [contrib.rocks](https://contrib.rocks).

## Community

Forums:
https://discourse.cataclysmdda.org

GitHub repo:
https://github.com/CleverRaven/Cataclysm-DDA

IRC:
`#CataclysmDDA` on [Libera Chat](https://libera.chat), https://web.libera.chat/#CataclysmDDA

Official Discord:
https://discord.gg/jFEc7Yp

## Frequently Asked Questions

#### Is there a tutorial?

Yes, you can find the tutorial in the **Special** menu at the main menu (be aware that due to many code changes the tutorial may not function). You can also access documentation in-game via the `?` key.

#### How can I change the key bindings?

Press the `?` key, followed by the `1` key to see the full list of key commands. Press the `+` key to add a key binding, select which action with the corresponding letter key `a-w`, and then the key you wish to assign to that action.

#### How can I start a new world?

**World** on the main menu will generate a fresh world for you. Select **Create World**.

#### I've found a bug. What should I do?

Please submit an issue on [our GitHub page](https://github.com/CleverRaven/Cataclysm-DDA/issues/) using [bug report template](https://github.com/CleverRaven/Cataclysm-DDA/issues/new?template=bug_report.md). If you're not able to, send an email to `kevin.granade@gmail.com`.

#### I would like to make a suggestion. What should I do?

Please submit an issue on [our GitHub page](https://github.com/CleverRaven/Cataclysm-DDA/issues/) using [feature request template](https://github.com/CleverRaven/Cataclysm-DDA/issues/new?template=feature_request.md).
