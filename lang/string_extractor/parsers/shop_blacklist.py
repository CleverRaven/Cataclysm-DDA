from ..write_text import write_text


def parse_shopkeeper_blacklist(json, origin):
    for entry in json["entries"]:
        if "message" in entry:
            comment = entry.get("//", None)
            write_text(entry["message"], origin,
                       comment=["Reason for a shopkeeper blacklist entry",
                                comment])
