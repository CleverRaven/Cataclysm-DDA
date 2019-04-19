time find . -name *json -type f -exec python -m json.tool {} >/dev/null \;
time find . -name *json -a -type f -print0 | xargs -0 -L 1 -P 2 python -m json.tool >/dev/null
time find . -name *json -a -type f -print0 | xargs -0 -L 1 -P 4 python -m json.tool >/dev/null
time find . -name *json -a -type f -print0 | xargs -0 -L 1 -P 8 python -m json.tool >/dev/null
time find . -name *json -a -type f -print0 | xargs -0 -L 1 -P 12 python -m json.tool >/dev/null
time find . -name *json -a -type f -print0 | xargs -0 -L 1 -P 16 python -m json.tool >/dev/null
