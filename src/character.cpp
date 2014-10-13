#include "character.h"

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
            add_msg_if_player(_("You stand up."));
            remove_effect("downed");
        }
        return false;
    }
    if (has_effect("lightsnare")) {
        if(x_in_y(get_str(), 12) || x_in_y(get_dex(), 8)) {
            remove_effect("lightsnare");
            add_msg_player_or_npc(m_good, _("You free yourself from the light snare!"),
                                    _("<npcname> frees themselves from the light snare!"));
            g->m.spawn_item(xpos(), ypos(), "string_36");
            g->m.spawn_item(xpos(), ypos(), "snare_trigger");
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
            g->m.spawn_item(xpos(), ypos(), "rope_6");
            g->m.spawn_item(xpos(), ypos(), "snare_trigger");
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
            g->m.spawn_item(xpos(), ypos(), "beartrap");
        } else {
            add_msg_if_player(m_bad, _("You try to free yourself from the bear trap, but can't get loose!"));
        }
        return false;
    }
    return Creature::move_effects();
}
void Character::add_effect(efftype_id eff_id, int dur, body_part bp, bool permanent, int intensity)
{
    Creature::add_effect(eff_id, dur, bp, permanent, intensity);
}
