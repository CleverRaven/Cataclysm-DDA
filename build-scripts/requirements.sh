#!/bin/bash

set -e

function just_json
{
    for filename in $(./build-scripts/files_changed || echo UNKNOWN)
    do
        if [[ ! "$filename" =~ \.(json|md)$ ]]
        then
            echo "$filename is not json or markdown, triggering full build."
            return 1
        fi
    done
    echo "Only json / markdown files changed, skipping full build."
    return 0
}

# Enable GitHub actions problem matchers
# (See https://github.com/actions/toolkit/blob/master/docs/problem-matchers.md)
echo "::add-matcher::build-scripts/problem-matchers/catch2.json"
echo "::add-matcher::build-scripts/problem-matchers/debugmsg.json"

if which travis_retry &>/dev/null
then
    travis_retry=travis_retry
else
    travis_retry=
fi

if [[ "$TRAVIS_EVENT_TYPE" == "pull_request" ]]; then
    if just_json; then
        export JUST_JSON=true
        export CODE_COVERAGE=""
    fi
fi

set -x
if [[ "$LIBBACKTRACE" == "1" ]]; then
    git clone https://github.com/ianlancetaylor/libbacktrace.git
    (
        cd libbacktrace
        git checkout 4d2dd0b172f2c9192f83ba93425f868f2a13c553
        ./configure
        make -j$(nproc)
        sudo make install
    )
fi

if [ -n "${CODE_COVERAGE}" ]; then
  $travis_retry pip install --user wheel --upgrade
  $travis_retry pip install --user pyyaml cpp-coveralls
  export CXXFLAGS="$CXXFLAGS --coverage"
  export LDFLAGS="$LDFLAGS --coverage"
fi

if [ -n "$CATA_CLANG_TIDY" ]; then
    $travis_retry pip install --user wheel --upgrade
    $travis_retry pip install --user compiledb lit
fi

# Influenced by https://github.com/zer0main/battleship/blob/master/build/windows/requirements.sh
if [ -n "${MXE_TARGET}" ]; then
  sudo apt update
  $travis_retry sudo apt-get --yes install wine64

  set +e
  retry=0
  until [[ "$retry" -ge 5 ]]; do
    curl -L -o mxe-x86_64.tar.xz https://github.com/BrettDong/MXE-GCC/releases/download/mxe-sdl-2-0-20/mxe-x86_64.tar.xz && curl -L -o mxe-x86_64.tar.xz.sha256 https://github.com/BrettDong/MXE-GCC/releases/download/mxe-sdl-2-0-20/mxe-x86_64.tar.xz.sha256 && shasum -a 256 -c ./mxe-x86_64.tar.xz.sha256 && break
    retry=$((retry+1))
    rm -f mxe-x86_64.tar.xz mxe-x86_64.tar.xz.sha256
    sleep 10
  done
  if [[ "$retry" -ge 5 ]]; then
    echo "Error downloading or checksum failed for MXE x86_64"
    exit 1
  fi
  set -e
  sudo tar xJf mxe-x86_64.tar.xz -C /opt

  export MXE_DIR=/opt/mxe
  export CROSS_COMPILATION="${MXE_DIR}/usr/bin/${MXE_TARGET}-"
  # Need to overwrite CXX to make the Makefile $CROSS logic work right.
  export CXX="$COMPILER"
  export CCACHE=1

  set +e
  retry=0
  until [[ "$retry" -ge 5 ]]; do
    curl -L -o SDL2-devel-2.26.2-mingw.tar.gz https://github.com/libsdl-org/SDL/releases/download/release-2.26.2/SDL2-devel-2.26.2-mingw.tar.gz && shasum -a 256 -c ./build-scripts/SDL2-devel-2.26.2-mingw.tar.gz.sha256 && break
    retry=$((retry+1))
    rm -f SDL2-devel-2.26.2-mingw.tar.gz
    sleep 10
  done
  if [[ "$retry" -ge 5 ]]; then
    echo "Error downloading or checksum failed for SDL2-devel-2.26.2-mingw.tar.gz"
    exit 1
  fi
  set -e
  sudo tar -xzf SDL2-devel-2.26.2-mingw.tar.gz -C ${MXE_DIR}/usr/${MXE_TARGET} --strip-components=2 SDL2-2.26.2/x86_64-w64-mingw32

  set +e
  retry=0
  until [[ "$retry" -ge 5 ]]; do
    curl -L -o libbacktrace-x86_64-w64-mingw32.tar.gz https://github.com/Qrox/libbacktrace/releases/download/2020-01-03/libbacktrace-x86_64-w64-mingw32.tar.gz && shasum -a 256 -c ./build-scripts/libbacktrace-x86_64-w64-mingw32-sha256 && break
    retry=$((retry+1))
    rm -f libbacktrace-x86_64-w64-mingw32.tar.gz
    sleep 10
  done
  if [[ "$retry" -ge 5 ]]; then
    echo "Error downloading or checksum failed for libbacktrace-x86_64-w64-mingw32.tar.gz"
    exit 1
  fi
  set -e
  sudo tar -xzf libbacktrace-x86_64-w64-mingw32.tar.gz --exclude=LICENSE -C ${MXE_DIR}/usr/${MXE_TARGET}
fi

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  HOMEBREW_NO_AUTO_UPDATE=yes HOMEBREW_NO_INSTALL_CLEANUP=yes brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer gettext ncurses ccache parallel
fi

if [[ "$NATIVE" == "android" ]]; then
  yes | sdkmanager "ndk-bundle"
fi

if [ -n "$WINE" ]
then
    # The build script will try to run things under wine in parallel, and I
    # think there are race conditions that can cause that to break.  So, run
    # something benign under wine in advance to trigger it to configure all the
    # one-time init stuff
    wine hostname
fi

# On GitHub actions environment variables are not saved between steps by
# default, so we need to explicitly save the ones that we care about
if [ -n "$GITHUB_ENV" ]
then
    for v in CROSS_COMPILATION CXX
    do
        if [ -n "${!v}" ]
        then
            printf "%s=%s\n" "$v" "${!v}" >> "$GITHUB_ENV"
        fi
    done
fi

set +x
