"""Write src/version.h and VERSION.txt from git, if available

All commands emulate the Makefile's "version" target.
Format is: [TAG] [SHA1][-dirty]

This script accepts arguments on the command line with the same format
as environment variables:
    VERSION=<version>
    VERSION_STRING=<version>
    ARTIFACT=<artifact>
    TIMESTAMP=<timestamp>

You could run this script like the following.
- On Unix: ARTIFACT=linux python version.py
- On Windows: python version.py ARTIFACT=linux
"""

from pathlib import Path
import datetime
import logging
import os
import subprocess
import sys


def is_cwd_root():
    '''Are we on the top-level source directory?

    Bot src/ and data/ must be there.
    Note: src/ exists in CMake build directories
    '''
    return (Path.cwd() / "src").is_dir() and (Path.cwd() / "data").is_dir()


def write_version_h(VERSION_STRING='unknown'):
    VERSION_H = None
    src_version_h = Path("src") / "version.h"
    try:
        VERSION_H = open(src_version_h, 'r').read()
    except FileNotFoundError:
        pass
    if not VERSION_H:
        text = ("//NOLINT(cata-header-guard)\n"
                f'#define VERSION "{VERSION_STRING}"\n')
        if VERSION_H != text:
            open(src_version_h, 'w').write(text)
            return
    logging.debug("Skip writing src/version.h")


def write_VERSION_TXT(GITSHA=None, TIMESTAMP=None, ARTIFACT=None):
    url = "https://github.com/CleverRaven/Cataclysm-DDA"
    if GITSHA:
        url = f"{url}/commit/{GITSHA}"
    timestamp = TIMESTAMP or datetime.datetime.now().strftime("%Y-%m-%d-%H%M")
    with open("VERSION.TXT", 'w') as VERSION_TXT:
        text = str()
        text += (f"build type: {ARTIFACT or 'Release'}\n"
                 f"build number: {timestamp}\n"
                 f"commit sha: {GITSHA or 'Unknown'}\n"
                 f"commit url: {url}")
        VERSION_TXT.write(text)


def main():
    logging.basicConfig(level=logging.DEBUG)  # DEBUG

    VERSION = VERSION_STRING = ARTIFACT = TIMESTAMP = None

    # Not using argparse
    for arg in sys.argv[1:]:
        arg = arg.split('=', 1)
        logging.debug(f"{arg}")
        if len(arg) == 2:
            locals()[arg[0]] = arg[1]

    ARTIFACT = ARTIFACT or os.environ.get('ARTIFACT', None) or 'Release'
    TIMESTAMP = TIMESTAMP or os.environ.get('TIMESTAMP', None)
    VERSION_STRING = VERSION or os.environ.get(
        'VERSION', None) or os.environ.get('VERSION_STRING', None)
    GITSHA = None

    while not is_cwd_root():
        # Assuming we started somewhere down, climb up to the source directory
        os.chdir(Path.cwd().parent)

    # Checking for .git/ may not work because of external worktrees
    try:
        git = subprocess.run(('git', 'rev-parse', '--is-inside-work-tree'),
                             capture_output=True)
    except FileNotFoundError:  # `git` command is missing
        write_version_h(VERSION_STRING=VERSION_STRING)
        write_VERSION_TXT(GITSHA=GITSHA, TIMESTAMP=TIMESTAMP,
                          ARTIFACT=ARTIFACT)
        raise SystemExit

    if git.returncode != 0:
        stdout = git.stdout.decode().strip()
        if 'true' != stdout:
            write_version_h(VERSION_STRING=VERSION_STRING)
            write_VERSION_TXT(
                GITSHA=GITSHA, TIMESTAMP=TIMESTAMP, ARTIFACT=ARTIFACT)
            raise SystemExit

    # Get the tag
    git = subprocess.run(('git', 'describe', '--tags', '--always',
                          '--match', '[0-9A-Z]*.[0-9A-Z]*',
                          '--match', 'cdda-experimental-*', '--exact-match'
                          ),
                         capture_output=True)
    GITVERSION = git.stdout.decode().strip()
    logging.debug(f"{GITVERSION=}")

    # Get the short SHA1
    git = subprocess.run(('git', 'rev-parse', '--short', 'HEAD'),
                         capture_output=True)
    GITSHA = git.stdout.decode().strip()
    logging.debug(f"{GITSHA=}")

    # Check if there are changes in the worktree
    DIRTYFLAG = str()
    git = subprocess.run(('git', '-c', 'core.safecrlf=false', 'diff', '--numstat', '--exit-code',),
                         capture_output=True)
    if git.returncode != 0:
        stat = git.stdout.decode().strip()
        # TODO filter lang/po
        DIRTYFLAG = "-dirty"
    logging.debug(f"{DIRTYFLAG=}")

    if GITVERSION:
        VERSION_STRING = f"{GITVERSION} {GITSHA}{DIRTYFLAG}"
    else:
        VERSION_STRING = f"{GITSHA}{DIRTYFLAG}"
    logging.debug(f"{VERSION_STRING=}")

    write_version_h(VERSION_STRING=VERSION_STRING)

    write_VERSION_TXT(GITSHA=GITSHA, TIMESTAMP=TIMESTAMP, ARTIFACT=ARTIFACT)

    print(VERSION_STRING)


if __name__ == "__main__":
    main()
