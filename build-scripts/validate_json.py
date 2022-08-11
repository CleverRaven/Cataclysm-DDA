#!/usr/bin/env python3

# Validate that all JSON files are syntactically correct;
# does NOT check indentation styles.

import glob
import json
import sys


def main():
    json_files = glob.glob('**/**.json', recursive=True)
    errors = 0
    total = 0
    for file_path in json_files:
        if not file_path.startswith("android/app/src/main/assets"):
            total += 1
            try:
                with open(file_path, encoding="utf-8") as fp:
                    _ = json.load(fp)
            except OSError as e:
                print("Error opening {}: {}".format(file_path, e))
                errors += 1
            except json.JSONDecodeError as e:
                print("Error parsing {}: {}".format(file_path, e))
                errors += 1
    if errors == 0:
        print("All {} JSON files healthy.".format(total))
        return 0
    else:
        print("Found {} erroneous files among {} JSON files in the repository."
              .format(errors, total))
        return min(errors, 255)


if __name__ == '__main__':
    sys.exit(main())
