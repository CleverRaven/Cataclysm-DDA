from ..helper import get_singular_name
from ..write_text import write_text


def parse_vehicle_part(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Vehicle part name")
    elif "id" in json:
        name = json["id"]

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of vehicle part \"{}\"".format(name))

    for v in json.get("variants", []):
        id = ""
        label = "Default"
        if "id" in v:
            id = v["id"]
        if "label" in v:
            label = v["label"]
        write_text(label, origin,
                   context="vpart_variants",
                   comment="Label of vehicle part variant \"{}\"".format(id))
        for base in json.get("variants_bases", []):
            based_id = base["id"]
            based_label = base["label"]
            if id != "":
                based_id = base["id"] + "_" + id
            if label != "":
                based_label = base["label"] + " " + label
            # awkward linter dictates 79 chars max, have to line break here
            cmt = "Label of vehicle part variant \"{}\"".format(based_id)
            write_text(based_label, origin,
                       context="vpart_variants",
                       comment=cmt)
