#!/usr/bin/env python3
"""
Print table of monster -> proficiencies from dissecting.

Does resolve inheritance implemented by copy-from
Prints EMPTY_PROFS_STRING if monster dissection provides no proficiencies,
"NO PROFS" by default.
"""

from util import import_data

# "NO PROFS" by default
EMPTY_PROFS_STRING = "NO PROFS"
# True: print only monsters that have give no proficiencies upon dissecting
ONLY_NO_PROFS = False
# can be 0 or 1
SORT_BY_COL = 1


def list_2d_to_table(list_2d, header):
    """return table as string from data in list_2d with header.

    list_2d is list of rows, header is list of headers.
    Every row must have same number of elements and same number of elements
    as the header has.

    Example:
    >>> print(list_2d_to_table(
            (
                (1, 2, 3),
                (4, 5, 6),
                ("hi there", "0", 5777)
            ),
            ("h1", "h2", "h3")
        ))
        ┌────────┬──┬────┐
        │h1      │h2│h3  │
        ├────────┼──┼────┤
        │       1│ 2│   3|
        │       4│ 5│   6|
        │hi there│ 0│5777|
        └────────┴──┴────┘
    """

    lens_list = []
    column_count = len(header)
    for column in range(column_count):
        len_max = len(header[column])
        for line in list_2d:
            assert(column_count == len(line))
            len_max = max(len_max, len(str(line[column])))
        lens_list.append(len_max)

    table = "┌%s┐\n" % "┬".join("─" * d for d in lens_list)

    table += "│"
    separator = ""
    for column in range(column_count):
        table += separator
        table += ("%%-%ds" % lens_list[column]) % header[column]
        separator = "│"
    table += "│\n"

    table += "├%s┤\n" % "┼".join("─" * d for d in lens_list)

    for line in list_2d:
        separator = ""
        for cell, width in zip(line, lens_list):
            table += ("│%%-%ds" % width) % cell
        table += "|\n"

    table += "└%s┘\n" % "┴".join("─" * d for d in lens_list)
    return table


def get_profs(monster):
    if "families" in monster:
        return monster["families"]
    elif "copy-from" in monster:
        return get_profs(monsters[monster["copy-from"]])
    else:
        return ()


if __name__ == "__main__":
    # make it dict for fast lookup:
    monsters = {}
    for data in import_data()[0]:
        if data["type"] == "MONSTER":

            if "id" in data:
                name = data["id"]
                # "This monster exists only for weakpoint testing purposes."
                if name == "weakpoint_mon":
                    continue
            elif "abstract" in data:
                name = data["abstract"]
            else:
                raise ValueError(
                    f"'id' nor 'abstract' defined for json entry:\n{data}")
            if name in monsters:
                raise ValueError(f"duplicate: {name}")
            monsters[name] = data

    monster_count = 0
    no_profs = 0
    results = []
    for name, monster in monsters.items():
        profs = get_profs(monster)
        if ONLY_NO_PROFS:
            if not profs:
                results.append((name,))
                no_profs += 1
        else:
            if not profs:
                profs = (EMPTY_PROFS_STRING,)
                no_profs += 1

            results.append((name, ", ".join(profs)))
        monster_count += 1

    if ONLY_NO_PROFS:
        results.sort()
        print(list_2d_to_table(results, ("monster id/abstract",)))
    else:
        results.sort(key=lambda line: (
            line[SORT_BY_COL], line[0 if SORT_BY_COL else 1]))
        print(list_2d_to_table(results, ("monster id/abstract",
                               "proficiencies from dissecting")))

    print(f"monster count: {monster_count}")
    print(f"no profs: {no_profs}")
