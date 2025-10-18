from ..helper import get_singular_name
from ..write_text import write_text


def parse_npc_class(json, origin):
    name = get_singular_name(json)
    comment = json.get("//")

    write_text(json.get("name"), origin,
               comment=["Name of an NPC class", comment])
    write_text(json.get("job_description"), origin,
               comment=[f"Job description of '{name}' NPC class", comment])

    for group in json.get("shopkeeper_item_group", []):
        if "refusal" not in group:
            continue
        commentg = group.get("//")
        write_text(group["refusal"], origin,
                   comment=["Refusal reason for a shop item group", commentg])
