#include "character.h"
#include "game.h"

Character::Character()
{
    Creature::set_speed_base(100);
}

Character::~Character()
{
}

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
            g->m.add_item_or_charges(posx(), posy(), string);
            g->m.add_item_or_charges(posx(), posy(), snare);
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
            g->m.add_item_or_charges(posx(), posy(), rope);
            g->m.add_item_or_charges(posx(), posy(), snare);
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
            g->m.add_item_or_charges(posx(), posy(), beartrap);
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
                    int dist = rl_dist(cx, cy, posx(), posy());
                    if (dist < curdist) {
                        curdist = dist;
                    }
                    dist = rl_dist(cx, cy, posx(), posy());
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
