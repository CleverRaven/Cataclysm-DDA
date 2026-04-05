# Companion to ci-iwyu-run.py for the IWYU suggester workflow.
# Runs IWYU and pipes the output through fix_includes.py to apply
# fixes in-place, then posts review comments with suggestion blocks
# for each diff hunk.
#
# Requires environment variables (set by the workflow):
#   GITHUB_REPOSITORY  - e.g. "CleverRaven/Cataclysm-DDA"
#   PR_NUMBER          - pull request number
#   COMMIT_SHA         - head commit SHA of the PR
#   GH_TOKEN           - GitHub token for API access

import os
import re
import subprocess
import sys

from pathlib import Path

CHANGED_FILES_INDEX = "files_changed"
GET_AFFECTED_FILES_SCRIPT = "build-scripts/get_affected_files.py"
BLACKLIST_PATH = "tools/iwyu/bad_files.txt"
COMMENT_MARKER = "<!-- iwyu-suggest -->"


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

    try:
        run_iwyu_and_fix(files_to_analyze)
    except Exception as e:
        # Don't let IWYU/fix_includes failures prevent posting
        # whatever partial changes were applied.
        print("IWYU run failed: %s" % e, file=sys.stderr)

    diff_text = get_diff()
    if not diff_text:
        print("No changes after IWYU -- nothing to suggest.")
        return

    file_hunks = parse_hunks(diff_text)
    print("Files with IWYU changes: %s" % ", ".join(file_hunks.keys()))

    post_suggestions(file_hunks)


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


def get_diff() -> str:
    result = subprocess.run(
        ["git", "diff", "--no-color"],
        capture_output=True, encoding="utf-8")
    return result.stdout.strip()


def parse_hunks(diff_text: str) -> dict[str, list[dict]]:
    """Parse a unified diff into per-file lists of hunks.

    Each hunk has old_start, old_count (the original line range)
    and new_lines (the replacement content for a suggestion block).
    """
    files = {}
    current_file = None
    in_hunk = False

    for line in diff_text.split("\n"):
        m = re.match(r"^diff --git a/.+ b/(.+)$", line)
        if m:
            current_file = m.group(1)
            files.setdefault(current_file, [])
            in_hunk = False
            continue

        m = re.match(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@", line)
        if m and current_file is not None:
            old_start = int(m.group(1))
            old_count = int(m.group(2)) if m.group(2) is not None else 1
            files[current_file].append({
                "old_start": old_start,
                "old_count": old_count,
                "new_lines": [],
            })
            in_hunk = True
            continue

        if in_hunk and current_file is not None and files[current_file]:
            hunk = files[current_file][-1]
            if line.startswith("+"):
                hunk["new_lines"].append(line[1:])
            elif line.startswith(" "):
                hunk["new_lines"].append(line[1:])
            # '-' lines are already counted in old_count;
            # '\' (no newline) lines are ignored.

    return files


def post_suggestions(file_hunks: dict[str, list[dict]]):
    """Post review comments with suggestion blocks for each hunk."""
    from github import Github, GithubException

    token = os.environ.get("GH_TOKEN", "")
    repo_name = os.environ.get("GITHUB_REPOSITORY", "")
    pr_number = os.environ.get("PR_NUMBER", "")
    commit_sha = os.environ.get("COMMIT_SHA", "")

    if not all([token, repo_name, pr_number, commit_sha]):
        print("Missing environment variables -- skipping comment posting.",
              file=sys.stderr)
        return

    print("::group::Posting IWYU suggestions")

    gh = Github(token)
    repo = gh.get_repo(repo_name)
    pr = repo.get_pull(int(pr_number))
    commit = repo.get_commit(commit_sha)

    # Delete stale IWYU comments from previous runs.
    for comment in pr.get_review_comments():
        if COMMENT_MARKER in comment.body:
            print("Deleting stale IWYU comment %d on %s"
                  % (comment.id, comment.path))
            comment.delete()

    # Post one suggestion per hunk.
    for path, hunks in file_hunks.items():
        for hunk in hunks:
            old_start = hunk["old_start"]
            old_count = hunk["old_count"]
            new_content = "\n".join(hunk["new_lines"])

            if old_count < 1:
                continue

            body = "%s\n```suggestion\n%s\n```" % (
                COMMENT_MARKER, new_content)
            end_line = old_start + old_count - 1

            try:
                print("Posting suggestion on %s lines %d-%d"
                      % (path, old_start, end_line))
                if old_count > 1:
                    pr.create_review_comment(
                        body=body, commit=commit, path=path,
                        line=end_line, start_line=old_start,
                        side="RIGHT")
                else:
                    pr.create_review_comment(
                        body=body, commit=commit, path=path,
                        line=old_start, side="RIGHT")
            except GithubException as e:
                print("Failed to post on %s:%d - %s"
                      % (path, old_start, e), file=sys.stderr)

    print("::endgroup::")


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
