"""Write src/version.h from git

If we are not in a git worktree, use environment VERSION and VERSION_STRING.
Otherwise, use the first command line argument.

All commands emulate the Makefile's "version" target.
Format is: [TAG] [SHA1][-dirty]
"""

import sys
if sys.version_info.major < 3 and sys.version_info.minor < 8:
    raise SystemExit(
        f"Version {sys.version_info.major}.{sys.version_info.mintor}"
        "not supported, 3.8 or later")

import subprocess
import os
import re
import logging
from pathlib import Path

logging.basicConfig(level=logging.INFO) # DEBUG
log = logging.getLogger()


def is_cwd_root():
    '''Are we on the top-level source directory?

    Bot src/ and data/ must be there.
    Note: src/ exists in CMake build directories
    '''
    return (Path.cwd() / "src").is_dir() and (Path.cwd() / "data").is_dir()


while not is_cwd_root():
    # Assuming we started somewhere down, climb up to the source directory
    os.chdir(Path.cwd().parent)

VERSION_STRING = os.environ.get(
    'VERSION', None) or os.environ.get('VERSION_STRING', None)

if len(sys.argv) > 1:
    VERSION_STRING = sys.argv[1]


def read_version_h():
    with open(Path("src") / "version.h", 'r') as version_h:
        version_h = version_h.read()
        if m := re.search('#define VERSION \"(.+)\"$', version_h):
            if groups := m.groups():
                OLDVERSION = groups[0]
                log.debug(f"{OLDVERSION=}")
                return OLDVERSION


def write_version_h():
    with open(Path("src") / "version.h", 'w') as version_h:
        VERSION_H = ("//NOLINT(cata-header-guard)\n"
                     f'#define VERSION "{VERSION_STRING}"\n')
        version_h.write(VERSION_H)
        print(VERSION_STRING, end = None)


# Checking for .git/ may not work because of external worktrees
git = subprocess.run(('git', 'rev-parse', '--is-inside-work-tree'),
                     capture_output=True)
if git.returncode != 0:
    stdout = git.stdout.decode().strip()
    if 'true' != stdout:
        write_version_h()
        raise SystemExit

# Get the tag
git = subprocess.run(('git', 'describe', '--tags', '--always',
                     '--match', '[0-9A-Z]*.[0-9A-Z]*',
                      '--match', 'cdda-experimental-*', '--exact-match'
                      ),
                     capture_output=True)
GITVERSION = git.stdout.decode().strip()
log.debug(f"{GITVERSION=}")

# Get the SHA1
git = subprocess.run(('git', 'rev-parse', '--short', 'HEAD'),
                     capture_output=True)
GITSHA = git.stdout.decode().strip()
log.debug(f"{GITSHA=}")

# Check if there are changes in the worktree
DIRTYFLAG = None
git = subprocess.run(('git', 'diff', '--numstat', '--exit-code'),
                     capture_output=True)
if git.returncode != 0:
    stat = git.stdout.decode().strip()
    # TODO filter lang/po
    DIRTYFLAG = "-dirty"
log.debug(f"{DIRTYFLAG=}")

if GITVERSION:
    VERSION_STRING = f"{GITVERSION} {GITSHA}{DIRTYFLAG}"
else:
    VERSION_STRING = f"{GITSHA}{DIRTYFLAG}"
log.debug(f"{VERSION_STRING=}")

OLDVERSION = read_version_h()

if VERSION_STRING != OLDVERSION:
    write_version_h()
else:
    log.debug("Skip writing src/version.h")