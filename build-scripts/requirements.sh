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
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
    $travis_retry sudo apt-get --yes install parallel
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
  $travis_retry sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 84C7C89FC632241A6999ED0A580873F586B72ED9
  sudo add-apt-repository 'deb [arch=amd64] https://mirror.mxe.cc/repos/apt xenial main'
  sudo dpkg --add-architecture i386
  # We need to treat apt-get update warnings as errors for which the exit code
  # is not sufficient.  The following workaround inspired by
  # https://unix.stackexchange.com/questions/175146/apt-get-update-exit-status/
  exec {fd}>&2
  $travis_retry bash -o pipefail -c \
      "sudo apt-get update 2>&1 | tee /dev/fd/$fd | ( ! grep -q -e '^Err:' -e '^[WE]:' )"
  exec {fd}>&-

  MXE2_TARGET=$(echo "$MXE_TARGET" | sed 's/_/-/g')
  export MXE_DIR=/usr/lib/mxe/usr/bin
  $travis_retry sudo apt-get --yes install \
      mxe-${MXE2_TARGET}-gcc \
      mxe-${MXE2_TARGET}-gettext \
      mxe-${MXE2_TARGET}-glib \
      mxe-${MXE2_TARGET}-sdl2 \
      mxe-${MXE2_TARGET}-sdl2-ttf \
      mxe-${MXE2_TARGET}-sdl2-image \
      mxe-${MXE2_TARGET}-sdl2-mixer \
      wine \
      wine32
  export PLATFORM='i686-w64-mingw32.static'
  export CROSS_COMPILATION="${MXE_DIR}/${PLATFORM}-"
  # Need to overwrite CXX to make the Makefile $CROSS logic work right.
  export CXX="$COMPILER"
  export CCACHE=1

  set +e
  retry=0
  until [[ "$retry" -ge 5 ]]; do
    curl -L -o libbacktrace-i686-w64-mingw32.tar.gz https://github.com/Qrox/libbacktrace/releases/download/2020-01-03/libbacktrace-i686-w64-mingw32.tar.gz && shasum -a 256 -c ./build-scripts/libbacktrace-i686-w64-mingw32-sha256 && break
    retry=$((retry+1))
    rm -f libbacktrace-i686-w64-mingw32.tar.gz
    sleep 10
  done
  if [[ "$retry" -ge 5 ]]; then
    echo "Error downloading or checksum failed for libbacktrace-i686-w64-mingw32.tar.gz"
    exit 1
  fi
  set -e
  sudo tar -xzf libbacktrace-i686-w64-mingw32.tar.gz --exclude=LICENSE -C ${MXE_DIR}/../${PLATFORM}
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
            printf "%s='%s'\n" "$v" "${!v}" >> "$GITHUB_ENV"
        fi
    done
fi

set +x
