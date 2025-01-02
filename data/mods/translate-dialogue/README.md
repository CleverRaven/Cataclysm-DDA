Use `tools/json_tools/update-translate-dialogue-mod.py` to update the mod.
This update script uses up-to-date strings and `//~` comments as "translation":
```json
{
  "id": "TALK_EXODII_MERCHANT_TalkJob",
  "type": "talk_topic",
  "dynamic_line": {
    "//~": "Business before pleasure.  What do you need?",
    "str": "Aye, business afore, 'tis said.  What'll ye tass to ol' Rubik?"
  }
}
vvvvvvvvvvvvvvvvvvvvvvvvv
{
  "id": "TALK_EXODII_MERCHANT_TalkJob",
  "type": "talk_topic",
  "dynamic_line": {
    "concatenate": [
      "Aye, business afore, 'tis said.  What'll ye tass to ol' Rubik?",
      "\"\n\n[TRANSLATE:] \"",
      "Business before pleasure.  What do you need?"
    ]
  }
}
```

Usage:
 * `cd path/to/Cataclysm-DDA/`
 * `python3 tools/json_tools/update-translate-dialogue-mod.py`
 * then lint via `json_formatter.cgi`, check `doc/JSON_STYLE.md`
 * You can also add `"//": "mod_update_script_compact"` to dynamic lines to make the script concatenate that line more compactly and without newlines, which is useful if the original dialog also uses concatenation.
