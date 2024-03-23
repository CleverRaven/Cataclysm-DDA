from ..write_text import write_text


def parse_vehicle_spawn(json, origin):
    for st in json.get("spawn_types", []):
        write_text(st.get("description"), origin,
                   comment="Vehicle Spawn Description")
