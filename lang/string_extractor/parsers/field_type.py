from ..write_text import write_text


def parse_field_type(json, origin):
    for fd in json.get("intensity_levels", []):
        if "name" in fd:
            write_text(fd["name"], origin, comment="Field intensity level")
