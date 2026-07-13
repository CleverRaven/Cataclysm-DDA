from ..write_text import write_text


def parse_activity_type(json, origin):
    write_text(json.get("verb"), origin,
               comment="A verb expressing an ongoing action")
