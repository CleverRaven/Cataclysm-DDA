#!/bin/bash

set -e

if [ -n "${CODE_COVERAGE}" ]; then
  travis_retry pip install --user pyyaml cpp-coveralls;
  export CXXFLAGS=--coverage;
  export LDFLAGS=--coverage;
fi

# Influenced by https://github.com/zer0main/battleship/blob/master/build/windows/requirements.sh
if [ -n "${MXE_TARGET}" ]; then
  echo "deb http://pkg.mxe.cc/repos/apt/debian wheezy main" \
    | sudo tee /etc/apt/sources.list.d/mxeapt.list
  travis_retry sudo apt-key adv --keyserver x-hkp://keyserver.ubuntu.com:80 \
    --recv-keys D43A795B73B16ABE9643FE1AFD8FFF16DB45C6AB
  # We need to treat apt-get update warnings as errors for which the exit code
  # is not sufficient.  The following workaround inspired by
  # https://unix.stackexchange.com/questions/175146/apt-get-update-exit-status/
  exec {fd}>&2
  travis_retry bash -o pipefail -c \
      "sudo apt-get update 2>&1 | tee /dev/fd/$fd | ( ! grep -q -e '^Err:' -e '^[WE]:' )"
  exec {fd}>&-

  MXE2_TARGET=$(echo "$MXE_TARGET" | sed 's/_/-/g')
  export MXE_DIR=/usr/lib/mxe/usr/bin
  travis_retry sudo apt-get --yes install mxe-${MXE2_TARGET}-gcc mxe-${MXE2_TARGET}-gettext mxe-${MXE2_TARGET}-glib mxe-${MXE2_TARGET}-sdl2 mxe-${MXE2_TARGET}-sdl2-ttf mxe-${MXE2_TARGET}-sdl2-image mxe-${MXE2_TARGET}-sdl2-mixer
  export PLATFORM='i686-w64-mingw32.static'
  export CROSS_COMPILATION='${MXE_DIR}/${PLATFORM}-'
  # Need to overwrite CXX to make the Makefile $CROSS logic work right.
  export CXX="$COMPILER"
  export CCACHE=1
fi

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  brew update
  brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer gettext ncurses lua coreutils
  brew link --force gettext ncurses
fi
