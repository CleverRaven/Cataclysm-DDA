from ..write_text import write_text


def parse_weather_type(json, origin):
    write_text(json.get("name"), origin, comment="Weather type name")
