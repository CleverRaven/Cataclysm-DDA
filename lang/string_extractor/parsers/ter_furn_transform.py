from ..write_text import write_text


def transform_result(result):
    """
    Canonicalize terrain furniture transform results.
    Transform result may be a single string, a list of strings,
    or a list of list of string and associated probability weights.
    """
    if type(result) is str:
        return result
    elif type(result) is list:
        # Using a list to reserve order and ensure stability across runs
        results = []
        for r in result:
            if type(r) is str and r not in results:
                results.append(r)
            elif type(r) is list and r[0] not in results:
                results.append(r[0])
        return ", ".join(results)


def parse_ter_furn_transform(json, origin):
    if "fail_message" in json:
        write_text(json["fail_message"], origin,
                   comment="Failure message of terrain furniture transform")
    if "terrain" in json:
        for terrain in json["terrain"]:
            if "message" in terrain:
                write_text(terrain["message"], origin,
                           comment="Message after transforming to \"{}\"".
                           format(transform_result(terrain["result"])))
    if "furniture" in json:
        for furniture in json["furniture"]:
            if "message" in furniture:
                write_text(furniture["message"], origin,
                           comment="Message after transforming to \"{}\""
                           .format(transform_result(furniture["result"])))
