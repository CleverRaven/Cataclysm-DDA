find . -name "*json" -type f -exec python -m json.tool {} >/dev/null \;
