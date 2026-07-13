from ..helper import get_singular_name
from ..write_text import write_text


def parse_mutation_category(json, origin):
    name = get_singular_name(json)

    write_text(json["name"], origin, comment="Mutation class name")

    for key in [
        "mutagen_message",
        "iv_message",
        "iv_sleep_message",
        "iv_sound_message",
        "junkie_message"
    ]:
        write_text(json.get(key), origin,
                   comment=f"Message of mutation class '{name}'")

    write_text(json.get("memorial_message"), origin,
               context="memorial_male",
               comment="Memorial message of mutation class "
               f"'{name}' for male")
    write_text(json.get("memorial_message"), origin,
               context="memorial_female",
               comment="Memorial message of mutation class "
               f"'{name}' for female")
