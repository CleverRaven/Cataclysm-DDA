#!/bin/sh
# run ./cataclysm with --jsonverify, spamming spacebar for debugmsg and stripping ncurses control sequences for
# TERM = screen, xterm, vt100, linux, xterm-color. Exit code > 0 means error
e=$'\033'
export TERM=xterm
out=$(yes ' ' |./cataclysm --jsonverify 2>&1)
rc=$?
echo -n "$out" |
  tr -d '\r' |
  sed -r "s/$e(\[([0-9]{1,2}(;[0-9]{1,2})?)?[m|K|d|H|J|B|@|r]|\[\??[0-9]+[hl]|[\(\)=<>][B0]?|[78])//g" |
  grep -v '^  Press spacebar\.\.\.$'
exit $rc
