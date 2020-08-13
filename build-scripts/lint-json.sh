find . -name "*json" -type f -exec python3 -m json.tool {} >/dev/null \;
