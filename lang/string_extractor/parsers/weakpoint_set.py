from ..helper import get_singular_name
from ..write_text import write_text


def parse_weakpoint_set(json, origin):
    for weakpoint in json.get("weakpoints", []):
        name = get_singular_name(weakpoint)
        write_text(weakpoint.get("name"), origin,
                   comment=[
                       "Description of weakpoint",
                       "ex. '[You hit the zombie in] a crack in the armor!'"])

        for effect in weakpoint.get("effects", []):
            write_text(effect.get("message"), origin,
                       comment=f"Effect of hitting '{name}' weakpoint")
