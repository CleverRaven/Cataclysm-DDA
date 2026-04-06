from ..write_text import write_text


def parse_shopkeeper_blacklist(json, origin):
    for entry in json["entries"]:
        comment = entry.get("//")
        write_text(
            entry.get("message"), origin,
            comment=["Reason for a shopkeeper blacklist entry", comment])

def parse_shopkeeper_whitelist(json, origin):
    for entry in json["entries"]:
        comment = entry.get("//")
        write_text(
            entry.get("message"), origin,
            comment=["Reason for a shopkeeper whitelist entry", comment])
