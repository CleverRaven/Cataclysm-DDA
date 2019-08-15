#!/bin/bash

set -eu

filename="${!#}"
printf "%s\n" "$filename"
clang-tidy "$@" || printf "%s\n" "$filename" >&3
