# This file is intended to be run on github CI, not locally
# It only exists because I found it way too hard to present nice
# log output *while* maintaining the correct exit status in bash.
#
# Arguably this is 3x as much code as a bash equivalent, but I
# appeal to maintanability.
#
# For local development on linux, use the following command line
# (assuming both the iwyu source root and its build dir are in the PATH):
# python iwyu_tool.py \
#     $(find src/ tests/ -maxdepth 1 -name '*.cpp' \
#           | grep -v -f tools/iwyu/bad_files.txt) \
#     -p build --jobs 4 -- \
#     -Xiwyu "--mapping_file=${PWD}/tools/iwyu/cata.imp" -Xiwyu --cxx17ns  \
#     -Xiwyu --comment_style=long -Xiwyu --max_line_length=1000

import argparse
import logging
import os
import subprocess
import sys

from pathlib import Path

# hardcoded paths that could probably be passed via command line
CHANGED_FILES_INDEX = "files_changed"
GET_AFFECTED_FILES_SCRIPT = "build-scripts/get_affected_files.py"
BLACKLIST_PATH = "tools/iwyu/bad_files.txt"
MARKER_FORCE_GLOBAL_RUN = "MARKER_CHECK_ALL"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--iwyu_tool_path", default="iwyu_tool.py")
    args = parser.parse_args()

    print("::group::Determining files to analyze")

    # files directly changed in this PR
    changed_files = get_changed_files()
    print("changed files:")
    print_long_list(changed_files)
    # files transitively impacted by the direct change above
    # (and also the directly changed files themselves)
    affected_files = get_affected_files(changed_files)
    print("affected files:")
    print_long_list(affected_files)
    # files that we feed to IWYU. This excludes files blacklisted for
    # whatever reason
    files_to_analyze = filter_analyzable_files(affected_files)
    print("files to analyze:")
    print_long_list(files_to_analyze)
    print("::endgroup::")

    if not files_to_analyze:
        print("Nothing to analyze!")
        sys.exit(0)

    # Run IWYU with the files provided. Forward its exit code.
    status = run_iwyu_on(args.iwyu_tool_path, files_to_analyze)
    sys.exit(status)


def get_changed_files() -> list[Path]:
    # The ci workflow places the list of files changed in the PR
    # into `./files_changed` file at the root of the project.
    files_index = Path(CHANGED_FILES_INDEX)
    if not files_index.exists():
        # For pushes to master (i.e. merges) this file is *not* created.
        # We need to handle this.
        # Pretend that the core iwyu definitions changed to
        # trigger global re-check
        logging.debug(
            "no changed files index present. This is "
            "likely a push to master. Will analyze the entire codebase.")
        return [Path(MARKER_FORCE_GLOBAL_RUN)]
    paths = []
    with open(files_index) as files_index:
        for line in files_index.readlines():
            line = line.strip()
            if not line:
                continue
            paths.append(Path(line))
    return paths


def get_affected_files(changed_files: list[Path]) -> list[Path]:
    # First of all, see if any of the changed_files are global enough
    # and would require re-run on the entire codebase.
    global_files = set(Path(x) for x in [
        MARKER_FORCE_GLOBAL_RUN,
        ".github/workflows/iwyu.yml",
        "build-scripts/ci-iwyu-run.py",
        "build-scripts/get_affected_files.py",
        "tools/iwyu",  # this is a directory
        "CMakeLists.txt",
        "src/CMakeLists.txt",
        "tests/CMakeLists.txt",
    ])
    for changed in changed_files:
        if changed in global_files or any(
                parent in global_files for parent in changed.parents):
            print(
                "File %s affects global IWYU configuration so "
                "we will analyze all files" % changed)
            return generate_global_file_list()

    # Now, build-scripts/get_affected_files.py generates a list of
    # transitively affected files given a list of directly changed files.
    # This is exactly what we need.
    # Except, it requires the list of changed files to be supplied
    # to it via a file on disk, not via command-line.
    # And, if you think about it, we already have the list of changed
    # files on disk. That's `./files_changed`. So just reuse that.
    # Yes, this violates the function signature. But it is a bit less code.
    out = subprocess.run(
        [GET_AFFECTED_FILES_SCRIPT,
         "--changed-files-list", CHANGED_FILES_INDEX],
        capture_output=True, encoding="utf-8")
    if out.returncode != 0:
        print("get_affected_files.py returned with error code %d"
              % out.returncode,
              file=sys.stderr)
        print("stdout:\n  %s" % out.stdout, file=sys.stderr)
        print("stderr:\n  %s" % out.stderr, file=sys.stderr)
        sys.exit(1)
    out_paths = []
    for line in out.stdout.splitlines():
        line = line.strip()
        if not line:
            continue
        out_paths.append(Path(line))
    return out_paths


# List of all non-third-party .cpp files in the codebase.
# Equivalent to linux `find src/ tests/ -maxdepth 1 -name '*.cpp'`
def generate_global_file_list() -> list[Path]:
    paths = []
    for d in [Path("src"), Path("tests")]:
        for item in os.listdir(d):
            p = d / item
            if p.is_file() and p.suffix == ".cpp":
                paths.append(p)
    paths.sort()
    return paths


# Exclude all the files we have, well, excluded
# This is equivalent to Linux ` | grep -v -f tools/iwyu/bad_files.txt `
def filter_analyzable_files(in_files: list[Path]) -> list[Path]:
    blacklist = []
    with open(BLACKLIST_PATH, "r") as bad_files:
        for line in bad_files.readlines():
            line = line.strip()
            if line.startswith("#") or not line:
                continue
            blacklist.append(line)

    analyzable_paths = []
    for p in in_files:
        if p.suffix != ".cpp":
            continue  # because IWYU does not work on .h files
        p_str = str(p.as_posix())
        fails = any((pattern in p_str) for pattern in blacklist)
        if not fails:
            analyzable_paths.append(p)
    return analyzable_paths


def run_iwyu_on(iwyu_tool_path: str, files: list[Path]) -> int:
    argslist = [iwyu_tool_path]
    argslist.extend(str(f) for f in files)
    argslist.extend(["-p", "build", "--output-format", "clang", "--jobs", "4"])
    argslist.extend(["--"])
    cdda_root = Path(__file__).parent.parent
    mapping_path = cdda_root / "tools/iwyu/cata.imp"
    argslist.extend([
        "-Xiwyu", "--mapping_file=%s" % mapping_path,
        "-Xiwyu", "--cxx17ns",
        "-Xiwyu", "--comment_style=long",
        "-Xiwyu", "--max_line_length=1000",
        "-Xiwyu", "--error=1"])

    print("::group::IWYU full output")
    print("Running: ")
    print_long_list(argslist)
    flush_both()
    # start the process, consume its stdout, leave stderr be
    proc = subprocess.Popen(argslist, stdout=subprocess.PIPE, encoding="utf-8")
    problem_lines = []
    while True:
        line = proc.stdout.readline()
        if line == '':
            break  # IWYU finished and closed the pipe
        line = line.strip()
        print(line)
        if "#includes/fwd-decls are correct" not in line:
            problem_lines.append(line)
    proc.wait()
    flush_both()
    print("Return code ", proc.returncode)
    print("::endgroup::")

    # remove the matcher to prevent double-posting the annotations
    print("::remove-matcher owner=gcc-problem-matcher::")
    print("\n")
    if problem_lines:
        print("Problems found:")
        for line in problem_lines:
            print(line)
    elif proc.returncode == 0:
        print("No issues found!")
    else:
        print("No suggestions provided, but the process still failed somehow?")

    return proc.returncode


# GHA truncates each line to 1024 characters.
# Work around that by splitting long line into several shorter ones.
def print_long_list(things: list):
    all_lines = []
    line = ""
    for thing in things:
        thing = str(thing)
        if len(line) + len(thing) > 1000:
            all_lines.append(line)
            line = ""
        line = "%s %s" % (line, thing)
    all_lines.append(line)
    for line in all_lines:
        print("  %s" % line)


def flush_both():
    sys.stdout.flush()
    sys.stderr.flush()


if __name__ == '__main__':
    main()
