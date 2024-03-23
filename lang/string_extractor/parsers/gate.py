from ..write_text import write_text


def parse_gate(json, origin):
    messages = json.get("messages", {})

    for i in messages:
        write_text(messages[i], origin,
                   comment="Message of {} action on a gate".format(i))
