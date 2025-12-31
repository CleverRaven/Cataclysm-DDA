from ..helper import get_singular_name
from ..write_text import write_text


def parse_martial_art(json, origin):
    name = get_singular_name(json)

    write_text(name, origin, comment="Name of martial art")
    write_text(json.get("description"), origin,
               comment=f"Description of martial art '{name}'")

    initiate = json.get("initiate", [])
    if type(initiate) is str:
        initiate = [initiate]
    for msg in initiate:
        write_text(msg, origin,
                   comment=f"Initiate message of martial art '{name}'")

    buffs = [
        *json.get("onhit_buffs", []),
        *json.get("static_buffs", []),
        *json.get("onmove_buffs", []),
        *json.get("ondodge_buffs", []),
        *json.get("onattack_buffs", []),
        *json.get("onpause_buffs", []),
        *json.get("onblock_buffs", []),
        *json.get("ongethit_buffs", []),
        *json.get("onmiss_buffs", []),
        *json.get("oncrit_buffs", []),
        *json.get("onkill_buffs", [])
    ]

    for buff in buffs:
        buff_name = get_singular_name(buff)

        write_text(buff_name, origin,
                   comment=f"Buff name of martial art '{name}'")

        write_text(buff["description"], origin,
                   comment=f"Description of buff '{buff_name}'")
