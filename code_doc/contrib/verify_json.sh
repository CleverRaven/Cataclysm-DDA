#!/bin/sh
# run ./cataclysm with --jsonverify, spamming spacebar for debugmsg and stripping ncurses control sequences for
# TERM = screen, xterm, vt100, linux, xterm-color. Exit code > 0 means error
export TERM=xterm
unset LC_CMAP
unset LANG
unset TERMCAP
out=$(yes ' ' |./cataclysm --jsonverify 2>&1)
rc=$?
echo -n "$out" |
  sed -r -e "s/.\x08//g" \
    -e "s/\x1B(\[\??([0-9]{1,4}(;[0-9]{1,2})?)?[a-zA-Z]|[\(\)><=][A-Z]?)//g" \
    -e "s/[\x00-\x09\x0B-\x1F]//g" \
    -e "/^  Press spacebar\.\.\.$/ d"
exit $rc
