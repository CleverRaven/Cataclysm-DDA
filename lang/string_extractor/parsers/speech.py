from ..write_text import write_text


def parse_speech(json, origin):
    speaker = ", ".join(json.get("speaker", []))
    if "sound" in json:
        write_text(json["sound"], origin, c_format=False,
                   comment="Speech from speaker {}".format(speaker))
