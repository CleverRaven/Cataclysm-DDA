from ..write_text import write_text


def parse_difficulty_impact(json, origin):
    comment = ["Aspect of gameplay affected by difficulty"]
    if "//" in json:
        comment.append(json["//"])
    write_text(json["name"], origin, comment=comment)
