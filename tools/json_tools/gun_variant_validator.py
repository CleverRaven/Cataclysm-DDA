#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args.add_argument("-v", "--verbose", action="store_true",
                  help="extra debugging output")
args.add_argument("-c", "--combine", action="store_true",
                  help="check for guns that should be combined as variants")
args.add_argument("-i", "--identifier", action="store_true",
                  help="check for guns that do not share an identifier "
                       "with their magazines")
args.add_argument("--only-good-idents", action="store_true",
                  help="if checking identifiers, output valid identifiers")
args.add_argument("-n", "--name", action="store_true",
                  help="check for guns with non-descriptive names")
args.add_argument("--no-failure", action="store_true",
                  help="do not exit as a failure if any of the checks fail")
args.add_argument("-t", "--table", action="store_true",
                  help="output collected data as a table and exit")
args_dict = vars(args.parse_args())

all_jos = dict()
VERBOSE = args_dict["verbose"]
LOADED_TYPES = {"GUN", "MAGAZINE"}
INHERITED_KEYS = [
    "id",
    "type",
    "name",
    "ammo",
    "flags",
    "volume",
    "weight",
    "modes",
    "barrel_length",
    "ranged_damage",
    "reload",
    "longest_side",
    "pocket_data",
    "dispersion",
    "recoil",
    "barrel_volume",
    "blackpowder_tolerance",
]
WEIGHT_UNITS = {"kg": 1000, "g": 1}
VOLUME_UNITS = {"L": 1000, "ml": 1}
LENGTH_UNITS = {"mm": 1}
UNIT_KEYS = [
    "weight",
    "volume",
    "barrel_volume",
    "longest_side",
    "barrel_length",
]
UNIT_UNITS = {
    "weight": WEIGHT_UNITS,
    "volume": VOLUME_UNITS,
    "barrel_volume": VOLUME_UNITS,
    "longest_side": LENGTH_UNITS,
    "barrel_length": LENGTH_UNITS,
}
GUNS_BLACKLIST = {
    # Well named, faction-specific weapon, with weird ammo
    "brogyeki",
}
AMMO_BLACKLIST = {
    "arrow",
    "bolt",
    "chemical_spray",
    "fishspear",
    "nail",
    "paintball",
    "signal_flare",
    "water",
}
VARIANT_CHECK_BLACKLIST = {
    "pamd68rubik",
}
VARIANT_CHECK_PAIR_BLACKLIST = {
    # FIXME: fix and remove these
    ("m17", "m18"),
    ("pamd68", "pamd68mountable"),
    ("type99", "type99_sniper"),
    ("glock_20", "glock_40"),
}
IDENTIFIER_CHECK_BLACKLIST = {
    # FIXME: fix and remove these
    "rm103a_pistol",
    "rm11b_sniper_rifle",
    "rm2000_smg",
    "rm51_assault_rifle",
    "rm614_lmg",
    "rm88_battle_rifle",
    "bigun",
    "sw629",
    "sw_500",
    "m2browning",
    "american_180",
    "ruger_lcr_22",
    "sig_mosquito",
    "fn_p90",
    "p50",
    "fn_ps90",
    "fn_fal_semi",
    "m134",
    "m1a",
    "m240",
    "m60",
    "m60_semi",
    "m110a1",
    "ak308",
    "rfb_308",
    "steyr_scout",
    "tac338",
    "obrez",
    "smg_45",
    "hptjhp",
    "cx4",
    "hk_mp5_semi_pistol",
    "hk_g3",
    "briefcase_smg",
    "mp18",
    "mauser_c96",
    "mauser_m714",
    "ksub2000",
    "smg_9mm",
    "colt_ro635",
    "arx160",
    "sks",
    "draco",
    "mk47",
    "mosin44",
    "mosin91_30",
    "model_10_revolver",
    "mr73",
    "ruger_lcr_38",
    "sw_619",
    "m1903",
    "m1918",
    "m249",
    "m249_semi",
    "ak556",
    "minidraco556",
    "mdrx",
    "sapra",
    "raging_bull",
    "raging_judge",
    "type99",
    "type99_sniper",
    "m202_flash",
    "sw_610",
    "STI_DS_10",
    "kord",
    "hpt3895",
    "m2carbine",
    "smg_40",
    "rm228",
    "needlegun",
    "needlepistol",
    "rm298",
}
NAME_CHECK_BLACKLIST = {
    # FIXME: fix and remove these
    "walther_ppk",
    "kp32",
    "1895sbl",
    "bfr",
    "sharps",
    "m107a1",
    "as50",
    "tac50",
    "bfg50",
    "fn_p90",
    "m134",
    "hk_mp7",
    "obrez",
    "pressin",
    "m1911-460",
    "m2010",
    "weatherby_5",
    "win70",
    "ruger_pr",
    "2_shot_special",
    "cop_38",
    "model_10_revolver",
    "mr73",
    "ruger_lcr_38",
    "sw_619",
    "m1918",
    "acr_300blk",
    "iwi_tavor_x95_300blk",
    "sig_mcx_rattler_sbr",
    "bond_410",
    "colt_lightning",
    "colt_saa",
    "p226_357sig",
    "glock_31",
    "p320_357sig",
    "famas",
    "fs2000",
    "scar_l",
    "m231pfw",
    "brogyaga",
    "raging_bull",
    "raging_judge",
    "m202_flash",
    "saiga_410",
    "shotgun_410",
    "mgl",
    "pseudo_m203",
    "ak74",
    "kord",
    "colt_army",
    "atgm_launcher",
    "xedra_gun",
    "90two40",
    "glock_22",
    "px4_40",
    "sig_40",
    "hi_power_40",
    "walther_ppq_40",
    "hptjcp",
    "rm228",
    "AT4",
    "af2011a1_38super",
    "m1911a1_38super",
    "colt_navy",
    "ruger_arr",
    "plasma_gun",
    "bbgun",
    "LAW",
    "needlegun",
    "needlepistol",
    "RPG",
    "skorpion_61",
}
BAD_IDENTIFIERS = [
    "10mm",
    ".22",
    ".338",
    ".38",
    ".40",
    ".44",
    ".45",
    ".500",
    "5x50mm",
    "5.7mm",
    "7.7mm",
    "8x40mm",
    "9x19mm",
    "-round",
    "magazine",
    "stripper",
    "speedloader",
]
TYPE_DESCRIPTORS = [
    "automagnum",
    "blunderbuss",
    "carbine",
    "coilgun",
    # Not great, but weird can get a pass
    "combination gun",
    "flamethrower",
    "flintlock",
    # Special faction-specific invented weapons get a pass
    "FSP",
    "gatling gun",
    "hand cannon",
    "handgun",
    "HMG",
    # Special faction-specific invented weapons get a pass
    "HWP",
    "launcher",
    "lever gun",
    "LMG",
    "machine gun",
    # This is as close as you can get without a giant name
    "operational briefcase",
    "pistol",
    "railgun",
    "revolver",
    "rifle",
    "shotgun",
    "six-shooter",
    "SMG",
    "submachine gun",
    "trenchgun",
]

"""
JSON parsing functions
"""


# Helper to parse quantities
def parse_unit(val, stops):
    # already parsed
    if type(val) is not str:
        return val
    total = 0
    unit = 0
    sign = 1
    unit_start = -1
    eaten = 0
    while len(val[eaten:]) != 0:
        if val[eaten].isdigit():
            unit *= 10
            unit += ord(val[eaten]) - ord('0')
        elif val[eaten] == '-':
            sign = -1
        # Non-space, non-decimal characters must be part of a unit
        elif not val[eaten].isspace():
            unit_start = eaten if unit_start == -1 else unit_start
            # We found a valid unit
            if val[unit_start:eaten + 1] in stops:
                unit *= stops[val[unit_start:eaten + 1]]
                total += unit * sign
                unit = 0
                sign = 1
                unit_start = -1
        eaten += 1
    return total


# Parse the gun data represented in JSON to what we care about
def simplify_gun_data(jo):
    # We only care about what modes it has, and how many bullets
    if "modes" in jo:
        modes = []
        for mode in jo["modes"]:
            modes.append(mode[2])
        jo["modes"] = sorted(modes)
    else:
        jo["modes"] = [1]
    # We only care how much damage it deals
    if "ranged_damage" in jo:
        if type(jo["ranged_damage"]) is not dict:
            raise ValueError("Multiple damage types not supported")
        jo["ranged_damage"] = jo["ranged_damage"]["amount"]
    else:
        jo["ranged_damage"] = 0
    # And we only care what magazines it takes (or how much ammo, if no mags)
    # Oh god why
    if "pocket_data" not in jo:
        if VERBOSE:
            print("\tRemoving %s, conversion kit gun" % jo["id"])
        return False
    for data in jo["pocket_data"]:
        # It does not take magazines
        if data["pocket_type"] == "MAGAZINE":
            # First value in dict
            quantity = next(iter(data["ammo_restriction"].values()))
            # Check that ammo restriction is uniform
            if (len(data["ammo_restriction"]) != 1 and
               len(set(data["ammo_restriction"].values())) != 1):
                raise ValueError("Only one quantity allowed for ammo")
            # TODO: figure out a better way to specify this
            jo["magazines"] = [quantity]
            # TODO: combine with magazines?
            if "allowed_speedloaders" in data:
                jo["speedloaders"] = data["allowed_speedloaders"]
        # It takes magazines (so much simpler!)
        elif data["pocket_type"] == "MAGAZINE_WELL":
            jo["magazines"] = sorted(data["item_restriction"])
        else:
            raise ValueError("Guns with other pockets not supported")
    # We don't need all of this data
    del jo["pocket_data"]
    return True


# Remove irrelevant keys and clean data to ease later checks
def simplify_object(jo):
    # Only guns are subject to intense scrutiny
    # everything else can be considered already simple
    if jo["type"] != "GUN":
        return True
    if "id" not in jo or jo["id"] in GUNS_BLACKLIST:
        if VERBOSE:
            name = str(jo)
            if "id" in jo:
                name = jo["id"]
            if "abstract" in jo:
                name = jo["abstract"]
            print("\tRemoving %s, blacklisted or abstract" % name)
        return False

    req_keys = {"weight", "volume", "ammo", "id"}
    extra_keys = {"longest_side", "pocket_data", "ranged_damage", "modes",
                  "recoil", "dispersion", "name"}
    # Drop all the other keys
    removed = list(filter(lambda key: key not in req_keys | extra_keys,
                          jo.keys()))
    # Need to iterate over removed because we can't delete from dict in for
    for key in removed:
        del jo[key]

    # guns without the required keys can't be checked
    for key in filter(lambda key: key not in jo, req_keys):
        if VERBOSE:
            print("\tRemoving %s, lacking %s" % (jo["id"], key))
        return False
    # We don't care about fake guns
    if "flags" in jo and "PSEUDO" in jo["flags"]:
        if VERBOSE:
            print("\tRemoving %s, pseudo gun" % jo["id"])
        return False

    return simplify_gun_data(jo)


# Given a JSON object, extract the player-visible name
def name_of(jo):
    if type(jo["name"]) is dict:
        if "str" in jo["name"]:
            return jo["name"]["str"]
        else:
            return jo["name"]["str_sp"]
    return jo["name"]


"""
JSON loading functions
"""


def extract_jos(path):
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!")
            return None

        for jo in json_data:
            if type(jo) is not dict:
                print(f"Incorrectly formatted JSON data at {path}")
                print("Ensure that the file is a JSON file consisting"
                      " of an array of objects!")
                break

            if "type" not in jo or jo["type"] not in LOADED_TYPES:
                continue

            ident = jo["id"] if "id" in jo else jo["abstract"]
            all_jos[ident] = jo


def copy_from_extend(jo, data):
    if "extend" not in jo:
        return
    subobject = jo["extend"]
    for key in INHERITED_KEYS:
        if key not in subobject:
            continue
        for val in subobject[key]:
            if key not in data:
                data[key] = [val]
            else:
                data[key].append(val)


def copy_from_delete(jo, data):
    if "delete" not in jo:
        return
    subobject = jo["delete"]
    for key in INHERITED_KEYS:
        if key not in subobject:
            continue
        for val in subobject[key]:
            if key not in data:
                raise ValueError("Invalid delete on %s" % key)
            else:
                data[key].remove(val)


def copy_from_relative(jo, data):
    if "relative" not in jo:
        return
    subobject = jo["relative"]
    for key in INHERITED_KEYS:
        if key not in subobject:
            continue
        if key not in data:
            raise ValueError("Invalid relative on %s" % key)
        if key in UNIT_KEYS:
            data[key] += parse_unit(subobject[key], UNIT_UNITS[key])
        elif (key == "ranged_damage" and
              type(data[key]) is dict and
              type(subobject[key]) is dict):
            data[key]["amount"] += subobject[key]["amount"]
        elif key in {"dispersion"}:
            data[key] += subobject[key]
        else:
            raise ValueError("Relative for %s not supported" % key)


def copy_from_proportional(jo, data):
    if "proportional" not in jo:
        return
    subobject = jo["proportional"]
    for key in INHERITED_KEYS:
        if key not in subobject:
            continue
        if key not in data:
            raise ValueError("Invalid proportional on %s" % key)
        if (key == "ranged_damage" and
           type(data[key]) is dict and
           type(subobject[key]) is dict):
            data[key]["amount"] *= subobject[key]["amount"]
        elif key in UNIT_KEYS or key in {"reload", "dispersion"}:
            data[key] *= subobject[key]
        else:
            raise ValueError("Proportional for %s not supported" % key)


def do_copy_from(jo):
    data = {}
    if "copy-from" in jo and jo["copy-from"] in all_jos:
        data = do_copy_from(all_jos[jo["copy-from"]])

    for key in INHERITED_KEYS:
        if key in jo:
            if key in UNIT_KEYS:
                data[key] = parse_unit(jo[key], UNIT_UNITS[key])
            elif key == "name":
                data[key] = name_of(jo)
            else:
                data[key] = jo[key]

    copy_from_extend(jo, data)
    copy_from_delete(jo, data)
    copy_from_relative(jo, data)
    copy_from_proportional(jo, data)

    return data


def load_all_json(directory):
    for root, directories, filenames in os.walk(directory):
        for filename in filenames:
            if not filename.endswith(".json"):
                continue
            path = os.path.join(root, filename)
            extract_jos(path)

    for gun in all_jos.keys():
        new = do_copy_from(all_jos[gun])
        all_jos[gun] = new


"""
CHECK 1: Should a gun be a variant?
"""


# Are these guns similar enough they should be variants?
def guns_are_similar(jo, gun):
    # For volume, weight, length, etc
    # Compare if they're in the same or an adjacent bucket
    def in_range(lhs, rhs, bucket_size):
        return abs((lhs // bucket_size) - (rhs // bucket_size)) < 2

    def in_pct(lhs, rhs, pct):
        low = 1 - pct
        high = 1 + pct
        return ((lhs >= rhs * low and lhs <= rhs * high) or
                (rhs >= lhs * low and rhs <= lhs * high))

    # "symmetric difference" of sets - check if they completely intersect
    def sets_match(lhs, rhs):
        return len(set(lhs) ^ set(rhs)) == 0

    if (gun["id"] in VARIANT_CHECK_BLACKLIST or
       jo["id"] in VARIANT_CHECK_BLACKLIST):
        return False
    if not sets_match(jo["ammo"], gun["ammo"]):
        return False
    if not sets_match(jo["modes"], gun["modes"]):
        return False
    if not in_range(jo["volume"], gun["volume"], 250):
        return False
    if not in_range(jo["weight"], gun["weight"], 250):
        return False
    if not in_range(jo["ranged_damage"], gun["ranged_damage"], 2):
        return False
    # If longest side isn't specified for either, don't care about it
    if ("longest_side" in jo and "longest_side" in gun and
       not in_range(jo["longest_side"], gun["longest_side"], 50)):
        return False
    # Something to check if dispersion is similar
    # This can be pretty niche, mostly matters for handcrafted guns
    if not in_pct(jo["dispersion"] if "dispersion" in jo else 0,
                  gun["dispersion"] if "dispersion" in gun else 0, 0.05):
        return False
    # If one has speedloaders but the other doesn't
    if ("speedloaders" in jo) != ("speedloaders" in gun):
        return False
    # If both have different speedloaders
    if ("speedloaders" in jo and "speedloaders" in gun and
       not sets_match(jo["speedloaders"], gun["speedloaders"])):
        return False
    if not sets_match(jo["magazines"], gun["magazines"]):
        return False
    if not in_range(jo["reload"] if "reload" in jo else 0,
                    gun["reload"] if "reload" in gun else 0,
                    125):
        return False
    if ("barrel_length" in jo and "barrel_length" in gun and
        # this can be super sensitive to small changes in mm
        # at the low end, the bucket size is ~1-to-1 with damage
        # ranges that can be combined. Most things won't be
        # at the low end, and it seems to middle ranges of size
        # have a 10-20 mm separation per damage rather than 2-3
        # TODO: some sort of statistical analysis based on the ammo
        #       and DT of gun determines bucket size?
       not in_range(jo["barrel_length"], gun["barrel_length"], 10)):
        return False
    # TODO: get values for comparing these
    #if ("barrel_volume" in jo and "barrel_volume" in gun and
    #    not in_range(jo["barrel_volume"], gun["barrel_volume"], 125)):
    #    return False
    #if ("" in jo and "blackpower_tolerance" in gun and
    #    not in_range(jo["blackpower_tolerance"],
    #                 gun["blackpower_tolerance"], 125)):
    #    return False
    #if ("recoil" in jo and "recoil" in gun and
    #    not in_range(jo["recoil"], gun["recoil"], 125)):
    #    return False
    # TODO: more with flags?
    return True


def find_variants(all_guns):
    similar_guns = []
    for i in range(len(all_guns)):
        gun = all_guns[i]
        # We've already checked this gun against the guns before it,
        # so only check against guns after
        for compared in all_guns[i + 1:]:
            lid = gun["id"]
            rid = compared["id"]
            # We expect the same gun to be similar
            if (guns_are_similar(gun, compared) and
                not ((lid, rid) in VARIANT_CHECK_PAIR_BLACKLIST or
                     (rid, lid) in VARIANT_CHECK_PAIR_BLACKLIST)):
                similar_guns.append((gun, compared))
    return similar_guns


"""
CHECK 2: Are the default gun names linking the gun and it's mags
"""


# https://stackoverflow.com/questions/2892931/longest-common-substring-from-more-than-two-strings/2894073#2894073
# Used to find the shared token
def longest_common_substring(dat):
    substr = ''
    if len(dat) > 1 and len(dat[0]) > 0:
        for i in range(len(dat[0])):
            for j in range(len(dat[0]) - i + 1):
                if j > len(substr) and all(dat[0][i:i + j] in x for x in dat):
                    substr = dat[0][i:i + j]
    return substr


# Remove common but non-meaningful tokens from names
def clean_names(names):
    ret = []
    for name in names:
        for token in BAD_IDENTIFIERS:
            name = name.replace(token, "")
        ret.append(name)
    return ret


# Find the common token with a gun and the mags it takes
def common_token(names):
    names = clean_names(names)
    # Assume the longest common substring will be the "identifier"
    # leading/trailing whitespace isn't meaningful in identifiers
    common_token = longest_common_substring(names).strip()
    # It can't be a meaningful identifier if it's 1 character long
    if len(common_token) < 2:
        return None
    # Some common identifiers that don't work
    if common_token in {"11", "to", "if", "ip", "rifle", "carbine"}:
        return None
    return common_token


# Print the identifier info for a gun and it's magazines/speedloaders
def print_identifier_info(token, gun, magazines):
    mags = ""
    alt_mags = ""
    for mag in magazines:
        name = name_of(all_jos[mag]) if mag in all_jos else ""
        # Skip STANAG names
        if "STANAG" in name:
            alt_mags += mag + ", "
            continue
        mags += mag + f" ({name})" + ", "
    print(token)
    print("  " + gun["id"] + " (" + name_of(gun) + ")")
    if (len(mags) > 0):
        print("  " + mags)
    if (len(alt_mags) > 0):
        print("  " + alt_mags)


def find_identifiers(all_guns):
    # The guns with good identifiers linking the gun and it's magazines
    good_tokens = []
    # The guns with nothing in common or otherwise poor identifiers
    bad_tokens = []
    for gun in all_guns:
        if gun["id"] in IDENTIFIER_CHECK_BLACKLIST:
            continue
        # The names of the gun and all the magazines it accepts
        names = [name_of(gun)]
        # The identifiers of the magazines it accepts
        mags = []
        for mag in gun["magazines"]:
            if mag not in all_jos:
                if VERBOSE and not type(mag) is not str:
                    print("\tnot checking magazine %s for %s" %
                          (mag, gun["id"]))
                continue
            mags.append(mag)
        # A gun may/may not have speedloaders, which we treat like mags
        for mag in gun["speedloaders"] if "speedloaders" in gun else []:
            if mag not in all_jos:
                if VERBOSE:
                    print("\tnot checking speedloader %s for %s" %
                          (mag, gun["id"]))
                continue
            mags.append(mag)
        # Add all the magazine names in
        for mag in mags:
            name = name_of(all_jos[mag])
            # Skip STANAG, because those are uber generic
            if "STANAG" not in name:
                names.append(name)
        # If it didn't have any mags, we can skip it
        if len(names) < 2:
            continue
        # Check if they've got a good token
        token = common_token(names)
        if token is None:
            bad_tokens.append((gun, mags))
        else:
            good_tokens.append((gun, mags, token))
    return good_tokens, bad_tokens


"""
CHECK 3: names say what they are, and aren't just gobbledygook
"""


def find_bad_names(all_guns):
    bad_names = []
    for gun in all_guns:
        if gun["id"] in NAME_CHECK_BLACKLIST:
            continue
        name = name_of(gun)
        good = False
        for type_descriptor in TYPE_DESCRIPTORS:
            if type_descriptor in name:
                good = True
                break
        if not good:
            bad_names.append((gun["id"], name))
    return bad_names


"""
CHECK 4: No dupes!
"""


def find_dupe_names(all_guns):
    all_names = {}
    for gun in all_guns:
        name = name_of(gun) + str(sorted(gun["ammo"]))
        if name in all_names:
            all_names[name].append(gun)
        else:
            all_names[name] = [gun]
    for key, value in all_names.items():
        if len(value) == 1:
            continue
        out = "ERROR: Guns have the same name and ammo (" + key + "):"
        for gun in value:
            out += " (" + gun["id"] + "),"
        print(out)


"""
Driver functions
"""


# True on failure
def check_combination(all_guns):
    if not args_dict["combine"]:
        return False

    print("==== VARIANT COMBINATION ====")
    similar_guns = find_variants(all_guns)
    for pair in similar_guns:
        print("ERROR: Guns %s and %s are too similar and should be combined" %
              (pair[0]["id"], pair[1]["id"]))

    return len(similar_guns) > 0


def check_identifiers(all_guns):
    if not args_dict["identifier"]:
        return False

    print("====  IDENTIFIER  CHECK  ====")
    good_tokens, bad_tokens = find_identifiers(all_guns)
    if args_dict["only_good_idents"]:
        for good in good_tokens:
            print_identifier_info(good[2], good[0], good[1])
    else:
        for bad in bad_tokens:
            print_identifier_info("ERROR: No identifier:", bad[0], bad[1])
        if VERBOSE:
            good_token_list = ""
            for good in good_tokens:
                good_token_list += good[2] + " (" + good[0]["id"] + "), "
            print("Valid Identifiers: " + good_token_list)

    return len(bad_tokens) > 0


def check_names(all_guns):
    if not args_dict["name"]:
        return False
    print("====  DESCRIPTIVE NAMES  ====")
    bad_names = find_bad_names(all_guns)
    for bad in bad_names:
        print("ERROR: Gun %s (%s) lacks a descriptive name" % (bad[1], bad[0]))

    find_dupe_names(all_guns)

    return len(bad_names) > 0


csv = True


def insert_start_char():
    return "" if csv else "| "


def insert_separator():
    return "," if csv else " | "


def table(all_guns):
    # Don't report these
    skipped = {
        "type",
        "barrel_volume",
        "blackpowder_tolerance",
        "pocket_data",
        "recoil"
    }
    # If we don't have these, report this
    defaults = {
        "flags": [],
        "barrel_length": "???",
        "reload": 0,
        "longest_side": "???",
        "speedloaders": [],
        "dispersion": "???"
    }
    # Do the inherited keys, except the skipped ones, plus these
    keys = INHERITED_KEYS + ["magazines", "speedloaders"]
    # Print a header
    out = insert_start_char()
    for key in keys:
        if key in skipped:
            continue
        out += key
        out += insert_separator()
    print(out)
    # Markdown needs an extra line
    if not csv:
        out = insert_start_char()
        for key in keys:
            if key in skipped:
                continue
            out += "----"
            out += insert_separator()
        print(out)
    # Print all the gun data
    for gun in all_guns:
        out = insert_start_char()
        for key in keys:
            if key in skipped:
                continue
            if key in gun and type(gun[key]) is list:
                for val in gun[key]:
                    out += "'" + str(val) + "' "
            else:
                out += str(gun[key]) if key in gun else str(defaults[key])
            out += insert_separator()
        print(out)


def main():
    # Load all the JSON for the types we care about
    load_all_json(args_dict["dir"])
    # Find all the guns and simplify them
    all_guns = []
    for jo in all_jos.values():
        if jo["type"] != "GUN":
            continue
        if ("flags" in jo and
            ("PRIMITIVE_RANGED_WEAPON" in jo["flags"] or
             "PSEUDO" in jo["flags"])):
            continue
        if not simplify_object(jo):
            continue
        # blacklisted ammo
        if len(set(jo["ammo"]).intersection(AMMO_BLACKLIST)) != 0:
            continue
        all_guns.append(jo)
    if args_dict["table"]:
        table(all_guns)
        os.sys.exit(0)

    failed = False
    if check_combination(all_guns):
        failed = True
    if check_identifiers(all_guns):
        failed = True
    if check_names(all_guns):
        failed = True

    if failed and not args_dict["no_failure"]:
        os.sys.exit(1)
    else:
        os.sys.exit(0)


if __name__ == "__main__":
    main()
