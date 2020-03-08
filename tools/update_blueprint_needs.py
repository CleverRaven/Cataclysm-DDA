#!/usr/bin/env python3

import getopt
import json
import os
import re
import subprocess
import sys

def print_help():
    print("\n"
          "Update faction camp blueprints with autocalculated requirements from unit test log.\n"
          "This tool requires tools/format/json_formatter.\n"
          "\n"
          "    --help              prints this message\n"
          "    --logfile=<logfile> specify the path to unit test log file\n"
          "    --action=<value>    what to do with reported inconsistencies. (optional)\n"
          "                            update: update with suggested requirements (default)\n"
          "                            suppress: suppress inconsistency warnings\n")

def main(argv):
    try:
        opts, args = getopt.getopt(argv, "", ["help", "logfile=", "action="])
    except getopt.GetoptError:
        print_help()
        return

    test_log = None
    suppress = False
    for opt, arg in opts:
        if opt == "--help":
            print_help()
            return
        if opt == "--logfile":
            test_log = arg
        if opt == "--action":
            if arg == "update":
                suppress = False
            elif arg == "suppress":
                suppress = True
            else:
                print_help();
                return
    if not test_log:
        print_help()
        return

    json_dirs = {
        "data/core",
        "data/json",
        "data/mods",
    };

    auto_update_blueprint = re.compile("~~~ auto-update-blueprint: (.+)")
    auto_update_blueprint_end = re.compile("~~~ end-auto-update")
    json_filename = re.compile(".+\\.json")

    update_blueprints = dict()

    with open(test_log, 'r', encoding='utf-8') as fs:
        while True:
            line = fs.readline()
            if not line:
                break
            match_result = auto_update_blueprint.match(line)
            if match_result:
                ident = match_result.group(1)
                reqs = ""
                complete = False
                while True:
                    line = fs.readline()
                    if not line:
                        complete = False
                        break
                    if auto_update_blueprint_end.match(line):
                        complete = True
                        break
                    else:
                        reqs += line
                if complete:
                    update_blueprints[ident] = json.loads(reqs);
                    print("{} needs updating".format(ident))

    if len(update_blueprints) == 0:
        print("no inconsistency reported in the test log")
        return

    for json_dir in json_dirs:
        print("walking dir {}".format(json_dir))
        for root, dirs, files in os.walk(json_dir):
            for file in files:
                json_path = os.path.join(root, file)
                content = None
                changed = False
                if json_filename.match(file):
                    with open(json_path, 'r', encoding='utf-8') as fs:
                        content = json.load(fs)
                if type(content) is list:
                    for obj in content:
                        if not (type(obj) is dict
                          and "type" in obj and obj["type"] == "recipe"
                          and ("result" in obj or "abstract" in obj)):
                            continue
                        ident = None;
                        if "abstract" in obj:
                            ident = obj["abstract"]
                        else:
                            ident = obj["result"]
                            if "id_suffix" in obj:
                                ident += "_" + obj["id_suffix"]
                        if ident in update_blueprints:
                            if suppress:
                                obj["check_blueprint_needs"] = False
                            else:
                                obj["blueprint_needs"] = update_blueprints[ident]
                            if not changed:
                                changed = True
                                print("updating {}".format(json_path))
                if changed:
                    with open(json_path, 'w', encoding='utf-8') as fs:
                        json.dump(content, fs, indent=2)
                    subprocess.run(["tools/format/json_formatter", json_path], stdout=subprocess.DEVNULL)

if __name__ == "__main__":
    main(sys.argv[1:])
