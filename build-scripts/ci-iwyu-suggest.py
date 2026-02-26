# Companion to ci-iwyu-run.py for the IWYU suggester workflow.
# Runs IWYU and pipes the output through fix_includes.py to apply
# fixes in-place. reviewdog/action-suggester then diffs against
# the original and posts suggested changes on the PR.

import os
import subprocess
import sys

from pathlib import Path

CHANGED_FILES_INDEX = "files_changed"
GET_AFFECTED_FILES_SCRIPT = "build-scripts/get_affected_files.py"
BLACKLIST_PATH = "tools/iwyu/bad_files.txt"


def main():
    print("::group::Determining files to analyze")

    changed_files = get_changed_files()
    print("changed files:")
    print_list(changed_files)

    affected_files = get_affected_files(changed_files)
    print("affected files:")
    print_list(affected_files)

    files_to_analyze = filter_analyzable_files(affected_files)
    print("files to analyze:")
    print_list(files_to_analyze)
    print("::endgroup::")

    if not files_to_analyze:
        print("Nothing to analyze!")
        return

    run_iwyu_and_fix(files_to_analyze)


def get_changed_files() -> list[Path]:
    files_index = Path(CHANGED_FILES_INDEX)
    if not files_index.exists():
        print("no changed files index -- analyzing entire codebase")
        return generate_global_file_list()
    paths = []
    with open(files_index) as f:
        for line in f.readlines():
            line = line.strip()
            if line:
                paths.append(Path(line))
    return paths


def get_affected_files(changed_files: list[Path]) -> list[Path]:
    global_files = set(Path(x) for x in [
        ".github/workflows/iwyu-linter.yml",
        "build-scripts/ci-iwyu-suggest.py",
        "build-scripts/get_affected_files.py",
        "tools/iwyu",
        "CMakeLists.txt",
        "src/CMakeLists.txt",
        "tests/CMakeLists.txt",
    ])
    for changed in changed_files:
        if changed in global_files or any(
                parent in global_files for parent in changed.parents):
            print("File %s affects global config, analyzing all files"
                  % changed)
            return generate_global_file_list()

    if not Path(CHANGED_FILES_INDEX).exists():
        return generate_global_file_list()

    out = subprocess.run(
        [GET_AFFECTED_FILES_SCRIPT,
         "--changed-files-list", CHANGED_FILES_INDEX],
        capture_output=True, encoding="utf-8")
    if out.returncode != 0:
        print("get_affected_files.py failed (code %d)" % out.returncode,
              file=sys.stderr)
        print("stdout: %s" % out.stdout, file=sys.stderr)
        print("stderr: %s" % out.stderr, file=sys.stderr)
        sys.exit(1)
    return [Path(line.strip()) for line in out.stdout.splitlines()
            if line.strip()]


def generate_global_file_list() -> list[Path]:
    paths = []
    for d in [Path("src"), Path("tests")]:
        for item in os.listdir(d):
            p = d / item
            if p.is_file() and p.suffix == ".cpp":
                paths.append(p)
    paths.sort()
    return paths


def filter_analyzable_files(in_files: list[Path]) -> list[Path]:
    blacklist = []
    with open(BLACKLIST_PATH, "r") as f:
        for line in f.readlines():
            line = line.strip()
            if line and not line.startswith("#"):
                blacklist.append(line)

    result = []
    for p in in_files:
        if p.suffix != ".cpp":
            continue
        p_str = str(p.as_posix())
        if not any(pattern in p_str for pattern in blacklist):
            result.append(p)
    return result


def run_iwyu_and_fix(files: list[Path]):
    cdda_root = Path(__file__).parent.parent
    mapping_path = cdda_root / "tools/iwyu/cata.imp"

    iwyu_args = ["iwyu_tool.py"]
    iwyu_args.extend(str(f) for f in files)
    iwyu_args.extend(["-p", "build", "--jobs", "4"])
    iwyu_args.extend(["--"])
    iwyu_args.extend([
        "-Xiwyu", "--mapping_file=%s" % mapping_path,
        "-Xiwyu", "--cxx17ns",
        "-Xiwyu", "--comment_style=long",
        "-Xiwyu", "--max_line_length=1000"])

    fix_args = ["fix_includes.py", "--nosafe_headers", "--reorder"]

    print("::group::IWYU + fix_includes output")
    print("Running IWYU: %s" % " ".join(iwyu_args[:5]))
    print("Piping through: %s" % " ".join(fix_args))
    sys.stdout.flush()

    # Only pipe stdout to fix_includes -- stderr must stay separate
    # so it doesn't corrupt the IWYU output format.
    iwyu_proc = subprocess.Popen(
        iwyu_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    fix_proc = subprocess.Popen(
        fix_args, stdin=iwyu_proc.stdout, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, encoding="utf-8")
    iwyu_proc.stdout.close()

    fix_output, _ = fix_proc.communicate()
    _, iwyu_stderr = iwyu_proc.communicate()
    iwyu_proc.wait()

    if iwyu_stderr:
        print("IWYU stderr:")
        print(iwyu_stderr.decode("utf-8", errors="replace"))
    if fix_output:
        print("fix_includes output:")
        print(fix_output)
    print("::endgroup::")
    print("IWYU exit code: %d, fix_includes exit code: %d"
          % (iwyu_proc.returncode, fix_proc.returncode))


def print_list(things: list):
    line = ""
    for thing in things:
        thing = str(thing)
        if len(line) + len(thing) > 1000:
            print("  %s" % line)
            line = ""
        line = "%s %s" % (line, thing)
    if line:
        print("  %s" % line)


if __name__ == '__main__':
    main()
