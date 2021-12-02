from ..helper import get_singular_name
from ..write_text import write_text


def parse_martial_art(json, origin):
    name = get_singular_name(json["name"])
    write_text(name, origin, comment="Name of martial art")

    if "description" in json:
        write_text(json["description"], origin, c_format=False,
                   comment="Description of martial art \"{}\"".format(name))

    if "initiate" in json:
        messages = json["initiate"]
        if type(messages) is str:
            messages = [messages]
        for msg in messages:
            write_text(msg, origin,
                       comment="Initiate message of martial art \"{}\"".
                       format(name))

    onhit_buffs = json.get("onhit_buffs", list())
    static_buffs = json.get("static_buffs", list())
    onmove_buffs = json.get("onmove_buffs", list())
    ondodge_buffs = json.get("ondodge_buffs", list())
    onattack_buffs = json.get("onattack_buffs", list())
    onpause_buffs = json.get("onpause_buffs", list())
    onblock_buffs = json.get("onblock_buffs", list())
    ongethit_buffs = json.get("ongethit_buffs", list())
    onmiss_buffs = json.get("onmiss_buffs", list())
    oncrit_buffs = json.get("oncrit_buffs", list())
    onkill_buffs = json.get("onkill_buffs", list())

    buffs = (onhit_buffs + static_buffs + onmove_buffs + ondodge_buffs +
             onattack_buffs + onpause_buffs + onblock_buffs + ongethit_buffs +
             onmiss_buffs + oncrit_buffs + onkill_buffs)

    for buff in buffs:
        buff_name = get_singular_name(buff["name"])
        write_text(buff_name, origin,
                   comment="Buff name of martial art \"{}\"".format(name))
        write_text(buff["description"], origin, c_format=False,
                   comment="Description of buff \"{0}\" in martial art "
                           "\"{1}\"".format(name, buff_name))
