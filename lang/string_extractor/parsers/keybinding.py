from ..write_text import write_text


def parse_keybinding(json, origin):
    write_text(json.get("name"), origin, comment="Key binding name")
