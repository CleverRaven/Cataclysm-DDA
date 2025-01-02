#!/usr/bin/env python3
"""
  Usage:
    `cd path/to/Cataclysm-DDA/`
    `python3 tools/json_tools/update-translate-dialogue-mod.py`
    then lint via `json_formatter.cgi`, check `doc/JSON_STYLE.md`

  You can also add `"//": "mod_update_script_compact"`
  to dynamic lines to make the script concatenate that line
  more compactly and without newlines, which is useful if
  the original dialog also uses concatenation.
"""

import json


def transform_dynamic_line(dl, id):
    if type(dl) is str or \
       type(dl) is int or \
       type(dl) is bool or \
       type(dl) is None:
        return dl

    if type(dl) is list:
        out = []
        for line in dl:
            out.append(transform_dynamic_line(line, id))
        return out

    if type(dl) is dict:
        if ("//~" in dl) and ("str" in dl):
            if ("//" in dl) and (dl["//"] == "mod_update_script_compact"):
                return {"concatenate": [
                    dl["str"],
                    " [TRANSLATE: ",
                    dl["//~"],
                    "]"]}
            return {"concatenate": [
                dl["str"],
                "\"\n\n[TRANSLATE:] \"",
                dl["//~"]]}
        out = {}
        for k in dl:
            out[k] = transform_dynamic_line(dl[k], id)
        return out

    return dl


def parse_dialogue(obj, dialogues):
    if "dynamic_line" not in obj:
        return

    out = {}
    out["id"] = obj["id"]
    out["type"] = obj["type"]
    dl = obj["dynamic_line"]

    out["dynamic_line"] = transform_dynamic_line(dl, out["id"])

    if "TRANSLATE:" in json.dumps(out):
        dialogues.append(out)


def main():
    input_dir = "data/json/npcs/exodii/"
    output_dir = "data/mods/translate-dialogue/"
    files = ["exodii_merchant_talk.json",
             "exodii_merchant_talk_exodization.json"]

    for f in files:
        dialogues = []
        with open(input_dir + f, encoding="utf-8") as fp:
            print(f"Reading from {input_dir+f}")
            json_data = json.load(fp)

        for obj in json_data:
            try:
                parse_dialogue(obj, dialogues)
            except Exception as err:
                print(f"Error: {err}")

        if dialogues:
            with open(output_dir + f, "w", encoding="utf-8") as fp:
                print(f"  Writing to {output_dir+f}")
                json.dump(dialogues, fp, ensure_ascii=False, indent="  ")


main()
