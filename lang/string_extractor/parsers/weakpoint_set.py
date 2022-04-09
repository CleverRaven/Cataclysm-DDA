from ..write_text import write_text


def parse_weakpoint_set(json, origin):
    if "weakpoints" in json:
        for wp in json["weakpoints"]:
            id = ""
            if "id" in wp:
                id = wp["id"]
            if "name" in wp:
                comment = "Sentence fragment describing " \
                          "the \"{}\" weakpoint".format(id)
                write_text(wp["name"], origin, comment=comment)
            if "effects" in wp:
                for fx in wp["effects"]:
                    if "message" in fx:
                        comment = "Message describing the effect of hitting " \
                                  "the \"{}\" weakpoint".format(id)
                        write_text(fx["message"], origin, comment=comment)
