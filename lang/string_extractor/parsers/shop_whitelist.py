from ..write_text import write_text


def parse_shopkeeper_whitelist(json, origin):
    write_text(
        json.get("message"), origin,
        comment="Reason for rejecting an item not in a shopkeeper whitelist")
    for entry in json["entries"]:
        comment = entry.get("//")
        write_text(
            entry.get("message"), origin,
            comment=["Reason for a shopkeeper whitelist entry", comment])
