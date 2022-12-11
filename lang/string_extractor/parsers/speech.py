from ..write_text import write_text


def parse_speech(json, origin):
    speaker = ""
    if "speaker" in json:
        if type(json["speaker"]) is list:
            speaker = ", ".join(json.get("speaker", []))
        elif type(json["speaker"]) is str:
            speaker = json["speaker"]
    if "sound" in json:
        write_text(json["sound"], origin, c_format=False,
                   comment="Speech from speaker {}".format(speaker))
