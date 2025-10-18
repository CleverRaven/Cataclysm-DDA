from ..write_text import write_text


def parse_gate(json, origin):
    for k, v in json.get("messages", {}).items():
        write_text(v, origin,
                   comment=f"Message of {k} action on a gate")
