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
    write_text(json.get("fail_message"), origin,
               comment="Failure message of terrain furniture transform")

    for terrain in json.get("terrain", []):
        write_text(terrain.get("message"), origin,
                   comment="Message after transforming to '{}'".
                   format(transform_result(terrain["result"])))

    for furniture in json.get("furniture", []):
        write_text(furniture.get("message"), origin,
                   comment="Message after transforming to '{}'"
                   .format(transform_result(furniture["result"])))
