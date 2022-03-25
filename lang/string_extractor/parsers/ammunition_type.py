from ..write_text import write_text


def parse_ammunition_type(json, origin):
    comment = json.get("//~")
    if "name" in json:
        write_text(json["name"], origin, comment=[
            "Name of an ammunition type",
            comment
        ])
