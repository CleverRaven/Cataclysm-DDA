from ..write_text import write_text


def parse_event_statistic(json, origin):
    write_text(json.get("description"), origin,
               comment="Description of event statistic",
               plural=True)
