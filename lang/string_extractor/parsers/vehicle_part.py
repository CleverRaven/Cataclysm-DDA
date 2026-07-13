from ..helper import get_singular_name
from ..write_text import write_text


def parse_vehicle_part(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Vehicle part name")
    write_text(json.get("description"), origin,
               comment=f"Description of vehicle part '{name}'")

    for v in json.get("variants", []):
        label = v.get("label", "Default")
        write_text(label, origin, context="vpart_variants",
                   comment="Label of vehicle part variant")

        for base in json.get("variants_bases", []):
            based_label = base["label"]
            if label != "":
                based_label = f"{based_label} {label}"
            write_text(based_label, origin, context="vpart_variants",
                       comment="Label of vehicle part variant")
