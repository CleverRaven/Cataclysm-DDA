from ..write_text import write_text


def parse_trap(json, origin):
    write_text(json["name"], origin, comment="Name of a trap")
    if "vehicle_data" in json and "sound" in json["vehicle_data"]:
        write_text(json["vehicle_data"]["sound"], origin,
                   comment="Trap-vehicle collision message for trap '{}'"
                   .format(json["name"]))
