from ..write_text import write_text


def parse_scenario(json, origin):
    name = ""
    if "name" in json:
        name = json["name"]
        write_text(name, origin, context="scenario_male",
                   comment="Scenario name for male")
        write_text(name, origin, context="scenario_female",
                   comment="Scenario name for female")
    if name == "":
        name = json["id"]

    if "description" in json:
        write_text(json["description"], origin, context="scen_desc_male",
                   comment="Description of scenario \"{}\" for male"
                   .format(name))
        write_text(json["description"], origin, context="scen_desc_female",
                   comment="Description of scenario \"{}\" for female"
                   .format(name))

    if "start_name" in json:
        write_text(json["start_name"], origin, context="start_name",
                   comment="Starting location of scenario \"{}\""
                   .format(name))
