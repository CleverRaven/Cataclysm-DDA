from ..write_text import write_text


# See `npc_template::load` in `npc.cpp`.
chatbin_snippets = [
    "<acknowledged>"
    "<camp_food_thanks>"
    "<camp_larder_empty>"
    "<camp_water_thanks>"
    "<cant_flee>"
    "<close_distance>"
    "<combat_noise_warning>"
    "<danger_close_distance>"
    "<done_mugging>"
    "<far_distance>"
    "<fire_bad>"
    "<fire_in_the_hole_h>"
    "<fire_in_the_hole>"
    "<general_danger_h>"
    "<general_danger>"
    "<heal_self>"
    "<hungry>"
    "<im_leaving_you>"
    "<its_safe_h>"
    "<its_safe>"
    "<keep_up>"
    "<kill_npc_h>"
    "<kill_npc>"
    "<kill_player_h>"
    "<let_me_pass>"
    "<lets_talk>"
    "<medium_distance>"
    "<monster_warning_h>"
    "<monster_warning>"
    "<movement_noise_warning>"
    "<need_batteries>"
    "<need_booze>"
    "<need_fuel>"
    "<no_to_thorazine>"
    "<run_away>"
    "<speech_warning>"
    "<thirsty>"
    "<wait>"
    "<warn_sleep>"
    "<yawn>"
    "<yes_to_lsd>"
    "snip_pulp_zombie"
    "snip_heal_player"
    "snip_mug_dontmove"
    "snip_wound_infected"
    "snip_wound_bite"
    "snip_radiation_sickness"
    "snip_bleeding"
    "snip_bleeding_badly"
    "snip_lost_blood"
    "snip_bye"
    "snip_consume_cant_accept"
    "snip_consume_cant_consume"
    "snip_consume_rotten"
    "snip_consume_eat"
    "snip_consume_need_item"
    "snip_consume_med"
    "snip_consume_nocharge"
    "snip_consume_use_med"
    "snip_give_nope"
    "snip_give_to_hallucination"
    "snip_give_cancel"
    "snip_give_dangerous"
    "snip_give_wield"
    "snip_give_weapon_weak"
    "snip_give_carry"
    "snip_give_carry_cant"
    "snip_give_carry_cant_few_space"
    "snip_give_carry_cant_no_space"
    "snip_give_carry_too_heavy"
    "snip_wear"
]


def parse_npc(json, origin):
    gender = "an"
    if "gender" in json:
        gender = "a {}".format(json["gender"])
    comment = json.get("//", None)
    if "name_unique" in json:
        write_text(json["name_unique"], origin,
                   comment=["Unique name of {} NPC".format(gender), comment])
    if "name_suffix" in json:
        write_text(json["name_suffix"], origin,
                   comment=["Name suffix of {} NPC".format(gender), comment])
    for snip in chatbin_snippets:
        if snip in json:
            write_text(json[snip], origin,
                       comment=["Chatbin snippet {} of {} NPC"
                                .format(snip, gender), comment])
