#!/usr/bin/env python3

import glob
import os
import sys


class IncludesParser:
    def __init__(self):
        self.includes_to_sources = {}

    def parse_includes_file(
            self,
            includes_file,
            source_file_path,
            relative_root,
    ):
        includes = includes_file.read().splitlines()
        for include in includes:
            if len(include) == 0 or include[0] != '.':
                # Out of actual output
                # gcc additionally prints an informational section like:
                # Multiple include guards may be useful for:
                break

            # Format is one or more '.' characters, followed by a space,
            # followed by the header
            include_actual = include[include.index(' ') + 1:]
            if include_actual[0] == '/':
                # system header, don't care
                continue

            include_actual = os.path.normpath(
                os.path.join(relative_root, include_actual))
            self.includes_to_sources.setdefault(
                include_actual,
                set(),
            ).add(source_file_path)

        # A source file also 'includes' itself
        self.includes_to_sources.setdefault(
            source_file_path,
            set(),
        ).add(source_file_path)

    # Parses includes files from a given folder
    # Maps include file path to a source file path by converting
    # folder_path/*.inc -> file_path_prefix/*.cpp
    def parse_includes_files_from(
            self,
            folder_path,
            source_path_prefix,
            include_path_prefix,
    ):
        # Load includes files list.
        folder_len = len(folder_path) + 1
        includes_files = glob.glob(
            os.path.join(folder_path, "**/*.inc"),
            recursive=True,
        )
        if len(includes_files) == 0:
            print(
                f"No includes files found in {folder_path}, " +
                "did you run `make includes`?",
                file=sys.stderr,
            )
            return False

        for includes_file_path in includes_files:
            source_file = includes_file_path[folder_len:-3] + "cpp"
            # Generally folder_path is <something>/obj,
            # but results might include `tiles`
            if source_file.startswith("tiles/"):
                source_file = source_file[6:]

            source_file = os.path.join(source_path_prefix, source_file)
            with open(includes_file_path, 'r') as includes_file:
                self.parse_includes_file(
                    includes_file,
                    source_file,
                    include_path_prefix,
                )

        return True

    def get_files_affected_by(self, changed_file):
        return self.includes_to_sources.get(changed_file, [])


def main(changed_files_file):
    parser = IncludesParser()

    # Load includes files list.
    if not parser.parse_includes_files_from("obj", "src", "."):
        return 1

    # Also for tests
    if not parser.parse_includes_files_from("tests/obj", "tests", "tests"):
        return 1

    changed_files = []
    lintable_files = set()
    with open(changed_files_file, 'r') as f:
        changed_files = f.read().splitlines()
        for changed_file in changed_files:
            for affected_file in parser.get_files_affected_by(changed_file):
                lintable_files.add(affected_file)

    for lintable_file in lintable_files:
        print(lintable_file)

    return 0


if __name__ == '__main__':
    if len(sys.argv) <= 1:
        print(
            "First argument must be a path to a list of changed files",
            file=sys.stderr,
        )
        sys.exit(1)
    sys.exit(main(sys.argv[1]))
