import sys, pathlib
for i, arg in enumerate(sys.argv):
    if arg == "--stamp" and i + 1 < len(sys.argv):
        p = pathlib.Path(sys.argv[i+1])
        p.parent.mkdir(parents=True, exist_ok=True)
        p.touch()
sys.exit(0)
