find tmp -name *json -a -type f -print0 | xargs -0 -L 1 -P 10 python -m json.tool >/dev/null
