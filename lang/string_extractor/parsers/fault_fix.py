from ..helper import get_singular_name
from ..write_text import write_text


def parse_fault_fix(json, origin):
    name = get_singular_name(json["name"])

    write_text(json["name"], origin,
               comment=f"Name of fault fix '{name}'")

    write_text(json.get("success_msg"), origin,
               comment=f"Success message of fault fix '{name}'")
