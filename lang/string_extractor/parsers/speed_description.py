from ..write_text import write_text


def parse_speed_description(json, origin):
    for speed in json.get("values", []):
        if descriptions := speed.get("descriptions", []):
            if not type(descriptions) is list:
                descriptions = [descriptions]
            for desc in descriptions:
                write_text(desc, origin,
                           comment="Speed description of monsters")
