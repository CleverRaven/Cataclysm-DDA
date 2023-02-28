#/bin/bash

# This script downloads the SDL2 framework for macOS
set -u

SDL2_VERSION=2.26.3
SDL2_TTF_VERSION=2.20.2
SDL2_IMAGE_VERSION=2.6.3
SDL2_MIXER_VERSION=2.6.3
TARGET_PATH=~/Library/Frameworks

function download_sdl_framework {
  local project_id=$1
  local version=$2

  local github_id=${project_id%2}
  local url=https://github.com/libsdl-org/$github_id/releases/download/release-$version/$project_id-$version.dmg
  local output_file=$project_id-$version.dmg
  
  echo "Downloading $project_id"
  curl -L -o $output_file $url
  hdiutil attach $output_file
  cp -R /Volumes/$project_id/$project_id.framework $TARGET_PATH
  hdiutil detach /Volumes/$project_id
  rm $output_file
}

pushd $(mktemp -d)

download_sdl_framework SDL2 $SDL2_VERSION
download_sdl_framework SDL2_ttf $SDL2_TTF_VERSION
download_sdl_framework SDL2_image $SDL2_IMAGE_VERSION
download_sdl_framework SDL2_mixer $SDL2_MIXER_VERSION

popd
