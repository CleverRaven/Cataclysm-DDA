from ..write_text import write_text


def parse_speech(json, origin):
    name = json.get("speaker", [])
    if type(name) is list:
        name = ", ".join(name) or "???"

    write_text(json.get("sound"), origin,
               comment=f"Speech from monsters: {name}")
