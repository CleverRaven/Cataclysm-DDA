"""Write src/version.h and VERSION.txt from git, if available

All commands emulate the Makefile's "version" target.
Format is: [TAG] [SHA1][-dirty]

Write VERSION.txt only when missing.

The optional command line arguments or environment variables are:
    VERSION|VERSION_STRING=<version>
    ARTIFACT=<artifact>
    TIMESTAMP=<timestamp>
"""

from pathlib import Path
import datetime
import logging
import re
import os
import subprocess
import sys
if sys.version_info.major < 3 and sys.version_info.minor < 8:
    raise SystemExit(
        f"Version {sys.version_info.major}.{sys.version_info.mintor}"
        "not supported, 3.8 or later")

VERSION = VERSION_STRING = ARTIFACT = TIMESTAMP = None


logging.basicConfig(level=logging.DEBUG)  # DEBUG
log = logging.getLogger()


def is_cwd_root():
    '''Are we on the top-level source directory?

    Bot src/ and data/ must be there.
    Note: src/ exists in CMake build directories
    '''
    return (Path.cwd() / "src").is_dir() and (Path.cwd() / "data").is_dir()


def read_version_h():
    try:
        with open(Path("src") / "version.h", 'r') as version_h:
            version_h = version_h.read()
            if m := re.search('#define VERSION \"(.+)\"$', version_h):
                if groups := m.groups():
                    OLDVERSION = groups[0]
                    log.debug(f"{OLDVERSION=}")
                    return OLDVERSION
    except FileNotFoundError:
        pass


def write_version_h():
    with open(Path("src") / "version.h", 'w') as version_h:
        VERSION_H = ("//NOLINT(cata-header-guard)\n"
                     f'#define VERSION "{VERSION_STRING}"\n')
        version_h.write(VERSION_H)


def write_VERSION_TXT():
    url = "https://github.com/CleverRaven/Cataclysm-DDA"
    if GITSHA:
        url = f"{url}/commit/{GITSHA}"
    timestamp = TIMESTAMP or datetime.datetime.now().strftime("%Y-%m-%d-%H%M")
    try:
        with open("VERSION.TXT", 'x') as VERSION_TXT:
            text = str()
            text += (f"build type: {ARTIFACT or 'Release'}\n"
                     f"build number: {timestamp}\n"
                     f"commit sha: {GITSHA or 'Unknown'}\n"
                     f"commit url: {url}")
            VERSION_TXT.write(text)
    except FileExistsError:
        log.debug('Skip writing VERSION.txt')


# Not using argparse
for arg in sys.argv[1:]:
    arg = arg.split('=', 1)
    log.debug(f"{arg}")
    if len(arg) == 2:
        globals()[arg[0]] = arg[1]

ARTIFACT = ARTIFACT or os.environ.get('ARTIFACT', None) or 'Release'
TIMESTAMP = TIMESTAMP or os.environ.get('TIMESTAMP', None)
VERSION_STRING = VERSION or os.environ.get(
    'VERSION', None) or os.environ.get('VERSION_STRING', None) or '0.I'
GITSHA = None

while not is_cwd_root():
    # Assuming we started somewhere down, climb up to the source directory
    os.chdir(Path.cwd().parent)

# Checking for .git/ may not work because of external worktrees
try:
    git = subprocess.run(('git', 'rev-parse', '--is-inside-work-tree'),
                         capture_output=True)
except FileNotFoundError:  # `git` command is missing
    write_version_h()
    write_VERSION_TXT()
    raise SystemExit

if git.returncode != 0:
    stdout = git.stdout.decode().strip()
    if 'true' != stdout:
        write_version_h()
        write_VERSION_TXT()
        raise SystemExit

# Get the tag
git = subprocess.run(('git', 'describe', '--tags', '--always',
                     '--match', '[0-9A-Z]*.[0-9A-Z]*',
                      '--match', 'cdda-experimental-*', '--exact-match'
                      ),
                     capture_output=True)
GITVERSION = git.stdout.decode().strip()
log.debug(f"{GITVERSION=}")

# Get the short SHA1
git = subprocess.run(('git', 'rev-parse', '--short', 'HEAD'),
                     capture_output=True)
GITSHA = git.stdout.decode().strip()
log.debug(f"{GITSHA=}")

# Check if there are changes in the worktree
DIRTYFLAG = str()
git = subprocess.run(('git', 'diff', '--numstat', '--exit-code',
                      '-c', 'core.safecrlf=false'),
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

write_VERSION_TXT()

print(VERSION_STRING, end=None)
