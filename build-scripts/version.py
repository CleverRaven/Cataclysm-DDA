#!/usr/bin/env python
"""Write src/version.h from git"""

import sys
if sys.version_info.major < 3 and sys.version_info.minor < 8:
    raise SystemExit(f"Version {sys.version_info.major}.{sys.version_info.mintor} not supported, 3.8 or later")

import os
from pathlib import Path

def is_cwd_root():
    '''Are we on the top-level source directory?
    
    Bot src/ and data/ must be there.
    Note: src/ exists in CMake build directories
    TODO git worktrees
    '''
    return (Path.cwd() / "src").is_dir() and (Path.cwd() / "data").is_dir()

while not is_cwd_root():
    # Assuming we started somewhere below
    # climb up to the source directory
    os.chdir(Path.cwd().parent)

VERSION = None
if len(sys.argv) > 1:
    VERSION=sys.argv[1]

import subprocess
git = subprocess.run(('git', 'describe', '--tags', '--always', 
                     '--match', '[0-9A-Z]*.[0-9A-Z]*',
                     '--match', 'cdda-experimental-*'
                    ,'--exact-match'
                     ),
                     capture_output=True)
GITVERSION=git.stdout.decode().strip()
git = subprocess.run(('git', 'rev-parse', '--short', 'HEAD'),
                     capture_output=True)
GITSHA=git.stdout.decode().strip()

git = subprocess.run(('git', 'diff', '--numstat', '--exit-code'))
if git.returncode != 0:
    stat=git.stdout.decode().strip()
    DIRTYFLAG="-dirty"

VERSION_STRING=(f"{GITVERSION} {GITSHA}{DIRTYFLAG}")

with open(Path("src") / "version.h", 'r') as version_h:
    version_h = version_h.read()
    import re
    m = re.search('#define VERSION (.+)$', version_h)
    OLDVERSION = m.groups()[0]

VERSION_H=f'''//NOLINT(cata-header-guard)
#define VERSION "{VERSION_STRING}"
'''
with open(Path("src") / "version.h", 'w') as version_h:
    version_h.write(VERSION_H)