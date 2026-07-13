from ..write_text import write_text


def parse_widget(json, origin):
    name = json["id"]

    write_text(json.get("label"), origin,
               comment=f"Label of UI widget '{name}'")

    write_text(json.get("description"), origin,
               comment=f"Description of UI widget '{name}'")

    write_text(json.get("string"), origin,
               comment=f"Text in UI widget '{name}'")

    for phrase in json.get("phrases", []):
        write_text(phrase.get("text"), origin,
                   comment=f"Text in portion of UI widget '{name}'")

    if "default_clause" in json:
        write_text(json["default_clause"].get("text"), origin,
                   comment=f"Default clause of UI widget '{name}'")

    for clause in json.get("clauses", []):
        write_text(clause.get("text"), origin,
                   comment=f"Clause text of UI widget '{name}'")
