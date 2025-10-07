import os
import json

# If you want to run this script you will likely need to edit this path
Magiclysm_path ="../../../data/mods/Magiclysm"

tier_0_scroll_list = []
tier_1_scroll_list = []
tier_2_scroll_list = []
tier_3_scroll_list = []
tier_0_spell_list = []
tier_1_spell_list = []
tier_2_spell_list = []
tier_3_spell_list = []
spell_data = []
custom_spell_data = []

# Helper function to make sure we don't read a spell twice
def scroll_not_yet_read(scroll_name):
    for scroll in tier_0_scroll_list:
        if scroll_name == scroll:
            return False
    for scroll in tier_1_scroll_list:
        if scroll_name == scroll:
            return False
    for scroll in tier_2_scroll_list:
        if scroll_name == scroll:
            return False
    for scroll in tier_3_scroll_list:
        if scroll_name == scroll:
            return False
    return True

# Don't load data we want to overwrite
def should_read_spell(spell_id):
    for spell in custom_spell_data:
        if spell_id == spell["id"]:
            return False
    return True

# Reads what scrolls exists in Magiclysm and what tier they are
def read_item_group(path):
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            failures.add(
                "Json Decode Error at:\n" + path
            )
            return None
        for jo in json_data:
            if isinstance(jo, dict):
                if ( jo["id"] == "spell_scroll_tier_0" ):
                    for scroll in jo["items"]:
                        if scroll_not_yet_read(scroll[0]):
                            tier_0_scroll_list.append(scroll[0])
                if ( jo["id"] == "spell_scroll_tier_1" ):
                    for scroll in jo["items"]:
                        if scroll_not_yet_read(scroll[0]):
                            tier_1_scroll_list.append(scroll[0])
                if ( jo["id"] == "spell_scroll_tier_2" ):
                    for scroll in jo["items"]:
                        if scroll_not_yet_read(scroll[0]):
                            tier_2_scroll_list.append(scroll[0])
                if ( jo["id"] == "spell_scroll_tier_3" ):
                    for scroll in jo["items"]:
                        if scroll_not_yet_read(scroll[0]):
                            tier_3_scroll_list.append(scroll[0])
    return

# Reads which spell is written on the scrolls we read earlier
def read_spell_scrolls(path):
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            failures.add(
                "Json Decode Error at:\n" + path
            )
            return None
        for jo in json_data:
            if isinstance(jo, dict):
                for scroll_name in tier_0_scroll_list:
                    if (
                        "id" in jo and jo["id"] == scroll_name
                    ):
                        tier_0_spell_list.append(jo["use_action"]["spells"][0])
                for scroll_name in tier_1_scroll_list:
                    if (
                        "id" in jo and jo["id"] == scroll_name
                    ):
                        tier_1_spell_list.append(jo["use_action"]["spells"][0])
                for scroll_name in tier_2_scroll_list:
                    if (
                        "id" in jo and jo["id"] == scroll_name
                    ):
                        tier_2_spell_list.append(jo["use_action"]["spells"][0])
                for scroll_name in tier_3_scroll_list:
                    if (
                        "id" in jo and jo["id"] == scroll_name
                    ):
                        tier_3_spell_list.append(jo["use_action"]["spells"][0])
    return

# Reads custom spell data from ./spell_data.txt
# Allows us to disable spells that should not be given,
# Add spells from our own mod,
# Or overwrite the default values to instead be whatever we want them to be
def read_custom_spell_data(path):
    with open(path, "r", encoding="utf-8") as json_file:
        json_data = json.load(json_file)
        for jo in json_data:
            custom_spell_data.append(jo)
    return


# Loops through all Magiclysm spells (except attunement spells)
# If their scroll exists in the loot groups, we save data about them
# We use their tier and difficulty to calculate a 'spell level' for the spells as well
def read_spell_data(path):
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            failures.add(
                "Json Decode Error at:\n" + path
            )
            return None
        for jo in json_data:
            if isinstance(jo, dict) and "id" in jo and "name" in jo and "description" in jo and should_read_spell(jo["id"]):
                difficulty = int(jo["difficulty"] if "difficulty" in jo else 0)
                new_spell_data = {
                  "id": str(jo["id"]),
                  "safe_id": str(jo["id"]).replace("-", "_"),
                  "level": -1,
                  "name": str(jo["name"]),
                  "description": str(jo["description"])
                }
                # Deals with if the name is not just a string.
                # Haven't figgured out a way to automatically deal with spells that contains the string "str". So we just deal with them manually.
                if "str" in jo["name"] and new_spell_data["id"] != "windstrike" and new_spell_data["id"] != "demon_possession_aura":
                    new_spell_data["name"] = str(jo["name"]["str"]) 
                for spell_name in tier_0_spell_list:
                    if (
                        jo["id"] == spell_name
                    ):
                        if difficulty <= 1:
                            new_spell_data["level"] = 0
                        elif difficulty <= 4:
                            new_spell_data["level"] = 1
                        else:
                            new_spell_data["level"] = 2
                        spell_data.append(new_spell_data)
                for spell_name in tier_1_spell_list:
                    if (
                        jo["id"] == spell_name
                    ):
                        new_spell_data["tier"] = 1
                        if difficulty <= 4:
                            new_spell_data["level"] = 3
                        else:
                            new_spell_data["level"] = 4
                        spell_data.append(new_spell_data)
                for spell_name in tier_2_spell_list:
                    if (
                        jo["id"] == spell_name
                    ):
                        new_spell_data["tier"] = 2
                        if difficulty <= 5:
                            new_spell_data["level"] = 5
                        else:
                            new_spell_data["level"] = 6
                        spell_data.append(new_spell_data)
                for spell_name in tier_3_spell_list:
                    if (
                        jo["id"] == spell_name
                    ):
                        new_spell_data["tier"] = 3
                        if difficulty <= 5:
                            new_spell_data["level"] = 7
                        else:
                            new_spell_data["level"] = 8
                        spell_data.append(new_spell_data)
    return

# Merges spell_data and custom_spell_data
# Does not merge spells that are disabled
def merge_spell_data():
    for spell in custom_spell_data:
        if "disabled" in spell and spell["disabled"] == True:
            continue
        else:
            spell_data.append(spell)
    return


# Writes the dialogue used to select spells to learn
def write_learn_spell(level):
    main_topic = {
        "type": "talk_topic",
        "id": "TALK_SORCERER_LEARN_SPELL_" + str( level ),
        "dynamic_line": "<u_val:sorcerer_level_" + str( level ) + "_spells_known> / <u_val:sorcerer_level_" + str( level ) + "_spells_known_slots> level " + str( level ) + " spells known.",
        "speaker_effect": { "effect": { "math": [ "curent_spell_slot = "+str( level ) ] } },
        "responses": [
            { "text": "Go Back.", "topic": "TALK_SORCERER_MENU_MAIN" },
            { "text": "Quit.", "topic": "TALK_DONE" }
            ]
        }
    all_topics = []
    for spell in spell_data:
        if int(spell["level"]) <= level:
            response = {
            "condition": { "math": [ "u_used_spell_slot_for_" + spell["safe_id"]+ " == 0" ] },
            "text": "Learn " + spell["name"] + " ( level "+ str(spell["level"]) + " )",
            "topic": "TALK_SORCERER_LEARN_SPELL_" + spell["id"] + "_at_level_" + str( level )
            }
            main_topic["responses"].append(response)
            new_other_topic = {
                "type": "talk_topic",
                "id": "TALK_SORCERER_LEARN_SPELL_" + spell["id"] + "_at_level_" + str( level ),
                "dynamic_line": spell["name"] + ": " + spell["description"],
                "responses": [
                  {
                    "text": "Select Spell.",
                    "topic": "TALK_SORCERER_MENU_MAIN",
                    "effect": [
                      { "math": [ "u_spell_level('" + spell["id"] + "') = u_current_sorcerer_level" ] },
                      { "math": [ "u_used_spell_slot_for_"+spell["safe_id"], "=", str( max( level, 0.5 ) ) ] },
                      { "math": [ "u_sorcerer_level_"+str(level)+"_spells_known", "++" ] },
                    ]
                  },
                  { "text": "Go Back.", "topic": "TALK_SORCERER_MENU_MAIN" },
                  { "text": "Quit.", "topic": "TALK_DONE" }
                ]
            }
            all_topics.append(new_other_topic)
    all_topics.append(main_topic)
    path = "../generated_code/learn_spell"
    isExist = os.path.exists(path)
    if not isExist:
        os.makedirs(path)
    with open(path + "/learn_spells_level_" + str( level ) + ".json", mode="wt") as f:
        f.write(json.dumps(all_topics, indent=2))

# Writes dialogue options to forget a spell known
def write_forget_spell():
    main_topic = {
        "type": "talk_topic",
        "id": "TALK_SORCERER_FORGET_SPELL",
        "dynamic_line": "Pick a spell to forget",
        "responses": [
            { "text": "Go Back.", "topic": "TALK_SORCERER_MENU_MAIN" },
            { "text": "Quit.", "topic": "TALK_DONE" }
            ]
        }
    for spell in spell_data:
        response = {
            "condition": { "math": [ "u_used_spell_slot_for_"+spell["safe_id"]+" > 0" ] },
            "text": "Forget " + spell["name"] + " ( level <u_val:used_spell_slot_for_" + spell["safe_id"] + "> )",
            "topic": "TALK_SORCERER_MENU_MAIN",
            "effect": [
                { "math": [ "u_spell_level('" + spell["id"] + "') = -1" ] },
                { "run_eocs": "EOC_sorcerer_forget_spell_refund_slots", "variables": { "forgotten_spell_level": { "u_val": "used_spell_slot_for_"+spell["safe_id"] } } },
                { "math": [ "u_used_spell_slot_for_"+spell["safe_id"]+" = 0" ] },
                { "math": [ "u_sorcerer_forget_spell_charge", "--" ] }
            ]
        }
        main_topic["responses"].append(response)
    path = "../generated_code/"
    isExist = os.path.exists(path)
    if not isExist:
        os.makedirs(path)
    with open(path + "/forget_spells.json", mode="wt") as f:
        f.write(json.dumps(main_topic, indent=2))


# Writes dialogue options to pick a favourite spell
def write_pick_favourite_spell():
    main_topic = {
        "type": "talk_topic",
        "id": "TALK_SORCERER_PICK_FAV_SPELL",
        "dynamic_line": "Pick a spell you have a special affinity for to gain a small boost in caster level for said spell.  The boost will quickly become obselete, but will help you get started on your journey.",
        "responses": [
            { "text": "Go Back.", "topic": "TALK_SORCERER_MENU_MAIN" },
            { "text": "Quit.", "topic": "TALK_DONE" }
            ]
        }
    for spell in spell_data:
        if spell["level"] <= 1:
            response = {
                "condition": { "math": [ "u_used_spell_slot_for_"+spell["safe_id"]+" > 0" ] },
                "text": "Pick " + spell["name"] + " ( level <u_val:used_spell_slot_for_" + spell["safe_id"] + "> )",
                "topic": "TALK_SORCERER_MENU_MAIN",
                "effect": [
                    { "math": [ "u_spell_level('" + spell["id"] + "') = max(3, u_spell_level('" + spell["id"] + "') )" ] },
                    { "math": [ "u_available_favourite_spells", "--" ] }
                ]
            }
            main_topic["responses"].append(response)
    path = "../generated_code/bloodlines/standard"
    isExist = os.path.exists(path)
    if not isExist:
        os.makedirs(path)
    with open(path + "/pick_favourite_spell.json", mode="wt") as f:
        f.write(json.dumps(main_topic, indent=2))

# Writes the EOCs that deals with keeping known spells atleast at the same level as your sorcerer level
def write_level_up_spells():
    main_topic = {
        "type": "effect_on_condition",
        "id": "EOC_level_up_sorcerer_spells",
        "effect": []
        }
    for spell in spell_data:
        effect = {
            "run_eocs": {
              "id": "sorcerer_level_up_"+spell["safe_id"],
              "condition": { "math": [ "u_used_spell_slot_for_"+spell["safe_id"]+" > 0" ] },
              "effect": [ { "math": [ "u_spell_level('" + spell["id"] + "') = max(u_current_sorcerer_level, u_spell_level('" + spell["id"] + "') )" ] } ]
            }
        }
        main_topic["effect"].append(effect)
    path = "../generated_code/"
    isExist = os.path.exists(path)
    if not isExist:
        os.makedirs(path)
    with open(path + "/level_up_spells.json", mode="wt") as f:
        f.write(json.dumps(main_topic, indent=2))


# Starts calling functions
read_item_group(Magiclysm_path + "/itemgroups/spellbooks.json")
read_spell_scrolls(Magiclysm_path + "/items/spell_scrolls.json")
read_custom_spell_data("spell_data.txt")

for root, directories, filenames in os.walk(Magiclysm_path + "/Spells"):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            read_spell_data(path)


merge_spell_data()
# Sort our spell data with higher leveled spells first
# This is to make sure that when you want to learn a high level spell slot, higher leveld spells are shown first            
spell_data = sorted(spell_data, key=lambda x: x["level"], reverse = True)


for number in range(0,10):
    write_learn_spell(number)

write_forget_spell()
write_level_up_spells()
write_pick_favourite_spell()

# Writes out the generated spell data for debugging purposes
path = "../generated_code/"
isExist = os.path.exists(path)
if not isExist:
    os.makedirs(path)
with open(path + "/spell_data_dump.txt", mode="wt") as f:
    f.write(json.dumps(spell_data, indent=2))

input("Generation successfull. Press enter to continue.")