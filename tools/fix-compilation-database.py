#!/usr/bin/env python3
"""
Fixes compilation database used by run-clang-tidy.py on Windows.
"""

import json
import os
import re
import shlex

compile_db = "build/compile_commands.json"
starts_with_drive_letter = re.compile("^/(\w)/(.*)$")

data = None
with open(compile_db, 'r', encoding="utf-8") as fs:
    data = json.load( fs )

    for j in range(len(data)):
        directory = data[j]["directory"]
        command = shlex.split(data[j]["command"])
        i = 0
        while i < len(command):
            if command[i].startswith("@"):
                rsp_path = os.path.join(directory, command[i][1:])
                with open(rsp_path, 'r', encoding="utf-8") as rsp:
                    newflags = shlex.split(rsp.read())
                    command = command[:i] + newflags + command[i+1:]
                    i = i + len(newflags)
            else:
                match_result = starts_with_drive_letter.match(command[i])
                if match_result:
                    command[i] = "{}:/{}".format(match_result.group(1), match_result.group(2))
                i = i + 1
        data[j]["command"] = " ".join([shlex.quote(s) for s in command])

with open(compile_db, 'w', encoding="utf-8") as fs:
    json.dump(data, fs, indent=2)
