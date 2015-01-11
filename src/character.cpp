#include "character.h"
#include "game.h"

Character::Character()
{
    Creature::set_speed_base(100);
};

Character::~Character()
{
};

field_id Character::bloodType() const
{
    return fd_blood;
}
field_id Character::gibType() const
{
    return fd_gibs_flesh;
}

bool Character::is_warm() const
{
    return true; // TODO: is there a mutation (plant?) that makes a npc not warm blooded?
}

const std::string &Character::symbol() const
{
    static const std::string character_symbol("@");
    return character_symbol;
}

void Character::store(JsonOut &jsout) const
{
    Creature::store( jsout );
    // Add members of this class here:
}

void Character::load(JsonObject &jsin)
{
    Creature::load( jsin );
    // Add members of this class here:
}

bool Character::move_effects()
{
    if (has_effect("downed")) {
        if (rng(0, 40) > get_dex() + int(get_str() / 2)) {
            add_msg_if_player(_("You struggle to stand."));
        } else {
            add_msg_player_or_npc(m_good, _("You stand up."),
                                    _("<npcname> stands up."));
            remove_effect("downed");
        }
        return false;
    }
    if (has_effect("webbed")) {
        if (x_in_y(get_str(), 6 * get_effect_int("webbed"))) {
            add_msg_player_or_npc(m_good, _("You free yourself from the webs!"),
                                    _("<npcname> frees themselves from the webs!"));
            remove_effect("webbed");
        } else {
            add_msg_if_player(_("You try to free yourself from the webs, but can't get loose!"));
        }
        return false;
    }
    if (has_effect("lightsnare")) {
        if(x_in_y(get_str(), 12) || x_in_y(get_dex(), 8)) {
            remove_effect("lightsnare");
            add_msg_player_or_npc(m_good, _("You free yourself from the light snare!"),
                                    _("<npcname> frees themselves from the light snare!"));
            item string("string_36", calendar::turn);
            item snare("snare_trigger", calendar::turn);
            g->m.add_item_or_charges(xpos(), ypos(), string);
            g->m.add_item_or_charges(xpos(), ypos(), snare);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the light snare, but can't get loose!"));
        }
        return false;
    }
    if (has_effect("heavysnare")) {
        if(x_in_y(get_str(), 32) || x_in_y(get_dex(), 16)) {
            remove_effect("heavysnare");
            add_msg_player_or_npc(m_good, _("You free yourself from the heavy snare!"),
                                    _("<npcname> frees themselves from the heavy snare!"));
            item rope("rope_6", calendar::turn);
            item snare("snare_trigger", calendar::turn);
            g->m.add_item_or_charges(xpos(), ypos(), rope);
            g->m.add_item_or_charges(xpos(), ypos(), snare);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the heavy snare, but can't get loose!"));
        }
        return false;
    }
    if (has_effect("beartrap")) {
        /* Real bear traps can't be removed without the proper tools or immense strength; eventually this should
           allow normal players two options: removal of the limb or removal of the trap from the ground
           (at which point the player could later remove it from the leg with the right tools). 
           As such we are currently making it a bit easier for players and NPC's to get out of bear traps.
        */
        if(x_in_y(get_str(), 100)) {
            remove_effect("beartrap");
            add_msg_player_or_npc(m_good, _("You free yourself from the bear trap!"),
                                    _("<npcname> frees themselves from the bear trap!"));
            item beartrap("beartrap", calendar::turn);
            g->m.add_item_or_charges(xpos(), ypos(), beartrap);
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the bear trap, but can't get loose!"));
        }
        return false;
    }
    if (has_effect("crushed")) {
        // Strength helps in getting free, but dex also helps you worm your way out of the rubble
        if(x_in_y(get_str() + get_dex() / 4, 100)) {
            remove_effect("crushed");
            add_msg_player_or_npc(m_good, _("You free yourself from the rubble!"),
                                    _("<npcname> frees themselves from the rubble!"));
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the rubble, but can't get loose!"));
        }
        return false;
    }
    if (has_effect("amigara")) {
        int curdist = 999, newdist = 999;
        for (int cx = 0; cx < SEEX * MAPSIZE; cx++) {
            for (int cy = 0; cy < SEEY * MAPSIZE; cy++) {
                if (g->m.ter(cx, cy) == t_fault) {
                    int dist = rl_dist(cx, cy, xpos(), ypos());
                    if (dist < curdist) {
                        curdist = dist;
                    }
                    dist = rl_dist(cx, cy, xpos(), ypos());
                    if (dist < newdist) {
                        newdist = dist;
                    }
                }
            }
        }
        if (newdist > curdist) {
            add_msg_if_player(m_info, _("You cannot pull yourself away from the faultline..."));
            return false;
        }
    }
    // Below this point are things that allow for movement if they succeed
    
    // Currently we only have one thing that forces movement if you succeed, should we get more
    // than this will need to be reworked to only have success effects if /all/ checks succeed
    if (has_effect("in_pit")) {
        if (rng(0, 40) > get_str() + int(get_dex() / 2)) {
            add_msg_if_player(m_bad, _("You try to escape the pit, but slip back in."));
            return false;
        } else {
            add_msg_player_or_npc(m_good, _("You escape the pit!"),
                                    _("<npcname> escapes the pit!"));
            remove_effect("in_pit");
        }
    }
    return Creature::move_effects();
}
void Character::add_effect(efftype_id eff_id, int dur, body_part bp, bool permanent, int intensity)
{
    Creature::add_effect(eff_id, dur, bp, permanent, intensity);
}

void Character::recalc_hp()
{
    int new_max_hp[num_hp_parts];
    for( auto &elem : new_max_hp ) {
        elem = 60 + str_max * 3;
        if (has_trait("HUGE")) {
            // Bad-Huge doesn't quite have the cardio/skeletal/etc to support the mass,
            // so no HP bonus from the ST above/beyond that from Large
            elem -= 6;
        }
        // You lose half the HP you'd expect from BENDY mutations.  Your gelatinous
        // structure can help with that, a bit.
        if (has_trait("BENDY2")) {
            elem += 3;
        }
        if (has_trait("BENDY3")) {
            elem += 6;
        }
        // Only the most extreme applies.
        if (has_trait("TOUGH")) {
            elem *= 1.2;
        } else if (has_trait("TOUGH2")) {
            elem *= 1.3;
        } else if (has_trait("TOUGH3")) {
            elem *= 1.4;
        } else if (has_trait("FLIMSY")) {
            elem *= .75;
        } else if (has_trait("FLIMSY2")) {
            elem *= .5;
        } else if (has_trait("FLIMSY3")) {
            elem *= .25;
        }
        // Mutated toughness stacks with starting, by design.
        if (has_trait("MUT_TOUGH")) {
            elem *= 1.2;
        } else if (has_trait("MUT_TOUGH2")) {
            elem *= 1.3;
        } else if (has_trait("MUT_TOUGH3")) {
            elem *= 1.4;
        }
    }
    if (has_trait("GLASSJAW"))
    {
        new_max_hp[hp_head] *= 0.8;
    }
    for (int i = 0; i < num_hp_parts; i++)
    {
        hp_cur[i] *= (float)new_max_hp[i]/(float)hp_max[i];
        hp_max[i] = new_max_hp[i];
    }
}


// This must be called when any of the following change:
// - effects
// - bionics
// - traits
// - underwater
// - clothes
// With the exception of clothes, all changes to these character attributes must
// occur through a function in this class which calls this function. Clothes are
// typically added/removed with wear() and takeoff(), but direct access to the
// 'wears' vector is still allowed due to refactor exhaustion.
void Character::recalc_sight_limits()
{
    sight_max = 9999;
    sight_boost = 0;
    sight_boost_cap = 0;

    // Set sight_max.
    if (has_effect("blind")) {
        sight_max = 0;
    } else if (has_effect("in_pit") ||
            (has_effect("boomered") && (!(has_trait("PER_SLIME_OK")))) ||
            (underwater && !has_bionic("bio_membrane") &&
                !has_trait("MEMBRANE") && !worn_with_flag("SWIM_GOGGLES") &&
                !has_trait("CEPH_EYES") && !has_trait("PER_SLIME_OK") ) ) {
        sight_max = 1;
    } else if (has_active_mutation("SHELL2")) {
        // You can kinda see out a bit.
        sight_max = 2;
    } else if ( (has_trait("MYOPIC") || has_trait("URSINE_EYE")) &&
            !is_wearing("glasses_eye") && !is_wearing("glasses_monocle") &&
            !is_wearing("glasses_bifocal") && !has_effect("contacts")) {
        sight_max = 4;
    } else if (has_trait("PER_SLIME")) {
        sight_max = 6;
    }

    // Set sight_boost and sight_boost_cap, based on night vision.
    // (A player will never have more than one night vision trait.)
    sight_boost_cap = 12;
    // Debug-only NV, by vache's request
    if (has_trait("DEBUG_NIGHTVISION")) {
        sight_boost = 59;
        sight_boost_cap = 59;
    } else if (has_nv() || has_trait("NIGHTVISION3") || has_trait("ELFA_FNV") || is_wearing("rm13_armor_on") ||
      (has_trait("CEPH_VISION")) ) {
        // Yes, I'm breaking the cap. I doubt the reality bubble shrinks at night.
        // BIRD_EYE represents excellent fine-detail vision so I think it works.
        if (has_trait("BIRD_EYE")) {
            sight_boost = 13;
        }
        else {
        sight_boost = sight_boost_cap;
        }
    } else if (has_trait("ELFA_NV")) {
        sight_boost = 6; // Elf-a and Bird eyes shouldn't coexist
    } else if (has_trait("NIGHTVISION2") || has_trait("FEL_NV") || has_trait("URSINE_EYE")) {
        if (has_trait("BIRD_EYE")) {
            sight_boost = 5;
        }
         else {
            sight_boost = 4;
         }
    } else if (has_trait("NIGHTVISION")) {
        if (has_trait("BIRD_EYE")) {
            sight_boost = 2;
        }
        else {
            sight_boost = 1;
        }
    }
}

bool Character::has_trait(const std::string &b) const
{
    // Look for active mutations and traits
    return my_mutations.find( b ) != my_mutations.end();
}

bool Character::has_base_trait(const std::string &b) const
{
    // Look only at base traits
    return my_traits.find( b ) != my_traits.end();
}

void Character::toggle_trait(const std::string &flag)
{
    toggle_str_set(my_traits, flag); //Toggles a base trait on the player
    toggle_str_set(my_mutations, flag); //Toggles corresponding trait in mutations list as well.
    if( has_trait( flag ) ) {
        mutation_effect(flag);
    } else {
        mutation_loss_effect(flag);
    }
    recalc_sight_limits();
}

void Character::toggle_mutation(const std::string &flag)
{
    toggle_str_set(my_mutations, flag); //Toggles a mutation on the player
    recalc_sight_limits();
}

void Character::toggle_str_set( std::unordered_set< std::string > &set, const std::string &str )
{
    auto toggled_element = std::find( set.begin(), set.end(), str );
    if( toggled_element == set.end() ) {
        char new_key = ' ';
        // Find a letter in inv_chars that isn't in trait_keys.
        for( const auto &letter : inv_chars ) {
            bool found = false;
            for( const auto &key : trait_keys ) {
                if( letter == key.second ) {
                    found = true;
                    break;
                }
            }
            if( !found ) {
                new_key = letter;
                break;
            }
        }
        set.insert( str );
        trait_keys[str] = new_key;
    } else {
        set.erase( toggled_element );
        trait_keys.erase(str);
    }
}

void Character::mutation_effect(std::string mut)
{
    bool is_u = is_player();
    bool destroy = false;
    std::vector<body_part> bps;

    if (mut == "TOUGH" || mut == "TOUGH2" || mut == "TOUGH3" || mut == "GLASSJAW" ||
        mut == "FLIMSY" || mut == "FLIMSY2" || mut == "FLIMSY3" ||
        mut == "MUT_TOUGH" || mut == "MUT_TOUGH2" || mut == "MUT_TOUGH3") {
        recalc_hp();

    } else if (mut == "WEBBED" || mut == "PAWS" || mut == "PAWS_LARGE" || mut == "ARM_TENTACLES" ||
               mut == "ARM_TENTACLES_4" || mut == "ARM_TENTACLES_8") {
        // Push off gloves
        bps.push_back(bp_hand_l);
        bps.push_back(bp_hand_r);

    } else if (mut == "TALONS") {
        // Destroy gloves
        destroy = true;
        bps.push_back(bp_hand_l);
        bps.push_back(bp_hand_r);

    } else if (mut == "BEAK" || mut == "BEAK_PECK" || mut == "BEAK_HUM" || mut == "MANDIBLES" ||
               mut == "SABER_TEETH") {
        // Destroy mouthwear
        destroy = true;
        bps.push_back(bp_mouth);

    } else if (mut == "MINOTAUR" || mut == "MUZZLE" || mut == "MUZZLE_BEAR" || mut == "MUZZLE_LONG" ||
               mut == "PROBOSCIS" || mut == "MUZZLE_RAT") {
        // Push off mouthwear
        bps.push_back(bp_mouth);

    } else if (mut == "HOOVES" || mut == "RAP_TALONS") {
        // Destroy footwear
        destroy = true;
        bps.push_back(bp_foot_l);
        bps.push_back(bp_foot_r);

    } else if (mut == "SHELL") {
        // Destroy torsowear
        destroy = true;
        bps.push_back(bp_torso);

    } else if ( (mut == "INSECT_ARMS") || (mut == "ARACHNID_ARMS") || (mut == "WINGS_BUTTERFLY") ) {
        // Push off torsowear
        bps.push_back(bp_torso);

    } else if (mut == "HORNS_CURLED" || mut == "CHITIN3") {
        // Push off all helmets
        bps.push_back(bp_head);

    } else if (mut == "HORNS_POINTED" || mut == "ANTENNAE" || mut == "ANTLERS") {
        // Push off non-cloth helmets
        bps.push_back(bp_head);

    } else if (mut == "LARGE" || mut == "LARGE_OK") {
        str_max += 2;
        recalc_hp();

    } else if (mut == "HUGE") {
        str_max += 4;
        // Bad-Huge gets less HP bonus than normal, this is handled in recalc_hp()
        recalc_hp();
        // And there goes your clothing; by now you shouldn't need it anymore
        add_msg(m_bad, _("You rip out of your clothing!"));
        destroy = true;
        bps.push_back(bp_torso);
        bps.push_back(bp_leg_l);
        bps.push_back(bp_leg_r);
        bps.push_back(bp_arm_l);
        bps.push_back(bp_arm_r);
        bps.push_back(bp_hand_l);
        bps.push_back(bp_hand_r);
        bps.push_back(bp_head);
        bps.push_back(bp_foot_l);
        bps.push_back(bp_foot_r);

    }  else if (mut == "HUGE_OK") {
        str_max += 4;
        recalc_hp();
        // Good-Huge still can't fit places but its heart's healthy enough for
        // going around being Huge, so you get the HP

    } else if (mut == "STOCKY_TROGLO") {
        dex_max -= 2;
        str_max += 2;
        recalc_hp();

    } else if (mut == "PRED3") {
        // Not so much "better at learning combat skills"
        // as "brain changes to focus on their development".
        // We are talking post-humanity here.
        int_max --;

    } else if (mut == "PRED4") {
        // Might be a bit harsh, but on the other claw
        // we are talking folks who really wanted to
        // transcend their humanity by this point.
        int_max -= 3;

    } else if (mut == "STR_UP") {
        str_max ++;
        recalc_hp();

    } else if (mut == "STR_UP_2") {
        str_max += 2;
        recalc_hp();

    } else if (mut == "STR_UP_3") {
        str_max += 4;
        recalc_hp();

    } else if (mut == "STR_UP_4") {
        str_max += 7;
        recalc_hp();

    } else if (mut == "STR_ALPHA") {
        if (str_max <= 6) {
            str_max = 8;
        } else if (str_max <= 7) {
            str_max = 11;
        } else if (str_max <= 14) {
            str_max = 15;
        } else {
            str_max = 18;
        }
        recalc_hp();
    } else if (mut == "DEX_UP") {
        dex_max ++;

    } else if (mut == "BENDY1") {
        dex_max ++;

    } else if (mut == "BENDY2") {
        dex_max += 3;
        str_max -= 2;
        recalc_hp();

    } else if (mut == "BENDY3") {
        dex_max += 4;
        str_max -= 4;
        recalc_hp();

    } else if (mut == "DEX_UP_2") {
        dex_max += 2;

    } else if (mut == "DEX_UP_3") {
        dex_max += 4;

    } else if (mut == "DEX_UP_4") {
        dex_max += 7;

    } else if (mut == "DEX_ALPHA") {
        if (dex_max <= 6) {
            dex_max = 8;
        } else if (dex_max <= 7) {
            dex_max = 11;
        } else if (dex_max <= 14) {
            dex_max = 15;
        } else {
            dex_max = 18;
        }
    } else if (mut == "INT_UP") {
        int_max ++;

    } else if (mut == "INT_UP_2") {
        int_max += 2;

    } else if (mut == "INT_UP_3") {
        int_max += 4;

    } else if (mut == "INT_UP_4") {
        int_max += 7;

    } else if (mut == "INT_ALPHA") {
        if (int_max <= 6) {
            int_max = 8;
        } else if (int_max <= 7) {
            int_max = 11;
        } else if (int_max <= 14) {
            int_max = 15;
        } else {
            int_max = 18;
        }
    } else if (mut == "INT_SLIME") {
        int_max *= 2; // Now, can you keep it? :-)

    } else if (mut == "PER_UP") {
        per_max ++;

    } else if (mut == "PER_UP_2") {
        per_max += 2;

    } else if (mut == "PER_UP_3") {
        per_max += 4;

    } else if (mut == "PER_UP_4") {
        per_max += 7;

    } else if (mut == "PER_ALPHA") {
        if (per_max <= 6) {
            per_max = 8;
        } else if (per_max <= 7) {
            per_max = 11;
        } else if (per_max <= 14) {
            per_max = 15;
        } else {
            per_max = 18;
        }
    } else if (mut == "PER_SLIME") {
        per_max -= 8;
        if (per_max <= 0) {
            per_max = 1;
        }

    } else if (mut == "PER_SLIME_OK") {
        per_max += 5;
    }

    std::string mutation_safe = "OVERSIZE";
    for (size_t i = 0; i < worn.size(); i++) {
        for( auto &bp : bps ) {
            if( ( worn[i].covers( bp ) ) && ( !( worn[i].has_flag( mutation_safe ) ) ) ) {
                if (destroy) {
                    if (is_u) {
                        add_msg(m_bad, _("Your %s is destroyed!"), worn[i].tname().c_str());
                    }

                    worn.erase(worn.begin() + i);

                } else {
                    if (is_u) {
                        add_msg(m_bad, _("Your %s is pushed off."), worn[i].tname().c_str());
                    }

                    int pos = player::worn_position_to_index(i);
                    g->m.add_item_or_charges(xpos(), ypos(), worn[i]);
                    i_rem(pos);
                }
                // Reset to the start of the vector
                i = 0;
            }
        }
    }
}

void Character::mutation_loss_effect(std::string mut)
{
    if (mut == "TOUGH" || mut == "TOUGH2" || mut == "TOUGH3" || mut == "GLASSJAW" ||
        mut == "FLIMSY" || mut == "FLIMSY2" || mut == "FLIMSY3" ||
        mut == "MUT_TOUGH" || mut == "MUT_TOUGH2" || mut == "MUT_TOUGH3") {
        recalc_hp();

    } else if (mut == "LARGE" || mut == "LARGE_OK") {
        str_max -= 2;
        recalc_hp();

    } else if (mut == "HUGE") {
        str_max -= 4;
        recalc_hp();
        // Losing Huge probably means either gaining Good-Huge or
        // going back to Large.  In any case, recalc_hp ought to
        // handle it.

    } else if (mut == "HUGE_OK") {
        str_max -= 4;
        recalc_hp();

    } else if (mut == "STOCKY_TROGLO") {
        dex_max += 2;
        str_max -= 2;
        recalc_hp();

    } else if (mut == "PRED3") {
        // Mostly for the Debug.
        int_max ++;

    } else if (mut == "PRED4") {
        int_max += 3;

    } else if (mut == "STR_UP") {
        str_max --;
        recalc_hp();

    } else if (mut == "STR_UP_2") {
        str_max -= 2;
        recalc_hp();

    } else if (mut == "STR_UP_3") {
        str_max -= 4;
        recalc_hp();

    } else if (mut == "STR_UP_4") {
        str_max -= 7;
        recalc_hp();

    } else if (mut == "STR_ALPHA") {
        if (str_max == 18) {
            str_max = 15;
        } else if (str_max == 15) {
            str_max = 8;
        } else if (str_max == 11) {
            str_max = 7;
        } else {
            str_max = 4;
        }
        recalc_hp();
    } else if (mut == "DEX_UP") {
        dex_max --;

    } else if (mut == "BENDY1") {
        dex_max --;

    } else if (mut == "BENDY2") {
        dex_max -= 3;
        str_max += 2;
        recalc_hp();

    } else if (mut == "BENDY3") {
        dex_max -= 4;
        str_max += 4;
        recalc_hp();

    } else if (mut == "DEX_UP_2") {
        dex_max -= 2;

    } else if (mut == "DEX_UP_3") {
        dex_max -= 4;

    } else if (mut == "DEX_UP_4") {
        dex_max -= 7;

    } else if (mut == "DEX_ALPHA") {
        if (dex_max == 18) {
            dex_max = 15;
        } else if (dex_max == 15) {
            dex_max = 8;
        } else if (dex_max == 11) {
            dex_max = 7;
        } else {
            dex_max = 4;
        }
    } else if (mut == "INT_UP") {
        int_max --;

    } else if (mut == "INT_UP_2") {
        int_max -= 2;

    } else if (mut == "INT_UP_3") {
        int_max -= 4;

    } else if (mut == "INT_UP_4") {
        int_max -= 7;

    } else if (mut == "INT_ALPHA") {
        if (int_max == 18) {
            int_max = 15;
        } else if (int_max == 15) {
            int_max = 8;
        } else if (int_max == 11) {
            int_max = 7;
        } else {
            int_max = 4;
        }
    } else if (mut == "INT_SLIME") {
        int_max /= 2; // In case you have a freak accident with the debug menu ;-)

    } else if (mut == "PER_UP") {
        per_max --;

    } else if (mut == "PER_UP_2") {
        per_max -= 2;

    } else if (mut == "PER_UP_3") {
        per_max -= 4;

    } else if (mut == "PER_UP_4") {
        per_max -= 7;

    } else if (mut == "PER_ALPHA") {
        if (per_max == 18) {
            per_max = 15;
        } else if (per_max == 15) {
            per_max = 8;
        } else if (per_max == 11) {
            per_max = 7;
        } else {
            per_max = 4;
        }
    } else if (mut == "PER_SLIME") {
        per_max += 8;

    } else if (mut == "PER_SLIME_OK") {
        per_max -= 5;

    }
}

bool Character::has_active_mutation(const std::string & b) const
{
    const auto &mut_iter = my_mutations.find( b );
    if( mut_iter == my_mutations.end() ) {
        return false;
    }
    return traits[*mut_iter].powered;
}

bool Character::has_bionic(const bionic_id & b) const
{
    for (auto &i : my_bionics) {
        if (i.id == b) {
            return true;
        }
    }
    return false;
}

bool Character::has_active_bionic(const bionic_id & b) const
{
    for (auto &i : my_bionics) {
        if (i.id == b) {
            return (i.powered);
        }
    }
    return false;
}

item& Character::i_add(item it)
{
 itype_id item_type_id = "null";
 if( it.type ) item_type_id = it.type->id;

 last_item = item_type_id;

 if (it.is_food() || it.is_ammo() || it.is_gun()  || it.is_armor() ||
     it.is_book() || it.is_tool() || it.is_weap() || it.is_food_container())
  inv.unsort();

    // if there's a desired invlet for this item type, try to use it
    bool keep_invlet = false;
    const std::set<char> cur_inv = allocated_invlets();
    for (auto iter : assigned_invlet) {
        if (iter.second == item_type_id && !cur_inv.count(iter.first)) {
            it.invlet = iter.first;
            keep_invlet = true;
            break;
        }
    }
    auto &item_in_inv = inv.add_item(it, keep_invlet);
    item_in_inv.on_pickup( *this );
    return item_in_inv;
}

item Character::i_rem(int pos)
{
 item tmp;
 if (pos == -1) {
     tmp = weapon;
     weapon = ret_null;
     return tmp;
 } else if (pos < -1 && pos > worn_position_to_index(worn.size())) {
     tmp = worn[worn_position_to_index(pos)];
     worn.erase(worn.begin() + worn_position_to_index(pos));
     return tmp;
 }
 return inv.remove_item(pos);
}

item Character::i_rem(const item *it)
{
    auto tmp = remove_items_with( [&it] (const item &i) { return &i == it; } );
    if( tmp.empty() ) {
        debugmsg( "did not found item %s to remove it!", it->tname().c_str() );
        return ret_null;
    }
    return tmp.front();
}

void Character::i_rem_keep_contents( const int pos )
{
    for( auto &content : i_rem( pos ).contents ) {
        i_add_or_drop( content );
    }
}

bool Character::i_add_or_drop(item& it, int qty) {
    bool retval = true;
    bool drop = false;
    inv.assign_empty_invlet(it);
    for (int i = 0; i < qty; ++i) {
        if (!drop && (!can_pickWeight(it.weight(), !OPTIONS["DANGEROUS_PICKUPS"])
                      || !can_pickVolume(it.volume()))) {
            drop = true;
        }
        if (drop) {
            retval &= g->m.add_item_or_charges(xpos(), ypos(), it);
        } else {
            i_add(it);
        }
    }
    return retval;
}

std::set<char> Character::allocated_invlets() const {
    std::set<char> invlets = inv.allocated_invlets();

    if (weapon.invlet != 0) {
        invlets.insert(weapon.invlet);
    }
    for( const auto &w : worn ) {
        if( w.invlet != 0 ) {
            invlets.insert( w.invlet );
        }
    }

    return invlets;
}

bool Character::has_active_item(const itype_id & id) const
{
    return has_item_with( [id]( const item & it ) {
        return it.active && it.typeId() == id;
    } );
}

item Character::remove_weapon()
{
    if( weapon.active ) {
        weapon.deactivate_charger_gun();
    }
 item tmp = weapon;
 weapon = ret_null;
 return tmp;
}

void Character::remove_mission_items( int mission_id )
{
    if( mission_id == -1 ) {
        return;
    }
    remove_items_with( has_mission_item_filter { mission_id } );
}

int Character::weight_carried() const
{
    int ret = 0;
    ret += weapon.weight();
    for (auto &i : worn) {
        ret += i.weight();
    }
    ret += inv.weight();
    return ret;
}

int Character::volume_carried() const
{
    return inv.volume();
}

int Character::weight_capacity() const
{
    // Get base capacity from creature,
    // then apply player-only mutation and trait effects.
    int ret = Creature::weight_capacity();
    if (has_trait("BADBACK")) {
        ret = int(ret * .65);
    }
    if (has_trait("STRONGBACK")) {
        ret = int(ret * 1.35);
    }
    if (has_trait("LIGHT_BONES")) {
        ret = int(ret * .80);
    }
    if (has_trait("HOLLOW_BONES")) {
        ret = int(ret * .60);
    }
    if (has_artifact_with(AEP_CARRY_MORE)) {
        ret += 22500;
    }
    if (ret < 0) {
        ret = 0;
    }
    return ret;
}

int Character::volume_capacity() const
{
    int ret = 2; // A small bonus (the overflow)
    for (auto &i : worn) {
        ret += i.get_storage();
    }
    if (has_bionic("bio_storage")) {
        ret += 8;
    }
    if (has_trait("SHELL")) {
        ret += 16;
    }
    if (has_trait("SHELL2") && !has_active_mutation("SHELL2")) {
        ret += 24;
    }
    if (has_trait("PACKMULE")) {
        ret = int(ret * 1.4);
    }
    if (has_trait("DISORGANIZED")) {
        ret = int(ret * 0.6);
    }
    if (ret < 2) {
        ret = 2;
    }
    return ret;
}

bool Character::can_pickVolume(int volume) const
{
    return (volume_carried() + volume <= volume_capacity());
}
bool Character::can_pickWeight(int weight, bool safe) const
{
    if (!safe)
    {
        // Character can carry up to four times their maximum weight
        return (weight_carried() + weight <= weight_capacity() * 4);
    }
    else
    {
        return (weight_carried() + weight <= weight_capacity());
    }
}

bool Character::has_artifact_with(const art_effect_passive effect) const
{
    if( weapon.has_effect_when_wielded( effect ) ) {
        return true;
    }
    for( auto & i : worn ) {
        if( i.has_effect_when_worn( effect ) ) {
            return true;
        }
    }
    return has_item_with( [effect]( const item & it ) {
        return it.has_effect_when_carried( effect );
    } );
}

bool Character::is_wearing(const itype_id & it) const
{
    for (auto &i : worn) {
        if (i.type->id == it) {
            return true;
        }
    }
    return false;
}

bool Character::is_wearing_on_bp(const itype_id & it, body_part bp) const
{
    for (auto &i : worn) {
        if (i.type->id == it && i.covers(bp)) {
            return true;
        }
    }
    return false;
}

bool Character::worn_with_flag( std::string flag ) const
{
    for (auto &i : worn) {
        if (i.has_flag( flag )) {
            return true;
        }
    }
    return false;
}

bool Character::has_nv()
{
    static bool nv = false;

    if( !nv_cached ) {
        nv_cached = true;
        nv = (worn_with_flag("GNV_EFFECT") ||
              has_active_bionic("bio_night_vision"));
    }

    return nv;
}
