from ..write_text import write_text


def parse_tool_quality(json, origin):
    write_text(json.get("name"), origin,
               comment="Name of tool quality")
