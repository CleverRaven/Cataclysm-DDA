from ..write_text import write_text


def parse_weather_type(json, origin):
    if "name" in json:
        write_text(json["name"], origin, comment="Weather type name")
