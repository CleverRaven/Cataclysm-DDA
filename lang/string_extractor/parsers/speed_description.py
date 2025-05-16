from ..write_text import write_text


def parse_speed_description(json, origin):
    for speed in json.get("values", []):
        if "descriptions" in speed:
            descriptions = []
            if type(speed["descriptions"]) is list:
                descriptions = speed["descriptions"]
            else:
                descriptions.append(speed["descriptions"])
            for desc in descriptions:
                write_text(desc, origin,
                           comment="Speed description of monsters")
