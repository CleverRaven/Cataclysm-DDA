find . -name *json -type f -exec sh -c 'python -m json.tool "$0" >/dev/null || (echo JSON parsing failed on "$0" && kill $PPID)' \{\} \;
