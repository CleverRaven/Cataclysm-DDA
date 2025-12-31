from ..helper import get_singular_name
from ..write_text import write_text


def parse_scenario(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin, context="scenario_male",
               comment="Scenario name for male")
    write_text(json.get("name"), origin, context="scenario_female",
               comment="Scenario name for female")

    write_text(json.get("description"), origin, context="scen_desc_male",
               comment=f"Description of scenario '{name}' for male")
    write_text(json.get("description"), origin, context="scen_desc_female",
               comment=f"Description of scenario '{name}' for female")

    write_text(json.get("start_name"), origin, context="start_name",
               comment=f"Starting location of scenario '{name}'")
