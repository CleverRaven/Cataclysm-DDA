#include "iuse.h"
#include "game.h"
#include "mapdata.h"
#include "output.h"
#include "debug.h"
#include "options.h"
#include "rng.h"
#include "line.h"
#include "mutation.h"
#include "player.h"
#include "disease.h"
#include "vehicle.h"
#include "uistate.h"
#include "action.h"
#include "monstergenerator.h"
#include "speech.h"
#include "overmapbuffer.h"
#include "json.h"
#include "messages.h"
#include "crafting.h"
#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <set>

#define RADIO_PER_TURN 25 // how many characters per turn of radio

#include "iuse_software.h"

void remove_double_ammo_mod( item &it, player &p )
{
    if( !it.item_tags.count( "DOUBLE_AMMO" ) ) {
        return;
    }
    p.add_msg_if_player( _( "You remove the double battery capacity mod of your %s!" ),
                         it.tname().c_str() );
    item mod( "battery_compartment", calendar::turn );
    p.i_add_or_drop( mod, 1 );
    it.item_tags.erase( "DOUBLE_AMMO" );
    // Easier to remove all batteries than to check for the actual real maximum
    if( it.charges > 0 ) {
        item batteries( "battery", calendar::turn );
        batteries.charges = it.charges;
        it.charges = 0;
        p.i_add_or_drop( batteries, 1 );
    }
}

void remove_recharge_mod( item &it, player &p )
{
    if( !it.item_tags.count( "RECHARGE" ) ) {
        return;
    }
    p.add_msg_if_player( _( "You remove the rechargeable powerpack from your %s!" ),
                         it.tname().c_str() );
    item mod( "rechargeable_battery", calendar::turn );
    mod.charges = it.charges;
    it.charges = 0;
    p.i_add_or_drop( mod, 1 );
    it.item_tags.erase( "RECHARGE" );
    it.item_tags.erase( "NO_UNLOAD" );
    it.item_tags.erase( "NO_RELOAD" );
}

void remove_atomic_mod( item &it, player &p )
{
    if( !it.item_tags.count( "ATOMIC_AMMO" ) ) {
        return;
    }
    p.add_msg_if_player( _( "You remove the plutonium cells of your %s!" ), it.tname().c_str() );
    item mod( "battery_atomic", calendar::turn );
    mod.charges = it.charges;
    it.charges = 0;
    p.i_add_or_drop( mod, 1 );
    it.item_tags.erase( "ATOMIC_AMMO" );
    it.item_tags.erase( "NO_UNLOAD" );
    it.item_tags.erase( "RADIOACTIVE" );
    it.item_tags.erase( "LEAK_DAM" );
}

void remove_ups_mod( item &it, player &p )
{
    if( !it.has_flag( "USE_UPS" ) ) {
        return;
    }
    p.add_msg_if_player( _( "You remove the UPS Conversion Pack of your %s!" ), it.tname().c_str() );
    item mod( "battery_ups", calendar::turn );
    p.i_add_or_drop( mod, 1 );
    it.charges = 0;
    it.item_tags.erase( "USE_UPS" );
    it.item_tags.erase( "NO_UNLOAD" );
    it.item_tags.erase( "NO_RELOAD" );
}

// Checks that the player does not have an active item with LITCIG flag.
bool check_litcig( player &u )
{
    auto cigs = u.items_with( []( const item & it ) {
        return it.active && it.has_flag( "LITCIG" );
    } );
    if( cigs.empty() ) {
        return true;
    }
    u.add_msg_if_player( m_info, _( "You're already smoking a %s!" ), cigs[0]->tname().c_str() );
    return false;
}

static bool item_inscription(player *p, item *cut, std::string verb, std::string gerund,
                             bool carveable)
{
    (void)p; //unused
    if (!cut->made_of(SOLID)) {
        std::string lower_verb = verb;
        std::transform(lower_verb.begin(), lower_verb.end(), lower_verb.begin(), ::tolower);
        add_msg(m_info, _("You can't %s an item that's not solid!"), lower_verb.c_str());
        return false;
    }
    if (carveable && !(cut->made_of("wood") || cut->made_of("plastic") ||
                       cut->made_of("glass") || cut->made_of("chitin") ||
                       cut->made_of("iron") || cut->made_of("steel") ||
                       cut->made_of("silver"))) {
        std::string lower_verb = verb;
        std::transform(lower_verb.begin(), lower_verb.end(), lower_verb.begin(), ::tolower);
        add_msg(m_info, _("You can't %1$s %2$s because of the material it is made of."),
                lower_verb.c_str(), cut->display_name().c_str());
        return false;
    }

    std::map<std::string, std::string>::const_iterator ent = cut->item_vars.find("item_note");

    bool hasnote = (ent != cut->item_vars.end());
    std::string message = "";
    std::string messageprefix = string_format(hasnote ? _("(To delete, input one '.')\n") : "") +
                                string_format(_("%1$s on the %2$s is: "),
                                        gerund.c_str(), cut->type_name().c_str());
    message = string_input_popup(string_format(_("%s what?"), verb.c_str()), 64,
                                 (hasnote ? cut->item_vars["item_note"] : message),
                                 messageprefix, "inscribe_item", 128);

    if (!message.empty()) {
        if (hasnote && message == ".") {
            cut->item_vars.erase("item_note");
            cut->item_vars.erase("item_note_type");
            cut->item_vars.erase("item_note_typez");
        } else {
            cut->item_vars["item_note"] = message;
            cut->item_vars["item_note_type"] = gerund;
        }
    }
    return true;
}

// Returns false if the inscription failed or if the player canceled the action. Otherwise, returns true.

static bool inscribe_item(player *p, std::string verb, std::string gerund, bool carveable)
{
    //Note: this part still strongly relies on English grammar.
    //Although it can be easily worked around in language like Chinese,
    //but might need to be reworked for some European languages that have more verb forms
    int pos = g->inv(string_format(_("%s on what?"), verb.c_str()));
    item *cut = &(p->i_at(pos));
    if (cut->type->id == "null") {
        add_msg(m_info, _("You do not have that item!"));
        return false;
    }
    return item_inscription(p, cut, verb, gerund, carveable);
}

// For an explosion (which releases some kind of gas), this function
// calculates the points around that explosion where to create those
// gas fields.
// Those points must have a clear line of sight and a clear path to
// the center of the explosion.
// They must also be passable (move_cost > 0).
std::vector<point> points_for_gas_cloud(const point &center, int radius)
{
    const std::vector<point> gas_sources = closest_points_first(radius, center.x, center.y);
    std::vector<point> result;
    int junk;
    for( const auto &p : gas_sources ) {

        if (g->m.move_cost(p.x, p.y) <= 0) {
            // A wall
            continue;
        }
        if (p.x != center.x || p.y != center.y) {
            if (!g->m.sees(center.x, center.y, p.x, p.y, radius, junk)) {
                // No clear line of sight
                continue;
            }
            if (!g->m.clear_path(center.x, center.y, p.x, p.y, radius, 1, 100, junk)) {
                // Can not splatter gas from center to that point, something is in the way
                continue;
            }
        }
        result.push_back(p);
    }
    return result;
}

/* iuse methods return the number of charges expended, which is usually it->charges_to_use().
 * Some items that don't normally use charges return 1 to indicate they're used up.
 * Regardless, returning 0 indicates the item has not been used up,
 * though it may have been successfully activated.
 */
int iuse::sewage(player *p, item *it, bool, point)
{
    if (!p->is_npc()) {
        p->add_memorial_log(pgettext("memorial_male", "Ate a sewage sample."),
                            pgettext("memorial_female", "Ate a sewage sample."));
    }
    p->vomit();
    if (one_in(4)) {
        p->mutate();
    }
    return it->type->charges_to_use();
}

int iuse::honeycomb(player *p, item *it, bool, point)
{
    g->m.spawn_item(p->posx, p->posy, "wax", 2);
    return it->type->charges_to_use();
}

int iuse::royal_jelly(player *p, item *it, bool, point)
{
    // TODO: Add other effects here; royal jelly is a cure-all!
    p->pkill += 5;
    std::string message;
    if (p->has_effect("fungus")) {
        message = _("You feel cleansed inside!");
        p->remove_effect("fungus");
    }
    if (p->has_effect("dermatik") || p->has_effect("bloodworms") ||
        p->has_effect("paincysts") || p->has_effect("brainworm") ||
        p->has_effect("tapeworm")) {
        message = _("You feel cleansed inside!");
        p->remove_effect("dermatik");
        p->remove_effect("bloodworms");
        p->remove_effect("paincysts");
        p->remove_effect("brainworm");
        p->remove_effect("tapeworm");
    }
    if (p->has_effect("blind")) {
        message = "";
        p->remove_effect("blind");
    }
    if (p->has_effect("poison") || p->has_effect("foodpoison") ||
        p->has_effect("badpoison") || p->has_effect("paralyzepoison") ||
        p->has_effect("tetanus") || p->has_effect("infected") || p->has_effect("stung")) {
        message = _("You feel much better!");
        p->remove_effect("poison");
        p->remove_effect("stung");
        p->remove_effect("badpoison");
        p->remove_effect("foodpoison");
        p->remove_effect("paralyzepoison");
        p->remove_effect("tetanus");
        p->remove_effect("infected");
    }
    if (p->has_effect("asthma")) {
        message = _("Your breathing clears up!");
        p->remove_effect("asthma");
    }
    if (p->has_effect("common_cold") || p->has_effect("flu")) {
        message = _("You feel healthier!");
        p->remove_effect("common_cold");
        p->remove_effect("flu");
    }
    p->add_msg_if_player(m_good, message.c_str());
    return it->type->charges_to_use();
}

static hp_part body_window(player *p, item *, std::string item_name,
                           int normal_bonus, int head_bonus, int torso_bonus,
                           int bleed, int bite, int infect, bool force)
{
    WINDOW *hp_window = newwin(10, 31, (TERMY - 10) / 2, (TERMX - 31) / 2);
    draw_border(hp_window);

    mvwprintz(hp_window, 1, 1, c_ltred, _("Use %s:"), item_name.c_str());
    nc_color color = c_ltgray;
    bool allowed_result[num_hp_parts] = { false };
    if (p->hp_cur[hp_head] < p->hp_max[hp_head] ||
        (p->has_effect("infected", bp_head)) ||
        (p->has_effect("bite", bp_head)) ||
        // By rights "bleed" ought to enable itself via HP loss, but...
        (p->has_effect("bleed", bp_head)) || force) {
        color = g->limb_color(p, bp_head, bleed, bite, infect);
        if (color != c_ltgray || head_bonus != 0) {
            mvwprintz(hp_window, 2, 1, color, _("1: Head"));
            allowed_result[hp_head] = true;
        }
    }
    if (p->hp_cur[hp_torso] < p->hp_max[hp_torso] ||
        (p->has_effect("infected", bp_torso)) ||
        (p->has_effect("bite", bp_torso)) ||
        (p->has_effect("bleed", bp_torso)) || force) {
        color = g->limb_color(p, bp_torso, bleed, bite, infect);
        if (color != c_ltgray || torso_bonus != 0) {
            mvwprintz(hp_window, 3, 1, color, _("2: Torso"));
            allowed_result[hp_torso] = true;
        }
    }
    if (p->hp_cur[hp_arm_l] < p->hp_max[hp_arm_l] ||
        (p->has_effect("infected", bp_arm_l)) ||
        (p->has_effect("bite", bp_arm_l)) ||
        (p->has_effect("bleed", bp_arm_l)) || force) {
        color = g->limb_color(p, bp_arm_l, bleed, bite, infect);
        if (color != c_ltgray || normal_bonus != 0) {
            mvwprintz(hp_window, 4, 1, color, _("3: Left Arm"));
            allowed_result[hp_arm_l] = true;
        }
    }
    if (p->hp_cur[hp_arm_r] < p->hp_max[hp_arm_r] ||
        (p->has_effect("infected", bp_arm_r)) ||
        (p->has_effect("bite", bp_arm_r)) ||
        (p->has_effect("bleed", bp_arm_r)) || force) {
        color = g->limb_color(p, bp_arm_r, bleed, bite, infect);
        if (color != c_ltgray || normal_bonus != 0) {
            mvwprintz(hp_window, 5, 1, color, _("4: Right Arm"));
            allowed_result[hp_arm_r] = true;
        }
    }
    if (p->hp_cur[hp_leg_l] < p->hp_max[hp_leg_l] ||
        (p->has_effect("infected", bp_leg_l)) ||
        (p->has_effect("bite", bp_leg_l)) ||
        (p->has_effect("bleed", bp_leg_l)) || force) {
        color = g->limb_color(p, bp_leg_l, bleed, bite, infect);
        if (color != c_ltgray || normal_bonus != 0) {
            mvwprintz(hp_window, 6, 1, color, _("5: Left Leg"));
            allowed_result[hp_leg_l] = true;
        }
    }
    if (p->hp_cur[hp_leg_r] < p->hp_max[hp_leg_r] ||
        (p->has_effect("infected", bp_leg_r)) ||
        (p->has_effect("bite", bp_leg_r)) ||
        (p->has_effect("bleed", bp_leg_r)) || force) {
        color = g->limb_color(p, bp_leg_r, bleed, bite, infect);
        if (color != c_ltgray || normal_bonus != 0) {
            mvwprintz(hp_window, 7, 1, color, _("6: Right Leg"));
            allowed_result[hp_leg_r] = true;
        }
    }
    mvwprintz(hp_window, 8, 1, c_ltgray, _("7: Exit"));
    std::string health_bar = "";
    for (int i = 0; i < num_hp_parts; i++) {
        if (allowed_result[i]) {
            // have printed the name of the body part, can select it
            int current_hp = p->hp_cur[i];
            if (current_hp != 0) {
                get_HP_Bar(current_hp, p->hp_max[i], color, health_bar, false);
                if (p->has_trait("SELFAWARE")) {
                    mvwprintz(hp_window, i + 2, 15, color, "%5d", current_hp);
                } else {
                    mvwprintz(hp_window, i + 2, 15, color, health_bar.c_str());
                }
            } else {
                // curhp is 0; requires surgical attention
                mvwprintz(hp_window, i + 2, 15, c_dkgray, "-----");
            }
            mvwprintz(hp_window, i + 2, 20, c_dkgray, " -> ");
            if (current_hp != 0) {
                switch (hp_part(i)) {
                    case hp_head:
                        current_hp += head_bonus;
                        break;
                    case hp_torso:
                        current_hp += torso_bonus;
                        break;
                    default:
                        current_hp += normal_bonus;
                        break;
                }
                if (current_hp > p->hp_max[i]) {
                    current_hp = p->hp_max[i];
                } else if (current_hp < 0) {
                    current_hp = 0;
                }
                get_HP_Bar(current_hp, p->hp_max[i], color, health_bar, false);
                if (p->has_trait("SELFAWARE")) {
                    mvwprintz(hp_window, i + 2, 24, color, "%5d", current_hp);
                } else {
                    mvwprintz(hp_window, i + 2, 24, color, health_bar.c_str());
                }
            } else {
                // curhp is 0; requires surgical attention
                mvwprintz(hp_window, i + 2, 24, c_dkgray, "-----");
            }
        }
    }
    wrefresh(hp_window);
    char ch;
    hp_part healed_part = num_hp_parts;
    do {
        ch = getch();
        if (ch == '1') {
            healed_part = hp_head;
        } else if (ch == '2') {
            healed_part = hp_torso;
        } else if (ch == '3') {
            if ((p->hp_cur[hp_arm_l] == 0) &&
                (!((p->has_effect("infected", bp_arm_l)) ||
                   (p->has_effect("bite", bp_arm_l)) ||
                   (p->has_effect("bleed", bp_arm_l))))) {
                p->add_msg_if_player(m_info, _("That arm is broken.  It needs surgical attention."));
                healed_part = num_hp_parts;
            } else {
                healed_part = hp_arm_l;
            }
        } else if (ch == '4') {
            if ((p->hp_cur[hp_arm_r] == 0) &&
                (!((p->has_effect("infected", bp_arm_r)) ||
                   (p->has_effect("bite", bp_arm_r)) ||
                   (p->has_effect("bleed", bp_arm_r))))) {
                p->add_msg_if_player(m_info, _("That arm is broken.  It needs surgical attention."));
                healed_part = num_hp_parts;
            } else {
                healed_part = hp_arm_r;
            }
        } else if (ch == '5') {
            if ((p->hp_cur[hp_leg_l] == 0) &&
                (!((p->has_effect("infected", bp_leg_l)) ||
                   (p->has_effect("bite", bp_leg_l)) ||
                   (p->has_effect("bleed", bp_leg_l))))) {
                p->add_msg_if_player(m_info, _("That leg is broken.  It needs surgical attention."));
                healed_part = num_hp_parts;
            } else {
                healed_part = hp_leg_l;
            }
        } else if (ch == '6') {
            if ((p->hp_cur[hp_leg_r] == 0) &&
                (!((p->has_effect("infected", bp_leg_r)) ||
                   (p->has_effect("bite", bp_leg_r)) ||
                   (p->has_effect("bleed", bp_leg_r))))) {
                p->add_msg_if_player(m_info, _("That leg is broken.  It needs surgical attention."));
                healed_part = num_hp_parts;
            } else {
                healed_part = hp_leg_r;
            }
        } else if (ch == '7' || ch == KEY_ESCAPE) {
            p->add_msg_if_player(_("Never mind."));
            healed_part = num_hp_parts;
            break;
        }
        if (healed_part < num_hp_parts && !allowed_result[healed_part]) {
            p->add_msg_if_player(_("Never mind."));
            healed_part = num_hp_parts;
            break;
        }
    } while (ch < '1' || ch > '7');
    werase(hp_window);
    wrefresh(hp_window);
    delwin(hp_window);
    refresh();

    return healed_part;
}

// returns true if we want to use the special action
static hp_part use_healing_item(player *p, item *it, int normal_power, int head_power,
                                int torso_power, int bleed,
                                int bite, int infect, bool force)
{
    hp_part healed = num_hp_parts;
    int bonus = p->skillLevel("firstaid");
    int head_bonus = 0;
    int normal_bonus = 0;
    int torso_bonus = 0;
    if (head_power > 0) {
        head_bonus = bonus * .8 + head_power;
    } else {
        head_bonus = head_power;
    }
    if (normal_power > 0) {
        normal_bonus = bonus + normal_power;
    } else {
        normal_bonus = normal_power;
    }
    if (torso_power > 0) {
        torso_bonus = bonus * 1.5 + torso_power;
    } else {
        torso_bonus = torso_power;
    }

    if (p->is_npc()) { // NPCs heal whichever has sustained the most damage
        int highest_damage = 0;
        for (int i = 0; i < num_hp_parts; i++) {
            int damage = p->hp_max[i] - p->hp_cur[i];
            if (i == hp_head) {
                damage *= 1.5;
            }
            if (i == hp_torso) {
                damage *= 1.2;
            }
            if (damage > highest_damage) {
                highest_damage = damage;
                healed = hp_part(i);
            }
        }
    } else { // Player--present a menu
        if (p->activity.type != ACT_FIRSTAID) {
            healed = body_window(p, it, it->tname(), normal_bonus, head_bonus,
                                 torso_bonus, bleed, bite, infect, force);
            if (healed == num_hp_parts) {
                return num_hp_parts; // canceled
            }
        }
        // Brick healing if using a first aid kit for the first time.
        // TODO: Base check on something other than the name.
        if (it->type->id == "1st_aid" && p->activity.type != ACT_FIRSTAID) {
            // Cancel and wait for activity completion.
            return healed;
        } else if (p->activity.type == ACT_FIRSTAID) {
            // Completed activity, extract body part from it.
            healed = (hp_part)p->activity.values[0];
        }
    }
    p->practice("firstaid", 8);
    int dam = 0;
    if (healed == hp_head) {
        dam = head_bonus;
    } else if (healed == hp_torso) {
        dam = torso_bonus;
    } else {
        dam = normal_bonus;
    }
    if ((p->hp_cur[healed] >= 1) && (dam > 0)) { // Prevent first-aid from mending limbs
        p->heal(healed, dam);
    } else if ((p->hp_cur[healed] >= 1) && (dam < 0)) {
        body_part bp;
        p->hp_convert( healed, bp );
        p->apply_damage( nullptr, bp, -dam ); //hurt takes + damage
    }

    body_part bp_healed = bp_torso;
    p->hp_convert(healed, bp_healed);

    if (p->has_effect("bleed", bp_healed)) {
        if (x_in_y(bleed, 100)) {
            p->remove_effect("bleed", bp_healed);
            p->add_msg_if_player(m_good, _("You stop the bleeding."));
        } else {
            p->add_msg_if_player(_("You fail to stop the bleeding."));
        }
    }
    if (p->has_effect("bite", bp_healed)) {
        if (x_in_y(bite, 100)) {
            p->remove_effect("bite", bp_healed);
            p->add_msg_if_player(m_good, _("You clean the wound."));
        } else {
            p->add_msg_if_player(m_warning, _("Your wound still aches."));
        }
    }
    if (p->has_effect("infected", bp_healed)) {
        if (x_in_y(infect, 100)) {
            int infected_dur = p->get_effect_dur("infected", bp_healed);
            p->remove_effect("infected", bp_healed);
            p->add_effect("recover", infected_dur);
            p->add_msg_if_player(m_good, _("You disinfect the wound."));
        } else {
            p->add_msg_if_player(m_warning, _("Your wound still hurts."));
        }
    }
    return healed;
}

int iuse::bandage(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    if (num_hp_parts != use_healing_item(p, it, 3, 1, 4, 90, 0, 0, false)) {
        if (it->type->id != "quikclot" || it->type->id != "bfipowder") {
            // Make bandages and rags take arbitrarily longer than hemostatic/antiseptic powders.
            p->moves -= 100;
        }
        p->add_msg_if_player(m_good, _("You use your %s."), it->tname().c_str());
        return it->type->charges_to_use();
    }
    return 0;
}

int iuse::firstaid(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    // Assign first aid long action.
    int healed = use_healing_item(p, it, 14, 10, 18, 95, 99, 95, false);
    if (healed != num_hp_parts) {
        p->assign_activity(ACT_FIRSTAID, 6000 / (p->skillLevel("firstaid") + 1), 0,
                           p->get_item_position(it), it->tname());
        p->activity.values.push_back(healed);
        p->moves = 0;
    }

    return 0;
}

// Used when finishing the first aid long action.
int iuse::completefirstaid(player *p, item *it, bool, point)
{
    if (num_hp_parts != use_healing_item(p, it, 14, 10, 18, 95, 99, 95, false)) {
        p->add_msg_if_player(_("You finish using the %s."), it->tname().c_str());
        p->add_effect("pkill1", 120);
    }
    return 0;
}

int iuse::disinfectant(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    if (num_hp_parts != use_healing_item(p, it, 6, 5, 9, 0, 95, 0, false)) {
        return it->type->charges_to_use();
    }
    return 0;
}

int iuse::xanax(player *p, item *it, bool, point)
{
    p->add_msg_if_player(_("You take some %s."), it->tname().c_str());
    p->add_effect("took_xanax", 900);
    return it->type->charges_to_use();
}

int iuse::caff(player *p, item *it, bool, point)
{
    it_comest *food = dynamic_cast<it_comest *> (it->type);
    p->fatigue -= food->stim * 3;
    return it->type->charges_to_use();
}

int iuse::atomic_caff(player *p, item *it, bool, point)
{
    p->add_msg_if_player(m_good, _("Wow! This %s has a kick."), it->tname().c_str());
    it_comest *food = dynamic_cast<it_comest *> (it->type);
    p->fatigue -= food->stim * 12;
    p->radiation += 8;
    return it->type->charges_to_use();
}

int iuse::raw_meat(player *p, item *it, bool, point)
{
    if ((one_in(32)) && !(p->has_effect("tapeworm") || p->has_bionic("bio_digestion") ||
                          p->has_trait("PARAIMMUNE") ||
                          // Hyper-Metabolism digests the thing before it can set up shop.
                          p->has_trait("EATHEALTH"))) {
        p->add_effect("tapeworm", 1, num_bp, true);
    }
    if ((one_in(64)) && !(p->has_effect("bloodworms") || p->has_bionic("bio_digestion") ||
                          p->has_trait("PARAIMMUNE"))) {
        p->add_effect("bloodworms", 1, num_bp, true);
    }
    if ((one_in(128)) && !(p->has_effect("brainworm") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE"))) {
        p->add_effect("brainworm", 1, num_bp, true);
    }
    if ((one_in(64)) && !(p->has_effect("paincysts") || p->has_bionic("bio_digestion") ||
                          p->has_trait("PARAIMMUNE"))) {
        p->add_effect("paincysts", 1, num_bp, true);
    }
    return it->type->charges_to_use();
}

int iuse::raw_fat(player *p, item *it, bool, point)
{
    if ((one_in(64)) && !(p->has_effect("tapeworm") || p->has_bionic("bio_digestion") ||
                          p->has_trait("PARAIMMUNE") ||
                          p->has_trait("EATHEALTH"))) {
        p->add_effect("tapeworm", 1, num_bp, true);
    }
    if ((one_in(128)) && !(p->has_effect("bloodworms") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE"))) {
        p->add_effect("bloodworms", 1, num_bp, true);
    }
    if ((one_in(128)) && !(p->has_effect("brainworm") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE"))) {
        p->add_effect("brainworm", 1, num_bp, true);
    }
    return it->type->charges_to_use();
}

int iuse::raw_bone(player *p, item *it, bool, point)
{
    if ((one_in(128)) && !(p->has_effect("bloodworms") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE"))) {
        p->add_effect("bloodworms", 1, num_bp, true);
    }
    return it->type->charges_to_use();
}

int iuse::raw_fish(player *p, item *it, bool, point)
{
    if ((one_in(256)) && !(p->has_effect("tapeworm") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE") ||
                           p->has_trait("EATHEALTH"))) {
        p->add_effect("tapeworm", 1, num_bp, true);
    }
    if ((one_in(256)) && !(p->has_effect("bloodworms") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE"))) {
        p->add_effect("bloodworms", 1, num_bp, true);
    }
    if ((one_in(256)) && !(p->has_effect("brainworm") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE"))) {
        p->add_effect("brainworm", 1, num_bp, true);
    }
    if ((one_in(256)) && !(p->has_effect("paincysts") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE"))) {
        p->add_effect("paincysts", 1, num_bp, true);
    }
    return it->type->charges_to_use();
}

int iuse::raw_wildveg(player *p, item *it, bool, point)
{
    if ((one_in(512)) && !(p->has_effect("tapeworm") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE") ||
                           p->has_trait("EATHEALTH"))) {
        p->add_effect("tapeworm", 1, num_bp, true);
    }
    if ((one_in(256)) && !(p->has_effect("bloodworms") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE"))) {
        p->add_effect("bloodworms", 1, num_bp, true);
    }
    if ((one_in(512)) && !(p->has_effect("brainworm") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE"))) {
        p->add_effect("brainworm", 1, num_bp, true);
    }
    if ((one_in(128)) && !(p->has_effect("paincysts") || p->has_bionic("bio_digestion") ||
                           p->has_trait("PARAIMMUNE"))) {
        p->add_effect("paincysts", 1, num_bp, true);
    }
    return it->type->charges_to_use();
}

int iuse::alcohol(player *p, item *it, bool, point)
{
    int duration = 680 - (10 * p->str_max); // Weaker characters are cheap drunks
    it_comest *food = dynamic_cast<it_comest *> (it->type);
    if (p->has_trait("ALCMET")) {
        duration = 180 - (10 * p->str_max);
        // Metabolizing the booze improves the nutritional value;
        // might not be healthy, and still causes Thirst problems, though
        p->hunger -= (abs(food->stim));
        // Metabolizing it cancels out depressant
        // effects, but doesn't make it any more stimulating
        if ((food->stim) < 0) {
            p->stim += (abs(food->stim));
        }
    } else if (p->has_trait("TOLERANCE")) {
        duration -= 300;
    } else if (p->has_trait("LIGHTWEIGHT")) {
        duration += 300;
    }
    if (!(p->has_trait("ALCMET"))) {
        p->pkill += 8;
    }
    p->add_effect("drunk", duration);
    return it->type->charges_to_use();
}

int iuse::alcohol_weak(player *p, item *it, bool, point)
{
    int duration = 340 - (6 * p->str_max);
    it_comest *food = dynamic_cast<it_comest *> (it->type);
    if (p->has_trait("ALCMET")) {
        duration = 90 - (6 * p->str_max);
        // Metabolizing the booze improves the nutritional value;
        // might not be healthy, and still causes Thirst problems, though
        p->hunger -= (abs(food->stim));
        // Metabolizing it cancels out the depressant
        p->stim += (abs(food->stim));
    } else if (p->has_trait("TOLERANCE")) {
        duration -= 120;
    } else if (p->has_trait("LIGHTWEIGHT")) {
        duration += 120;
    }
    if (!(p->has_trait("ALCMET"))) {
        p->pkill += 4;
    }
    p->add_effect("drunk", duration);
    return it->type->charges_to_use();
}

int iuse::alcohol_strong(player *p, item *it, bool, point)
{
    int duration = 900 - (12 * p->str_max);
    it_comest *food = dynamic_cast<it_comest *> (it->type);
    if (p->has_trait("ALCMET")) {
        duration = 250 - (10 * p->str_max);
        // Metabolizing the booze improves the nutritional
        // value; might not be healthy, and still
        // causes Thirst problems, though
        p->hunger -= (abs(food->stim));
        // Metabolizing it cancels out depressant
        // effects, but doesn't make it any more
        // stimulating
        if ((food->stim) < 0) {
            p->stim += (abs(food->stim));
        }
    } else if (p->has_trait("TOLERANCE")) {
        duration -= 450;
    } else if (p->has_trait("LIGHTWEIGHT")) {
        duration += 450;
    }
    if (!(p->has_trait("ALCMET"))) {
        p->pkill += 12;
    }
    p->add_effect("drunk", duration);
    return it->type->charges_to_use();
}

/**
 * Entry point for intentional bodily intake of smoke via paraphernalia: pipe,
 * crack pipe, etc.
 *
 * @param p
 * @param it the apparatus with which to do the smoking.
 * @param
 * @return
 */
int iuse::smoking_pipe(player *p, item *it, bool, point)
{
    bool hasFire = (p->has_charges("fire", 1));
    // Hardcoded for now, would like to get away from this.
    std::vector<std::string> smokable_ids;
    smokable_ids.push_back("tobacco");
    smokable_ids.push_back("weed");
    // What is available in our area (inventory right now) to smoke.
    std::vector<std::string> smokable_choices;

    // Fail fast(er) if we can't/shouldn't smoke.
    if( !check_litcig( *p ) ) {
        return 0;
    }
    if (!hasFire) {
        p->add_msg_if_player(m_info, _("You don't have anything to light it with!"));
        return 0;
    }

    // Figure out what we can smoke, if anything.
    for (auto s_id : smokable_ids) {
        if (p->has_amount(s_id, 1)) {
            smokable_choices.push_back(s_id);
        }
    }
    if (smokable_choices.size() == 0) {
        p->add_msg_if_player(m_info, _("You need to find something to smoke."));
        return 0;
    }
    const size_t choice = uimenu(true, _("What would you like to smoke?"), smokable_choices) - 1;
    if (choice >= smokable_choices.size()) {
        // Chose not to smoke.
        return 0;
    }
    // Finally we can smoke.
    std::string id_to_smoke = smokable_choices[(size_t)choice];
    // We trust from this point on that we've checked for the existence of
    // consumables and as such will now consume.
    p->use_charges("fire", 1);
    /// \todo More content goes into a single toke than a cig/cigar. Should pipe effects be stronger?
    if ("tobacco" == id_to_smoke) {
        p->add_msg_if_player(m_neutral, _("You smoke some tobacco out of your %s."), it->tname().c_str());
        p->use_charges("tobacco", 1);
        p->thirst += 1;
        p->hunger -= 2;
        p->add_effect("cig", 200);
        for (int i = 0; i < 3; i++) {
            g->m.add_field(p->posx + int(rng(-2, 2)), p->posy + int(rng(-2, 2)), fd_cigsmoke, 2);
        }
        if (p->get_effect_dur("cig") > (100 * (p->addiction_level(ADD_CIG)))) {
            p->add_msg_if_player(m_bad, _("Ugh, too much smoke... you cough heavily."));
            g->sound(p->posx, p->posy, 10, "");
        }
        p->moves -= 250;
    } else if ("weed" == id_to_smoke) {
        if (!(p->has_effect("weed_high"))) {
            p->add_msg_if_player(m_good, _("You smoke some weed.  Good stuff, man!"));
        } else {
            p->add_msg_if_player(m_info, _("You smoke some more weed."));
        }
        p->use_charges("weed", 1);
        p->hunger += 4;
        p->thirst += 6;
        if (p->pkill < 5) {
            p->pkill += 3;
            p->pkill *= 2;
        }
        int duration = 90;
        if (p->has_trait("TOLERANCE")) {
            duration = 60;
        } else if (p->has_trait("LIGHTWEIGHT")) {
            duration = 120;
        }
        p->add_effect("weed_high", duration);
        p->moves -= 40;
        // breathe out some smoke
        for (int i = 0; i < 3; i++) {
            g->m.add_field(p->posx + int(rng(-2, 2)), p->posy + int(rng(-2, 2)), fd_weedsmoke, 2);
        }
        if (one_in(5)) {
            weed_msg(p);
        }
    }
    return 0;
}

/**
 * Entry point for intentional bodily intake of smoke via paper wrapped one
 * time use items: cigars, cigarettes, etc.
 *
 * @param p
 * @param it the item to be smoked.
 * @param
 * @return
 */
int iuse::smoking(player *p, item *it, bool, point)
{
    bool hasFire = (p->has_charges("fire", 1));

    // make sure we're not already smoking something
    if( !check_litcig( *p ) ) {
        return 0;
    }

    if (!hasFire) {
        p->add_msg_if_player(m_info, _("You don't have anything to light it with!"));
        return 0;
    }

    item cig;
    if (it->type->id == "cig") {
        cig = item("cig_lit", int(calendar::turn));
        cig.item_counter = 40;
        p->thirst += 2;
        p->hunger -= 3;
    } else if (it->type->id == "handrolled_cig") {
        // This transforms the hand-rolled into a normal cig, which isn't exactly
        // what I want, but leaving it for now.
        cig = item("cig_lit", int(calendar::turn));
        cig.item_counter = 40;
        p->thirst += 2;
        p->hunger -= 3;
    } else if (it->type->id == "cigar") {
        cig = item("cigar_lit", int(calendar::turn));
        cig.item_counter = 120;
        p->thirst += 3;
        p->hunger -= 4;
    } else if (it->type->id == "joint") {
        cig = item("joint_lit", int(calendar::turn));
        cig.item_counter = 40;
        p->hunger += 4;
        p->thirst += 6;
        if (p->pkill < 5) {
            p->pkill += 3;
            p->pkill *= 2;
        }
    } else {
        p->add_msg_if_player(m_bad,
                             _("Please let the devs know you should be able to smoke a %s but the smoking code does not know how."),
                             it->tname().c_str());
        return 0;
    }
    // If we're here, we better have a cig to light.
    p->use_charges_if_avail("fire", 1);
    cig.active = true;
    p->inv.add_item(cig, false, true);
    p->add_msg_if_player(m_neutral, _("You light a %s."), cig.tname().c_str());

    // Parting messages
    if (it->type->id == "joint") {
        // Would group with the joint, but awkward to mutter before lighting up.
        if (one_in(5)) {
            weed_msg(p);
        }
    }
    if (p->get_effect_dur("cig") > (100 * (p->addiction_level(ADD_CIG) + 1))) {
        p->add_msg_if_player(m_bad, _("Ugh, too much smoke... you feel nasty."));
    }

    return it->type->charges_to_use();
}


int iuse::ecig(player *p, item *it, bool, point)
{
    if (it->type->id == "ecig") {
        p->add_msg_if_player(m_neutral, _("You take a puff from your electronic cigarette."));
    } else if (it->type->id == "advanced_ecig") {
        if (p->inv.has_components("nicotine_liquid", 1)) {
            p->add_msg_if_player(m_neutral, _("You inhale some vapor from your advanced electronic cigarette."));
            p->inv.use_charges("nicotine_liquid", 1);
        } else {
            p->add_msg_if_player(m_info, _("You don't have any nicotine liquid!"));
            return 0;
        }
    }

    p->thirst += 1;
    p->hunger -= 1;
    p->add_effect("cig", 100);
    if (p->get_effect_dur("cig") > (100 * (p->addiction_level(ADD_CIG) + 1))) {
        p->add_msg_if_player(m_bad, _("Ugh, too much nicotine... you feel nasty."));
    }
    return it->type->charges_to_use();
}

int iuse::antibiotic(player *p, item *it, bool, point)
{
    p->add_msg_if_player(m_neutral, _("You take some antibiotics."));
    if (p->has_effect("infected")) {
        // cheap model of antibiotic resistance, but it's something.
        if (x_in_y(95, 100)) {
            // Add recovery effect for each infected wound
            int infected_tot = 0;
            for (int i = 0; i < num_bp; ++i) {
                int infected_dur = p->get_effect_dur("infected", body_part(i));
                if (infected_dur > 0) {
                    infected_tot += infected_dur;
                }
            }
            p->add_effect("recover", infected_tot);
            // Remove all infected wounds
            p->remove_effect("infected");
        }
    }
    if (p->has_effect("tetanus")) {
        if (one_in(3)) {
            p->remove_effect("tetanus");
            p->add_msg_if_player(m_good, _("The muscle spasms start to go away."));
        } else {
            p->add_msg_if_player(m_warning, _("The medication does nothing to help the spasms."));
        }
    }
    return it->type->charges_to_use();
}

int iuse::eyedrops(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    if (it->charges < 1) {
        p->add_msg_if_player(_("You're out of %s."), it->tname().c_str());
        return false;
    }
    p->add_msg_if_player(_("You use your %s."), it->tname().c_str());
    p->moves -= 150;
    if (p->has_effect("boomered")) {
        p->remove_effect("boomered");
        p->add_msg_if_player(m_good, _("You wash the slime from your eyes."));
    }
    return it->type->charges_to_use();
}

int iuse::fungicide(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    p->add_msg_if_player(_("You use your fungicide."));
    if (p->has_effect("fungus") && (one_in(3))) {
        p->remove_effect("fungus");
        p->add_msg_if_player(m_warning,
                             _("You feel a burning sensation under your skin that quickly fades away."));
    }
    if (p->has_effect("spores") && (one_in(2))) {
        if (!p->has_effect("fungus")) {
            p->add_msg_if_player(m_warning, _("Your skin grows warm for a moment."));
        }
        p->remove_effect("spores");
        int spore_count = rng(1, 6);
        if (spore_count > 0) {
            monster spore(GetMType("mon_spore"));
            for (int i = p->posx - 1; i <= p->posx + 1; i++) {
                for (int j = p->posy - 1; j <= p->posy + 1; j++) {
                    if (spore_count == 0) {
                        break;
                    }
                    if (i == p->posx && j == p->posy) {
                        continue;
                    }
                    if (g->m.move_cost(i, j) > 0 && x_in_y(spore_count, 8)) {
                        const int zid = g->mon_at(i, j);
                        if (zid >= 0) {  // Spores hit a monster
                            if (g->u_see(i, j) &&
                                !g->zombie(zid).type->in_species("FUNGUS")) {
                                add_msg(m_warning, _("The %s is covered in tiny spores!"),
                                        g->zombie(zid).name().c_str());
                            }
                            monster &critter = g->zombie( zid );
                            if( !critter.make_fungus() ) {
                                critter.die( p ); // counts as kill by player
                            }
                        } else {
                            spore.spawn(i, j);
                            g->add_zombie(spore);
                        }
                        spore_count--;
                    }
                }
                if (spore_count == 0) {
                    break;
                }
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::antifungal(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    p->add_msg_if_player(_("You take some antifungal medication."));
    if (p->has_effect("fungus")) {
        p->remove_effect("fungus");
        p->add_msg_if_player(m_warning,
                             _("You feel a burning sensation under your skin that quickly fades away."));
    }
    if (p->has_effect("spores")) {
        if (!p->has_effect("fungus")) {
            p->add_msg_if_player(m_warning, _("Your skin grows warm for a moment."));
        }
    }
    return it->type->charges_to_use();
}

int iuse::antiparasitic(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    p->add_msg_if_player(_("You take some antiparasitic medication."));
    if (p->has_effect("dermatik")) {
        p->remove_effect("dermatik");
        p->add_msg_if_player(m_good, _("The itching sensation under your skin fades away."));
    }
    if (p->has_effect("tapeworm")) {
        p->remove_effect("tapeworm");
        p->hunger--;  // You just digested the tapeworm.
        if (p->has_trait("NOPAIN")) {
            p->add_msg_if_player(m_good, _("Your bowels clench as something inside them dies."));
        } else {
            p->add_msg_if_player(m_mixed, _("Your bowels spasm painfully as something inside them dies."));
            p->mod_pain(rng(8, 24));
        }
    }
    if (p->has_effect("bloodworms")) {
        p->remove_effect("bloodworms");
        p->add_msg_if_player(_("Your skin prickles and your veins itch for a few moments."));
    }
    if (p->has_effect("brainworm")) {
        p->remove_effect("brainworm");
        if (p->has_trait("NOPAIN")) {
            p->add_msg_if_player(m_good, _("The pressure inside your head feels better already."));
        } else {
            p->add_msg_if_player(m_mixed,
                                 _("Your head pounds like a sore tooth as something inside of it dies."));
            p->mod_pain(rng(8, 24));
        }
    }
    if (p->has_effect("paincysts")) {
        p->remove_effect("paincysts");
        if (p->has_trait("NOPAIN")) {
            p->add_msg_if_player(m_good, _("The stiffness in your joints goes away."));
        } else {
            p->add_msg_if_player(m_good, _("The pain in your joints goes away."));
        }
    }
    return it->type->charges_to_use();
}

int iuse::anticonvulsant(player *p, item *it, bool, point)
{
    p->add_msg_if_player(_("You take some anticonvulsant medication."));
    int duration = 4800 - p->str_cur * rng(0, 100);
    if (p->has_trait("TOLERANCE")) {
        duration -= 600;
    }
    if (p->has_trait("LIGHTWEIGHT")) {
        duration += 1200;
    }
    p->add_effect("valium", duration);
    p->add_effect("high", duration);
    if (p->has_effect("shakes")) {
        p->remove_effect("shakes");
        p->add_msg_if_player(m_good, _("You stop shaking."));
    }
    return it->type->charges_to_use();
}

int iuse::weed_brownie(player *p, item *it, bool, point)
{
    p->add_msg_if_player(_("You scarf down the delicious brownie. It tastes a little funny though..."));
    int duration = 120;
    if (p->has_trait("TOLERANCE")) {
        duration = 90;
    }
    if (p->has_trait("LIGHTWEIGHT")) {
        duration = 150;
    }
    p->hunger += 2;
    p->thirst += 6;
    if (p->pkill < 5) {
        p->pkill += 3;
        p->pkill *= 2;
    }
    p->add_effect("weed_high", duration);
    p->moves -= 100;
    if (one_in(5)) {
        weed_msg(p);
    }
    return it->type->charges_to_use();
}

int iuse::coke(player *p, item *it, bool, point)
{
    p->add_msg_if_player(_("You snort a bump of coke."));
    int duration = 21 - p->str_cur + rng(0, 10);
    if (p->has_trait("TOLERANCE")) {
        duration -= 10; // Symmetry would cause problems :-/
    }
    if (p->has_trait("LIGHTWEIGHT")) {
        duration += 20;
    }
    p->hunger -= 8;
    p->add_effect("high", duration);
    return it->type->charges_to_use();
}

int iuse::grack(player *p, item *it, bool, point)
{
    // Grack requires a fire source AND a pipe.
    if (p->has_amount("apparatus", 1) && p->use_charges_if_avail("fire", 1)) {
        p->add_msg_if_player(m_neutral, _("You smoke some Grack Cocaine."));
        p->add_msg_if_player(m_good, _("Time seems to stop."));
        int duration = 1000;
        if (p->has_trait("TOLERANCE")) {
            duration -= 10;
        } else if (p->has_trait("LIGHTWEIGHT")) {
            duration += 10;
        }
        p->hunger -= 10;
        p->add_effect("grack", duration);
        return it->type->charges_to_use();
    }
    return 0;
}

int iuse::meth(player *p, item *it, bool, point)
{
    int duration = 10 * (60 - p->str_cur);
    if (p->has_amount("apparatus", 1) && p->use_charges_if_avail("fire", 1)) {
        p->add_msg_if_player(m_neutral, _("You smoke your meth."));
        p->add_msg_if_player(m_good, _("The world seems to sharpen."));
        p->fatigue -= 375;
        if (p->has_trait("TOLERANCE")) {
            duration *= 1.2;
        } else {
            duration *= (p->has_trait("LIGHTWEIGHT") ? 1.8 : 1.5);
        }
        // breathe out some smoke
        for (int i = 0; i < 3; i++) {
            g->m.add_field(p->posx + int(rng(-2, 2)), p->posy + int(rng(-2, 2)), fd_methsmoke, 2);
        }
    } else {
        p->add_msg_if_player(_("You snort some crystal meth."));
        p->fatigue -= 300;
    }
    if (!p->has_effect("meth")) {
        duration += 600;
    }
    if (duration > 0) {
        int hungerpen = (p->str_cur < 10 ? 20 : 40 - p->str_cur);
        p->hunger -= hungerpen;
        p->add_effect("meth", duration);
    }
    return it->type->charges_to_use();
}

int iuse::vitamins(player *p, item *it, bool, point)
{
    p->add_msg_if_player(_("You take some vitamins."));
    p->mod_healthy_mod(50);
    return it->type->charges_to_use();
}

int iuse::vaccine(player *p, item *it, bool, point)
{
    p->add_msg_if_player(_("You inject the vaccine."));
    p->add_msg_if_player(m_good, _("You feel tough."));
    p->mod_healthy_mod(200);
    p->mod_pain(3);
    item syringe( "syringe", it->bday );
    p->i_add( syringe );
    return it->type->charges_to_use();
}

int iuse::flu_vaccine(player *p, item *it, bool, point)
{
    p->add_msg_if_player(_("You inject the vaccine."));
    p->add_msg_if_player(m_good, _("You no longer need to fear the flu."));
    p->add_effect("flushot", 1, num_bp, true);
    p->mod_pain(3);
    item syringe( "syringe", it->bday );
    p->i_add( syringe );
    return it->type->charges_to_use();
}

int iuse::poison(player *p, item *it, bool, point)
{
    if ((p->has_trait("EATDEAD"))) {
        return it->type->charges_to_use();
    } else if ((p->has_trait("EATPOISON")) && (!(one_in(p->str_cur / 2)))) {
        return it->type->charges_to_use();
    }
    p->add_effect("poison", 600);
    p->add_effect("foodpoison", 1800);
    return it->type->charges_to_use();
}

/**
 * Hallucinogenic with a fun effect. Specifically used to have a comestible
 * give a morale boost without it being noticeable by examining the item (ie,
 * for magic mushrooms).
 */
int iuse::fun_hallu(player *p, item *it, bool, point)
{
    it_comest *comest = dynamic_cast<it_comest *>(it->type);

    //Fake a normal food morale effect
    p->add_morale(MORALE_FOOD_GOOD, 18, 36, 60, 30, false, comest);
    if (!p->has_effect("hallu")) {
        p->add_effect("hallu", 3600);
    }
    return it->type->charges_to_use();
}

int iuse::thorazine(player *p, item *it, bool, point)
{
    p->fatigue += 5;
    p->remove_effect("hallu");
    p->remove_effect("visuals");
    p->remove_effect("high");
    if (!p->has_effect("dermatik")) {
        p->remove_effect("formication");
    }
    if (one_in(50)) {  // adverse reaction
        p->add_msg_if_player(m_bad, _("You feel completely exhausted."));
        p->fatigue += 15;
    } else {
        p->add_msg_if_player(m_warning, _("You feel a bit wobbly."));
    }
    return it->type->charges_to_use();
}

int iuse::prozac(player *p, item *it, bool, point)
{
    if (!p->has_effect("took_prozac") && p->morale_level() < 0) {
        p->add_effect("took_prozac", 7200);
    } else {
        p->stim += 3;
    }
    if (one_in(150)) {  // adverse reaction
        p->add_msg_if_player(m_warning, _("You suddenly feel hollow inside."));
    }
    return it->type->charges_to_use();
}

int iuse::sleep(player *p, item *it, bool, point)
{
    p->fatigue += 40;
    p->add_msg_if_player(m_warning, _("You feel very sleepy..."));
    return it->type->charges_to_use();
}

int iuse::iodine(player *p, item *it, bool, point)
{
    p->add_effect("iodine", 1200);
    p->add_msg_if_player(_("You take an iodine tablet."));
    return it->type->charges_to_use();
}

int iuse::datura(player *p, item *it, bool, point)
{
    p->add_effect("datura", rng(2000, 8000));
    p->add_msg_if_player(_("You eat the datura seed."));
    return it->type->charges_to_use();
}

int iuse::flumed(player *p, item *it, bool, point)
{
    p->add_effect("took_flumed", 6000);
    p->add_msg_if_player(_("You take some %s"), it->tname().c_str());
    return it->type->charges_to_use();
}

int iuse::flusleep(player *p, item *it, bool, point)
{
    p->add_effect("took_flumed", 7200);
    p->fatigue += 30;
    p->add_msg_if_player(_("You take some %s"), it->tname().c_str());
    p->add_msg_if_player(m_warning, _("You feel very sleepy..."));
    return it->type->charges_to_use();
}

int iuse::inhaler(player *p, item *it, bool, point)
{
    p->remove_effect("asthma");
    p->add_msg_if_player(m_neutral, _("You take a puff from your inhaler."));
    if (one_in(50)) {  // adverse reaction
        p->add_msg_if_player(m_bad, _("Your heart begins to race."));
        p->fatigue -= 10;
    }
    return it->type->charges_to_use();
}

int iuse::oxygen_bottle(player *p, item *it, bool, point)
{
    p->moves -= 500;
    p->add_msg_if_player(m_neutral, _("You breathe deeply from the %s"), it->tname().c_str());
    if (p->has_effect("smoke")) {
        p->remove_effect("smoke");
    } else if (p->has_effect("asthma")) {
        p->remove_effect("asthma");
    } else if (p->stim < 16) {
        p->stim += 8;
        p->pkill += 2;
    }
    p->pkill += 2;
    return it->type->charges_to_use();
}

int iuse::blech(player *p, item *it, bool, point)
{
    // TODO: Add more effects?
    if (it->is_drink()) {
        if (!query_yn(_("This looks unhealthy, sure you want to drink it?"))) {
            return 0;
        }
    } else { //Assume that if a blech consumable isn't a drink, it will be eaten.
        if (!query_yn(_("This looks unhealthy, sure you want to eat it?"))) {
            return 0;
        }
    }
    p->add_msg_if_player(m_bad, _("Blech, that burns your throat!"));
    if (it->type->id != "soap") { // soap burns but doesn't make you throw up
        p->vomit();
    }
    return it->type->charges_to_use();
}

int iuse::plantblech(player *p, item *it, bool, point pos)
{
    if (p->has_trait("THRESH_PLANT")) {
        double multiplier = -1;
        if (p->has_trait("CHLOROMORPH")) {
            multiplier = -3;
            p->add_msg_if_player(m_good, _("The meal is revitalizing."));
        } else{
            p->add_msg_if_player(m_good, _("Oddly enough, this doesn't taste so bad."));
        }
        it_comest *food = dynamic_cast<it_comest*>(it->type);
        //reverses the harmful values of drinking fertilizer
        p->hunger += food->nutr * multiplier;
        p->thirst -= food->quench * multiplier;
        p->mod_healthy_mod(food->healthy * multiplier);
        p->add_morale(MORALE_FOOD_GOOD, -10 * multiplier, 60, 60, 30, false, food);
        return it->type->charges_to_use();
    } else {
        return blech( p, it, true, pos );
    }
}

int iuse::chew(player *p, item *it, bool, point)
{
    // TODO: Add more effects?
    p->add_msg_if_player(_("You chew your %s."), it->tname().c_str());
    return it->type->charges_to_use();
}

static int marloss_reject_mutagen( player *p, item *it )
{
    // I've been unable to replicate the rejections on marloss berries
    // but best to be careful-KA101.
    if( (it->type->can_use( "MYCUS" )) || (it->type->can_use( "MARLOSS" )) ||
      (it->type->can_use( "MARLOSS_SEED" )) || (it->type->can_use( "MARLOSS_GEL" )) ) {
        return 0;
    }
    if (p->has_trait("THRESH_MARLOSS")) {
        p->add_msg_if_player(m_warning, _("The %s burns white-hot inside you, and you collapse to the ground!"), it->tname().c_str());
        p->vomit();
        p->mod_pain(35);
        // Lose a significant amount of HP, probably about 25-33%
        p->hurtall(rng(20, 35));
        // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        p->fall_asleep((3000 - p->int_cur * 10));
        // Marloss is too thoroughly into your body to be dislodged by orals.
        p->toggle_mutation("MUTAGEN_AVOID");
        p->add_msg_if_player(m_warning, _("That was some toxic %s!  Let's stick with Marloss next time, that's safe."), it->tname().c_str());
        p->add_memorial_log(pgettext("memorial_male", "Suffered a toxic marloss/mutagen reaction."),
                            pgettext("memorial_female", "Suffered a toxic marloss/mutagen reaction."));
        return it->type->charges_to_use();
    }

    if (p->has_trait("THRESH_MYCUS")) {
        p->add_msg_if_player(m_info, _("This is a contaminant.  We reject it from the Mycus."));
        if( p->has_trait("M_SPORES") || p->has_trait("M_FERTILE") ||
            p->has_trait("M_BLOSSOMS") || p->has_trait("M_BLOOM") ) {
            p->add_msg_if_player(m_good, _("We empty the %s and reflexively dispense spores onto the mess."));
            g->m.ter_set(p->posx, p->posy, t_fungus);
            p->add_memorial_log(pgettext("memorial_male", "Destroyed a harmful invader."),
                                pgettext("memorial_female", "Destroyed a harmful invader."));
            return it->type->charges_to_use();
        }
        else {
            p->add_msg_if_player(m_bad, _("We must eliminate this contaminant at the earliest opportunity."));
            return it->type->charges_to_use();
        }
    }
    return 0;
}

static int marloss_reject_mut_iv( player *p, item *it )
{
    if( it->type->can_use( "MYCUS" ) ) {
        return 0;
    }
    if (p->has_trait("THRESH_MARLOSS")) {
        p->add_msg_if_player(m_warning, _("The %s sears your insides white-hot, and you collapse to the ground!"), it->tname().c_str());
        p->vomit();
        p->mod_pain(55);
        // Lose a significant amount of HP, probably about 25-33%
        p->hurtall(rng(30, 45));
         // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        p->fall_asleep((4000 - p->int_cur * 10));
        // Injection does the trick.  Burn the fungus out.
        p->toggle_mutation("THRESH_MARLOSS");
        p->toggle_mutation("MUTAGEN_AVOID");
        //~ Recall that Marloss mutations made you feel warmth spreading throughout your body.  That's gone.
        p->add_msg_if_player(m_warning, _("You feel a cold burn inside, as though everything warm has left you."));
        if( it->type->can_use("PURIFY_IV") ) {
            p->add_msg_if_player(m_warning, _("It was probably that marloss--how did you know to call it \"marloss\" anyway?"));
            p->add_msg_if_player(m_warning, _("Best to stay clear of that alien crap in future."));
            p->add_memorial_log(pgettext("memorial_male", "Burned out a particularly nasty fungal infestation."),
                                pgettext("memorial_female", "Burned out a particularly nasty fungal infestation."));
        } else {
            p->add_memorial_log(pgettext("memorial_male", "Suffered a toxic marloss/mutagen reaction."),
                                pgettext("memorial_female", "Suffered a toxic marloss/mutagen reaction."));
        }
        return it->type->charges_to_use();
    }

    if (p->has_trait("THRESH_MYCUS")) {
        p->add_msg_if_player(m_info, _("This is a contaminant.  We reject it from the Mycus."));
        if( p->has_trait("M_SPORES") || p->has_trait("M_FERTILE") ||
            p->has_trait("M_BLOSSOMS") || p->has_trait("M_BLOOM") ) {
            p->add_msg_if_player(m_good, _("We empty the %s and reflexively dispense spores onto the mess."));
            g->m.ter_set(p->posx, p->posy, t_fungus);
            p->add_memorial_log(pgettext("memorial_male", "Destroyed a harmful invader."),
                                pgettext("memorial_female", "Destroyed a harmful invader."));
            return it->type->charges_to_use();
        } else {
            p->add_msg_if_player(m_bad, _("We must eliminate this contaminant at the earliest opportunity."));
            return it->type->charges_to_use();
        }
    }
    return 0;
}

int iuse::mutagen(player *p, item *it, bool, point)
{
    if (p->has_trait("MUTAGEN_AVOID")) {
         //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time? uh-uh.  You're not drinking that chemical stuff."));
        return 0;
    }

    if (!p->is_npc() && !(p->has_trait("THRESH_MYCUS"))) {
        p->add_memorial_log(pgettext("memorial_male", "Consumed mutagen."),
                            pgettext("memorial_female", "Consumed mutagen."));
    }

    if( marloss_reject_mutagen( p, it) ) {
        return it->type->charges_to_use();
    }

    if (p->has_trait("MUT_JUNKIE")) {
        p->add_msg_if_player(m_good, _("You quiver with anticipation..."));
        p->add_morale(MORALE_MUTAGEN, 5, 50);
    }
    std::string mutation_category;
    // Generic "mutagen".
    if (it->has_flag("MUTAGEN_STRONG")) {
        mutation_category = "";
        p->mutate();
        p->mod_pain(2 * rng(1, 5));
        p->hunger += 10;
        p->fatigue += 5;
        p->thirst += 10;
        if (!one_in(3)) {
            p->mutate();
            p->mod_pain(2 * rng(1, 5));
            p->hunger += 10;
            p->fatigue += 5;
            p->thirst += 10;
            if (one_in(4)) {
                p->add_msg_if_player(m_bad, _("You suddenly feel dizzy, and collapse to the ground."));
                p->add_effect("downed", 1);
            }
        }
        if (one_in(2)) {
            p->mutate();
            p->mod_pain(2 * rng(1, 5));
            p->hunger += 10;
            p->fatigue += 5;
            p->thirst += 10;
            p->add_msg_if_player(m_bad, _("Oops.  You must've blacked out for a minute there."));
            //Should be about 3 min, less 6 sec/IN point.
            p->fall_asleep((30 - p->int_cur));
        }
    }
    if (it->has_flag("MUTAGEN_WEAK")) {
        mutation_category = "";
        // Stuff like the limbs, the tainted tornado, etc.
        if (!one_in(3)) {
            p->mutate();
            p->mod_pain(2 * rng(1, 5));
            p->hunger += 10;
            p->fatigue += 5;
            p->thirst += 10;
            if (one_in(4)) {
                p->add_msg_if_player(m_bad, _("You suddenly feel dizzy, and collapse to the ground."));
                p->add_effect("downed", 1);
            }
        }
    } else {
        // Categorized/targeted mutagens go here.
        if (it->has_flag("MUTAGEN_PLANT")) {
            p->add_msg_if_player(_("You feel much closer to nature."));
            mutation_category = "MUTCAT_PLANT";
        } else if (it->has_flag("MUTAGEN_INSECT")) {
            p->add_msg_if_player(_("You hear buzzing, and feel your body harden."));
            mutation_category = "MUTCAT_INSECT";
        } else if (it->has_flag("MUTAGEN_SPIDER")) {
            p->add_msg_if_player(_("You feel insidious."));
            mutation_category = "MUTCAT_SPIDER";
        } else if (it->has_flag("MUTAGEN_SLIME")) {
            p->add_msg_if_player(_("Your body loses all rigidity for a moment."));
            mutation_category = "MUTCAT_SLIME";
        } else if (it->has_flag("MUTAGEN_FISH")) {
            p->add_msg_if_player(_("You are overcome by an overwhelming longing for the ocean."));
            mutation_category = "MUTCAT_FISH";
        } else if (it->has_flag("MUTAGEN_RAT")) {
            p->add_msg_if_player(_("You feel a momentary nausea."));
            mutation_category = "MUTCAT_RAT";
        } else if (it->has_flag("MUTAGEN_BEAST")) {
            p->add_msg_if_player(_("Your heart races and you see blood for a moment."));
            mutation_category = "MUTCAT_BEAST";
        } else if (it->has_flag("MUTAGEN_URSINE")) {
            p->add_msg_if_player(_("You feel an urge to...patrol? the forests?"));
            mutation_category = "MUTCAT_URSINE";
        } else if (it->has_flag("MUTAGEN_FELINE")) {
            p->add_msg_if_player(_("As you lap up the last of the mutagen, you wonder why..."));
            mutation_category = "MUTCAT_FELINE";
        } else if (it->has_flag("MUTAGEN_LUPINE")) {
            p->add_msg_if_player(_("You feel an urge to mark your territory. But then it passes."));
            mutation_category = "MUTCAT_LUPINE";
        } else if (it->has_flag("MUTAGEN_CATTLE")) {
            p->add_msg_if_player(_("Your mind and body slow down. You feel peaceful."));
            mutation_category = "MUTCAT_CATTLE";
        } else if (it->has_flag("MUTAGEN_CEPHALOPOD")) {
            p->add_msg_if_player(
                _("Your mind is overcome by images of eldritch horrors...and then they pass."));
            mutation_category = "MUTCAT_CEPHALOPOD";
        } else if (it->has_flag("MUTAGEN_BIRD")) {
            p->add_msg_if_player(_("Your body lightens and you long for the sky."));
            mutation_category = "MUTCAT_BIRD";
        } else if (it->has_flag("MUTAGEN_LIZARD")) {
            p->add_msg_if_player(_("For a heartbeat, your body cools down."));
            mutation_category = "MUTCAT_LIZARD";
        } else if (it->has_flag("MUTAGEN_TROGLOBITE")) {
            p->add_msg_if_player(_("You yearn for a cool, dark place to hide."));
            mutation_category = "MUTCAT_TROGLOBITE";
        } else if (it->has_flag("MUTAGEN_ALPHA")) {
            p->add_msg_if_player(_("You feel...better. Somehow."));
            mutation_category = "MUTCAT_ALPHA";
        } else if (it->has_flag("MUTAGEN_MEDICAL")) {
            p->add_msg_if_player(
                _("You can feel the blood rushing through your veins and a strange, medicated feeling washes over your senses."));
            mutation_category = "MUTCAT_MEDICAL";
        } else if (it->has_flag("MUTAGEN_CHIMERA")) {
            p->add_msg_if_player(_("You need to roar, bask, bite, and flap.  NOW."));
            mutation_category = "MUTCAT_CHIMERA";
        } else if (it->has_flag("MUTAGEN_ELFA")) {
            p->add_msg_if_player(_("Nature is becoming one with you..."));
            mutation_category = "MUTCAT_ELFA";
        } else if (it->has_flag("MUTAGEN_RAPTOR")) {
            p->add_msg_if_player(_("Mmm...sweet, bloody flavor...tastes like victory."));
            mutation_category = "MUTCAT_RAPTOR";
        }  // Yep, orals take a bit out of you too
        p->mutate_category(mutation_category);
        p->mod_pain(2 * rng(1, 5));
        p->hunger += 10;
        p->fatigue += 5;
        p->thirst += 10;
        if (one_in(4)) {
            p->add_msg_if_player(m_bad, _("You suddenly feel dizzy, and collapse to the ground."));
            p->add_effect("downed", 1);
        }
    }
    return it->type->charges_to_use();
}

int iuse::mut_iv(player *p, item *it, bool, point)
{
    if (p->has_trait("MUTAGEN_AVOID")) {
         //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time? uh-uh.  You're not injecting that chemical stuff."));
        return 0;
    }

    if (!p->is_npc() && !(p->has_trait("THRESH_MYCUS"))) {
        p->add_memorial_log(pgettext("memorial_male", "Injected mutagen."),
                            pgettext("memorial_female", "Injected mutagen."));
    }

    if( marloss_reject_mut_iv( p, it) ) {
        return it->type->charges_to_use();
    }

    if (p->has_trait("MUT_JUNKIE")) {
        p->add_msg_if_player(m_good, _("You quiver with anticipation..."));
        p->add_morale(MORALE_MUTAGEN, 10, 100);
    }
    std::string mutation_category;
    if (it->has_flag("MUTAGEN_STRONG")) {
        // 3 guaranteed mutations, 75%/66%/66% for the 4th/5th/6th,
        // 6-16 Pain per shot and potential knockdown/KO.
        mutation_category = "";
        if (p->has_trait("MUT_JUNKIE")) {
            p->add_msg_if_player(m_good, _("Oh, yeah! That's the stuff!"));
            g->sound(p->posx, p->posy, 15 + 3 * p->str_cur, _("YES! YES! YESSS!!!"));
        } else if (p->has_trait("NOPAIN")) {
            p->add_msg_if_player(_("You inject yourself."));
        } else {
            p->add_msg_if_player(m_bad, _("You inject yoursel-arRGH!"));
            g->sound(p->posx, p->posy, 15 + 3 * p->str_cur, _("You scream in agony!!"));
        }
        p->mutate();
        p->mod_pain(1 * rng(1, 4));
        //Standard IV-mutagen effect: 10 hunger/thirst & 5 Fatigue *per mutation*.
        // Numbers may vary based on mutagen.
        p->hunger += 10;
        p->fatigue += 5;
        p->thirst += 10;
        p->mutate();
        p->mod_pain(2 * rng(1, 3));
        p->hunger += 10;
        p->fatigue += 5;
        p->thirst += 10;
        p->mutate();
        p->hunger += 10;
        p->fatigue += 5;
        p->thirst += 10;
        p->mod_pain(3 * rng(1, 2));
        if (!one_in(4)) {
            p->mutate();
            p->hunger += 10;
            p->fatigue += 5;
            p->thirst += 10;
        }
        if (!one_in(3)) {
            p->mutate();
            p->hunger += 10;
            p->fatigue += 5;
            p->thirst += 10;
            p->add_msg_if_player(m_bad, _("You writhe and collapse to the ground."));
            p->add_effect("downed", rng(1, 4));
        }
        if (!one_in(3)) {
            //Jackpot! ...kinda, don't wanna go unconscious in dangerous territory
            p->mutate();
            p->hunger += 10;
            p->fatigue += 5;
            p->thirst += 10;
            p->add_msg_if_player(m_bad, _("It all goes dark..."));
            //Should be about 40 min, less 30 sec/IN point.
            p->fall_asleep((400 - p->int_cur * 5));
        }
    } else if (it->has_flag("MUTAGEN_ALPHA")) {
        //5-15 pain, 66% for each of the followups, so slightly better odds (designed for injection).
        mutation_category = "MUTCAT_ALPHA";
        p->add_msg_if_player(_("You took that shot like a champ!"));
        p->mutate_category("MUTCAT_ALPHA");
        p->mod_pain(3 * rng(1, 5));
        // Alpha doesn't make a lot of massive morphological changes, so less nutrients needed.
        p->hunger += 3;
        p->fatigue += 5;
        p->thirst += 3;
        if (!one_in(3)) {
            p->mutate_category("MUTCAT_ALPHA");
            p->hunger += 3;
            p->fatigue += 5;
            p->thirst += 3;
        }
        if (!one_in(3)) {
            p->mutate_category("MUTCAT_ALPHA");
            p->hunger += 3;
            p->fatigue += 5;
            p->thirst += 3;
        }
    } else if (it->has_flag("MUTAGEN_MEDICAL")) {
        // 2-6 pain, same as Alpha--since specifically intended for medical applications.
        mutation_category = "MUTCAT_MEDICAL";
        if (p->has_trait("MUT_JUNKIE")) {
            p->add_msg_if_player(_("Ahh, there it is. You can feel the mutagen again."));
        } else if (!(p->has_trait("MUT_JUNKIE"))) {
            p->add_msg_if_player(
                _("You can feel the blood in your medication stream. It's a strange feeling."));
        }
        p->mutate_category("MUTCAT_MEDICAL");
        p->mod_pain(2 * rng(1, 3));
        //Medical's are pretty much all physiology, IIRC
        p->hunger += 3;
        p->fatigue += 5;
        p->thirst += 3;
        if (!one_in(3)) {
            p->mutate_category("MUTCAT_MEDICAL");
            p->hunger += 3;
            p->fatigue += 5;
            p->thirst += 3;
        }
        if (!one_in(3)) {
            p->mutate_category("MUTCAT_MEDICAL");
            p->hunger += 3;
            p->fatigue += 5;
            p->thirst += 3;
        }
    } else if (it->has_flag("MUTAGEN_CHIMERA")) {
        // 24-36 pain, Scream,, -40 Morale,
        // but two guaranteed mutations and 75% each for third and fourth.
        mutation_category = "MUTCAT_CHIMERA";
        p->add_msg_if_player(m_bad, _("everyanimalthateverlived..bursting.from.YOU!"));
        p->mutate_category("MUTCAT_CHIMERA");
        p->mod_pain(4 * rng(1, 4));
        //Chimera's all about the massive morphological changes Done Quick, so lotsa nutrition needed.
        p->hunger += 20;
        p->fatigue += 20;
        p->thirst += 20;
        p->mutate_category("MUTCAT_CHIMERA");
        if (!(p->has_trait("NOPAIN"))) {
            p->mod_pain(20);
            g->sound(p->posx, p->posy, 25 + 3 * p->str_cur, _("You roar in agony!!"));
            p->add_morale(MORALE_MUTAGEN_CHIMERA, -40, -200);
        }
        p->hunger += 20;
        p->fatigue += 20;
        p->thirst += 20;
        if (!one_in(4)) {
            p->mutate_category("MUTCAT_CHIMERA");
            p->hunger += 20;
            p->fatigue += 10;
            p->thirst += 20;
        }
        if (!one_in(4)) {
            p->mutate_category("MUTCAT_CHIMERA");
            p->hunger += 20;
            p->thirst += 10;
            p->mod_pain(5);
            // Out for a while--long enough to receive another two injections
            // and wake up in hostile territory.
            p->add_msg_if_player(m_bad, _("With a final *pop*, you go out like a light."));
            p->fall_asleep(800 - p->int_cur * 5);
        }
    } else {
        // These categories for the most part share their effects,
        // so print their messages and any special effects,
        // then handle the mutation at the end in combined code.
        if (it->has_flag("MUTAGEN_PLANT")) {
            p->add_msg_if_player(_("You inject some nutrients into your phloem."));
            mutation_category = "MUTCAT_PLANT";
        } else if (it->has_flag("MUTAGEN_INSECT")) {
            p->add_msg_if_player(_("You sting yourself...for the Queen."));
            mutation_category = "MUTCAT_INSECT";
        } else if (it->has_flag("MUTAGEN_SPIDER")) {
            p->add_msg_if_player(_("Mmm...the *special* venom."));
            mutation_category = "MUTCAT_SPIDER";
        } else if (it->has_flag("MUTAGEN_SLIME")) {
            if (p->has_trait("MUT_JUNKIE")) {
                p->add_msg_if_player(_("Maybe if you drank enough, you'd become mutagen..."));
            } else if (!(p->has_trait("MUT_JUNKIE"))) {
                p->add_msg_if_player(_("This stuff takes you back. Downright primordial!"));
            }
            mutation_category = "MUTCAT_SLIME";
        } else if (it->has_flag("MUTAGEN_FISH")) {
            p->add_msg_if_player(_("Your pulse pounds as the waves."));
            mutation_category = "MUTCAT_FISH";
        } else if (it->has_flag("MUTAGEN_URSINE")) {
            p->add_msg_if_player(_("You feel yourself quite equipped for wilderness survival."));
            mutation_category = "MUTCAT_URSINE";
        } else if (it->has_flag("MUTAGEN_LUPINE")) {
            p->add_msg_if_player(_("As the mutagen hits you, your ears twitch and you stifle a yipe."));
            mutation_category = "MUTCAT_LUPINE";
        } else if (it->has_flag("MUTAGEN_FELINE")) {
            p->add_msg_if_player(_("Your back arches as the mutagen takes hold."));
            mutation_category = "MUTCAT_FELINE";
        } else if (it->has_flag("MUTAGEN_RAT")) {
            p->add_msg_if_player(_("You squeak as the shot hits you."));
            //~Sound of ratlike squeaking
            g->sound(p->posx, p->posy, 10, _("Eep!"));
            mutation_category = "MUTCAT_RAT";
        } else if (it->has_flag("MUTAGEN_BEAST")) {
            p->add_msg_if_player(_("Your heart races wildly as the injection takes hold."));
            mutation_category = "MUTCAT_BEAST";
        } else if (it->has_flag("MUTAGEN_CATTLE")) {
            //~rBGH is a bovine growth hormone, unpopular with consumers
            p->add_msg_if_player(_("You wonder if this is what rBGH feels like..."));
            mutation_category = "MUTCAT_CATTLE";
        } else if (it->has_flag("MUTAGEN_CEPHALOPOD")) {
            //~Zork reference, but it's talking about your blood vessels
            p->add_msg_if_player(_("You watch the mutagen flow through a maze of little twisty passages.\n\
            All the same."));
            mutation_category = "MUTCAT_CEPHALOPOD";
        } else if (it->has_flag("MUTAGEN_BIRD")) {
            p->add_msg_if_player(_("Your arms spasm in an oddly wavelike motion."));
            mutation_category = "MUTCAT_BIRD";
        } else if (it->has_flag("MUTAGEN_LIZARD")) {
            p->add_msg_if_player(_("Your blood cools down. The feeling is..different."));
            mutation_category = "MUTCAT_LIZARD";
        } else if (it->has_flag("MUTAGEN_TROGLOBITE")) {
            p->add_msg_if_player(_("As you press the plunger, it all goes so bright..."));
            mutation_category = "MUTCAT_TROGLOBITE";
        } else if (it->has_flag("MUTAGEN_ELFA")) {
            // 3-15 pain, morale boost, but no more mutagenic than cat-9s
            p->add_msg_if_player(_("Everything goes green for a second.\n\
            It's painfully beautiful..."));
            p->fall_asleep(20); //Should be out for two minutes.  Ecstasy Of Green
            // Extra helping of pain.
            p->mod_pain(rng(1, 5));
            p->add_morale(MORALE_MUTAGEN_ELFA, 25, 100);
            mutation_category = "MUTCAT_ELFA";
        } else if (it->has_flag("MUTAGEN_RAPTOR")) {
            //Little more painful than average, but nowhere near as harsh & effective as Chimera.
            p->add_msg_if_player(_("You distinctly smell the mutagen mixing with your blood\n\
            ...and then it passes."));
            mutation_category = "MUTCAT_RAPTOR";
        }
        p->mutate_category(mutation_category);
        p->mod_pain(2 * rng(1, 5));
        p->hunger += 10;
        // EkarusRyndren had the idea to add Fatigue and knockout,
        // though that's a bit much for every case
        p->fatigue += 5;
        p->thirst += 10;
        if (!one_in(3)) {
            p->mutate_category(mutation_category);
            p->hunger += 10;
            p->fatigue += 5;
            p->thirst += 10;
        }
        if (one_in(2)) {
            p->mutate_category(mutation_category);
            p->hunger += 10;
            p->fatigue += 5;
            p->thirst += 10;
        }
    }
    // Threshold-check.  You only get to cross once!
    if (p->crossed_threshold() == false) {
        // Threshold-breaching
        std::string primary = p->get_highest_category();
        int total = ((p->mutation_category_level["MUTCAT_LIZARD"]) +
                     (p->mutation_category_level["MUTCAT_BIRD"]) +
                     (p->mutation_category_level["MUTCAT_FISH"]) +
                     (p->mutation_category_level["MUTCAT_BEAST"]) +
                     (p->mutation_category_level["MUTCAT_FELINE"]) +
                     (p->mutation_category_level["MUTCAT_LUPINE"]) +
                     (p->mutation_category_level["MUTCAT_URSINE"]) +
                     (p->mutation_category_level["MUTCAT_CATTLE"]) +
                     (p->mutation_category_level["MUTCAT_INSECT"]) +
                     (p->mutation_category_level["MUTCAT_PLANT"]) +
                     (p->mutation_category_level["MUTCAT_SLIME"]) +
                     (p->mutation_category_level["MUTCAT_TROGLOBITE"]) +
                     (p->mutation_category_level["MUTCAT_CEPHALOPOD"]) +
                     (p->mutation_category_level["MUTCAT_SPIDER"]) +
                     (p->mutation_category_level["MUTCAT_RAT"]) +
                     (p->mutation_category_level["MUTCAT_MEDICAL"]) +
                     (p->mutation_category_level["MUTCAT_ALPHA"]) +
                     (p->mutation_category_level["MUTCAT_ELFA"]) +
                     (p->mutation_category_level["MUTCAT_CHIMERA"]) +
                     (p->mutation_category_level["MUTCAT_RAPTOR"]));
        // Only if you were pushing for more in your primary category.
        // You wanted to be more like it and less human.
        // That said, you're required to have hit third-stage dreams first.
        if ((mutation_category == primary) && (p->mutation_category_level[primary] > 50)) {
            // Little help for the categories that have a lot of crossover.
            // Starting with Ursine as that's... a bear to get.  8-)
            // Will add others if there's serious/demonstrable need.
            int booster = 0;
            if (mutation_category == "MUTCAT_URSINE") {
                booster = 50;
            }
            int breacher = (p->mutation_category_level[primary]) + booster;
            if (x_in_y(breacher, total)) {
                p->add_msg_if_player(m_good,
                                     _("Something strains mightily for a moment...and then..you're...FREE!"));
                if (mutation_category == "MUTCAT_LIZARD") {
                    p->toggle_mutation("THRESH_LIZARD");
                    p->add_memorial_log(pgettext("memorial_male", "Shed the ugly human skin."),
                                        pgettext("memorial_female", "Shed the ugly human skin."));
                } else if (mutation_category == "MUTCAT_BIRD") {
                    p->toggle_mutation("THRESH_BIRD");
                    p->add_memorial_log(pgettext("memorial_male", "Broke free of humanity."),
                                        pgettext("memorial_female", "Broke free of humanity."));
                } else if (mutation_category == "MUTCAT_FISH") {
                    p->toggle_mutation("THRESH_FISH");
                    p->add_memorial_log(pgettext("memorial_male", "Went deep."),
                                        pgettext("memorial_female", "Went deep."));
                } else if (mutation_category == "MUTCAT_BEAST") {
                    p->toggle_mutation("THRESH_BEAST");
                    p->add_memorial_log(pgettext("memorial_male", "Embraced his bestial nature."),
                                        pgettext("memorial_female", "Embraced her bestial nature."));
                } else if (mutation_category == "MUTCAT_FELINE") {
                    p->toggle_mutation("THRESH_FELINE");
                    p->add_memorial_log(pgettext("memorial_male", "Realized the dream."),
                                        pgettext("memorial_female", "Realized the dream."));
                } else if (mutation_category == "MUTCAT_LUPINE") {
                    p->toggle_mutation("THRESH_LUPINE");
                    p->add_memorial_log(pgettext("memorial_male", "Wolfed out."),
                                        pgettext("memorial_female", "Wolfed out."));
                } else if (mutation_category == "MUTCAT_URSINE") {
                    p->toggle_mutation("THRESH_URSINE");
                    // Manually removing Carnivore, since it tends to creep in
                    if (p->has_trait("CARNIVORE")) {
                        p->toggle_mutation("CARNIVORE");
                        p->add_msg_if_player(_("Your appetite for blood fades."));
                    }
                    p->add_memorial_log(pgettext("memorial_male", "Became one with the bears."),
                                        pgettext("memorial_female", "Became one with the bears."));
                } else if (mutation_category == "MUTCAT_CATTLE") {
                    p->toggle_mutation("THRESH_CATTLE");
                    p->add_memorial_log(pgettext("memorial_male", "Stopped worrying and learned to love the cowbell."),
                                        pgettext("memorial_female", "Stopped worrying and learned to love the cowbell."));
                } else if (mutation_category == "MUTCAT_INSECT") {
                    p->toggle_mutation("THRESH_INSECT");
                    p->add_memorial_log(pgettext("memorial_male", "Metamorphosed."),
                                        pgettext("memorial_female", "Metamorphosed."));
                } else if (mutation_category == "MUTCAT_PLANT") {
                    p->toggle_mutation("THRESH_PLANT");
                    p->add_memorial_log(pgettext("memorial_male", "Bloomed forth."),
                                        pgettext("memorial_female", "Bloomed forth."));
                } else if (mutation_category == "MUTCAT_SLIME") {
                    p->toggle_mutation("THRESH_SLIME");
                    p->add_memorial_log(pgettext("memorial_male", "Gave up on rigid human norms."),
                                        pgettext("memorial_female", "Gave up on rigid human norms."));
                } else if (mutation_category == "MUTCAT_TROGLOBITE") {
                    p->toggle_mutation("THRESH_TROGLOBITE");
                    p->add_memorial_log(pgettext("memorial_male", "Adapted to underground living."),
                                        pgettext("memorial_female", "Adapted to underground living."));
                } else if (mutation_category == "MUTCAT_CEPHALOPOD") {
                    p->toggle_mutation("THRESH_CEPHALOPOD");
                    p->add_memorial_log(pgettext("memorial_male", "Began living the dreams."),
                                        pgettext("memorial_female", "Began living the dreams."));
                } else if (mutation_category == "MUTCAT_SPIDER") {
                    p->toggle_mutation("THRESH_SPIDER");
                    p->add_memorial_log(pgettext("memorial_male", "Found a place in the web of life."),
                                        pgettext("memorial_female", "Found a place in the web of life."));
                } else if (mutation_category == "MUTCAT_RAT") {
                    p->toggle_mutation("THRESH_RAT");
                    p->add_memorial_log(pgettext("memorial_male", "Found that survival *is* everything."),
                                        pgettext("memorial_female", "Found that survival *is* everything."));
                } else if (mutation_category == "MUTCAT_MEDICAL") {
                    p->toggle_mutation("THRESH_MEDICAL");
                    p->add_memorial_log(pgettext("memorial_male", "Resumed clinical trials."),
                                        pgettext("memorial_female", "Resumed clinical trials."));
                } else if (mutation_category == "MUTCAT_ALPHA") {
                    p->toggle_mutation("THRESH_ALPHA");
                    p->add_memorial_log(pgettext("memorial_male", "Started representing."),
                                        pgettext("memorial_female", "Started representing."));
                } else if (mutation_category == "MUTCAT_ELFA") {
                    p->toggle_mutation("THRESH_ELFA");
                    p->add_memorial_log(pgettext("memorial_male", "Accepted a more natural way of life."),
                                        pgettext("memorial_female", "Accepted a more natural way of life."));
                } else if (mutation_category == "MUTCAT_CHIMERA") {
                    p->toggle_mutation("THRESH_CHIMERA");
                    p->add_memorial_log(pgettext("memorial_male", "United disunity."),
                                        pgettext("memorial_female", "United disunity."));
                } else if (mutation_category == "MUTCAT_RAPTOR") {
                    p->toggle_mutation("THRESH_RAPTOR");
                    p->add_memorial_log(pgettext("memorial_male", "Hatched."),
                                        pgettext("memorial_female", "Hatched."));
                }
            } else if (p->mutation_category_level[primary] > 100) {
                //~NOPAIN is a post-Threshold trait, so you shouldn't
                //~legitimately have it and get here!
                if (p->has_trait("NOPAIN")) {
                    p->add_msg_if_player(m_bad, _("You feel extremely Bugged."));
                } else {
                    p->add_msg_if_player(m_bad, _("You stagger with a piercing headache!"));
                    p->pain += 8;
                    p->add_effect("stunned", rng(3, 5));
                }
            } else if (p->mutation_category_level[primary] > 80) {
                if (p->has_trait("NOPAIN")) {
                    p->add_msg_if_player(m_bad, _("You feel very Bugged."));
                } else {
                    p->add_msg_if_player(m_bad, _("Your head throbs with memories of your life, before all this..."));
                    p->pain += 6;
                    p->add_effect("stunned", rng(2, 4));
                }
            } else if (p->mutation_category_level[primary] > 60) {
                if (p->has_trait("NOPAIN")) {
                    p->add_msg_if_player(m_bad, _("You feel Bugged."));
                } else {
                    p->add_msg_if_player(m_bad, _("Images of your past life flash before you."));
                    p->add_effect("stunned", rng(2, 3));
                }
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::purifier(player *p, item *it, bool, point)
{
    if (p->has_trait("MUTAGEN_AVOID")) {
         //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time? uh-uh.  You're not drinking that chemical stuff."));
        return 0;
    }

    if (!p->is_npc() && !(p->has_trait("THRESH_MYCUS"))) {
        p->add_memorial_log(pgettext("memorial_male", "Consumed purifier."),
                            pgettext("memorial_female", "Consumed purifier."));
    }

    if( marloss_reject_mutagen( p, it ) ) {
        it->type->charges_to_use();
    }

    std::vector<std::string> valid; // Which flags the player has
    for( auto &traits_iter : traits ) {
        if( p->has_trait( traits_iter.first ) && !p->has_base_trait( traits_iter.first ) ) {
            //Looks for active mutation
            valid.push_back( traits_iter.first );
        }
    }
    if (valid.empty()) {
        p->add_msg_if_player(_("You feel cleansed."));
        return it->type->charges_to_use();
    }
    int num_cured = rng(1, valid.size());
    if (num_cured > 4) {
        num_cured = 4;
    }
    for (int i = 0; i < num_cured && !valid.empty(); i++) {
        int index = rng(0, valid.size() - 1);
        if (p->purifiable(valid[index])) {
            p->remove_mutation(valid[index]);
        } else {
            p->add_msg_if_player(m_warning, _("You feel a slight itching inside, but it passes."));
        }
        valid.erase(valid.begin() + index);
    }
    return it->type->charges_to_use();
}

int iuse::purify_iv(player *p, item *it, bool, point)
{
    if (p->has_trait("MUTAGEN_AVOID")) {
         //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time? uh-uh.  You're not injecting that chemical stuff."));
        return 0;
    }

    if (!p->is_npc() && !(p->has_trait("THRESH_MYCUS"))) {
        p->add_memorial_log(pgettext("memorial_male", "Injected purifier."),
                            pgettext("memorial_female", "Injected purifier."));
    }

    if( marloss_reject_mut_iv( p, it ) ) {
        return it->type->charges_to_use();
    }

    std::vector<std::string> valid; // Which flags the player has
    for( auto &traits_iter : traits ) {
        if( p->has_trait( traits_iter.first ) && !p->has_base_trait( traits_iter.first ) ) {
            //Looks for active mutation
            valid.push_back( traits_iter.first );
        }
    }
    if (valid.empty()) {
        p->add_msg_if_player(_("You feel cleansed."));
        return it->type->charges_to_use();
    }
    int num_cured = rng(4,
                        valid.size()); //Essentially a double-strength purifier, but guaranteed at least 4.  Double-edged and all
    if (num_cured > 8) {
        num_cured = 8;
    }
    for (int i = 0; i < num_cured && !valid.empty(); i++) {
        int index = rng(0, valid.size() - 1);
        if (p->purifiable(valid[index])) {
            p->remove_mutation(valid[index]);
        } else {
            p->add_msg_if_player(m_warning, _("You feel a distinct burning inside, but it passes."));
        }
        valid.erase(valid.begin() + index);
        if (!(p->has_trait("NOPAIN"))) {
            p->mod_pain(2 * num_cured); //Hurts worse as it fixes more
            p->add_msg_if_player(m_warning, _("Feels like you're on fire, but you're OK."));
        }
        p->thirst += 2 * num_cured;
        p->hunger += 2 * num_cured;
        p->fatigue += 2 * num_cured;
    }
    return it->type->charges_to_use();
}

int iuse::marloss(player *p, item *it, bool t, point pos)
{
    if (p->is_npc()) {
        return it->type->charges_to_use();
    }
    if (p->has_trait("MARLOSS_AVOID")) {
        //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time? uh-uh.  You're not eating that alien poison sac."));
        return 0;
    }
    if (p->has_trait("THRESH_MYCUS")) {
        p->add_msg_if_player(m_info, _("We no longer require this scaffolding.  We reserve it for other uses."));
        return 0;
    }
    // If we have the marloss in our veins, we are a "breeder" and will spread
    // the fungus.
    p->add_memorial_log(pgettext("memorial_male", "Ate a marloss berry."),
                        pgettext("memorial_female", "Ate a marloss berry."));

    if (p->has_trait("MARLOSS") || p->has_trait("THRESH_MARLOSS")) {
        p->add_msg_if_player(m_good,
                             _("As you eat the berry, you have a near-religious experience, feeling at one with your surroundings..."));
        p->add_morale(MORALE_MARLOSS, 100, 1000);
        p->add_addiction(ADD_MARLOSS_B, 50);
        p->add_addiction(ADD_MARLOSS_Y, 50);
        p->hunger = -100;
        monster spore(GetMType("mon_spore"));
        spore.friendly = -1;
        int spore_spawned = 0;
        for (int x = p->posx - 4; x <= p->posx + 4; x++) {
            for (int y = p->posy - 4; y <= p->posy + 4; y++) {
                if (rng(0, 10) > trig_dist(x, y, p->posx, p->posy) &&
                    rng(0, 10) > trig_dist(x, y, p->posx, p->posy)) {
                    g->m.marlossify(x, y);
                }
                bool moveOK = (g->m.move_cost(x, y) > 0);
                bool monOK = g->mon_at(x, y) == -1;
                bool posOK = (p->posx != x || p->posy != y);
                if (moveOK && monOK && posOK &&
                    one_in(10 + 5 * trig_dist(x, y, p->posx, p->posy)) &&
                    (spore_spawned == 0 || one_in(spore_spawned * 2))) {
                    spore.spawn(x, y);
                    g->add_zombie(spore);
                    spore_spawned++;
                }
            }
        }
        return it->type->charges_to_use();
    }

    /* If we're not already carriers of Marloss, roll for a random effect:
     * 1 - Mutate
     * 2 - Mutate
     * 3 - Mutate
     * 4 - Purify
     * 5 - Purify
     * 6 - Cleanse radiation + Purify
     * 7 - Fully satiate
     * 8 - Vomit
     * 9 - Give Marloss mutation
     */
    int effect = rng(1, 9);
    if (effect <= 3) {
        p->add_msg_if_player(_("This berry tastes extremely strange!"));
        p->mutate();
        // Gruss dich, mutation drain, missed you!
        p->mod_pain(2 * rng(1, 5));
        p->hunger += 10;
        p->fatigue += 5;
        p->thirst += 10;
    } else if (effect <= 6) { // Radiation cleanse is below
        p->add_msg_if_player(m_good, _("This berry makes you feel better all over."));
        p->pkill += 30;
        this->purifier(p, it, t, pos);
        if (effect == 6) {
            p->radiation = 0;
        }
    } else if (effect == 7) {
        p->add_msg_if_player(m_good, _("This berry is delicious, and very filling!"));
        p->hunger = -100;
    } else if (effect == 8) {
        p->add_msg_if_player(m_bad, _("You take one bite, and immediately vomit!"));
        p->vomit();
    } else if (p->crossed_threshold()) { // Mycus Rejection.  Goo already present fights off the fungus.
        p->add_msg_if_player(m_bad, _("You feel a familiar warmth, but suddenly it surges into an excruciating burn as you convulse, vomiting, and black out..."));
        p->add_memorial_log(pgettext("memorial_male", "Suffered Marloss Rejection."),
                        pgettext("memorial_female", "Suffered Marloss Rejection."));
        p->vomit();
        p->vomit(); // Yes, make sure you're empty.
        p->mod_pain(90);
        p->hurtall(rng(40, 65));// No good way to say "lose half your current HP"
        p->fall_asleep((6000 - p->int_cur * 10)); // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        p->toggle_mutation("MARLOSS_BLUE");
        p->toggle_mutation("MARLOSS");
        p->toggle_mutation("MARLOSS_AVOID"); // And if you survive it's etched in your RNA, so you're unlikely to repeat the experiment.
        p->rem_addiction(ADD_MARLOSS_R);
        p->rem_addiction(ADD_MARLOSS_B);
        p->rem_addiction(ADD_MARLOSS_Y);
    } else if ( (p->has_trait("MARLOSS_BLUE") && p->has_trait("MARLOSS_YELLOW")) && (!p->has_trait("MARLOSS")) ) {
        p->add_msg_if_player(m_bad, _("You feel a familiar warmth, but suddenly it surges into painful burning as you convulse and collapse to the ground..."));
        p->fall_asleep((400 - p->int_cur * 5));
        p->toggle_mutation("MARLOSS_BLUE");
        p->toggle_mutation("MARLOSS_YELLOW");
        p->toggle_mutation("THRESH_MARLOSS");
        p->rem_addiction(ADD_MARLOSS_R);
        g->m.ter_set(p->posx, p->posy, t_marloss);
        p->add_memorial_log(pgettext("memorial_male", "Opened the Marloss Gateway."),
                        pgettext("memorial_female", "Opened the Marloss Gateway."));
        p->add_msg_if_player(m_good, _("You wake up in a marloss bush.  Almost *cradled* in it, actually, as though it grew there for you."));
        //~ Beginning to hear the Mycus while conscious: that's it speaking
        p->add_msg_if_player(m_good, _("unity. together we have reached the door. we provide the final key. now to pass through..."));
    } else if (!p->has_trait("MARLOSS")) {
        p->add_msg_if_player(_("You feel a strange warmth spreading throughout your body..."));
        p->toggle_mutation("MARLOSS");
        p->add_addiction(ADD_MARLOSS_B, 60);
        p->add_addiction(ADD_MARLOSS_Y, 60);
        p->rem_addiction(ADD_MARLOSS_R);
    }
    return it->type->charges_to_use();
}

int iuse::marloss_seed(player *p, item *it, bool t, point pos)
{
    if (p->is_npc()) {
        return it->type->charges_to_use();
    }
    if (p->has_trait("MARLOSS_AVOID")) {
        //~"Uh-uh" is a sound used for "nope", "no", etc.  "Drek" is a borrowed synonym for "shit".
        p->add_msg_if_player(m_warning, _("After what happened that last time? uh-uh.  You're not eating that alien drek."));
        return 0;
    }
    if (p->has_trait("THRESH_MYCUS")) {
        p->add_msg_if_player(m_info, _("We no longer require this scaffolding.  We reserve it for other uses."));
        return 0;
    }
    if (!(query_yn(_("Sure you want to eat the %s? You could plant it in a mound of dirt."),
                 it->tname().c_str())) ) {
        return 0; // Save the seed for later!
    }
    // If we have the marloss in our veins, we are a "breeder" and will spread
    // the fungus.
    p->add_memorial_log(pgettext("memorial_male", "Ate a marloss seed."),
                        pgettext("memorial_female", "Ate a marloss seed."));

    if (p->has_trait("MARLOSS_BLUE") || p->has_trait("THRESH_MARLOSS")) {
        p->add_msg_if_player(m_good,
                             _("As you eat the seed, you have a near-religious experience, feeling at one with your surroundings..."));
        p->add_morale(MORALE_MARLOSS, 100, 1000);
        p->add_addiction(ADD_MARLOSS_R, 50);
        p->add_addiction(ADD_MARLOSS_Y, 50);
        p->hunger = -100;
        monster spore(GetMType("mon_spore"));
        spore.friendly = -1;
        int spore_spawned = 0;
        for (int x = p->posx - 4; x <= p->posx + 4; x++) {
            for (int y = p->posy - 4; y <= p->posy + 4; y++) {
                if (rng(0, 10) > trig_dist(x, y, p->posx, p->posy) &&
                    rng(0, 10) > trig_dist(x, y, p->posx, p->posy)) {
                    g->m.marlossify(x, y);
                }
                bool moveOK = (g->m.move_cost(x, y) > 0);
                bool monOK = g->mon_at(x, y) == -1;
                bool posOK = (p->posx != x || p->posy != y);
                if (moveOK && monOK && posOK &&
                    one_in(10 + 5 * trig_dist(x, y, p->posx, p->posy)) &&
                    (spore_spawned == 0 || one_in(spore_spawned * 2))) {
                    spore.spawn(x, y);
                    g->add_zombie(spore);
                    spore_spawned++;
                }
            }
        }
        return it->type->charges_to_use();
    }

    /* If we're not already carriers of Marloss, roll for a random effect:
     * 1 - Mutate
     * 2 - Mutate
     * 3 - Mutate
     * 4 - Purify
     * 5 - Purify
     * 6 - Cleanse radiation + Purify
     * 7 - Fully satiate
     * 8 - Vomit
     * 9 - Give Marloss mutation
     */
    int effect = rng(1, 9);
    if (effect <= 3) {
        p->add_msg_if_player(_("This seed tastes extremely strange!"));
        p->mutate();
        // HELLO MY NAME IS MUTATION DRAIN YOU KILLED MY MUTAGEN PREPARE TO DIE! ;-)
        p->mod_pain(2 * rng(1, 5));
        p->hunger += 10;
        p->fatigue += 5;
        p->thirst += 10;
    } else if (effect <= 6) { // Radiation cleanse is below
        p->add_msg_if_player(m_good, _("This seed makes you feel better all over."));
        p->pkill += 30;
        this->purifier(p, it, t, pos);
        if (effect == 6) {
            p->radiation = 0;
        }
    } else if (effect == 7) {
        p->add_msg_if_player(m_good, _("This seed is delicious, and very filling!"));
        p->hunger = -100;
    } else if (effect == 8) {
        p->add_msg_if_player(m_bad, _("You take one bite, and immediately vomit!"));
        p->vomit();
    } else if (p->crossed_threshold()) { // Mycus Rejection.  Goo already present fights off the fungus.
        p->add_msg_if_player(m_bad, _("You feel a familiar warmth, but suddenly it surges into an excruciating burn as you convulse, vomiting, and black out..."));
        p->add_memorial_log(pgettext("memorial_male", "Suffered Marloss Rejection."),
                        pgettext("memorial_female", "Suffered Marloss Rejection."));
        p->vomit();
        p->vomit(); // Yes, make sure you're empty.
        p->mod_pain(90);
        p->hurtall(rng(40, 65));// No good way to say "lose half your current HP"
        p->fall_asleep((6000 - p->int_cur * 10)); // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        p->toggle_mutation("MARLOSS_BLUE");
        p->toggle_mutation("MARLOSS");
        p->toggle_mutation("MARLOSS_AVOID"); // And if you survive it's etched in your RNA, so you're unlikely to repeat the experiment.
        p->rem_addiction(ADD_MARLOSS_R);
        p->rem_addiction(ADD_MARLOSS_B);
        p->rem_addiction(ADD_MARLOSS_Y);
    } else if ( (p->has_trait("MARLOSS") && p->has_trait("MARLOSS_YELLOW")) && (!p->has_trait("MARLOSS_BLUE")) ) {
        p->add_msg_if_player(m_bad, _("You feel a familiar warmth, but suddenly it surges into painful burning as you convulse and collapse to the ground..."));
        p->fall_asleep((400 - p->int_cur * 5));
        p->toggle_mutation("MARLOSS");
        p->toggle_mutation("MARLOSS_YELLOW");
        p->toggle_mutation("THRESH_MARLOSS");
        p->rem_addiction(ADD_MARLOSS_B);
        g->m.ter_set(p->posx, p->posy, t_marloss);
        p->add_memorial_log(pgettext("memorial_male", "Opened the Marloss Gateway."),
                        pgettext("memorial_female", "Opened the Marloss Gateway."));
        p->add_msg_if_player(m_good, _("You wake up in a marloss bush.  Almost *cradled* in it, actually, as though it grew there for you."));
        //~ Beginning to hear the Mycus while conscious: that's it speaking
        p->add_msg_if_player(m_good, _("unity. together we have reached the door. we provide the final key. now to pass through..."));
    } else if (!p->has_trait("MARLOSS_BLUE")) {
        p->add_msg_if_player(_("You feel a strange warmth spreading throughout your body..."));
        p->toggle_mutation("MARLOSS_BLUE");
        p->add_addiction(ADD_MARLOSS_R, 60);
        p->add_addiction(ADD_MARLOSS_Y, 60);
        p->rem_addiction(ADD_MARLOSS_B);
    }
    return it->type->charges_to_use();
}

int iuse::marloss_gel(player *p, item *it, bool t, point pos)
{
    if (p->is_npc()) {
        return it->type->charges_to_use();
    }
    if (p->has_trait("MARLOSS_AVOID")) {
        //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time? uh-uh.  You're not eating that alien slime."));
        return 0;
    }
    if (p->has_trait("THRESH_MYCUS")) {
        p->add_msg_if_player(m_info, _("We no longer require this scaffolding.  We reserve it for other uses."));
        return 0;
    }
    // If we have the marloss in our veins, we are a "breeder" and will spread
    // the fungus.
    p->add_memorial_log(pgettext("memorial_male", "Ate some marloss jelly."),
                        pgettext("memorial_female", "Ate some marloss jelly."));

    if (p->has_trait("MARLOSS_YELLOW") || p->has_trait("THRESH_MARLOSS")) {
        p->add_msg_if_player(m_good,
                             _("As you eat the jelly, you have a near-religious experience, feeling at one with your surroundings..."));
        p->add_morale(MORALE_MARLOSS, 100, 1000);
        p->add_addiction(ADD_MARLOSS_R, 50);
        p->add_addiction(ADD_MARLOSS_B, 50);
        p->hunger = -100;
        monster spore(GetMType("mon_spore"));
        spore.friendly = -1;
        int spore_spawned = 0;
        for (int x = p->posx - 4; x <= p->posx + 4; x++) {
            for (int y = p->posy - 4; y <= p->posy + 4; y++) {
                if (rng(0, 10) > trig_dist(x, y, p->posx, p->posy) &&
                    rng(0, 10) > trig_dist(x, y, p->posx, p->posy)) {
                    g->m.marlossify(x, y);
                }
                bool moveOK = (g->m.move_cost(x, y) > 0);
                bool monOK = g->mon_at(x, y) == -1;
                bool posOK = (p->posx != x || p->posy != y);
                if (moveOK && monOK && posOK &&
                    one_in(10 + 5 * trig_dist(x, y, p->posx, p->posy)) &&
                    (spore_spawned == 0 || one_in(spore_spawned * 2))) {
                    spore.spawn(x, y);
                    g->add_zombie(spore);
                    spore_spawned++;
                }
            }
        }
        return it->type->charges_to_use();
    }

    /* If we're not already carriers of Marloss, roll for a random effect:
     * 1 - Mutate
     * 2 - Mutate
     * 3 - Mutate
     * 4 - Purify
     * 5 - Purify
     * 6 - Cleanse radiation + Purify
     * 7 - Fully satiate
     * 8 - Vomit
     * 9 - Give Marloss mutation
     */
    int effect = rng(1, 9);
    if (effect <= 3) {
        p->add_msg_if_player(_("This jelly tastes extremely strange!"));
        p->mutate();
        // hihi! wavewave! mutation draindrain!
        p->mod_pain(2 * rng(1, 5));
        p->hunger += 10;
        p->fatigue += 5;
        p->thirst += 10;
    } else if (effect <= 6) { // Radiation cleanse is below
        p->add_msg_if_player(m_good, _("This jelly makes you feel better all over."));
        p->pkill += 30;
        this->purifier(p, it, t, pos);
        if (effect == 6) {
            p->radiation = 0;
        }
    } else if (effect == 7) {
        p->add_msg_if_player(m_good, _("This jelly is delicious, and very filling!"));
        p->hunger = -100;
    } else if (effect == 8) {
        p->add_msg_if_player(m_bad, _("You take one bite, and immediately vomit!"));
        p->vomit();
    } else if (p->crossed_threshold()) { // Mycus Rejection.  Goo already present fights off the fungus.
        p->add_msg_if_player(m_bad, _("You feel a familiar warmth, but suddenly it surges into an excruciating burn as you convulse, vomiting, and black out..."));
        p->add_memorial_log(pgettext("memorial_male", "Suffered Marloss Rejection."),
                        pgettext("memorial_female", "Suffered Marloss Rejection."));
        p->vomit();
        p->vomit(); // Yes, make sure you're empty.
        p->mod_pain(90);
        p->hurtall(rng(40, 65));// No good way to say "lose half your current HP"
        p->fall_asleep((6000 - p->int_cur * 10)); // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        p->toggle_mutation("MARLOSS_BLUE");
        p->toggle_mutation("MARLOSS");
        p->toggle_mutation("MARLOSS_AVOID"); // And if you survive it's etched in your RNA, so you're unlikely to repeat the experiment.
        p->rem_addiction(ADD_MARLOSS_R);
        p->rem_addiction(ADD_MARLOSS_B);
        p->rem_addiction(ADD_MARLOSS_Y);
    } else if ( (p->has_trait("MARLOSS_BLUE") && p->has_trait("MARLOSS")) && (!p->has_trait("MARLOSS_YELLOW")) ) {
        p->add_msg_if_player(m_bad, _("You feel a familiar warmth, but suddenly it surges into painful burning as you convulse and collapse to the ground..."));
        p->fall_asleep((400 - p->int_cur * 5));
        p->toggle_mutation("MARLOSS_BLUE");
        p->toggle_mutation("MARLOSS");
        p->rem_addiction(ADD_MARLOSS_Y);
        p->toggle_mutation("THRESH_MARLOSS");
        g->m.ter_set(p->posx, p->posy, t_marloss);
        p->add_memorial_log(pgettext("memorial_male", "Opened the Marloss Gateway."),
                        pgettext("memorial_female", "Opened the Marloss Gateway."));
        p->add_msg_if_player(m_good, _("You wake up in a marloss bush.  Almost *cradled* in it, actually, as though it grew there for you."));
        //~ Beginning to hear the Mycus while conscious: that's it speaking
        p->add_msg_if_player(m_good, _("unity. together we have reached the door. we provide the final key. now to pass through..."));
    } else if (!p->has_trait("MARLOSS_YELLOW")) {
        p->add_msg_if_player(_("You feel a strange warmth spreading throughout your body..."));
        p->toggle_mutation("MARLOSS_YELLOW");
        p->add_addiction(ADD_MARLOSS_R, 60);
        p->add_addiction(ADD_MARLOSS_B, 60);
        p->rem_addiction(ADD_MARLOSS_Y);
    }
    return it->type->charges_to_use();
}

int iuse::mycus(player *p, item *it, bool t, point pos)
{
    if (p->is_npc()) {
        return it->type->charges_to_use();
    }
    // Welcome our guide.  Welcome.  To. The Mycus.
    if (p->has_trait("THRESH_MARLOSS")) {
        p->add_memorial_log(pgettext("memorial_male", "Became one with the Mycus."),
                        pgettext("memorial_female", "Became one with the Mycus."));
        p->add_msg_if_player(m_neutral, _("The apple tastes amazing, and you finish it quickly, not even noticing the lack of any core or seeds."));
        p->add_msg_if_player(m_good, _("You feel better all over."));
        p->pkill += 30;
        this->purifier(p, it, t, pos); // Clear out some of that goo you may have floating around
        p->radiation = 0;
        p->healall(4); // Can't make you a whole new person, but not for lack of trying
        p->add_msg_if_player(m_good, _("As the apple settles in, you feel ecstasy radiating through every part of your body..."));
        p->add_morale(MORALE_MARLOSS, 1000, 1000); // Last time you'll ever have it this good.  So enjoy.
        p->add_msg_if_player(m_good, _("Your eyes roll back in your head.  Everything dissolves into a blissful haze..."));
        p->fall_asleep((3000 - p->int_cur * 10));
        p->toggle_mutation("THRESH_MARLOSS");
        p->toggle_mutation("THRESH_MYCUS");
        //~ The Mycus does not use the term (or encourage the concept of) "you".  The PC is a local/native organism, but is now the Mycus.
        //~ It still understands the concept, but uninitelligent fungaloids and mind-bent symbiotes should not need it.
        //~ We are the Mycus.
        p->add_msg_if_player(m_good, _("We welcome into us.  We have endured long in this forbidding world."));
        p->add_msg_if_player(m_good, _("The natives have a saying: \"E Pluribus Unum\"  Out of many, one."));
        p->add_msg_if_player(m_good, _("We welcome the union of our lines in our local guide.  We will prosper, and unite this world."));
        p->add_msg_if_player(m_good, _("Even now, our fruits adapt to better serve local physiology."));
        p->add_msg_if_player(m_good, _("As, in time, shall we adapt to better welcome those who have not received us."));
        for (int x = p->posx - 3; x <= p->posx + 3; x++) {
            for (int y = p->posy - 3; y <= p->posy + 3; y++) {
                g->m.marlossify(x, y);
            }
        }
        p->rem_addiction(ADD_MARLOSS_R);
        p->rem_addiction(ADD_MARLOSS_B);
        p->rem_addiction(ADD_MARLOSS_Y);
    }
    else if (p->has_trait("THRESH_MYCUS") && !p->has_trait("M_DEPENDENT")) { // OK, now set the hook.
        if (!one_in(3)) {
            p->mutate_category("MUTCAT_MYCUS");
            p->hunger += 10;
            p->fatigue += 5;
            p->thirst += 10;
            p->add_morale(MORALE_MARLOSS, 25, 200); // still covers up mutation pain
        }
    } else if (p->has_trait("THRESH_MYCUS")) {
        p->pkill += 5;
        p->stim += 5;
    } else { // In case someone gets one without having been adapted first.
        // Marloss is the Mycus' method of co-opting humans.  Mycus fruit is for symbiotes' maintenance and development.
        p->add_msg_if_player(_("This apple tastes really weird!  You're not sure it's good for you..."));
        p->mutate();
        p->mod_pain(2 * rng(1, 5));
        p->hunger += 10;
        p->fatigue += 5;
        p->thirst += 10;
        p->vomit(); // no hunger/quench benefit for you
        p->mod_healthy_mod(-8);
    }
    return it->type->charges_to_use();
}

// TOOLS below this point!

int iuse::dogfood(player *p, item *, bool, point)
{
    int dirx, diry;
    if (!choose_adjacent(_("Put the dog food where?"), dirx, diry)) {
        return 0;
    }
    p->moves -= 15;
    int mon_dex = g->mon_at(dirx, diry);
    if (mon_dex != -1) {
        if (g->zombie(mon_dex).type->id == "mon_dog") {
            p->add_msg_if_player(m_good, _("The dog seems to like you!"));
            g->zombie(mon_dex).friendly = -1;
            g->zombie(mon_dex).add_effect("pet", 1, num_bp, true);
        } else {
            p->add_msg_if_player(_("The %s seems quite unimpressed!"),
                                 g->zombie(mon_dex).name().c_str());
        }
    } else {
        p->add_msg_if_player(m_bad, _("You spill the dogfood all over the ground."));
    }
    return 1;
}

int iuse::catfood(player *p, item *, bool, point)
{
    int dirx, diry;
    if (!choose_adjacent(_("Put the cat food where?"), dirx, diry)) {
        return 0;
    }
    p->moves -= 15;
    int mon_dex = g->mon_at(dirx, diry);
    if (mon_dex != -1) {
        if (g->zombie(mon_dex).type->id == "mon_cat") {
            p->add_msg_if_player(m_good,
                                 _("The cat seems to like you! Or maybe it just tolerates your presence better. It's hard to tell with cats."));
            g->zombie(mon_dex).friendly = -1;
        } else {
            p->add_msg_if_player(_("The %s seems quite unimpressed!"),
                                 g->zombie(mon_dex).name().c_str());
        }
    } else {
        p->add_msg_if_player(m_bad, _("You spill the cat food all over the ground."));
    }
    return 1;
}

static bool prep_firestarter_use(player *p, item *it, point &pos)
{
    if ((it->charges == 0) && (!it->has_flag("LENS"))){ // lenses do not need charges
        return false;
    }
    if( p->is_underwater() ) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return false;
    }
    if( !choose_adjacent(_("Light where?"), pos.x, pos.y) ) {
        return false;
    }
    if( pos.x == p->posx && pos.y == p->posy ) {
        p->add_msg_if_player(m_info, _("You would set yourself on fire."));
        p->add_msg_if_player(_("But you're already smokin' hot."));
        return false;
    }
    if( g->m.get_field( pos, fd_fire ) ) {
        // check if there's already a fire
        p->add_msg_if_player(m_info, _("There is already a fire."));
        return false;
    }
    if( g->m.flammable_items_at(pos.x, pos.y) ||
        g->m.has_flag("FLAMMABLE", pos.x, pos.y) || g->m.has_flag("FLAMMABLE_ASH", pos.x, pos.y) ) {
        return true;
    } else {
        p->add_msg_if_player(m_info, _("There's nothing to light there."));
        return false;
    }
}

int iuse::resolve_firestarter_use(player *p, item *, point pos)
{
    if( g->m.add_field( pos, fd_fire, 1, 100 ) ) {
        p->add_msg_if_player(_("You successfully light a fire."));
    }
    return 0;
}

int iuse::calculate_time_for_lens_fire (player *p, float light_level) {
    // base moves based on sunlight levels... 1 minute when sunny (80 lighting),
    // ~10 minutes when clear (60 lighting)
    float moves_base = std::pow( 80 / light_level, 8 ) * 1000 ;
    // survival 0 takes 3 * moves_base, survival 1 takes 1,5 * moves_base,
    // max moves capped at moves_base
    float moves_modifier = 1 / (p->skillLevel("survival") * 0.33 + 0.33);
    if (moves_modifier < 1) {
        moves_modifier = 1;
    }
    return int(moves_base * moves_modifier);
}

int iuse::firestarter(player *p, item *it, bool t, point pos)
{
    if (it->has_flag("LENS")) {
        // Needs the correct weather, light and to be outside.
        if( (g->weather == WEATHER_CLEAR || g->weather == WEATHER_SUNNY) &&
            g->natural_light_level() >= 60 && !g->m.has_flag("INDOORS", pos.x, pos.y) ) {
            if( prep_firestarter_use(p, it, pos ) ) {
                // turns needed for activity.
                const int turns = calculate_time_for_lens_fire(p, g->natural_light_level());
                if( turns/1000 > 1 ) {
                    // If it takes less than a minute, no need to inform the player about time.
                    p->add_msg_if_player(m_info, _("If the current weather holds, it will take around %d minutes to light a fire."), turns / 1000);
                }
                p->assign_activity(ACT_START_FIRE, turns, -1, p->get_item_position(it), it->tname());
                // Keep natural_light_level for comparing throughout the activity.
                p->activity.values.push_back(g->natural_light_level());
                p->activity.placement = pos;
                p->practice("survival", 5);
            }
        } else {
            p->add_msg_if_player(_("You need direct sunlight to light a fire with this."));
        }
    } else if( it->has_flag("FIRE_DRILL") ) {
        if( prep_firestarter_use(p, it, pos) ) {
            float skillLevel = float(p->skillLevel("survival"));
            // success chance is 100% but time spent is min 5 minutes at skill == 5 and
            // it increases for lower skill levels.
            // max time is 1 hour for 0 survival
            const float moves_base = 5 * 1000;
            if( skillLevel < 1 ) {
                // avoid dividing by zero. scaled so that skill level 0 means 60 minutes work
                skillLevel = 0.536;
            }
            // At survival=5 modifier=1, at survival=1 modifier=~6.
            float moves_modifier = std::pow( 5 / skillLevel, 1.113 );
            if (moves_modifier < 1) {
                moves_modifier = 1; // activity time improvement is capped at skillevel 5
            }
            const int turns = int (moves_base * moves_modifier);
            p->add_msg_if_player(m_info, _("At your skill level, it will take around %d minutes to light a fire."), turns / 1000);
            p->assign_activity(ACT_START_FIRE, turns, -1, p->get_item_position(it), it->tname());
            p->activity.placement = pos;
            p->practice("survival", 10);
            it->charges -= it->type->charges_to_use() * round(moves_modifier);
            return 0;
        }
    } else if (it->has_flag("REFILLABLE_LIGHTER")) {
        if( p->is_underwater() ) {
            p->add_msg_if_player(_("The lighter is extinguished."));
            it->make("ref_lighter");
            it->active = false;
            return 0;
        }
        if (t) {
            if (it->charges < it->type->charges_to_use()) {
                p->add_msg_if_player(_("The lighter burns out."));
                it->make("ref_lighter");
                it->active = false;
            }
        } else if (it->charges <= 0) {
            p->add_msg_if_player(_("The %s winks out."), it->tname().c_str());
        } else { // Turning it off
            int choice = menu(true, _("refillable lighter (lit)"), _("extinguish"),
                              _("light something"), _("cancel"), NULL);
            switch (choice) {
                case 1: {
                    p->add_msg_if_player(_("You extinguish the lighter."));
                    it->make("ref_lighter");
                    it->active = false;
                    return 0;
                }
                break;
                case 2:
                    if( prep_firestarter_use(p, it, pos) ) {
                        p->moves -= 15;
                        resolve_firestarter_use(p, it, pos);
                        return it->type->charges_to_use();
                    }
            }
        }
        return it->type->charges_to_use();
    } else { // common ligher or matches
        if( prep_firestarter_use(p, it, pos) ) {
            p->moves -= 15;
            resolve_firestarter_use( p, it, pos );
            return it->type->charges_to_use();
        }
    }
    return 0;
}

int iuse::sew(player *p, item *it, bool, point)
{
    if (it->charges == 0) {
        return 0;
    }
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    //minimum LL_LOW of LL_DARK + (ELFA_NV or atomic_light)
    if (p->fine_detail_vision_mod() > 4) {
        add_msg(m_info, _("You can't see to sew!"));
        return 0;
    }
    int thread_used = 1;

    int pos = g->inv_for_filter( _("Repair what?"), []( const item & itm ) {
        return itm.made_of( "cotton" ) ||
               itm.made_of( "leather" ) ||
               itm.made_of( "fur" ) ||
               itm.made_of( "nomex" ) ||
               itm.made_of( "felt_patch" );
    } );
    item *fix = &(p->i_at(pos));
    if (fix == NULL || fix->is_null()) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    //some items are made from more than one material. we should try to use both items if one type of repair item is missing
    itype_id repair_item = "none";
    std::vector<std::string> plurals;
    std::vector<itype_id> repair_items;
    std::string plural = "";
    //translation note: add <plural> tag to keep them unique
    if (fix->made_of("cotton")) {
        repair_items.push_back("rag");
        plurals.push_back(rm_prefix(_("<plural>rags")));
    }
    if (fix->made_of("leather")) {
        repair_items.push_back("leather");
        plurals.push_back(rm_prefix(_("<plural>leather")));
    }
    if (fix->made_of("fur")) {
        repair_items.push_back("fur");
        plurals.push_back(rm_prefix(_("<plural>fur")));
    }
    if (fix->made_of("nomex")) {
        repair_items.push_back("nomex");
        plurals.push_back(rm_prefix(_("<plural>nomex")));
    }
    if (fix->made_of("wool")) {
        repair_items.push_back("felt_patch");
        plurals.push_back(rm_prefix(_("<plural>wool")));
    }
    if (repair_items.empty()) {
        p->add_msg_if_player(m_info, _("Your %s is not made of fabric, leather, fur, or wool."),
                             fix->tname().c_str());
        return 0;
    }
    if( fix == it || std::find(repair_items.begin(), repair_items.end(), fix->typeId()) != repair_items.end()) {
        p->add_msg_if_player(m_info, _("This can be used to repair other items, not itself."));
        return 0;
    }

    int items_needed = (fix->damage > 2 || fix->damage == 0) ? 1 : 0;

    // this will cause issues if/when NPCs start being able to sew.
    // but, then again, it'll cause issues when they start crafting, too.
    const inventory &crafting_inv = p->crafting_inventory();
    bool bFound = false;
    //go through all discovered repair items and see if we have any of them available
    for( auto &repair_items_i : repair_items ) {
        if( crafting_inv.has_amount( repair_items_i, items_needed ) ) {
            //we've found enough of a material, use this one
            repair_item = repair_items_i;
            bFound = true;
        }
    }
    if (!bFound) {
        for (unsigned int i = 0; i < repair_items.size(); i++) {
            p->add_msg_if_player(m_info, _("You don't have enough %s to do that."), plurals[i].c_str());
        }
        return 0;
    }

    std::vector<item_comp> comps;
    comps.push_back(item_comp(repair_item, items_needed));

    if (fix->damage > 0) {
        p->moves -= 500 * p->fine_detail_vision_mod();
        p->practice("tailor", 8);
        int rn = dice(4, 2 + p->skillLevel("tailor"));
        rn -= rng(fix->damage, fix->damage * 2);
        if (p->dex_cur < 8 && one_in(p->dex_cur)) {
            rn -= rng(2, 6);
        }
        if (p->dex_cur >= 8 && (p->dex_cur >= 16 || one_in(16 - p->dex_cur))) {
            rn += rng(2, 6);
        }
        if (p->dex_cur > 16) {
            rn += rng(0, p->dex_cur - 16);
        }
        if (rn <= 4) {
            p->add_msg_if_player(m_bad, _("You damage your %s further!"), fix->tname().c_str());
            fix->damage++;
            if (fix->damage >= 5) {
                p->add_msg_if_player(m_bad, _("You destroy it!"));
                p->i_rem(pos);
            }
        } else if (rn <= 6) {
            p->add_msg_if_player(m_bad, _("You don't repair your %s, but you waste lots of thread."),
                                 fix->tname().c_str());
            thread_used = rng(1, 8);
        } else if (rn <= 8) {
            p->add_msg_if_player(m_mixed, _("You repair your %s, but waste lots of thread."),
                                 fix->tname().c_str());
            if (fix->damage >= 3) {
                p->consume_items(comps);
            }
            fix->damage--;
            thread_used = rng(1, 8);
        } else if (rn <= 16) {
            p->add_msg_if_player(m_good, _("You repair your %s!"), fix->tname().c_str());
            if (fix->damage >= 3) {
                p->consume_items(comps);
            }
            fix->damage--;
        } else {
            p->add_msg_if_player(m_good, _("You repair your %s completely!"), fix->tname().c_str());
            if (fix->damage >= 3) {
                p->consume_items(comps);
            }
            fix->damage = 0;
        }
    } else if (fix->damage == 0 || (fix->has_flag("VARSIZE") && !fix->has_flag("FIT"))) {
        p->moves -= 500 * p->fine_detail_vision_mod();
        p->practice("tailor", 10);
        int rn = dice(4, 2 + p->skillLevel("tailor"));
        if (p->dex_cur < 8 && one_in(p->dex_cur)) {
            rn -= rng(2, 6);
        }
        if (p->dex_cur >= 16 || (p->dex_cur > 8 && one_in(16 - p->dex_cur))) {
            rn += rng(2, 6);
        }
        if (p->dex_cur > 16) {
            rn += rng(0, p->dex_cur - 16);
        }
        if (rn <= 4) {
            p->add_msg_if_player(m_bad, _("You damage your %s!"), fix->tname().c_str());
            fix->damage++;
        } else if (rn >= 12 && fix->has_flag("VARSIZE") && !fix->has_flag("FIT")) {
            p->add_msg_if_player(m_good, _("You take your %s in, improving the fit."), fix->tname().c_str());
            fix->item_tags.insert("FIT");
        } else if (rn >= 12 && (fix->has_flag("FIT") || !fix->has_flag("VARSIZE"))) {
            p->add_msg_if_player(m_good, _("You make your %s extra sturdy."), fix->tname().c_str());
            fix->damage--;
            p->consume_items(comps);
        } else {
            p->add_msg_if_player(m_neutral, _("You practice your sewing."));
        }
    } else {
        p->add_msg_if_player(m_info, _("Your %s is already enhanced."), fix->tname().c_str());
        return 0;
    }

    return thread_used;
}

int iuse::extra_battery(player *p, item *, bool, point)
{
    int inventory_index = g->inv_for_filter( _("Modify what?"), []( const item & itm ) {
        it_tool *tl = dynamic_cast<it_tool *>(itm.type);
        return tl != nullptr && tl->ammo == "battery";
    } );
    item *modded = &( p->i_at( inventory_index ) );

    if (modded == NULL || modded->is_null()) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    if (!modded->is_tool()) {
        p->add_msg_if_player(m_info, _("This mod can only be used on tools."));
        return 0;
    }

    it_tool *tool = dynamic_cast<it_tool *>(modded->type);
    if (tool->ammo != "battery") {
        p->add_msg_if_player(m_info, _("That item does not use batteries!"));
        return 0;
    }

    if (modded->has_flag("DOUBLE_AMMO")) {
        p->add_msg_if_player(m_info, _("That item has already had its battery capacity doubled."));
        return 0;
    }

    remove_atomic_mod(*modded, *p);
    remove_recharge_mod(*modded, *p);
    remove_ups_mod(*modded, *p);

    p->add_msg_if_player( _( "You double the battery capacity of your %s!" ), modded->tname().c_str() );
    modded->item_tags.insert("DOUBLE_AMMO");
    return 1;
}

int iuse::rechargeable_battery(player *p, item *it, bool, point)
{
    int inventory_index = g->inv_for_filter( _("Modify what?"), []( const item & itm ) {
        it_tool *tl = dynamic_cast<it_tool *>(itm.type);
        return tl != nullptr && tl->ammo == "battery";
    } );
    item *modded = &( p->i_at( inventory_index ) );

    if (modded == NULL || modded->is_null()) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    if (!modded->is_tool()) {
        p->add_msg_if_player(m_info, _("This mod can only be used on tools."));
        return 0;
    }

    it_tool *tool = dynamic_cast<it_tool *>(modded->type);
    if (tool->ammo != "battery") {
        p->add_msg_if_player(m_info, _("That item does not use batteries!"));
        return 0;
    }

    if (modded->has_flag("RECHARGE")) {
        p->add_msg_if_player(m_info, _("That item already has a rechargeable battery pack."));
        return 0;
    }

    remove_atomic_mod(*modded, *p);
    remove_ups_mod(*modded, *p);
    remove_double_ammo_mod(*modded, *p);
    remove_ammo( modded, *p ); // remove batteries, replaced by charges from mod

    p->add_msg_if_player( _( "You replace the battery compartment of your %s with a rechargeable battery pack!" ), modded->tname().c_str() );
    modded->charges = it->charges;
    modded->item_tags.insert("RECHARGE");
    modded->item_tags.insert("NO_UNLOAD");
    modded->item_tags.insert("NO_RELOAD");
    return 1;
}

int iuse::atomic_battery(player *p, item *it, bool, point)
{
    int inventory_index = g->inv_for_filter( _("Modify what?"), []( const item & itm ) {
        it_tool *tl = dynamic_cast<it_tool *>(itm.type);
        return tl != nullptr && tl->ammo == "battery";
    } );
    item *modded = &( p->i_at( inventory_index ) );

    if (modded == NULL || modded->is_null()) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    if (!modded->is_tool()) {
        p->add_msg_if_player(m_info, _("This mod can only be used on tools."));
        return 0;
    }

    it_tool *tool = dynamic_cast<it_tool *>(modded->type);
    if (tool->ammo != "battery") {
        p->add_msg_if_player(m_info, _("That item does not use batteries!"));
        return 0;
    }

    if (modded->has_flag("ATOMIC_AMMO")) {
        p->add_msg_if_player(m_info,
                             _("That item has already had its battery modded to accept plutonium cells."));
        return 0;
    }

    remove_double_ammo_mod( *modded, *p );
    remove_recharge_mod( *modded, *p );
    remove_ups_mod( *modded, *p );
    remove_ammo( modded, *p ); // remove batteries, item::charges is now plutonium

    p->add_msg_if_player( _( "You modify your %s to run off plutonium cells!" ), modded->tname().c_str() );
    modded->item_tags.insert("ATOMIC_AMMO");
    modded->item_tags.insert("RADIOACTIVE");
    modded->item_tags.insert("LEAK_DAM");
    modded->item_tags.insert("NO_UNLOAD");
    modded->charges = it->charges;
    return 1;
}
int iuse::ups_battery(player *p, item *, bool, point)
{
    int inventory_index = g->inv_for_filter( _("Modify what?"), []( const item & itm ) {
        it_tool *tl = dynamic_cast<it_tool *>(itm.type);
        return tl != nullptr && tl->ammo == "battery";
    } );
    item *modded = &( p->i_at( inventory_index ) );

    if (modded == NULL || modded->is_null()) {
        p->add_msg_if_player(_("You do not have that item!"));
        return 0;
    }
    if (!modded->is_tool()) {
        p->add_msg_if_player(_("This mod can only be used on tools."));
        return 0;
    }

    it_tool *tool = dynamic_cast<it_tool *>(modded->type);
    if (tool->ammo != "battery") {
        p->add_msg_if_player(_("That item does not use batteries!"));
        return 0;
    }

    if (modded->has_flag("USE_UPS")) {
        p->add_msg_if_player(_("That item has already had its battery modded to use a UPS!"));
        return 0;
    }
    if( modded->typeId() == "UPS_off" || modded->typeId() == "adv_UPS_off" ) {
        p->add_msg_if_player( _( "You want to power a UPS with another UPS?  Very clever." ) );
        return 0;
    }

    remove_double_ammo_mod(*modded, *p);
    remove_recharge_mod(*modded, *p);
    remove_atomic_mod(*modded, *p);
    remove_ammo(modded, *p);

    p->add_msg_if_player( _( "You modify your %s to run off a UPS!" ), modded->tname().c_str() );
    modded->item_tags.insert("USE_UPS");
    modded->item_tags.insert("NO_UNLOAD");
    modded->item_tags.insert("NO_RELOAD");
    //Perhaps keep the modded charges at 1 or 0?
    modded->charges = 0;
    return 1;
}

int iuse::remove_all_mods(player *p, item *, bool, point)
{
    int inventory_index = g->inv_for_filter( _( "Detach battery mods from what?" ), []( const item & itm ) {
        it_tool *tl = dynamic_cast<it_tool *>(itm.type);
        return tl != nullptr && ( itm.has_flag("DOUBLE_AMMO") || itm.has_flag("RECHARGE") || 
                                  itm.has_flag("USE_UPS") || itm.has_flag("ATOMIC_AMMO") );
    } );
    item *modded = &( p->i_at( inventory_index ) );
    if (modded == NULL || modded->is_null()) {
        p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
        return 0;
    }

    if (!modded->is_tool()) {
        p->add_msg_if_player( m_info, _( "Only battery mods for tools can be removed this way." ) );
        return 0;
    }

    it_tool *tool = dynamic_cast<it_tool *>(modded->type);
    if (tool->ammo != "battery") {
        p->add_msg_if_player( m_info, _( "That item does not use batteries!" ) );
        return 0;
    }

    remove_double_ammo_mod( *modded, *p );
    remove_recharge_mod( *modded, *p );
    remove_atomic_mod( *modded, *p );
    remove_ups_mod( *modded, *p );
    return 0;
}

int iuse::fishing_rod(player *p, item *it, bool, point)
{
    int dirx, diry;

    if (!choose_adjacent(_("Fish where?"), dirx, diry)) {
        return 0;
    }

    if (!g->m.has_flag("FISHABLE", dirx, diry)) {
        p->add_msg_if_player(m_info, _("You can't fish there!"));
        return 0;
    }
    point op = overmapbuffer::ms_to_omt_copy( g->m.getabs( dirx, diry ) );
    if (!otermap[overmap_buffer.ter(op.x, op.y, g->levz)].is_river) {
        p->add_msg_if_player(m_info, _("That water does not contain any fish. Try a river instead."));
        return 0;
    }
    std::vector<monster*> fishables = g->get_fishable(60);
    if ( fishables.size() < 1){
        p->add_msg_if_player(m_info, _("There are no fish around. Try another spot.")); // maybe let the player find that out by himself?
        return 0;
    }
    p->rooted_message();

    p->add_msg_if_player(_("You cast your line and wait to hook something..."));

    p->assign_activity(ACT_FISH, 30000, 0, p->get_item_position(it), it->tname());

    return 0;
}

int iuse::fish_trap(player *p, item *it, bool t, point pos)
{
    if (!t) {
        // Handle deploying fish trap.
        if (it->active) {
            it->active = false;
            return 0;
        }

        if (it->charges < 0) {
            it->charges = 0;
            return 0;
        }

        if (p->is_underwater()) {
            p->add_msg_if_player(m_info, _("You can't do that while underwater."));
            return 0;
        }

        if (it->charges == 0) {
            p->add_msg_if_player(_("Fishes are not silly to go in here without bait."));
            return 0;
        }

        int dirx, diry;

        if (!choose_adjacent(_("Put fish trap where?"), dirx, diry)) {
            return 0;
        }
        if (!g->m.has_flag("FISHABLE", dirx, diry)) {
            p->add_msg_if_player(m_info, _("You can't fish there!"));
            return 0;
        }
        point op = overmapbuffer::ms_to_omt_copy(g->m.getabs(dirx, diry));
        if (!otermap[overmap_buffer.ter(op.x, op.y, g->levz)].is_river) {
            p->add_msg_if_player(m_info, _("That water does not contain any fish, try a river instead."));
            return 0;
        }
        std::vector<monster*> fishables = g->get_fishable(60);
        if ( fishables.size() < 1){
            p->add_msg_if_player(m_info, _("There is no fish around. Try another spot.")); // maybe let the player find that out by himself?
            return 0;
        }
        it->active = true;
        it->bday = calendar::turn;
        g->m.add_item_or_charges(dirx, diry, *it);
        p->i_rem(it);
        p->add_msg_if_player(m_info, _("You place the fish trap, in three hours or so you may catch some fish."));

        return 0;

    } else {
        // Handle processing fish trap over time.
        if (it->charges == 0) {
            it->active = false;
            return 0;
        }
        //after 3 hours.
        if (calendar::turn - it->bday > 1800) {
            it->active = false;

            if (!g->m.has_flag("FISHABLE", pos.x, pos.y)) {
                return 0;
            }
            point op = overmapbuffer::ms_to_omt_copy( g->m.getabs( pos.x, pos.y ) );
            if (!otermap[overmap_buffer.ter(op.x, op.y, g->levz)].is_river) {
                return 0;
            }
            int success = -50;
            const int surv = p->skillLevel("survival");
            const int attempts = rng(it->charges, it->charges * it->charges);
            for (int i = 0; i < attempts; i++) {
                success += rng(surv, surv * surv);
            }

            it->charges = rng(-1, it->charges);
            if (it->charges < 0) {
                it->charges = 0;
            }

            int fishes = 0;

            if (success < 0) {
                fishes = 0;
            } else if (success < 300) {
                fishes = 1;
            } else if (success < 1500) {
                fishes = 2;
            } else {
                fishes = rng(3, 5);
            }

            if (fishes == 0) {
                it->charges = 0;
                p->practice("survival", rng(5, 15));

                return 0;
            }
            std::vector<monster*> fishables = g->get_fishable(60); //get the fishables around the trap's spot
            for (int i = 0; i < fishes; i++) {
                p->practice("survival", rng(3, 10));
                if (fishables.size() > 1){
                    g->catch_a_monster(fishables, pos.x, pos.y, p, 180000); //catch the fish! 180000 is the time spent fishing.
                } else {
                    //there will always be a chance that the player will get lucky and catch a fish
                    //not existing in the fishables vector. (maybe it was in range, but wandered off)
                    //lets say it is a 5% chance per fish to catch
                    if (one_in(20)) {
                        item fish;
                        std::vector<std::string> fish_group = MonsterGroupManager::GetMonstersFromGroup("GROUP_FISH");
                        std::string fish_mon = fish_group[rng(1, fish_group.size()) - 1];
                        fish.make_corpse("corpse", GetMType(fish_mon), it->bday + rng(0, 1800)); //we don't know when it was caught. its random
                        //Yes, we can put fishes in the trap like knives in the boot,
                        //and then get fishes via activation of the item,
                        //but it's not as comfortable as if you just put fishes in the same tile with the trap.
                        //Also: corpses and comestibles do not rot in containers like this, but on the ground they will rot.
                        g->m.add_item_or_charges(pos.x, pos.y, fish);
                        break; //this can happen only once
                    }
                }
            }
        }
        return 0;
    }
}

int iuse::extinguisher(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    g->draw();
    int x, y;
    // If anyone other than the player wants to use one of these,
    // they're going to need to figure out how to aim it.
    if (!choose_adjacent(_("Spray where?"), x, y)) {
        return 0;
    }

    p->moves -= 140;

    // Reduce the strength of fire (if any) in the target tile.
    g->m.adjust_field_strength(point(x, y), fd_fire, 0 - rng(2, 3));

    // Also spray monsters in that tile.
    int mondex = g->mon_at(x, y);
    if (mondex != -1) {
        g->zombie(mondex).moves -= 150;
        if (g->u_see(&(g->zombie(mondex)))) {
            p->add_msg_if_player(_("The %s is sprayed!"), g->zombie(mondex).name().c_str());
        }
        if (g->zombie(mondex).made_of(LIQUID)) {
            if (g->u_see(&(g->zombie(mondex)))) {
                p->add_msg_if_player(_("The %s is frozen!"), g->zombie(mondex).name().c_str());
            }
            monster &critter = g->zombie( mondex );
            critter.apply_damage( p, bp_torso, rng( 20, 60 ) );
            critter.set_speed_base( critter.get_speed_base() / 2 );
        }
    }

    // Slightly reduce the strength of fire immediately behind the target tile.
    if (g->m.move_cost(x, y) != 0) {
        x += (x - p->posx);
        y += (y - p->posy);

        g->m.adjust_field_strength(point(x, y), fd_fire, std::min(0 - rng(0, 1) + rng(0, 1), 0L));
    }

    return it->type->charges_to_use();
}

int iuse::rm13armor_off(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_info, _("The RM13 combat armor's fuel cells are dead."),
                             it->tname().c_str());
        return 0;
    } else {
        std::string oname = it->type->id + "_on";
        p->add_msg_if_player(_("You activate your RM13 combat armor."));
        p->add_msg_if_player(_("Rivtech Model 13 RivOS v2.19:   ONLINE."));
        p->add_msg_if_player(_("CBRN defense system:            ONLINE."));
        p->add_msg_if_player(_("Acoustic dampening system:      ONLINE."));
        p->add_msg_if_player(_("Thermal regulation system:      ONLINE."));
        p->add_msg_if_player(_("Vision enhancement system:      ONLINE."));
        p->add_msg_if_player(_("Electro-reactive armor system:  ONLINE."));
        p->add_msg_if_player(_("All systems nominal."));
        it->make(oname);
        it->active = true;
        return it->type->charges_to_use();
    }
}

int iuse::rm13armor_on(player *p, item *it, bool t, point)
{
    if (t) { // Normal use
    } else { // Turning it off
        std::string oname = it->type->id;
        if (oname.length() > 3 && oname.compare(oname.length() - 3, 3, "_on") == 0) {
            oname.erase(oname.length() - 3, 3);
        } else {
            debugmsg("no item type to turn it into (%s)!", oname.c_str());
            return 0;
        }
        p->add_msg_if_player(_("RivOS v2.19 shutdown sequence initiated."));
        p->add_msg_if_player(_("Shutting down."));
        p->add_msg_if_player(_("Your RM13 combat armor turns off."));
        it->make(oname);
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::unpack_item(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    std::string oname = it->type->id + "_on";
    p->moves -= 300;
    p->add_msg_if_player(_("You unpack your %s for use."), it->tname().c_str());
    it->make(oname);
    it->active = false;
    return 0;
}

int iuse::pack_item(player *p, item *it, bool t, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (t) { // Normal use
        // Numbers below -1 are reserved for worn items
    } else if (p->get_item_position(it) < -1) {
        p->add_msg_if_player(m_info, _("You can't pack your %s until you take it off."),
                             it->tname().c_str());
        return 0;
    } else { // Turning it off
        std::string oname = it->type->id;
        if (oname.length() > 3 && oname.compare(oname.length() - 3, 3, "_on") == 0) {
            oname.erase(oname.length() - 3, 3);
        } else {
            debugmsg("no item type to turn it into (%s)!", oname.c_str());
            return 0;
        }
        p->moves -= 500;
        p->add_msg_if_player(_("You pack your %s for storage."), it->tname().c_str());
        it->make(oname);
        it->active = false;
    }
    return 0;
}

static bool cauterize_effect(player *p, item *it, bool force = true)
{
    hp_part hpart = use_healing_item(p, it, -2, -2, -2, 100, 50, 0, force);
    if (hpart != num_hp_parts) {
        p->add_msg_if_player(m_neutral, _("You cauterize yourself."));
        if (!(p->has_trait("NOPAIN"))) {
            p->mod_pain(15);
            p->add_msg_if_player(m_bad, _("It hurts like hell!"));
        } else {
            p->add_msg_if_player(m_neutral, _("It itches a little."));
        }
        body_part bp = num_bp;
        p->hp_convert(hpart, bp);
        if (p->has_effect("bite", bp)) {
            p->add_effect("bite", 2600, bp, true);
        }
        return true;
    }
    return 0;
}

static int cauterize_elec(player *p, item *it)
{
    if (it->charges == 0) {
        p->add_msg_if_player(m_info, _("You need batteries to cauterize wounds."));
        return 0;
    } else if (!p->has_effect("bite") && !p->has_effect("bleed") && !p->is_underwater()) {
        if ((p->has_trait("MASOCHIST") || p->has_trait("MASOCHIST_MED") || p->has_trait("CENOBITE")) &&
            query_yn(_("Cauterize yourself for fun?"))) {
            return cauterize_effect(p, it, true) ? it->type->charges_to_use() : 0;
        } else {
            p->add_msg_if_player(m_info,
                                 _("You are not bleeding or bitten, there is no need to cauterize yourself."));
            return 0;
        }
    } else if (p->is_npc() || query_yn(_("Cauterize any open wounds?"))) {
        return cauterize_effect(p, it, true) ? it->type->charges_to_use() : 0;
    }
    return 0;
}

int iuse::solder_weld(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    int choice = 2;
    int charges_used = (dynamic_cast<it_tool *>(it->type))->charges_to_use();

    // Option for cauterization only if player has the incentive to do so
    // One does not check for open wounds with a soldering iron.
    if ((p->has_effect("bite") || p->has_effect("bleed")) && !p->is_underwater()) {
        choice = menu(true, _("Using soldering item:"), _("Cauterize wound"),
                      _("Repair plastic/metal/kevlar item"), _("Cancel"), NULL);
    } else if (p->has_trait("MASOCHIST") || p->has_trait("MASOCHIST_MED") ||
               p->has_trait("CENOBITE")) {
        // Masochists might be wounded too, let's not ask twice.
        choice = menu(true, _("Using soldering item:"), _("Cauterize yourself for fun"),
                      _("Repair plastic/metal/kevlar item"), _("Cancel"), NULL);
    }

    switch (choice) {
        case 1:
            return cauterize_elec(p, it);
            break;
        case 2: {
            if (it->charges <= 0) {
                p->add_msg_if_player(m_info, _("Your tool does not have enough charges to do that."));
                return 0;
            }

            int pos = g->inv_for_filter( _("Repair what?"), []( const item & itm ) {
                return itm.made_of( "kevlar" ) ||
                       itm.made_of( "plastic" ) ||
                       itm.made_of( "iron" ) ||
                       itm.made_of( "steel" ) ||
                       itm.made_of( "hardsteel" );
            } );
            item *fix = &(p->i_at(pos));
            if (fix == NULL || fix->is_null()) {
                p->add_msg_if_player(m_info, _("You do not have that item!"));
                return 0;
            }
            if (fix->is_gun()) {
                p->add_msg_if_player(m_info, _("That requires gunsmithing tools."));
                return 0;
            }
            if (fix->is_ammo()) {
                p->add_msg_if_player(m_info, _("You cannot repair this type of item."));
                return 0;
            }
            itype_id repair_item = "none";
            std::vector<std::string> repairitem_names;
            std::vector<itype_id> repair_items;
            if (fix->made_of("kevlar")) {
                repair_items.push_back("kevlar_plate");
                repairitem_names.push_back(_("kevlar plates"));
            }
            if (fix->made_of("plastic")) {
                repair_items.push_back("plastic_chunk");
                repairitem_names.push_back(_("plastic chunks"));
            }
            if (fix->made_of("iron") || fix->made_of("steel") || fix->made_of("hardsteel")) {
                repair_items.push_back("scrap");
                repairitem_names.push_back(_("scrap metal"));
            }
            if (repair_items.empty()) {
                p->add_msg_if_player(m_info, _("Your %s is not made of plastic, metal, or kevlar."),
                                     fix->tname().c_str());
                return 0;
            }
            if( fix == it || std::find(repair_items.begin(), repair_items.end(), fix->typeId()) != repair_items.end()) {
                p->add_msg_if_player(m_info, _("This can be used to repair other items, not itself."));
                return 0;
            }

            //repairing or modifying items requires at least 1 repair item,
            // otherwise number is related to size of item
            int items_needed = ceil(fix->volume() * 0.25);

            // this will cause issues if/when NPCs start being able to sew.
            // but, then again, it'll cause issues when they start crafting, too.
            const inventory &crafting_inv = p->crafting_inventory();

            bool bFound = false;
            //go through all discovered repair items and see if we have any of them available
            for( auto &repair_items_i : repair_items ) {
                if( crafting_inv.has_amount( repair_items_i, items_needed ) ) {
                    //we've found enough of a material, use this one
                    repair_item = repair_items_i;
                    bFound = true;
                }
            }
            if (!bFound) {
                for (unsigned int i = 0; i < repair_items.size(); i++) {
                    p->add_msg_if_player(m_info, _("You don't have enough %s to do that."),
                                         repairitem_names[i].c_str());
                }
                return 0;
            }

            std::vector<item_comp> comps;
            comps.push_back(item_comp(repair_item, items_needed));

            if (fix->damage > 0) {
                p->moves -= 500 * p->fine_detail_vision_mod();
                p->practice("mechanics", 8);
                int rn = dice(4, 2 + p->skillLevel("mechanics"));
                rn -= rng(fix->damage, fix->damage * 2);
                if (p->dex_cur < 8 && one_in(p->dex_cur)) {
                    rn -= rng(2, 6);
                }
                if (p->dex_cur >= 8 && (p->dex_cur >= 16 || one_in(16 - p->dex_cur))) {
                    rn += rng(2, 6);
                }
                if (p->dex_cur > 16) {
                    rn += rng(0, p->dex_cur - 16);
                }
                if (rn <= 4) {
                    p->add_msg_if_player(m_bad, _("You damage your %s further!"), fix->tname().c_str());
                    fix->damage++;
                    if (fix->damage >= 5) {
                        p->add_msg_if_player(m_bad, _("You destroy it!"));
                        p->i_rem(pos);
                    }
                } else if (rn <= 6) {
                    p->add_msg_if_player(m_bad, _("You don't repair your %s, and you waste lots of charge."),
                                         fix->tname().c_str());
                    charges_used += rng(1, 8);
                } else if (rn <= 8) {
                    p->add_msg_if_player(m_mixed, _("You repair your %s, but you waste lots of charge."),
                                         fix->tname().c_str());
                    if (fix->damage >= 3) {
                        p->consume_items(comps);
                    }
                    fix->damage--;
                    charges_used += rng(1, 8);
                } else if (rn <= 16) {
                    p->add_msg_if_player(m_good, _("You repair your %s!"), fix->tname().c_str());
                    if (fix->damage >= 3) {
                        p->consume_items(comps);
                    }
                    fix->damage--;
                } else {
                    p->add_msg_if_player(m_good, _("You repair your %s completely!"), fix->tname().c_str());
                    if (fix->damage >= 3) {
                        p->consume_items(comps);
                    }
                    fix->damage = 0;
                }
            } else if (fix->damage == 0 || (fix->has_flag("VARSIZE") && !fix->has_flag("FIT"))) {
                p->moves -= 500 * p->fine_detail_vision_mod();
                p->practice("mechanics", 10);
                int rn = dice(4, 2 + p->skillLevel("mechanics"));
                if (p->dex_cur < 8 && one_in(p->dex_cur)) {
                    rn -= rng(2, 6);
                }
                if (p->dex_cur >= 16 || (p->dex_cur > 8 && one_in(16 - p->dex_cur))) {
                    rn += rng(2, 6);
                }
                if (p->dex_cur > 16) {
                    rn += rng(0, p->dex_cur - 16);
                }
                if (rn <= 4) {
                    p->add_msg_if_player(m_bad, _("You damage your %s!"), fix->tname().c_str());
                    fix->damage++;
                } else if (rn >= 12 && fix->has_flag("VARSIZE") && !fix->has_flag("FIT")) {
                    p->add_msg_if_player(m_good, _("You take your %s in, improving the fit."),
                                         fix->tname().c_str());
                    fix->item_tags.insert("FIT");
                } else if (rn >= 12 && (fix->has_flag("FIT") || !fix->has_flag("VARSIZE"))) {
                    p->add_msg_if_player(m_good, _("You make your %s extra sturdy."), fix->tname().c_str());
                    fix->damage--;
                    p->consume_items(comps);
                } else {
                    p->add_msg_if_player(m_neutral, _("You practice your soldering."));
                }
            } else {
                p->add_msg_if_player(m_info, _("Your %s is already enhanced."), fix->tname().c_str());
                return 0;
            }
            return charges_used;
        }
        break;
        case 3:
            break;
        default:
            break;
    };
    return 0;
}


int iuse::water_purifier(player *p, item *it, bool, point)
{
    int pos = g->inv_for_filter( _("Purify what?"), []( const item & itm ) {
        return !itm.contents.empty() && 
               ( itm.contents[0].type->id == "water" ||
                 itm.contents[0].type->id == "salt_water" );
    } );
    if (!p->has_item(pos)) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    if (p->i_at(pos).contents.empty()) {
        p->add_msg_if_player(m_info, _("You can only purify water."));
        return 0;
    }
    item *pure = &(p->i_at(pos).contents[0]);
    if (pure->type->id != "water" && pure->type->id != "salt_water") {
        p->add_msg_if_player(m_info, _("You can only purify water."));
        return 0;
    }
    if (pure->charges > it->charges) {
        p->add_msg_if_player(m_info,
                             _("You don't have enough charges in your purifier to purify all of the water."));
        return 0;
    }
    p->moves -= 150;
    pure->make("water_clean");
    pure->poison = 0;
    return pure->charges;
}

int iuse::two_way_radio(player *p, item *it, bool, point)
{
    WINDOW *w = newwin(6, 36, (TERMY - 6) / 2, (TERMX - 36) / 2);
    WINDOW_PTR wptr(w);
    draw_border(w);
    // TODO: More options here.  Thoughts...
    //       > Respond to the SOS of an NPC
    //       > Report something to a faction
    //       > Call another player
    // TODO: Should probably be a ui menu anyway.
    fold_and_print(w, 1, 1, 999, c_white,
                   _(
                       "1: Radio a faction for help...\n"
                       "2: Call Acquaintance...\n"
                       "3: General S.O.S.\n"
                       "0: Cancel"));
    wrefresh(w);
    char ch = getch();
    if (ch == '1') {
        p->moves -= 300;
        faction *fac = g->list_factions(_("Call for help..."));
        if (fac == NULL) {
            return 0;
        }
        int bonus = 0;
        if (fac->goal == FACGOAL_CIVILIZATION) {
            bonus += 2;
        }
        if (fac->has_job(FACJOB_MERCENARIES)) {
            bonus += 4;
        }
        if (fac->has_job(FACJOB_DOCTORS)) {
            bonus += 2;
        }
        if (fac->has_value(FACVAL_CHARITABLE)) {
            bonus += 3;
        }
        if (fac->has_value(FACVAL_LONERS)) {
            bonus -= 3;
        }
        if (fac->has_value(FACVAL_TREACHERY)) {
            bonus -= rng(0, 8);
        }
        bonus += fac->respects_u + 3 * fac->likes_u;
        if (bonus >= 25) {
            popup(_("They reply, \"Help is on the way!\""));
            //~ %s is faction name
            p->add_memorial_log(pgettext("memorial_male", "Called for help from %s."),
                                  pgettext("memorial_female", "Called for help from %s."),
                                  fac->name.c_str());
            /* Disabled until event::faction_id and associated code
             * is updated to accept a std::string.
            g->add_event(EVENT_HELP, int(calendar::turn) + fac->response_time(), fac->id);
            */
            fac->respects_u -= rng(0, 8);
            fac->likes_u -= rng(3, 5);
        } else if (bonus >= -5) {
            popup(_("They reply, \"Sorry, you're on your own!\""));
            fac->respects_u -= rng(0, 5);
        } else {
            popup(_("They reply, \"Hah!  We hope you die!\""));
            fac->respects_u -= rng(1, 8);
        }

    } else if (ch == '2') { // Call Acquaintance
        // TODO: Implement me!
    } else if (ch == '3') { // General S.O.S.
        p->moves -= 150;
        std::vector<npc *> in_range;
        std::vector<npc *> npcs = overmap_buffer.get_npcs_near_player(30);
        for( auto &npc : npcs ) {
            if( npc->op_of_u.value >= 4 ) {
                in_range.push_back( npc );
            }
        }
        if (!in_range.empty()) {
            npc *coming = in_range[rng(0, in_range.size() - 1)];
            popup(ngettext("A reply!  %s says, \"I'm on my way; give me %d minute!\"",
                           "A reply!  %s says, \"I'm on my way; give me %d minutes!\"", coming->minutes_to_u()),
                  coming->name.c_str(), coming->minutes_to_u());
            p->add_memorial_log(pgettext("memorial_male", "Called for help from %s."),
                                  pgettext("memorial_female", "Called for help from %s."),
                                  coming->name.c_str());
            coming->mission = NPC_MISSION_RESCUE_U;
        } else {
            popup(_("No-one seems to reply..."));
        }
    } else {
        return 0;
    }
    wptr.reset();
    refresh();
    return it->type->charges_to_use();
}

int iuse::radio_off(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(_("It's dead."));
    } else {
        p->add_msg_if_player(_("You turn the radio on."));
        it->make("radio_on");
        it->active = true;
    }
    return it->type->charges_to_use();
}

static radio_tower *find_radio_station(int frequency)
{
    radio_tower *tower = NULL;
    for (size_t k = 0; k < g->cur_om->radios.size(); k++) {
        tower = &g->cur_om->radios[k];
        if (0 < tower->strength - rl_dist(tower->x, tower->y, g->levx, g->levy) &&
            tower->frequency == frequency) {
            return tower;
        }
    }
    return NULL;
}

int iuse::directional_antenna(player *p, item *it, bool, point)
{
    // Find out if we have an active radio
    auto radios = p->items_with( []( const item & it ) {
        return it.typeId() == "radio_on";
    } );
    if( radios.empty() ) {
        add_msg(m_info, _("Must have an active radio to check for signal direction."));
        return 0;
    }
    const item radio = *radios.front();
    // Find the radio station its tuned to (if any)
    radio_tower *tower = find_radio_station(radio.frequency);
    if (tower == NULL) {
        add_msg(m_info, _("You can't find the direction if your radio isn't tuned."));
        return 0;
    }
    // Report direction.
    direction angle = direction_from(g->levx, g->levy, tower->x, tower->y);
    add_msg(_("The signal seems strongest to the %s."), direction_name(angle).c_str());
    return it->type->charges_to_use();
}

int iuse::radio_on(player *p, item *it, bool t, point pos)
{
    if (t) {
        // Normal use
        std::string message = _("Radio: Kssssssssssssh.");
        radio_tower *selected_tower = find_radio_station(it->frequency);
        if (selected_tower != NULL) {
            if (selected_tower->type == MESSAGE_BROADCAST) {
                message = selected_tower->message;
            } else if (selected_tower->type == WEATHER_RADIO) {
                message = weather_forecast(*selected_tower);
            }

            int signal_strength = selected_tower->strength -
                                  rl_dist(selected_tower->x, selected_tower->y, g->levx, g->levy);

            for( auto &elem : message ) {
                if (dice(10, 100) > dice(10, signal_strength * 3)) {
                    if (!one_in(10)) {
                        elem = '#';
                    } else {
                        elem = char( rng( 'a', 'z' ) );
                    }
                }
            }

            std::vector<std::string> segments = foldstring(message, RADIO_PER_TURN);
            int index = calendar::turn % (segments.size());
            std::stringstream messtream;
            messtream << string_format(_("radio: %s"), segments[index].c_str());
            message = messtream.str();
        }
        g->ambient_sound(pos.x, pos.y, 6, message.c_str());
    } else { // Activated
        int ch = 2;
        if (it->charges > 0) {
            ch = menu(true, _("Radio:"), _("Scan"), _("Turn off"), NULL);
        }

        switch (ch) {
            case 1: {
                int old_frequency = it->frequency;
                radio_tower *tower = NULL;
                radio_tower *lowest_tower = NULL;
                radio_tower *lowest_larger_tower = NULL;

                for (size_t k = 0; k < g->cur_om->radios.size(); k++) {
                    tower = &g->cur_om->radios[k];

                    if (tower->strength - rl_dist(tower->x, tower->y, g->levx, g->levy) > 0 &&
                        tower->frequency != old_frequency) {
                        if (tower->frequency > old_frequency &&
                            (lowest_larger_tower == NULL ||
                             tower->frequency < lowest_larger_tower->frequency)) {
                            lowest_larger_tower = tower;
                        } else if (lowest_tower == NULL ||
                                   tower->frequency < lowest_tower->frequency) {
                            lowest_tower = tower;
                        }
                    }
                }
                if (lowest_larger_tower != NULL) {
                    it->frequency = lowest_larger_tower->frequency;
                } else if (lowest_tower != NULL) {
                    it->frequency = lowest_tower->frequency;
                }
            }
            break;
            case 2:
                p->add_msg_if_player(_("The radio dies."));
                it->make("radio");
                it->active = false;
                break;
            case 3:
                break;
        }
    }
    return it->type->charges_to_use();
}

int iuse::noise_emitter_off(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(_("It's dead."));
    } else {
        p->add_msg_if_player(_("You turn the noise emitter on."));
        it->make("noise_emitter_on");
        it->active = true;
    }
    return it->type->charges_to_use();
}

int iuse::airhorn(player *p, item *it, bool, point pos)
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(_("You depress the button but no sound comes out."));
    } else {
        p->add_msg_if_player(_("You honk your airhorn."));
        g->sound(pos.x, pos.y, 50, _("HOOOOONK!"));
    }
    return it->type->charges_to_use();
}

int iuse::horn_bicycle(player *p, item *it, bool, point pos)
{
    g->sound(pos.x, pos.y, 15, _("honk."));
    p->add_msg_if_player(_("You honk the bicycle horn."));
    return it->type->charges_to_use();
}

int iuse::noise_emitter_on(player *p, item *it, bool t, point pos)
{
    if (t) { // Normal use
        //~ the sound of a noise emitter when turned on
        g->ambient_sound(pos.x, pos.y, 30, _("KXSHHHHRRCRKLKKK!"));
    } else { // Turning it off
        p->add_msg_if_player(_("The infernal racket dies as the noise emitter turns off."));
        it->make("noise_emitter");
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::ma_manual(player *p, item *it, bool, point)
{
    // strip "manual_" from the start of the item id, add the rest to "style_"
    std::string style_to_learn = "style_" + it->type->id.substr(7);

    if (p->has_martialart(style_to_learn)) {
        p->add_msg_if_player(m_info, _("You already know all this book has to teach."));
        return 0;
    }

    p->ma_styles.push_back(style_to_learn);

    p->add_msg_if_player(m_good, _("You learn what you can, and stow the book for further study."));

    return 1;
}

bool pry_nails(player *p, ter_id &type, int dirx, int diry)
{
    int nails = 0, boards = 0;
    ter_id newter;
    if (type == t_fence_h || type == t_fence_v) {
        nails = 6;
        boards = 3;
        newter = t_fence_post;
        p->add_msg_if_player(_("You pry out the fence post."));
    } else if (type == t_window_boarded) {
        nails = 8;
        boards = 4;
        newter = t_window_frame;
        p->add_msg_if_player(_("You pry the boards from the window."));
    } else if (type == t_window_boarded_noglass) {
        nails = 8;
        boards = 4;
        newter = t_window_empty;
        p->add_msg_if_player(_("You pry the boards from the window frame."));
    } else if ( type == t_door_boarded || type == t_door_boarded_damaged ||
            type == t_rdoor_boarded || type == t_rdoor_boarded_damaged ||
            type == t_door_boarded_peep || type == t_door_boarded_damaged_peep ) {
        nails = 8;
        boards = 4;
        if (type == t_door_boarded) {
            newter = t_door_c;
        } else if (type == t_door_boarded_damaged) {
            newter = t_door_b;
        } else if (type == t_door_boarded_peep) {
            newter = t_door_c_peep;
        } else if (type == t_door_boarded_damaged_peep) {
            newter = t_door_b_peep;
        } else if (type == t_rdoor_boarded) {
            newter = t_rdoor_c;
        } else { // if (type == t_rdoor_boarded_damaged)
            newter = t_rdoor_b;
        }
        p->add_msg_if_player(_("You pry the boards from the door."));
    } else {
        return false;
    }
    p->practice("carpentry", 1, 1);
    p->moves -= 500;
    g->m.spawn_item(p->posx, p->posy, "nail", 0, nails);
    g->m.spawn_item(p->posx, p->posy, "2x4", boards);
    g->m.ter_set(dirx, diry, newter);
    return true;
}

int iuse::hammer(player *p, item *it, bool, point)
{
    g->draw();
    int x, y;
    // If anyone other than the player wants to use one of these,
    // they're going to need to figure out how to aim it.
    if (!choose_adjacent(_("Pry where?"), x, y)) {
        return 0;
    }

    if (x == p->posx && y == p->posy) {
        p->add_msg_if_player(_("You try to hit yourself with the hammer."));
        p->add_msg_if_player(_("But you can't touch this."));
        return 0;
    }

    ter_id type = g->m.ter(x, y);
    if (pry_nails(p, type, x, y)) {
        return it->type->charges_to_use();
    } else {
        p->add_msg_if_player(m_info, _("There's nothing to pry there."));
    }
    return 0;
}

int iuse::crowbar(player *p, item *it, bool, point)
{
    int dirx, diry;
    if (!choose_adjacent(_("Pry where?"), dirx, diry)) {
        return 0;
    }

    if (dirx == p->posx && diry == p->posy) {
        p->add_msg_if_player(m_info, _("You attempt to pry open your wallet"));
        p->add_msg_if_player(m_info, _("but alas. You are just too miserly."));
        return 0;
    }
    ter_id type = g->m.ter(dirx, diry);
    const char *succ_action;
    const char *fail_action;
    ter_id new_type = t_null;
    bool noisy;
    int difficulty;

    if (type == t_door_c || type == t_door_locked || type == t_door_locked_alarm ||
        type == t_door_locked_interior) {
        succ_action = _("You pry open the door.");
        fail_action = _("You pry, but cannot pry open the door.");
        new_type = t_door_o;
        noisy = true;
        difficulty = 6;
    } else if (type == t_door_locked_peep) {
        succ_action = _("You pry open the door.");
        fail_action = _("You pry, but cannot pry open the door.");
        new_type = t_door_o_peep;
        noisy = true;
        difficulty = 6;
    } else if (type == t_door_bar_locked) {
        succ_action = _("You pry open the door.");
        fail_action = _("You pry, but cannot pry open the door.");
        new_type = t_door_bar_o;
        noisy = false;
        difficulty = 10;
    } else if (type == t_manhole_cover) {
        succ_action = _("You lift the manhole cover.");
        fail_action = _("You pry, but cannot lift the manhole cover.");
        new_type = t_manhole;
        noisy = false;
        difficulty = 12;
    } else if (g->m.furn(dirx, diry) == f_crate_c) {
        succ_action = _("You pop open the crate.");
        fail_action = _("You pry, but cannot pop open the crate.");
        noisy = true;
        difficulty = 6;
    } else if (type == t_window_domestic || type == t_curtains) {
        succ_action = _("You pry open the window.");
        fail_action = _("You pry, but cannot pry open the window.");
        new_type = t_window_open;
        noisy = true;
        difficulty = 6;
    } else if (pry_nails(p, type, dirx, diry)) {
        return it->type->charges_to_use();
    } else {
        p->add_msg_if_player(m_info, _("There's nothing to pry there."));
        return 0;
    }

    p->practice("mechanics", 1);
    p->moves -= (difficulty * 25) - ((p->str_cur + p->skillLevel("mechanics")) * 5);
    if (dice(4, difficulty) < dice(2, p->skillLevel("mechanics")) + dice(2, p->str_cur)) {
        p->practice("mechanics", 1);
        p->add_msg_if_player(m_good, succ_action);
        if (g->m.furn(dirx, diry) == f_crate_c) {
            g->m.furn_set(dirx, diry, f_crate_o);
        } else {
            g->m.ter_set(dirx, diry, new_type);
        }
        if (noisy) {
            g->sound(dirx, diry, 12, _("crunch!"));
        }
        if (type == t_manhole_cover) {
            g->m.spawn_item(dirx, diry, "manhole_cover");
        }
        if (type == t_door_locked_alarm) {
            p->add_memorial_log(pgettext("memorial_male", "Set off an alarm."),
                                  pgettext("memorial_female", "Set off an alarm."));
            g->sound(p->posx, p->posy, 40, _("An alarm sounds!"));
            if (!g->event_queued(EVENT_WANTED)) {
                g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, g->get_abs_levx(), g->get_abs_levy());
            }
        }
    } else {
        if (type == t_window_domestic || type == t_curtains) {
            //chance of breaking the glass if pry attempt fails
            if (dice(4, difficulty) > dice(2, p->skillLevel("mechanics")) + dice(2, p->str_cur)) {
                p->add_msg_if_player(m_mixed, _("You break the glass."));
                g->sound(dirx, diry, 24, _("glass breaking!"));
                g->m.ter_set(dirx, diry, t_window_frame);
                g->m.spawn_item(dirx, diry, "sheet", 2);
                g->m.spawn_item(dirx, diry, "stick");
                g->m.spawn_item(dirx, diry, "string_36");
                return it->type->charges_to_use();
            }
        }
        p->add_msg_if_player(fail_action);
    }
    return it->type->charges_to_use();
}

int iuse::makemound(player *p, item *it, bool, point)
{
    if (g->m.has_flag("DIGGABLE", p->posx, p->posy) && !g->m.has_flag("PLANT", p->posx, p->posy)) {
        p->add_msg_if_player(_("You churn up the earth here."));
        p->moves = -300;
        g->m.ter_set(p->posx, p->posy, t_dirtmound);
        return it->type->charges_to_use();
    } else {
        p->add_msg_if_player(_("You can't churn up this ground."));
        return 0;
    }
}

//TODO remove this?
int iuse::dig(player *p, item *it, bool, point)
{
    p->add_msg_if_player(m_info, _("You can dig a pit via the construction menu--hit *"));
    return it->type->charges_to_use();
}

int iuse::siphon(player *p, item *it, bool, point)
{
    int posx = 0;
    int posy = 0;
    if (!choose_adjacent(_("Siphon from where?"), posx, posy)) {
        return 0;
    }

    vehicle *veh = g->m.veh_at(posx, posy);
    if (veh == NULL) {
        p->add_msg_if_player(m_info, _("There's no vehicle there."));
        return 0;
    }
    if (veh->fuel_left("gasoline") == 0) {
        p->add_msg_if_player(m_info, _("That vehicle has no fuel to siphon."));
        return 0;
    }
    std::map<point, vehicle *> foundv;
    vehicle *fillv = NULL;
    for (int x = p->posx - 1; x < p->posx + 2; x++) {
        for (int y = p->posy - 1; y < p->posy + 2; y++) {
            fillv = g->m.veh_at(x, y);
            if (fillv != NULL &&
                fillv != veh &&
                foundv.find(point(fillv->posx, fillv->posy)) == foundv.end() &&
                fillv->fuel_capacity("gasoline") > 0) {
                foundv[point(fillv->posx, fillv->posy)] = fillv;
            }
        }
    }
    fillv = NULL;
    if (!foundv.empty()) {
        uimenu fmenu;
        fmenu.text = _("Fill what?");
        fmenu.addentry(_("Nearby vehicle (%d)"), foundv.size());
        fmenu.addentry(_("Container"));
        fmenu.addentry(_("Never mind"));
        fmenu.query();
        if (fmenu.ret == 0) {
            if (foundv.size() > 1) {
                if (choose_adjacent(_("Fill which vehicle?"), posx, posy)) {
                    fillv = g->m.veh_at(posx, posy);
                } else {
                    return 0;
                }
            } else {
                fillv = foundv.begin()->second;

            }
        } else if (fmenu.ret != 1) {
            return 0;
        }
    }
    if (fillv != NULL) {
        int want = fillv->fuel_capacity("gasoline") - fillv->fuel_left("gasoline");
        int got = veh->drain("gasoline", want);
        fillv->refill("gasoline", got);
        add_msg(ngettext("Siphoned %d unit of %s from the %s into the %s%s",
                         "Siphoned %d units of %s from the %s into the %s%s", got), got,
                "gasoline", veh->name.c_str(), fillv->name.c_str(),
                (got < want ? _(", draining the tank completely.") : _(", receiving tank is full.")));
        p->moves -= 200;
    } else {
        if (p->siphon(veh, "gasoline")) {
            p->moves -= 200;
        }
    }
    return it->type->charges_to_use();
}

int iuse::combatsaw_off(player *p, item *it, bool, point)
{
    p->moves -= 60;
    if (it->charges > 0 && !p->is_underwater()) {
        g->sound(p->posx, p->posy, 30,
                 _("With a snarl, the combat chainsaw screams to life!"));
        it->make("combatsaw_on");
        it->active = true;
    } else {
        p->add_msg_if_player(_("You yank the cord, but nothing happens."));
    }
    return it->type->charges_to_use();
}

int iuse::combatsaw_on(player *p, item *it, bool t, point)
{
    if (t) { // Effects while simply on
        if (p->is_underwater()) {
            p->add_msg_if_player(_("Your chainsaw gurgles in the water and stops."));
            it->make("combatsaw_off");
            it->active = false;
        } else if (one_in(12)) {
            g->ambient_sound(p->posx, p->posy, 18, _("Your combat chainsaw growls."));
        }
    } else { // Toggling
        p->add_msg_if_player(_("Your combat chainsaw goes quiet."));
        it->make("combatsaw_off");
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::chainsaw_off(player *p, item *it, bool, point)
{
    p->moves -= 80;
    if (rng(0, 10) - it->damage > 5 && it->charges > 0 && !p->is_underwater()) {
        g->sound(p->posx, p->posy, 20,
                 _("With a roar, the chainsaw leaps to life!"));
        it->make("chainsaw_on");
        it->active = true;
    } else {
        p->add_msg_if_player(_("You yank the cord, but nothing happens."));
    }
    return it->type->charges_to_use();
}

int iuse::chainsaw_on(player *p, item *it, bool t, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(_("Your chainsaw gurgles in the water and stops."));
        it->make("chainsaw_off");
        it->active = false;
    } else if (t) { // Effects while simply on
        if (one_in(15)) {
            g->ambient_sound(p->posx, p->posy, 12, _("Your chainsaw rumbles."));
        }
    } else { // Toggling
        p->add_msg_if_player(_("Your chainsaw dies."));
        it->make("chainsaw_off");
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::elec_chainsaw_off(player *p, item *it, bool, point)
{
    p->moves -= 80;
    if (rng(0, 10) - it->damage > 5 && it->charges > 0 && !p->is_underwater()) {
        g->sound(p->posx, p->posy, 20,
                 _("With a roar, the electric chainsaw leaps to life!"));
        it->make("elec_chainsaw_on");
        it->active = true;
    } else {
        p->add_msg_if_player(_("You flip the switch, but nothing happens."));
    }
    return it->type->charges_to_use();
}
int iuse::elec_chainsaw_on(player *p, item *it, bool t, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(_("Your chainsaw gurgles in the water and stops."));
        it->make("elec_chainsaw_off");
        it->active = false;
    } else if (t) { // Effects while simply on
        if (one_in(15)) {
            g->ambient_sound(p->posx, p->posy, 12, _("Your electric chainsaw rumbles."));
        }
    } else { // Toggling
        p->add_msg_if_player(_("Your electric chainsaw dies."));
        it->make("elec_chainsaw_off");
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::cs_lajatang_off(player *p, item *it, bool, point)
{
    p->moves -= 80;
    if (rng(0, 10) - it->damage > 5 && it->charges > 1) {
        g->sound(p->posx, p->posy, 40,
                 _("With a roar, the chainsaws leap to life!"));
        it->make("cs_lajatang_on");
        it->active = true;
    } else {
        p->add_msg_if_player(_("You yank the cords, but nothing happens."));
    }
    return it->type->charges_to_use();
}

int iuse::cs_lajatang_on(player *p, item *it, bool t, point)
{
    if (t) { // Effects while simply on
        if (one_in(15)) {
            g->ambient_sound(p->posx, p->posy, 12, _("Your chainsaws rumble."));
        }
        //Deduct an additional charge (since there are two of them)
        if (it->charges > 0) {
            it->charges--;
        }
    } else { // Toggling
        p->add_msg_if_player(_("Your chainsaws die."));
        it->make("cs_lajatang_off");
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::carver_off(player *p, item *it, bool, point)
{
    p->moves -= 80;
    if (it->charges > 0) {
        g->sound(p->posx, p->posy, 20,
                 _("The electric carver's serrated blades start buzzing!"));
        it->make("carver_on");
        it->active = true;
    } else {
        p->add_msg_if_player(_("You pull the trigger but nothing happens."));
    }
    return it->type->charges_to_use();
}

int iuse::carver_on(player *p, item *it, bool t, point)
{
    if (t) { // Effects while simply on
        if (one_in(10)) {
            g->ambient_sound(p->posx, p->posy, 8, _("Your electric carver buzzes."));
        }
    } else { // Toggling
        p->add_msg_if_player(_("Your electric carver dies."));
        it->make("carver_off");
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::trimmer_off(player *p, item *it, bool, point)
{
    p->moves -= 80;
    if (rng(0, 10) - it->damage > 3 && it->charges > 0) {
        g->sound(p->posx, p->posy, 15,
                 _("With a roar, the hedge trimmer leaps to life!"));
        it->make("trimmer_on");
        it->active = true;
    } else {
        p->add_msg_if_player(_("You yank the cord, but nothing happens."));
    }
    return it->type->charges_to_use();
}

int iuse::trimmer_on(player *p, item *it, bool t, point)
{
    if (t) { // Effects while simply on
        if (one_in(15)) {
            g->ambient_sound(p->posx, p->posy, 10, _("Your hedge trimmer rumbles."));
        }
    } else { // Toggling
        p->add_msg_if_player(_("Your hedge trimmer dies."));
        it->make("trimmer_off");
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::circsaw_on(player *p, item *it, bool t, point)
{
    if (t) { // Effects while simply on
        if (one_in(15)) {
            g->ambient_sound(p->posx, p->posy, 7, _("Your circular saw buzzes."));
        }
    } else { // Toggling
        p->add_msg_if_player(_("Your circular saw powers off."));
        it->make("circsaw_off");
        it->active = false;
    }
    return 0;
}

int iuse::shishkebab_off(player *p, item *it, bool, point pos)
{
    int choice = menu(true, _("What's the plan?"), _("Bring the heat!"),
                      _("Cut 'em up!"), _("I'm good."), NULL);
    switch (choice) {
        case 1: {
            p->moves -= 10;
            if (it->charges <= 0) {
                p->add_msg_if_player(m_info, _("This thing needs some fuel!"));
            } else if (rng(0, 10) - it->damage > 5 && !p->is_underwater()) {
                g->sound(p->posx, p->posy, 10, _("Let's dance, Zeds!"));
                it->make("shishkebab_on");
                it->active = true;
            } else {
                p->add_msg_if_player(_("Aw, dangit. It fails to start!"));
            }
            return it->type->charges_to_use();
        }
        break;
        case 2: {
            return iuse::knife(p, it, false, pos);
        }
        default:
            return 0;
    }
}

int iuse::shishkebab_on(player *p, item *it, bool t, point pos)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(_("Your shishkebab hisses in the water and goes out."));
        it->make("shishkebab_off");
        it->active = false;
    } else if (t) {
        // Effects while simply on
        if (one_in(25)) {
            g->ambient_sound(p->posx, p->posy, 10, _("Your shishkebab crackles!"));
        }

        if (one_in(75)) {
            p->add_msg_if_player(m_bad, _("Bummer, man! Your shishkebab's flame flickers and dies out."));
            it->make("shishkebab_off");
            it->active = false;
        }
    } else if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_bad, _("Uncool, outta gas! Your shishkebab's flame goes out."));
        it->make("shishkebab_off");
        it->active = false;
    } else {
        int choice = menu(true, _("What's the plan?"), _("Chill out"),
                          _("Torch something!"), _("Keep groovin'"), NULL);
        switch (choice) {
            case 1: {
                p->add_msg_if_player(_("Peace out. Your shishkebab's flame dies."));
                it->make("shishkebab_off");
                it->active = false;
            }
            break;
            case 2: {
                if( prep_firestarter_use(p, it, pos) ) {
                    p->moves -= 5;
                    resolve_firestarter_use(p, it, pos);
                }
            }
            default:
                return 0;
        }
    }
    return it->type->charges_to_use();
}

int iuse::firemachete_off(player *p, item *it, bool, point pos)
{
    int choice = menu(true,
                      _("No. 9"), _("Turn on"), _("Use as a knife"), _("Cancel"), NULL);
    switch (choice) {
        case 1: {
            p->moves -= 10;
            if (rng(0, 10) - it->damage > 2 && it->charges > 0 && !p->is_underwater()) {
                g->sound(p->posx, p->posy, 10, _("Your No. 9 glows!"));
                it->make("firemachete_on");
                it->active = true;
            } else {
                p->add_msg_if_player(_("Click."));
            }
        }
        break;
        case 2: {
            iuse::knife(p, it, false, pos);
        }
        default:
            return 0;
    }
    return it->type->charges_to_use();
}

int iuse::firemachete_on(player *p, item *it, bool t, point pos)
{
    if (t) {  // Effects while simply on
        if (p->is_underwater()) {
            p->add_msg_if_player(_("Your No. 9 hisses in the water and goes out."));
            it->make("firemachete_off");
            it->active = false;
        } else if (one_in(25)) {
            g->ambient_sound(p->posx, p->posy, 5, _("Your No. 9 hisses."));
        }
        if (one_in(100)) {
            p->add_msg_if_player(m_bad, _("Your No. 9 cuts out!"));
            it->make("firemachete_off");
            it->active = false;
        }
    } else if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_info, _("Out of ammo!"));
        it->make("firemachete_off");
        it->active = false;
    } else {
        int choice = menu(true, _("No. 9"), _("Turn off"), _("Light something"), _("Cancel"), NULL);
        switch (choice) {
            case 1: {
                p->add_msg_if_player(_("Your No. 9 goes dark."));
                it->make("firemachete_off");
                it->active = false;
            }
            break;
            case 2: {
                if( prep_firestarter_use(p, it, pos) ) {
                    p->moves -= 5;
                    resolve_firestarter_use(p, it, pos);
                }
            }
            default:
                return 0;
        }
    }
    return it->type->charges_to_use();
}

int iuse::broadfire_off(player *p, item *it, bool t, point pos)
{
    int choice = menu(true, _("What will thou do?"), _("Ready for battle!"),
                      _("Perform peasant work?"), _("Reconsider thy strategy"), NULL);
    switch (choice) {
        case 1: {
            p->moves -= 10;
            if (it->charges > 0 && !p->is_underwater()) {
                g->sound(p->posx, p->posy, 10,
                         _("Charge!!"));
                it->make("broadfire_on");
                it->active = true;
            } else {
                p->add_msg_if_player(m_info, _("No strength to fight!"));
            }
        }
        break;
        case 2: {
            return iuse::knife(p, it, t, pos);
        }
    }
    return it->type->charges_to_use();
}

int iuse::broadfire_on(player *p, item *it, bool t, point pos)
{
    if (t) {  // Effects while simply on
        if (p->is_underwater()) {
            p->add_msg_if_player(_("Your sword hisses in the water and goes out."));
            it->make("broadfire_off");
            it->active = false;
        } else if (one_in(35)) {
            p->add_msg_if_player(_("Your blade burns for combat!"));
        }
    } else if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_bad, _("Thy strength fades!"));
        it->make("broadfire_off");
        it->active = false;
    } else {
        int choice = menu(true, _("What will thou do?"), _("Retreat!"),
                          _("Burn and Pillage!"), _("Keep Fighting!"), NULL);
        switch (choice) {
            case 1: {
                p->add_msg_if_player(_("Run away!"));
                it->make("broadfire_off");
                it->active = false;
            }
            break;
            case 2: {
                if( prep_firestarter_use(p, it, pos) ) {
                    p->moves -= 5;
                    resolve_firestarter_use(p, it, pos);
                }
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::firekatana_off(player *p, item *it, bool t, point pos)
{
    int choice = menu(true, _("The Dark of Night."), _("Daybreak"),
                      _("The Moonlight's Edge"), _("Eternal Night"), NULL);
    switch (choice) {
        case 1: {
            p->moves -= 10;
            if (it->charges > 0 && !p->is_underwater()) {
                g->sound(p->posx, p->posy, 10,
                         _("The Sun rises."));
                it->make("firekatana_on");
                it->active = true;
            } else {
                p->add_msg_if_player(_("Time stands still."));
            }
        }
        break;
        case 2: {
            return iuse::knife(p, it, t, pos);
        }
    }
    return it->type->charges_to_use();
}

int iuse::firekatana_on(player *p, item *it, bool t, point pos)
{
    if (t) {  // Effects while simply on
        if (p->is_underwater()) {
            p->add_msg_if_player(_("Your sword hisses in the water and goes out."));
            it->make("firekatana_off");
            it->active = false;
        } else if (one_in(35)) {
            p->add_msg_if_player(_("The Sun shines brightly."));
        }
    } else if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_bad, _("The Light Fades."));
        it->make("firekatana_off");
        it->active = false;
    } else {
        int choice = menu(true, _("The Light of Day."), _("Nightfall"),
                          _("Blazing Heat"), _("Endless Day"), NULL);
        switch (choice) {
            case 1: {
                p->add_msg_if_player(_("The Sun sets."));
                it->make("firekatana_off");
                it->active = false;
            }
            break;
            case 2: {
                if( prep_firestarter_use(p, it, pos) ) {
                    p->moves -= 5;
                    resolve_firestarter_use(p, it, pos);
                    return it->type->charges_to_use();
                }
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::zweifire_off(player *p, item *it, bool t, point pos)
{
    int choice = menu(true, _("Was willst du tun?"), _("Die Flamme entfachen."),
                      _("Als Messer verwenden."), _("Nichts tun."), NULL);
    switch (choice) {
        case 1: {
            p->moves -= 10;
            if (it->charges > 0 && !p->is_underwater()) {
                g->sound(p->posx, p->posy, 10,
                         _("Die Klinge deines Schwertes brennt!"));
                it->make("zweifire_on");
                it->active = true;
            } else {
                p->add_msg_if_player(m_bad, _("Dein Flammenschwert hat keinen Brennstoff mehr."));
            }
        }
        break;
        case 2: {
            return iuse::knife(p, it, t, pos);
        }
        default:
            return 0;
    }
    return it->type->charges_to_use();
}

int iuse::zweifire_on(player *p, item *it, bool t, point pos)
{
    if (t) {  // Effects while simply on
        if (p->is_underwater()) {
            p->add_msg_if_player(_("Dein Schwert zischt und erlischt."));
            it->make("zweifire_off");
            it->active = false;
        } else if (one_in(35)) {
            //~ (Flammenschwert) "The fire on your blade burns brightly!"
            p->add_msg_if_player(_("Das Feuer um deine Schwertklinge leuchtet hell!"));
        }
    } else if (it->charges < it->type->charges_to_use()) {
        //~ (Flammenschwert) "Your Flammenscwhert (firesword) is out of fuel!"
        p->add_msg_if_player(m_bad, _("Deinem Flammenschwert ist der Brennstoff ausgegangen!"));
        it->make("zweifire_off");
        it->active = false;
    } else {
        int choice = menu(true,
                          //~ (Flammenschwert) "What will you do?"
                          _("Was willst du tun?"),
                          //~ (Flammenschwert) "Extinguish the flame."
                          _("Die Flamme erloschen."),
                          //~ (Flammenschwert) "Start a fire."
                          _("Ein Feuer entfachen."),
                          //~ (Flammenschwert) "Do nothing."
                          _("Nichts tun."), NULL);
        switch (choice) {
            case 1: {
                //~ (Flammenschwert) "The flames on your sword die out."
                p->add_msg_if_player(_("Die Flamme deines Schwertes erlischt."));
                it->make("zweifire_off");
                it->active = false;
            }
            break;
            case 2: {
                if( prep_firestarter_use(p, it, pos) ) {
                    p->moves -= 5;
                    resolve_firestarter_use(p, it, pos);
                    return it->type->charges_to_use();
                }
            }
            default:
                return 0;
        }
    }
    return it->type->charges_to_use();
}

int iuse::jackhammer(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    int dirx, diry;
    if (!choose_adjacent(_("Drill where?"), dirx, diry)) {
        return 0;
    }

    if (dirx == p->posx && diry == p->posy) {
        p->add_msg_if_player(_("My god! Let's talk it over OK?"));
        p->add_msg_if_player(_("Don't do anything rash.."));
        return 0;
    }
    if (g->m.is_bashable(dirx, diry) && g->m.has_flag("SUPPORTS_ROOF", dirx, diry) &&
        g->m.ter(dirx, diry) != t_tree) {
        g->m.destroy(dirx, diry, true);
        p->moves -= 500;
        //~ the sound of a jackhammer
        g->sound(dirx, diry, 45, _("TATATATATATATAT!"));
    } else if (g->m.move_cost(dirx, diry) == 2 && g->levz != -1 &&
               g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
        g->m.destroy(dirx, diry, true);
        p->moves -= 500;
        g->sound(dirx, diry, 45, _("TATATATATATATAT!"));
    } else {
        p->add_msg_if_player(m_info, _("You can't drill there."));
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::jacqueshammer(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    // translator comments for everything to reduce confusion
    int dirx, diry;
    g->draw();
    //~ (jacqueshammer) "Drill where?"
    if (!choose_direction(_("Percer dans quelle direction?"), dirx, diry)) {
        //~ (jacqueshammer) "Invalid direction"
        p->add_msg_if_player(m_info, _("Direction invalide"));
        return 0;
    }
    if (dirx == 0 && diry == 0) {
        //~ (jacqueshammer) "My god! Let's talk it over, OK?"
        p->add_msg_if_player(_("Mon dieu! Nous allons en parler OK?"));
        //~ (jacqueshammer) "Don't do anything rash."
        p->add_msg_if_player(_("Ne pas faire eruption rien.."));
        return 0;
    }
    dirx += p->posx;
    diry += p->posy;
    if (g->m.is_bashable(dirx, diry) && g->m.has_flag("SUPPORTS_ROOF", dirx, diry) &&
        g->m.ter(dirx, diry) != t_tree) {
        g->m.destroy(dirx, diry, true);
        // This looked like 50 minutes, but seems more like 50 seconds.  Needs checked.
        p->moves -= 500;
        //~ the sound of a "jacqueshammer"
        g->sound(dirx, diry, 45, _("OHOHOHOHOHOHOHOHO!"));
    } else if (g->m.move_cost(dirx, diry) == 2 && g->levz != -1 &&
               g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
        g->m.destroy(dirx, diry, true);
        p->moves -= 500;
        g->sound(dirx, diry, 45, _("OHOHOHOHOHOHOHOHO!"));
    } else {
        //~ (jacqueshammer) "You can't drill there."
        p->add_msg_if_player(m_info, _("Vous ne pouvez pas percer la-bas.."));
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::pickaxe(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    int dirx, diry;
    if (!choose_adjacent(_("Mine where?"), dirx, diry)) {
        return 0;
    }

    if (dirx == p->posx && diry == p->posy) {
        p->add_msg_if_player(_("Mining the depths of your experience,"));
        p->add_msg_if_player(_("you realize that it's best not to dig"));
        p->add_msg_if_player(_("yourself into a hole. You stop digging."));
        return 0;
    }
    int turns;
    if (g->m.is_bashable(dirx, diry) && g->m.has_flag("SUPPORTS_ROOF", dirx, diry) &&
        g->m.ter(dirx, diry) != t_tree) {
        // Takes about 100 minutes (not quite two hours) base time.  Construction skill can speed this: 3 min off per level.
        turns = (100000 - 3000 * p->skillLevel("carpentry"));
    } else if (g->m.move_cost(dirx, diry) == 2 && g->levz == 0 &&
               g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
        turns = 20000;
    } else {
        p->add_msg_if_player(m_info, _("You can't mine there."));
        return 0;
    }
    p->assign_activity(ACT_PICKAXE, turns, -1, p->get_item_position(it));
    p->activity.placement = point(dirx, diry);
    p->add_msg_if_player(_("You attack the %s with your %s."),
                         g->m.tername(dirx, diry).c_str(), it->tname().c_str());
    return 0; // handled when the activity finishes
}

void on_turn_activity_pickaxe(player *p)
{
    const int dirx = p->activity.placement.x;
    const int diry = p->activity.placement.y;
    if (calendar::turn % MINUTES(1) == 0) { // each turn is too much
        //~ Sound of a Pickaxe at work!
        g->sound(dirx, diry, 30, _("CHNK! CHNK! CHNK!"));
        if (p->activity.moves_left <= 91000 && p->activity.moves_left > 89000) {
            p->add_msg_if_player(m_info, _("Ugh.  You figure it'll take about an hour and a half at this rate."));
        }
        if (p->activity.moves_left <= 71000 && p->activity.moves_left > 69000) {
            p->add_msg_if_player(m_info, _("If it keeps up like this, you might be through in an hour."));
        }
        if (p->activity.moves_left <= 31000 && p->activity.moves_left > 29000) {
            p->add_msg_if_player(m_info, _("Feels like you're making good progress.  Another half an hour, maybe?"));
        }
        if (p->activity.moves_left <= 11000 && p->activity.moves_left > 9000) {
            p->add_msg_if_player(m_info, _("That's got it.  Ten more minutes of work and it's open."));
        }
    }
}

void on_finish_activity_pickaxe(player *p)
{
    const int dirx = p->activity.placement.x;
    const int diry = p->activity.placement.y;
    item *it = &p->i_at(p->activity.position);
    if (g->m.is_bashable(dirx, diry) && g->m.has_flag("SUPPORTS_ROOF", dirx, diry) &&
        g->m.ter(dirx, diry) != t_tree) {
        // Tunneling through solid rock is hungry, sweaty, tiring, backbreaking work
        // Betcha wish you'd opted for the J-Hammer ;P
        p->hunger += 15;
        if (p->has_trait("STOCKY_TROGLO")) {
            p->fatigue += 20; // Yep, dwarves can dig longer before tiring
        } else {
            p->fatigue += 30;
        }
        p->thirst += 15;
        p->mod_pain(2 * rng(1, 3));
        // Mining is construction work!
        p->practice("carpentry", 5);
    } else if (g->m.move_cost(dirx, diry) == 2 && g->levz == 0 &&
               g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
        //Breaking up concrete on the surface? not nearly as bad
        p->hunger += 5;
        p->fatigue += 10;
        p->thirst += 5;
    }
    g->m.destroy(dirx, diry, true);
    it->charges = std::max(long(0), it->charges - it->type->charges_to_use());
    if (it->charges == 0 && it->destroyed_at_zero_charges()) {
        p->i_rem(p->activity.position);
    }
}

void on_turn_activity_burrow(player *p)
{
    const int dirx = p->activity.placement.x;
    const int diry = p->activity.placement.y;
    if (calendar::turn % MINUTES(1) == 0) { // each turn is too much
        //~ Sound of a Rat mutant burrowing!
        g->sound(dirx, diry, 10, _("ScratchCrunchScrabbleScurry."));
        if (p->activity.moves_left <= 91000 && p->activity.moves_left > 89000) {
            p->add_msg_if_player(m_info, _("You figure it'll take about an hour and a half at this rate."));
        }
        if (p->activity.moves_left <= 71000 && p->activity.moves_left > 69000) {
            p->add_msg_if_player(m_info, _("About an hour left to go."));
        }
        if (p->activity.moves_left <= 31000 && p->activity.moves_left > 29000) {
            p->add_msg_if_player(m_info, _("Shouldn't be more than half an hour or so now!"));
        }
        if (p->activity.moves_left <= 11000 && p->activity.moves_left > 9000) {
            p->add_msg_if_player(m_info, _("Almost there! Ten more minutes of work and you'll be through."));
        }
    }
}

void on_finish_activity_burrow(player *p)
{
    const int dirx = p->activity.placement.x;
    const int diry = p->activity.placement.y;
    if (g->m.is_bashable(dirx, diry) && g->m.has_flag("SUPPORTS_ROOF", dirx, diry) &&
        g->m.ter(dirx, diry) != t_tree) {
        // Tunneling through solid rock is hungry, sweaty, tiring, backbreaking work
        // Not quite as bad as the pickaxe, though
        p->hunger += 10;
        p->fatigue += 15;
        p->thirst += 10;
        p->mod_pain(3 * rng(1, 3));
        // Mining is construction work!
        p->practice("carpentry", 5);
    } else if (g->m.move_cost(dirx, diry) == 2 && g->levz == 0 &&
               g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
        //Breaking up concrete on the surface? not nearly as bad
        p->hunger += 5;
        p->fatigue += 10;
        p->thirst += 5;
    }
    g->m.destroy(dirx, diry, true);
}

int iuse::set_trap(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player( _("You can't do that while underwater."));
        return 0;
    }
    int dirx = 0;
    int diry = 0;
    if( !choose_adjacent( string_format(_("Place %s where?"), it->tname().c_str()), dirx, diry) ) {
        return 0;
    }

    if (dirx == p->posx && diry == p->posy) {
        p->add_msg_if_player(m_info, _("Yeah. Place the %s at your feet."), it->tname().c_str());
        p->add_msg_if_player(m_info, _("Real damn smart move."));
        return 0;
    }
    int posx = dirx;
    int posy = diry;
    if (g->m.move_cost(posx, posy) != 2) {
        p->add_msg_if_player(m_info, _("You can't place a %s there."), it->tname().c_str());
        return 0;
    }

    const trap_id existing_trap = g->m.tr_at(posx, posy);
    if (existing_trap != tr_null) {
        const struct trap &t = *traplist[existing_trap];
        if (t.can_see(*p, posx, posy)) {
            p->add_msg_if_player(m_info, _("You can't place a %s there. It contains a trap already."),
                                 it->tname().c_str());
        } else {
            p->add_msg_if_player(m_bad, _("You trigger a %s!"), t.name.c_str());
            t.trigger(p, posx, posy);
        }
        return 0;
    }

    trap_id type = tr_null;
    ter_id ter;
    bool buried = false;
    bool set = false;
    std::stringstream message;
    int practice = 0;

    if (it->type->id == "cot") {
        message << _("You unfold the cot and place it on the ground.");
        type = tr_cot;
        practice = 0;
    } else if (it->type->id == "rollmat") {
        message << _("You unroll the mat and lay it on the ground.");
        type = tr_rollmat;
        practice = 0;
    } else if (it->type->id == "fur_rollmat") {
        message << _("You unroll the fur mat and lay it on the ground.");
        type = tr_fur_rollmat;
        practice = 0;
    } else if (it->type->id == "brazier") {
        message << _("You place the brazier securely.");
        type = tr_brazier;
        practice = 0;
    } else if (it->type->id == "boobytrap") {
        message << _("You set the booby trap up and activate the grenade.");
        type = tr_boobytrap;
        practice = 4;
    } else if (it->type->id == "bubblewrap") {
        message << _("You set the bubble wrap on the ground, ready to be popped.");
        type = tr_bubblewrap;
        practice = 2;
    } else if (it->type->id == "beartrap") {
        buried = ((p->has_amount("shovel", 1) || p->has_amount("e_tool", 1)) &&
                  g->m.has_flag("DIGGABLE", posx, posy) &&
                  query_yn(_("Bury the beartrap?")));
        type = (buried ? tr_beartrap_buried : tr_beartrap);
        message << (buried ? _("You bury the beartrap.") : _("You set the beartrap."));
        practice = (buried ? 7 : 4);
    } else if (it->type->id == "board_trap") {
        message << string_format("You set the board trap on the %s, nails facing up.",
                                 g->m.tername(posx, posy).c_str());
        type = tr_nailboard;
        practice = 2;
    } else if (it->type->id == "caltrops") {
        message << string_format("You scatter the caltrops on the %s.",
                                 g->m.tername(posx, posy).c_str());
        type = tr_caltrops;
        practice = 2;
    } else if (it->type->id == "telepad") {
        message << _("You place the telepad.");
        type = tr_telepad;
        practice = 10;
    } else if (it->type->id == "funnel") {
        message << _("You place the funnel, waiting to collect rain.");
        type = tr_funnel;
        practice = 0;
    } else if (it->type->id == "makeshift_funnel") {
        message << _("You place the makeshift funnel, waiting to collect rain.");
        type = tr_makeshift_funnel;
        practice = 0;
    } else if (it->type->id == "leather_funnel") {
        message << _("You place the leather funnel, waiting to collect rain.");
        type = tr_leather_funnel;
        practice = 0;
    } else if (it->type->id == "tripwire") {
        // Must have a connection between solid squares.
        if ((g->m.move_cost(posx, posy - 1) != 2 &&
             g->m.move_cost(posx, posy + 1) != 2) ||
            (g->m.move_cost(posx + 1, posy) != 2 &&
             g->m.move_cost(posx - 1, posy) != 2) ||
            (g->m.move_cost(posx - 1, posy - 1) != 2 &&
             g->m.move_cost(posx + 1, posy + 1) != 2) ||
            (g->m.move_cost(posx + 1, posy - 1) != 2 &&
             g->m.move_cost(posx - 1, posy + 1) != 2)) {
            message << _("You string up the tripwire.");
            type = tr_tripwire;
            practice = 3;
        } else {
            p->add_msg_if_player(m_info, _("You must place the tripwire between two solid tiles."));
            return 0;
        }
    } else if (it->type->id == "crossbow_trap") {
        message << _("You set the crossbow trap.");
        type = tr_crossbow;
        practice = 4;
    } else if (it->type->id == "shotgun_trap") {
        message << _("You set the shotgun trap.");
        type = tr_shotgun_2;
        practice = 5;
    } else if (it->type->id == "blade_trap") {
        posx = (dirx - p->posx) * 2 + p->posx; //math correction for blade trap
        posy = (diry - p->posy) * 2 + p->posy;
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (g->m.move_cost(posx + i, posy + j) != 2) {
                    p->add_msg_if_player(m_info,
                                         _("That trap needs a 3x3 space to be clear, centered two tiles from you."));
                    return 0;
                }
            }
        }
        message << _("You set the blade trap two squares away.");
        type = tr_engine;
        practice = 12;
    } else if (it->type->id == "light_snare_kit") {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                ter = g->m.ter(posx + j, posy + i);
                if (ter == t_tree_young && !set) {
                    message << _("You set the snare trap.");
                    type = tr_light_snare;
                    practice = 2;
                    set = true;
                }
            }
        }
        if (!set) {
            p->add_msg_if_player(m_info, _("Invalid Placement."));
            return 0;
        }
    } else if (it->type->id == "heavy_snare_kit") {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                ter = g->m.ter(posx + j, posy + i);
                if (ter == t_tree && !set) {
                    message << _("You set the snare trap.");
                    type = tr_heavy_snare;
                    practice = 4;
                    set = true;
                }
            }
        }
        if (!set) {
            p->add_msg_if_player(m_info, _("Invalid Placement."));
            return 0;
        }
    } else if (it->type->id == "landmine") {
        buried = ((p->has_amount("shovel", 1) || p->has_amount("e_tool", 1)) &&
                  g->m.has_flag("DIGGABLE", posx, posy) &&
                  query_yn(_("Bury the land mine?")));
        type = (buried ? tr_landmine_buried : tr_landmine);
        message << (buried ? _("You bury the land mine.") : _("You set the land mine."));
        practice = (buried ? 7 : 4);
    } else {
        p->add_msg_if_player(_("Tried to set a trap.  But got confused! %s"), it->tname().c_str());
    }

    if (buried) {
        if (!p->has_amount("shovel", 1) && !p->has_amount("e_tool", 1)) {
            p->add_msg_if_player(m_info, _("You need a shovel."));
            return 0;
        } else if (!g->m.has_flag("DIGGABLE", posx, posy)) {
            p->add_msg_if_player(m_info, _("You can't dig in that %s"), g->m.tername(posx, posy).c_str());
            return 0;
        }
    }

    p->add_msg_if_player(message.str().c_str());
    p->practice("traps", practice);
    trap *tr = traplist[type];
    g->m.add_trap(posx, posy, type);
    if (!tr->can_see(*p, posx, posy)) {
        p->add_known_trap(posx, posy, tr->id);
    }
    p->moves -= 100 + practice * 25;
    if (type == tr_engine) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i != 0 || j != 0) {
                    g->m.add_trap(posx + i, posy + j, tr_blade);
                }
            }
        }
    }
    return 1;
}

int iuse::geiger(player *p, item *it, bool t, point pos)
{
    if (t) { // Every-turn use when it's on
        const int rads = g->m.get_radiation(pos.x, pos.y);
        if (rads == 0) {
            return it->type->charges_to_use();
        }
        if( !g->sound( pos.x, pos.y, 6, "" ) ) {
            // can not hear it, but may have alarmed other creatures
            return it->type->charges_to_use();
        }
        if (rads > 50) {
            add_msg(m_warning, _("The geiger counter buzzes intensely."));
        } else if (rads > 35) {
            add_msg(m_warning, _("The geiger counter clicks wildly."));
        } else if (rads > 25) {
            add_msg(m_warning, _("The geiger counter clicks rapidly."));
        } else if (rads > 15) {
            add_msg(m_warning, _("The geiger counter clicks steadily."));
        } else if (rads > 8) {
            add_msg(m_warning, _("The geiger counter clicks slowly."));
        } else if (rads > 4) {
            add_msg(_("The geiger counter clicks intermittently."));
        } else {
            add_msg(_("The geiger counter clicks once."));
        }
        return it->type->charges_to_use();
    }
    // Otherwise, we're activating the geiger counter
    it_tool *type = dynamic_cast<it_tool *>(it->type);
    bool is_on = (type->id == "geiger_on");
    if (is_on) {
        add_msg(_("The geiger counter's SCANNING LED flicks off."));
        it->make("geiger_off");
        it->active = false;
        return 0;
    }
    std::string toggle_text = is_on ? _("Turn continuous scan off") : _("Turn continuous scan on");
    int ch = menu(true, _("Geiger counter:"), _("Scan yourself"), _("Scan the ground"),
                  toggle_text.c_str(), _("Cancel"), NULL);
    switch (ch) {
        case 1:
            p->add_msg_if_player(m_info, _("Your radiation level: %d (%d from items)"), p->radiation,
                                 p->leak_level("RADIOACTIVE"));
            break;
        case 2:
            p->add_msg_if_player(m_info, _("The ground's radiation level: %d"),
                                 g->m.get_radiation(p->posx, p->posy));
            break;
        case 3:
            p->add_msg_if_player(_("The geiger counter's scan LED flicks on."));
            it->make("geiger_on");
            it->active = true;
            break;
        case 4:
            return 0;
    }
    return it->type->charges_to_use();
}

int iuse::teleport(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    p->moves -= 100;
    g->teleport(p);
    return it->type->charges_to_use();
}

int iuse::can_goo(player *p, item *it, bool, point)
{
    it->make("canister_empty");
    int tries = 0, goox, gooy;
    do {
        goox = p->posx + rng(-2, 2);
        gooy = p->posy + rng(-2, 2);
        tries++;
    } while (g->m.move_cost(goox, gooy) == 0 && tries < 10);
    if (tries == 10) {
        return 0;
    }
    int mondex = g->mon_at(goox, gooy);
    if (mondex != -1) {
        if (g->u_see(goox, gooy)) {
            add_msg(_("Black goo emerges from the canister and envelopes a %s!"),
                    g->zombie(mondex).name().c_str());
        }
        g->zombie(mondex).poly(GetMType("mon_blob"));

        g->zombie(mondex).set_speed_base( g->zombie(mondex).get_speed_base() - rng(5, 25) );
        g->zombie(mondex).hp = g->zombie(mondex).get_speed();
    } else {
        if (g->u_see(goox, gooy)) {
            add_msg(_("Living black goo emerges from the canister!"));
        }
        monster goo(GetMType("mon_blob"));
        goo.friendly = -1;
        goo.spawn(goox, gooy);
        g->add_zombie(goo);
    }
    tries = 0;
    while (!one_in(4) && tries < 10) {
        tries = 0;
        do {
            goox = p->posx + rng(-2, 2);
            gooy = p->posy + rng(-2, 2);
            tries++;
        } while (g->m.move_cost(goox, gooy) == 0 &&
                 g->m.tr_at(goox, gooy) == tr_null && tries < 10);
        if (tries < 10) {
            if (g->u_see(goox, gooy)) {
                add_msg(m_warning, _("A nearby splatter of goo forms into a goo pit."));
            }
            g->m.add_trap(goox, gooy, tr_goo);
        } else {
            return 0;
        }
    }
    return it->type->charges_to_use();
}

int iuse::throwable_extinguisher_act(player *, item *it, bool, point pos)
{
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }
    if( g->m.get_field( pos, fd_fire ) != nullptr ) {
        // Reduce the strength of fire (if any) in the target tile.
        g->m.adjust_field_strength(pos, fd_fire, 0 - 1);
        // Slightly reduce the strength of fire around and in the target tile.
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                if ((g->m.move_cost(pos.x + x, pos.y + y) != 0) && (x == 0 || y == 0)) {
                    g->m.adjust_field_strength(point(pos.x + x, pos.y + y), fd_fire, 0 - rng(0, 1));
                }
            }
        }
        return 1;
    }
    it->active = false;
    return 0;
}

int iuse::pipebomb_act(player *, item *it, bool t, point pos)
{
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }
    if (t) { // Simple timer effects
        //~ the sound of a lit fuse
        g->sound(pos.x, pos.y, 0, _("ssss...")); // Vol 0 = only heard if you hold it
    } else if (it->charges > 0) {
        add_msg(m_info, _("You've already lit the %s, try throwing it instead."), it->tname().c_str());
        return 0;
    } else { // The timer has run down
        if (one_in(10)) {
            // Fizzled, but we may not have seen it to know that
            if (g->u_see(pos.x, pos.y)) {
                add_msg(_("The pipe bomb fizzles out."));
            }
        } else {
            g->explosion(pos.x, pos.y, rng(6, 14), rng(0, 4), false);
        }
    }
    return 0;
}

int iuse::granade(player *p, item *it, bool, point)
{
    p->add_msg_if_player(_("You pull the pin on the Granade."));
    it->make("granade_act");
    it->charges = 5;
    it->active = true;
    return it->type->charges_to_use();
}

int iuse::granade_act(player *, item *it, bool t, point pos)
{
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }
    if (t) { // Simple timer effects
        g->sound(pos.x, pos.y, 0, _("Merged!"));  // Vol 0 = only heard if you hold it
    } else if (it->charges > 0) {
        add_msg(m_info, _("You've already pulled the %s's pin, try throwing it instead."),
                it->tname().c_str());
        return 0;
    } else { // When that timer runs down...
        int explosion_radius = 3;
        int effect_roll = rng(1, 5);
        switch (effect_roll) {
            case 1:
                g->sound(pos.x, pos.y, 100, _("BUGFIXES!!"));
                g->draw_explosion(pos.x, pos.y, explosion_radius, c_ltcyan);
                for (int i = -explosion_radius; i <= explosion_radius; i++) {
                    for (int j = -explosion_radius; j <= explosion_radius; j++) {
                        const int zid = g->mon_at(pos.x + i, pos.y + j);
                        if (zid != -1 &&
                            (g->zombie(zid).type->in_species("INSECT") ||
                             g->zombie(zid).is_hallucination())) {
                            g->zombie( zid ).die_in_explosion( nullptr );
                        }
                    }
                }
                break;

            case 2:
                g->sound(pos.x, pos.y, 100, _("BUFFS!!"));
                g->draw_explosion(pos.x, pos.y, explosion_radius, c_green);
                for (int i = -explosion_radius; i <= explosion_radius; i++) {
                    for (int j = -explosion_radius; j <= explosion_radius; j++) {
                        const int mon_hit = g->mon_at(pos.x + i, pos.y + j);
                        if (mon_hit != -1) {
                            g->zombie(mon_hit).set_speed_base(
                                g->zombie(mon_hit).get_speed_base() * rng_float(1.1, 2.0) );
                            g->zombie(mon_hit).hp *= rng_float(1.1, 2.0);
                        } else if (g->npc_at(pos.x + i, pos.y + j) != -1) {
                            int npc_hit = g->npc_at(pos.x + i, pos.y + j);
                            g->active_npc[npc_hit]->str_max += rng(0, g->active_npc[npc_hit]->str_max / 2);
                            g->active_npc[npc_hit]->dex_max += rng(0, g->active_npc[npc_hit]->dex_max / 2);
                            g->active_npc[npc_hit]->int_max += rng(0, g->active_npc[npc_hit]->int_max / 2);
                            g->active_npc[npc_hit]->per_max += rng(0, g->active_npc[npc_hit]->per_max / 2);
                        } else if (g->u.posx == pos.x + i && g->u.posy == pos.y + j) {
                            g->u.str_max += rng(0, g->u.str_max / 2);
                            g->u.dex_max += rng(0, g->u.dex_max / 2);
                            g->u.int_max += rng(0, g->u.int_max / 2);
                            g->u.per_max += rng(0, g->u.per_max / 2);
                            g->u.recalc_hp();
                            for (int part = 0; part < num_hp_parts; part++) {
                                g->u.hp_cur[part] *= 1 + rng(0, 20) * .1;
                                if (g->u.hp_cur[part] > g->u.hp_max[part]) {
                                    g->u.hp_cur[part] = g->u.hp_max[part];
                                }
                            }
                        }
                    }
                }
                break;

            case 3:
                g->sound(pos.x, pos.y, 100, _("NERFS!!"));
                g->draw_explosion(pos.x, pos.y, explosion_radius, c_red);
                for (int i = -explosion_radius; i <= explosion_radius; i++) {
                    for (int j = -explosion_radius; j <= explosion_radius; j++) {
                        const int mon_hit = g->mon_at(pos.x + i, pos.y + j);
                        if (mon_hit != -1) {
                            g->zombie(mon_hit).set_speed_base(
                                rng( 0, g->zombie(mon_hit).get_speed_base() ) );
                            g->zombie(mon_hit).hp = rng(1, g->zombie(mon_hit).hp);
                        } else if (g->npc_at(pos.x + i, pos.y + j) != -1) {
                            int npc_hit = g->npc_at(pos.x + i, pos.y + j);
                            g->active_npc[npc_hit]->str_max -= rng(0, g->active_npc[npc_hit]->str_max / 2);
                            g->active_npc[npc_hit]->dex_max -= rng(0, g->active_npc[npc_hit]->dex_max / 2);
                            g->active_npc[npc_hit]->int_max -= rng(0, g->active_npc[npc_hit]->int_max / 2);
                            g->active_npc[npc_hit]->per_max -= rng(0, g->active_npc[npc_hit]->per_max / 2);
                        } else if (g->u.posx == pos.x + i && g->u.posy == pos.y + j) {
                            g->u.str_max -= rng(0, g->u.str_max / 2);
                            g->u.dex_max -= rng(0, g->u.dex_max / 2);
                            g->u.int_max -= rng(0, g->u.int_max / 2);
                            g->u.per_max -= rng(0, g->u.per_max / 2);
                            g->u.recalc_hp();
                            for (int part = 0; part < num_hp_parts; part++) {
                                if (g->u.hp_cur[part] > 0) {
                                    g->u.hp_cur[part] = rng(1, g->u.hp_cur[part]);
                                }
                            }
                        }
                    }
                }
                break;

            case 4:
                g->sound(pos.x, pos.y, 100, _("REVERTS!!"));
                g->draw_explosion(pos.x, pos.y, explosion_radius, c_pink);
                for (int i = -explosion_radius; i <= explosion_radius; i++) {
                    for (int j = -explosion_radius; j <= explosion_radius; j++) {
                        const int mon_hit = g->mon_at(pos.x + i, pos.y + j);
                        if (mon_hit != -1) {
                            g->zombie(mon_hit).set_speed_base( g->zombie(mon_hit).type->speed );
                            g->zombie(mon_hit).hp = g->zombie(mon_hit).type->hp;
                            g->zombie(mon_hit).clear_effects();
                        } else if (g->npc_at(pos.x + i, pos.y + j) != -1) {
                            int npc_hit = g->npc_at(pos.x + i, pos.y + j);
                            g->active_npc[npc_hit]->environmental_revert_effect();
                        } else if (g->u.posx == pos.x + i && g->u.posy == pos.y + j) {
                            g->u.environmental_revert_effect();
                        }
                    }
                }
                break;
            case 5:
                g->sound(pos.x, pos.y, 100, _("BEES!!"));
                g->draw_explosion(pos.x, pos.y, explosion_radius, c_yellow);
                for (int i = -explosion_radius; i <= explosion_radius; i++) {
                    for (int j = -explosion_radius; j <= explosion_radius; j++) {
                        if (one_in(5) && -1 == g->mon_at(pos.x + i, pos.y + j) &&
                            -1 == g->npc_at(pos.x + i, pos.y + j)) {
                            g->m.add_field(pos.x + i, pos.y + j, fd_bees, rng(1, 3));
                        }
                    }
                }
                break;
        }
    }
    return it->type->charges_to_use();
}

int iuse::c4(player *p, item *it, bool, point)
{
    int time = query_int(_("Set the timer to (0 to cancel)?"));
    if (time <= 0) {
        p->add_msg_if_player(_("Never mind."));
        return 0;
    }
    p->add_msg_if_player(_("You set the timer to %d."), time);
    it->make("c4armed");
    it->charges = time;
    it->active = true;
    return it->type->charges_to_use();
}

int iuse::acidbomb_act(player *p, item *it, bool, point pos)
{
    if (!p->has_item(it)) {
        if (pos.x == -999) {
            pos = point(p->posx, p->posy);
        }
        it->charges = -1;
        for (int x = pos.x - 1; x <= pos.x + 1; x++) {
            for (int y = pos.y - 1; y <= pos.y + 1; y++) {
                g->m.add_field(x, y, fd_acid, 3);
            }
        }
    }
    return 0;
}

int iuse::grenade_inc_act(player *p, item *it, bool t, point pos)
{
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }

    if (t) { // Simple timer effects
        g->sound(pos.x, pos.y, 0, _("Tick!")); // Vol 0 = only heard if you hold it
    } else if (it->charges > 0) {
        p->add_msg_if_player(m_info, _("You've already released the handle, try throwing it instead."));
        return 0;
    } else {  // blow up
        int num_flames= rng(3,5);
        for (int current_flame = 0; current_flame < num_flames; current_flame++){
            std::vector<point> flames = line_to(pos.x, pos.y, pos.x + rng(-5,5), pos.y + rng(-5,5), 0);
            for( auto &flame : flames ) {
                g->m.add_field( flame.x, flame.y, fd_fire, rng( 0, 2 ) );
            }
        }
        g->explosion(pos.x, pos.y, 8, 0, true);
        for (int i = -2; i <= 2; i++) {
            for (int j = -2; j <= 2; j++) {
                g->m.add_field(pos.x + i, pos.y + j, fd_incendiary, 3);
            }
        }

    }
    return 0;
}

int iuse::arrow_flamable(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (!p->use_charges_if_avail("fire", 1)) {
        p->add_msg_if_player(m_info, _("You need a lighter!"));
        return 0;
    }
    p->add_msg_if_player(_("You light the arrow!."));
    p->moves -= 150;
    if (it->charges == 1) {
        it->make("arrow_flamming");
        return 0;
    }
    item lit_arrow(*it);
    lit_arrow.make("arrow_flamming");
    lit_arrow.charges = 1;
    p->i_add(lit_arrow);
    return 1;
}

int iuse::molotov(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (!p->use_charges_if_avail("fire", 1)) {
        p->add_msg_if_player(m_info, _("You need a lighter!"));
        return 0;
    }
    p->add_msg_if_player(_("You light the molotov cocktail."));
    p->moves -= 150;
    it->make("molotov_lit");
    it->bday = int(calendar::turn);
    it->active = true;
    return it->type->charges_to_use();
}

int iuse::molotov_lit(player *p, item *it, bool t, point pos)
{
    int age = int(calendar::turn) - it->bday;
    if (p->has_item(it)) {
        it->charges += 1;
        if (age >= 5) { // More than 5 turns old = chance of going out
            if (rng(1, 50) < age) {
                p->add_msg_if_player(_("Your lit molotov goes out."));
                it->make("molotov");
                it->active = false;
            }
        }
    } else {
        if (!t) {
            g->explosion(pos.x, pos.y, 8, 0, true);
        }
    }
    return 0;
}

int iuse::firecracker_pack(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (!p->has_charges("fire", 1)) {
        p->add_msg_if_player(m_info, _("You need a lighter!"));
        return 0;
    }
    WINDOW *w = newwin(5, 41, (TERMY - 5) / 2, (TERMX - 41) / 2);
    WINDOW_PTR wptr( w );
    draw_border(w);
    int mid_x = getmaxx(w) / 2;
    int tmpx = 5;
    // TODO: Should probably be a input box anyway.
    mvwprintz(w, 1, 2, c_white, _("How many do you want to light? (1-%d)"), it->charges);
    mvwprintz(w, 2, mid_x, c_white, "1");
    tmpx += shortcut_print(w, 3, tmpx, c_white, c_ltred, _("<I>ncrease")) + 1;
    tmpx += shortcut_print(w, 3, tmpx, c_white, c_ltred, _("<D>ecrease")) + 1;
    tmpx += shortcut_print(w, 3, tmpx, c_white, c_ltred, _("<A>ccept")) + 1;
    shortcut_print(w, 3, tmpx, c_white, c_ltred, _("<C>ancel"));
    wrefresh(w);
    bool close = false;
    long charges = 1;
    char ch = getch();
    while (!close) {
        if (ch == 'I') {
            charges++;
            if (charges > it->charges) {
                charges = it->charges;
            }
            mvwprintz(w, 2, mid_x, c_white, "%d", charges);
            wrefresh(w);
        } else if (ch == 'D') {
            charges--;
            if (charges < 1) {
                charges = 1;
            }
            mvwprintz(w, 2, mid_x, c_white, "%d ",
                      charges); //Trailing space clears the second digit when decreasing from 10 to 9
            wrefresh(w);
        } else if (ch == 'A') {
            p->use_charges("fire", 1);
            if (charges == it->charges) {
                p->add_msg_if_player(_("You light the pack of firecrackers."));
                it->make("firecracker_pack_act");
                it->charges = charges;
                it->bday = calendar::turn;
                it->active = true;
                return 0; // don't use any charges at all. it has became a new item
            } else {
                if (charges == 1) {
                    p->add_msg_if_player(_("You light one firecracker."));
                    item new_it = item("firecracker_act", int(calendar::turn));
                    new_it.charges = 2;
                    new_it.active = true;
                    p->i_add(new_it);
                } else {
                    p->add_msg_if_player(ngettext("You light a string of %d firecracker.",
                                                  "You light a string of %d firecrackers.", charges), charges);
                    item new_it = item("firecracker_pack_act", int(calendar::turn));
                    new_it.charges = charges;
                    new_it.active = true;
                    p->i_add(new_it);
                }
                if (it->charges == 1) {
                    it->make("firecracker");
                }
            }
            close = true;
        } else if (ch == 'C') {
            return 0; // don't use any charges at all
        }
        if (!close) {
            ch = getch();
        }
    }
    return charges;
}

int iuse::firecracker_pack_act(player *, item *it, bool, point pos)
{
    int current_turn = calendar::turn;
    int timer = current_turn - it->bday;
    if (timer < 2) {
        g->sound(pos.x, pos.y, 0, _("ssss..."));
        it->damage += 1;
    } else if (it->charges > 0) {
        int ex = rng(3, 5);
        int i = 0;
        if (ex > it->charges) {
            ex = it->charges;
        }
        for (i = 0; i < ex; i++) {
            g->sound(pos.x, pos.y, 20, _("Bang!"));
        }
        it->charges -= ex;
    }
    return 0;
}

int iuse::firecracker(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (!p->use_charges_if_avail("fire", 1)) {
        p->add_msg_if_player(m_info, _("You need a lighter!"));
        return 0;
    }
    p->add_msg_if_player(_("You light the firecracker."));
    it->make("firecracker_act");
    it->charges = 2;
    it->active = true;
    return it->type->charges_to_use();
}

int iuse::firecracker_act(player *, item *it, bool t, point pos)
{
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }
    if (t) {// Simple timer effects
        g->sound(pos.x, pos.y, 0, _("ssss..."));
    } else if (it->charges > 0) {
        add_msg(m_info, _("You've already lit the %s, try throwing it instead."), it->tname().c_str());
        return 0;
    } else { // When that timer runs down...
        g->sound(pos.x, pos.y, 20, _("Bang!"));
    }
    return 0;
}

int iuse::mininuke(player *p, item *it, bool, point)
{
    int time = query_int(_("Set the timer to (0 to cancel)?"));
    if (time <= 0) {
        p->add_msg_if_player(_("Never mind."));
        return 0;
    }
    p->add_msg_if_player(_("You set the timer to %d."), time);
    if (!p->is_npc()) {
        p->add_memorial_log(pgettext("memorial_male", "Activated a mininuke."),
                            pgettext("memorial_female", "Activated a mininuke."));
    }
    it->make("mininuke_act");
    it->charges = time;
    it->active = true;
    return it->type->charges_to_use();
}

int iuse::pheromone(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    point pos(p->posx, p->posy);

    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }

    p->add_msg_player_or_npc(_("You squeeze the pheromone ball..."),
                             _("<npcname> squeezes the pheromone ball..."));

    p->moves -= 15;

    int converts = 0;
    for (int x = pos.x - 4; x <= pos.x + 4; x++) {
        for (int y = pos.y - 4; y <= pos.y + 4; y++) {
            int mondex = g->mon_at(x, y);
            if( mondex == -1 ) {
                continue;
            }
            monster &critter = g->zombie( mondex );
            if( critter.type->in_species( "ZOMBIE" ) && critter.friendly == 0 && rng( 0, 500 ) > critter.hp ) {
                converts++;
                critter.make_friendly();
            }
        }
    }

    if (g->u_see(p)) {
        if (converts == 0) {
            add_msg(_("...but nothing happens."));
        } else if (converts == 1) {
            add_msg(m_good, _("...and a nearby zombie turns friendly!"));
        } else {
            add_msg(m_good, _("...and several nearby zombies turn friendly!"));
        }
    }
    return it->type->charges_to_use();
}


int iuse::portal(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    g->m.add_trap(p->posx + rng(-2, 2), p->posy + rng(-2, 2), tr_portal);
    return it->type->charges_to_use();
}

int iuse::tazer(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    int dirx, diry;
    if (!choose_adjacent(_("Shock where?"), dirx, diry)) {
        return 0;
    }

    if (dirx == p->posx && diry == p->posy) {
        p->add_msg_if_player(m_info, _("Umm. No."));
        return 0;
    }
    int mondex = g->mon_at(dirx, diry);
    int npcdex = g->npc_at(dirx, diry);
    if (mondex == -1 && npcdex == -1) {
        p->add_msg_if_player(_("Electricity crackles in the air."));
        return it->type->charges_to_use();
    }

    int numdice = 3 + (p->dex_cur / 2.5) + p->skillLevel("melee") * 2;
    p->moves -= 100;

    if (mondex != -1) {
        monster *z = &(g->zombie(mondex));
        switch (z->type->size) {
            case MS_TINY:
                numdice -= 2;
                break;
            case MS_SMALL:
                numdice -= 1;
                break;
            case MS_MEDIUM:
                break;
            case MS_LARGE:
                numdice += 2;
                break;
            case MS_HUGE:
                numdice += 4;
                break;
        }
        int mondice = z->get_dodge();
        if (dice(numdice, 10) < dice(mondice, 10)) { // A miss!
            p->add_msg_if_player(_("You attempt to shock the %s, but miss."), z->name().c_str());
            return it->type->charges_to_use();
        }
        p->add_msg_if_player(m_good, _("You shock the %s!"), z->name().c_str());
        int shock = rng(5, 25);
        z->moves -= shock * 100;
        z->apply_damage( p, bp_torso, shock );
        return it->type->charges_to_use();
    }

    if (npcdex != -1) {
        npc *foe = g->active_npc[npcdex];
        if (foe->attitude != NPCATT_FLEE) {
            foe->attitude = NPCATT_KILL;
        }
        if (foe->str_max >= 17) {
            numdice++;    // Minor bonus against huge people
        } else if (foe->str_max <= 5) {
            numdice--;    // Minor penalty against tiny people
        }
        if (dice(numdice, 10) <= dice(foe->get_dodge(), 6)) {
            p->add_msg_if_player(_("You attempt to shock %s, but miss."), foe->name.c_str());
            return it->type->charges_to_use();
        }
        p->add_msg_if_player(m_good, _("You shock %s!"), foe->name.c_str());
        int shock = rng(5, 20);
        foe->moves -= shock * 100;
        foe->hurtall(shock);
        if (foe->hp_cur[hp_head] <= 0 || foe->hp_cur[hp_torso] <= 0) {
            foe->die( p );
            g->active_npc.erase(g->active_npc.begin() + npcdex);
        }
    }
    return it->type->charges_to_use();
}

int iuse::tazer2(player *p, item *it, bool, point)
{
    if (it->charges >= 100) {
        int dirx, diry;

        if (!choose_adjacent(_("Shock"), dirx, diry)) {
            return 0;
        }

        if (dirx == p->posx && diry == p->posy) {
            p->add_msg_if_player(m_info, _("Umm. No."));
            return 0;
        }

        int mondex = g->mon_at(dirx, diry);
        int npcdex = g->npc_at(dirx, diry);

        if (mondex == -1 && npcdex == -1) {
            p->add_msg_if_player(_("Electricity crackles in the air."));
            return 100;
        }

        int numdice = 3 + (p->dex_cur / 2.5) + p->skillLevel("melee") * 2;
        p->moves -= 100;

        if (mondex != -1) {
            monster *z = &(g->zombie(mondex));

            switch (z->type->size) {
                case MS_TINY:
                    numdice -= 2;
                    break;

                case MS_SMALL:
                    numdice -= 1;
                    break;

                case MS_MEDIUM:
                    break;

                case MS_LARGE:
                    numdice += 2;
                    break;

                case MS_HUGE:
                    numdice += 4;
                    break;
            }

            int mondice = z->get_dodge();

            if (dice(numdice, 10) < dice(mondice, 10)) { // A miss!
                p->add_msg_if_player(_("You attempt to shock the %s, but miss."),
                                     z->name().c_str());
                return 100;
            }

            p->add_msg_if_player(m_good, _("You shock the %s!"), z->name().c_str());
            int shock = rng(5, 25);
            z->moves -= shock * 100;
            z->apply_damage( p, bp_torso, shock );

            return 100;
        }

        if (npcdex != -1) {
            npc *foe = g->active_npc[npcdex];

            if (foe->attitude != NPCATT_FLEE) {
                foe->attitude = NPCATT_KILL;
            }

            if (foe->str_max >= 17) {
                numdice++;    // Minor bonus against huge people
            } else if (foe->str_max <= 5) {
                numdice--;    // Minor penalty against tiny people
            }

            if (dice(numdice, 10) <= dice(foe->get_dodge(), 6)) {
                p->add_msg_if_player(_("You attempt to shock %s, but miss."), foe->name.c_str());
                return it->charges -= 100;
            }

            p->add_msg_if_player(m_good, _("You shock %s!"), foe->name.c_str());
            int shock = rng(5, 20);
            foe->moves -= shock * 100;
            foe->hurtall(shock);

            if (foe->hp_cur[hp_head] <= 0 || foe->hp_cur[hp_torso] <= 0) {
                foe->die( p );
                g->active_npc.erase(g->active_npc.begin() + npcdex);
            }
        }

        return 100;
    } else {
        p->add_msg_if_player(m_info, _("Insufficient power"));
    }

    return 0;
}

int iuse::shocktonfa_off(player *p, item *it, bool t, point pos)
{
    int choice = menu(true, _("tactical tonfa"), _("Zap something"),
                      _("Turn on light"), _("Cancel"), NULL);

    switch (choice) {
        case 1: {
            return iuse::tazer2(p, it, t, pos);
        }
        break;

        case 2: {
            if (it->charges <= 0) {
                p->add_msg_if_player(m_info, _("The batteries are dead."));
                return 0;
            } else {
                p->add_msg_if_player(_("You turn the light on."));
                it->make("shocktonfa_on");
                it->active = true;
                return it->type->charges_to_use();
            }
        }
    }
    return 0;
}

int iuse::shocktonfa_on(player *p, item *it, bool t, point pos)
{
    if (t) {  // Effects while simply on

    } else {
        if (it->charges <= 0) {
            p->add_msg_if_player(m_info, _("Your tactical tonfa is out of power"));
            it->make("shocktonfa_off");
            it->active = false;
        } else {
            int choice = menu(true, _("tactical tonfa"), _("Zap something"),
                              _("Turn off light"), _("cancel"), NULL);

            switch (choice) {
                case 1: {
                    return iuse::tazer2(p, it, t, pos);
                }
                break;

                case 2: {
                    p->add_msg_if_player(_("You turn off the light"));
                    it->make("shocktonfa_off");
                    it->active = false;
                }
            }
        }
    }
    return 0;
}

int iuse::mp3(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_info, _("The mp3 player's batteries are dead."));
    } else if (p->has_active_item("mp3_on")) {
        p->add_msg_if_player(m_info, _("You are already listening to an mp3 player!"));
    } else {
        p->add_msg_if_player(m_info, _("You put in the earbuds and start listening to music."));
        it->make("mp3_on");
        it->active = true;
    }
    return it->type->charges_to_use();
}

std::string get_random_music_description(player *p)
{
    std::string sound = "";

    switch (rng(1, 10)) {
        case 1:
            sound = _("a sweet guitar solo!");
            p->stim++;
            break;
        case 2:
            sound = _("a funky bassline.");
            break;
        case 3:
            sound = _("some amazing vocals.");
            break;
        case 4:
            sound = _("some pumping bass.");
            break;
        case 5:
            sound = _("dramatic classical music.");
            if (p->int_cur >= 10) {
                p->add_morale(MORALE_MUSIC, 1, (50 + p->int_cur*2), 5, 2);
            }
            break;
    }
    if (one_in(50)) {
        sound = _("some bass-heavy post-glam speed polka");
    }

    return sound;
}

void iuse::play_music( player *p, point source, int volume )
{
    std::string sound = "";
    if( int(calendar::turn) % 50 == 0 ) {
        // Every 5 minutes, describe the music
        std::string description = get_random_music_description(p);
        if( !description.empty() ) {
            sound = string_format( _("You listen to %s"), description.c_str() );
        }
    }

    if( g->ambient_sound( source.x, source.y, volume, sound ) && !p->has_effect("music") ){
        p->add_effect("music", 1);
        p->add_morale(MORALE_MUSIC, 1, 50, 5, 2);
    }
}

int iuse::mp3_on(player *p, item *it, bool t, point)
{
    if (t) { // Normal use
        play_music( p, p->pos(), 0 );
    } else { // Turning it off
        p->add_msg_if_player(_("The mp3 player turns off."));
        it->make("mp3");
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::portable_game(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (p->has_trait("ILLITERATE")) {
        add_msg(_("You're illiterate!"));
        return 0;
    } else if (it->charges < 15) {
        p->add_msg_if_player(m_info, _("The %s's batteries are dead."), it->tname().c_str());
        return 0;
    } else {
        std::string loaded_software = "robot_finds_kitten";

        uimenu as_m;
        as_m.text = _("What do you want to play?");
        as_m.entries.push_back(uimenu_entry(1, true, '1', _("Robot finds Kitten")));
        as_m.entries.push_back(uimenu_entry(2, true, '2', _("S N A K E")));
        as_m.entries.push_back(uimenu_entry(3, true, '3', _("Sokoban")));
        as_m.entries.push_back(uimenu_entry(4, true, '4', _("Cancel")));
        as_m.query();

        switch (as_m.ret) {
            case 1:
                loaded_software = "robot_finds_kitten";
                p->rooted_message();
                break;
            case 2:
                loaded_software = "snake_game";
                p->rooted_message();
                break;
            case 3:
                loaded_software = "sokoban_game";
                p->rooted_message();
                break;
            case 4: //Cancel
                return 0;
        }

        //Play in 15-minute chunks
        int time = 15000;

        p->add_msg_if_player(_("You play on your %s for a while."), it->tname().c_str());
        p->assign_activity(ACT_GAME, time, -1, p->get_item_position(it), "gaming");

        std::map<std::string, std::string> game_data;
        game_data.clear();
        int game_score = 0;

        play_videogame(loaded_software, game_data, game_score);

        if (game_data.find("end_message") != game_data.end()) {
            p->add_msg_if_player("%s", game_data["end_message"].c_str());
        }

        if (game_score != 0) {
            if (game_data.find("moraletype") != game_data.end()) {
                std::string moraletype = game_data.find("moraletype")->second;
                if (moraletype == "MORALE_GAME_FOUND_KITTEN") {
                    p->add_morale(MORALE_GAME_FOUND_KITTEN, game_score, 110);
                } /*else if ( ...*/
            } else {
                p->add_morale(MORALE_GAME, game_score, 110);
            }
        }

    }
    return it->type->charges_to_use();
}

int iuse::vibe(player *p, item *it, bool, point)
{
    if ((p->is_underwater()) && (!((p->has_trait("GILLS")) || (p->is_wearing("rebreather_on")) ||
                                   (p->is_wearing("rebreather_xl_on")) || (p->is_wearing("mask_h20survivor_on"))))) {
        p->add_msg_if_player(m_info, _("It's waterproof, but oxygen maybe?"));
        return 0;
    }
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_info, _("The %s's batteries are dead."), it->tname().c_str());
        return 0;
    }
    if (p->fatigue >= 383) {
        p->add_msg_if_player(m_info, _("*Your* batteries are dead."));
        return 0;
    } else {
        int time = 20000; // 20 minutes per
        p->add_msg_if_player(_("You fire up your %s and start getting the tension out."),
                             it->tname().c_str());
        p->assign_activity(ACT_VIBE, time, -1, p->get_item_position(it), "de-stressing");
    }
    return it->type->charges_to_use();
}

int iuse::vortex(player *p, item *it, bool, point)
{
    std::vector<point> spawn;
    for (int i = -3; i <= 3; i++) {
        if (g->is_empty(p->posx - 3, p->posy + i)) {
            spawn.push_back(point(p->posx - 3, p->posy + i));
        }
        if (g->is_empty(p->posx + 3, p->posy + i)) {
            spawn.push_back(point(p->posx + 3, p->posy + i));
        }
        if (g->is_empty(p->posx + i, p->posy - 3)) {
            spawn.push_back(point(p->posx + i, p->posy - 3));
        }
        if (g->is_empty(p->posx + i, p->posy + 3)) {
            spawn.push_back(point(p->posx + i, p->posy + 3));
        }
    }
    if (spawn.empty()) {
        p->add_msg_if_player(m_warning, _("Air swirls around you for a moment."));
        it->make("spiral_stone");
        return it->type->charges_to_use();
    }

    p->add_msg_if_player(m_warning, _("Air swirls all over..."));
    int index = rng(0, spawn.size() - 1);
    p->moves -= 100;
    it->make("spiral_stone");
    monster mvortex(GetMType("mon_vortex"), spawn[index].x, spawn[index].y);
    mvortex.friendly = -1;
    g->add_zombie(mvortex);
    return it->type->charges_to_use();
}

int iuse::dog_whistle(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    p->add_msg_if_player(_("You blow your dog whistle."));
    for (size_t i = 0; i < g->num_zombies(); i++) {
        if (g->zombie(i).friendly != 0 && g->zombie(i).type->id == "mon_dog") {
            bool u_see = g->u_see(&(g->zombie(i)));
            if (g->zombie(i).has_effect("docile")) {
                if (u_see) {
                    p->add_msg_if_player(_("Your %s looks ready to attack."), g->zombie(i).name().c_str());
                }
                g->zombie(i).remove_effect("docile");
            } else {
                if (u_see) {
                    p->add_msg_if_player(_("Your %s goes docile."), g->zombie(i).name().c_str());
                }
                g->zombie(i).add_effect("docile", 1, num_bp, true);
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::vacutainer(player *p, item *it, bool, point)
{
    if (p->is_npc()) {
        return 0;    // No NPCs for now!
    }

    if (!it->contents.empty()) {
        p->add_msg_if_player(m_info, _("That %s is full!"), it->tname().c_str());
        return 0;
    }

    item blood("blood", calendar::turn);
    bool drew_blood = false;
    for( auto &map_it : g->m.i_at(p->posx, p->posy) ) {
        if( map_it.corpse != NULL && map_it.type->id == "corpse" &&
            query_yn(_("Draw blood from %s?"), map_it.tname().c_str()) ) {
            blood.corpse = map_it.corpse;
            drew_blood = true;
        }
    }

    if (!drew_blood && query_yn(_("Draw your own blood?"))) {
        drew_blood = true;
    }

    if (!drew_blood) {
        return it->type->charges_to_use();
    }

    it->put_in(blood);
    return it->type->charges_to_use();
}

void make_zlave(player *p)
{
    auto &items = g->m.i_at(p->posx, p->posy);
    std::vector<const item *> corpses;

    const int cancel = 0;

    for( auto &it : items ) {
        if( it.is_corpse() && it.corpse->in_species("ZOMBIE") && it.corpse->mat == "flesh" &&
            it.corpse->sym == "Z" && it.active && it.item_vars.find("zlave") == it.item_vars.end() ) {
            corpses.push_back( &it );
        }
    }

    if (corpses.empty()) {
        p->add_msg_if_player(_("No suitable corpses"));
        return;
    }

    int tolerance_level = 9;
    if( p->has_trait("PSYCHOPATH") || p->has_trait("SAPIOVORE") ) {
        tolerance_level = 0;
    } else if( p->has_trait("PRED4") ) {
        tolerance_level = 5;
    } else if( p->has_trait("PRED3") ) {
        tolerance_level = 7;
    }

    // Survival skill increases your willingness to get things done,
    // but it doesn't make you feel any less bad about it.
    if( p->morale_level() <= (15 * (tolerance_level - p->skillLevel("survival") )) - 150 ) {
        add_msg(m_neutral, _("The prospect of cutting up the copse and letting it rise again as a slave is too much for you to deal with right now."));
        return;
    }

    uimenu amenu;

    amenu.selected = 0;
    amenu.text = _("Selectively butcher the downed zombie into a zombie slave?");
    amenu.addentry(cancel, true, 'q', _("Cancel"));
    for (size_t i = 0; i < corpses.size(); i++) {
        amenu.addentry(i + 1, true, -1, corpses[i]->display_name().c_str());
    }

    amenu.query();

    if (cancel == amenu.ret) {
        p->add_msg_if_player(_("Make love, not zlave."));
        return;
    }

    if( tolerance_level == 0 ) {
        // You just don't care, no message.
    } else if( tolerance_level <= 5 ) {
        add_msg(m_neutral, _("Well, it's more constructive than just chopping 'em into gooey meat..."));
    } else {
        add_msg(m_bad, _("You feel horrible for mutilating and enslaving someone's corpse."));

        int moraleMalus = -50 * (5.0 / (float) p->skillLevel("survival"));
        int maxMalus = -250 * (5.0 / (float)p->skillLevel("survival"));
        int duration = 300 * (5.0 / (float)p->skillLevel("survival"));
        int decayDelay = 30 * (5.0 / (float)p->skillLevel("survival"));

        if (p->has_trait("PACIFIST")) {
            moraleMalus *= 5;
            maxMalus *= 3;
        } else if (p->has_trait("PRED1")) {
            moraleMalus /= 4;
        } else if (p->has_trait("PRED2")) {
            moraleMalus /= 5;
        }

        p->add_morale(MORALE_MUTILATE_CORPSE, moraleMalus, maxMalus, duration, decayDelay);
    }

    const int selected_corpse = amenu.ret - 1;

    const item *body = corpses[selected_corpse];
    const mtype *mt = body->corpse;

    // HP range for zombies is roughly 36 to 120, with the really big ones having 180 and 480 hp.
    // Speed range is 20 - 120 (for humanoids, dogs get way faster)
    // This gives us a difficulty ranging rougly from 10 - 40, with up to +25 for corpse damage.
    // An average zombie with an undamaged corpse is 0 + 8 + 14 = 22.
    int difficulty = (body->damage * 5) + (mt->hp / 10) + (mt->speed / 5);
    // 0 - 30
    int skills = p->skillLevel("survival") + p->skillLevel("firstaid") + (p->dex_cur / 2);
    skills *= 2;

    int success = rng(0, skills) - rng(0, difficulty);

    const int moves = difficulty * 1200 / p->skillLevel("firstaid");

    p->assign_activity(ACT_MAKE_ZLAVE, moves);
    p->activity.values.push_back(success);
    p->activity.str_values.push_back(corpses[selected_corpse]->display_name());
}

// *it here is the item that is a candidate for being chopped up.
// This is the former valid_to_cut_up with all the messages and queries
bool static try_to_cut_up(player *p, item *it)
{
    int pos = p->get_item_position(it);
    // If a material is made of different items than what is in this set, we
    // do not cut it up.
    std::vector<std::string> material_id_white_list;
    material_id_white_list.push_back("cotton");
    material_id_white_list.push_back("leather");
    material_id_white_list.push_back("fur");
    material_id_white_list.push_back("nomex");
    material_id_white_list.push_back("kevlar");
    material_id_white_list.push_back("plastic");
    material_id_white_list.push_back("wood");
    material_id_white_list.push_back("wool");

    if (it->is_null()) {
        add_msg(m_info, _("You do not have that item."));
        return false;
    }
    // There must be some historical significance to these items.
    if (!it->is_salvageable()) {
        add_msg(m_info, _("Can't salvage anything from %s."), it->tname().c_str());
        if (it->is_disassemblable()) {
            add_msg(m_info, _("Try disassembling the %s instead."), it->tname().c_str());
        }
        return false;
    }
    if( !it->only_made_of(material_id_white_list) ) {
        add_msg(m_info, _("The %s is made of material that cannot be cut up."), it->tname().c_str());
        return false;
    }
    if (it->is_container() && !it->contents.empty()) {
        add_msg(m_info, _("Please empty the %s before cutting it up."), it->tname().c_str());
        return false;
    }
    if (it->volume() == 0) {
        add_msg(m_info, _("The %s is too small to salvage material from."), it->tname().c_str());
        return false;
    }
    // Softer warnings at the end so we don't ask permission and then tell them no.
    if (it == &p->weapon) {
        if(!query_yn(_("You are wielding that, are you sure?"))) {
            return false;
        }
    } else if( pos == INT_MIN ) {
        // Not in inventory
        return true;
    } else if( pos < -1 ) {
        if(!query_yn(_("You're wearing that, are you sure?"))) {
            return false;
        }
    }

    return true;
}

// This is new silent valid_to_cut_up
bool iuse::valid_to_cut_up(const item *it)
{
    // If a material is made of different items than what is in this set, we
    // do not cut it up.
    std::vector<std::string> material_id_white_list;
    material_id_white_list.push_back("cotton");
    material_id_white_list.push_back("leather");
    material_id_white_list.push_back("fur");
    material_id_white_list.push_back("nomex");
    material_id_white_list.push_back("kevlar");
    material_id_white_list.push_back("plastic");
    material_id_white_list.push_back("wood");
    material_id_white_list.push_back("wool");

    if (it->is_null()) {
        return false;
    }
    // There must be some historical significance to these items.
    if (!it->is_salvageable()) {
        return false;
    }
    if( !it->only_made_of(material_id_white_list) ) {
        return false;
    }
    if (it->is_container() && !it->contents.empty()) {
        return false;
    }
    if (it->volume() == 0) {
        return false;
    }

    return true;
}

// function returns charges from *it during the cutting process of the *cut.
// *it cuts
// *cut gets cut
int iuse::cut_up(player *p, item *it, item *cut, bool)
{
    int pos = p->get_item_position(cut);
    // total number of raw components == total volume of item.
    // This can go awry if there is a volume / recipe mismatch.
    int count = cut->volume();
    // Chance of us losing a material component to entropy.
    int entropy_threshold = std::max(5, 10 - p->skillLevel("fabrication"));
    // What material components can we get back?
    std::vector<std::string> cut_material_components = cut->made_of();
    // What materials do we salvage (ids and counts).
    std::map<std::string, int> materials_salvaged;

    // Final just in case check (that perhaps was not done elsewhere);
    if (cut == it) {
        add_msg(m_info, _("You can not cut the %s with itself."), it->tname().c_str());
        return 0;
    }

    // Time based on number of components.
    p->moves -= 25 * count;
    // Not much practice, and you won't get very far ripping things up.
    Skill *isFab = Skill::skill("fabrication");
    p->practice(isFab, rng(0, 5), 1);

    // Higher fabrication, less chance of entropy, but still a chance.
    if (rng(1, 10) <= entropy_threshold) {
        count -= 1;
    }
    // Fail dex roll, potentially lose more parts.
    if (dice(3, 4) > p->dex_cur) {
        count -= rng(0, 2);
    }
    // If more than 1 material component can still be be salvaged,
    // chance of losing more components if the item is damaged.
    // If the item being cut is not damaged, no additional losses will be incurred.
    if (count > 0 && cut->damage > 0) {
        float component_success_chance = std::min(std::pow(0.8, cut->damage), 1.0);
        for(int i = count; i > 0; i--) {
            if(component_success_chance < rng_float(0,1)) {
                count--;
            }
        }
    }

    // Decided to split components evenly. Since salvage will likely change
    // soon after I write this, I'll go with the one that is cleaner.
    for (auto material : cut_material_components) {
        material_type * mt = material_type::find_material(material);
        std::string salvaged_id = mt->salvage_id();
        float salvage_multiplier = mt->salvage_multiplier();
        materials_salvaged[salvaged_id] = count * salvage_multiplier / cut_material_components.size();
    }

    add_msg(m_info, _("You try to salvage materials from the %s."), cut->tname().c_str());

    // Clean up before removing the item.
    remove_ammo(cut, *p);
    // Original item has been consumed.
    if( pos != INT_MIN ) {
        p->i_rem(pos);
    } else {
        g->m.i_rem( p->posx, p->posy, cut );
    }

    for (auto salvaged : materials_salvaged) {
        std::string mat_name = salvaged.first;
        int amount = salvaged.second;
        item result(mat_name, int(calendar::turn));
        if (amount > 0) {
            add_msg(m_good, ngettext("Salvaged %1$i %2$s.", "Salvaged %1$i %2$ss.", amount),
                    amount, result.display_name().c_str());
            if( pos != INT_MIN ) {
                p->i_add_or_drop(result, amount);
            } else {
                for( int i = 0; i < amount; i++ ) {
                    g->m.spawn_an_item( p->posx, p->posy, result, amount, 0 );
                }
            }
        } else {
            add_msg(m_bad, _("Could not salvage a %s."), result.display_name().c_str());
        }
    }
    // No matter what, cutting has been done by the time we get here.
    return it->type->charges_to_use();
}

// item is what is doing the carving, item to be carved is chosen here.
// Queries player for the item into which we will carve.
// Return value is charges used (if any) for carving based on *it passed in.
// Return value can be passed back from an iuse function.
static int carve_writing(player *p, item *it)
{
    int pos = g->inv(_("Carve writing into what?"));
    item *cut = &(p->i_at(pos));

    if (cut->is_null()) {
        add_msg(m_info, _("You do not have that item."));
        return 0;
    }
    if (cut == it) {
        add_msg(m_info, _("You can not cut the %s with itself."), it->tname().c_str());
        return 0;
    }

    // Adding this check in as a formality, but I'm pretty sure whatever
    // in reality items used to carve will not burn charges for writing.
    if (item_inscription(p, cut, _("Carve"), _("Carved"), true)) {
        return it->type->charges_to_use();
    } else {
        return 0;
    }
}

static int cauterize_flame(player *p, item *it)
{
    bool has_disease = p->has_effect("bite") || p->has_effect("bleed");
    bool did_cauterize = false;
    if (!p->has_charges("fire", 4)) {
        p->add_msg_if_player(m_info, _("You need a source of flame (4 charges worth) before you can cauterize yourself."));
    } else if (has_disease && !p->is_underwater()) {
        did_cauterize = cauterize_effect(p, it, !has_disease);
    } else if (!has_disease && !p->is_underwater()) {
        if ((p->has_trait("MASOCHIST") || p->has_trait("MASOCHIST_MED") || p->has_trait("CENOBITE")) &&
            query_yn(_("Cauterize yourself for fun?"))) {
            did_cauterize = cauterize_effect(p, it, true);
        } else {
            p->add_msg_if_player(m_info, _("You are not bleeding or bitten, there is no need to cauterize yourself."));
        }
    }
    if (did_cauterize) {
        p->use_charges("fire", 4);
    }
    return 0;
}

int iuse::knife(player *p, item *it, bool t, point)
{
    int choice = -1;
    const int menu_cut_up_item = 0;
    const int menu_carve_writing = 1;
    const int menu_cauterize = 2;
    const int menu_make_zlave = 3;
    const int menu_cancel = 4;
    item *cut;

    uimenu kmenu;
    kmenu.text = _("Using cutting instrument:");
    kmenu.addentry(menu_cut_up_item, true, -1, _("Cut up fabric/plastic/kevlar/wood/nomex/wool"));
    kmenu.addentry(menu_carve_writing, true, -1, _("Carve writing into item"));
    kmenu.addentry(menu_cauterize, true, -1, _("Cauterize"));
    if( p->skillLevel( "survival" ) > 1 && p->skillLevel( "firstaid" ) > 1 ) {
        kmenu.addentry(menu_make_zlave, true, -1, _("Make zombie slave"));
    }

    kmenu.addentry(menu_cancel, true, 'q', _("Cancel"));
    kmenu.query();
    choice = kmenu.ret;

    if (choice == menu_cauterize) {
        cauterize_flame(p, it);
        return it->type->charges_to_use();
    } else if (choice == menu_cut_up_item) {
        int inventory_index = g->inv_for_salvage(_("Cut up what?"));
        cut = &(p->i_at(inventory_index));
        if (!try_to_cut_up(p, cut)) {
            // Messages should have already been displayed.
            return 0;
        }
        return cut_up(p, it, cut, t);
    } else if (choice == menu_carve_writing) {
        return carve_writing(p, it);
    } else if( choice == menu_make_zlave ) {
        make_zlave( p );
        return 0;
    } else {
        return 0;
    }
}

int iuse::cut_log_into_planks(player *p, item *it)
{
    p->moves -= 300;
    add_msg(_("You cut the log into planks."));
    item plank("2x4", int(calendar::turn));
    item scrap("splinter", int(calendar::turn));
    int planks = (rng(1, 3) + (p->skillLevel("carpentry") * 2));
    int scraps = 12 - planks;
    if (planks >= 12) {
        planks = 12;
    }
    if (scraps >= planks) {
        add_msg(m_bad, _("You waste a lot of the wood."));
    }
    p->i_add_or_drop(plank, planks);
    p->i_add_or_drop(scrap, scraps);
    return it->type->charges_to_use();
}

int iuse::lumber(player *p, item *it, bool, point)
{
    int pos = g->inv_for_filter( _("Cut up what?"), []( const item & itm ) {
        return itm.type->id == "log";
    } );
    item *cut = &( p->i_at( pos ) );
    if (cut->type->id == "null") {
        add_msg(m_info, _("You do not have that item!"));
        return 0;
    }
    if (cut->type->id == "log") {
        p->i_rem( it );
        cut_log_into_planks(p, it);
        return it->type->charges_to_use();
    } else {
        add_msg(m_info, _("You can't cut that up!"));
        return it->type->charges_to_use();
    }
}


int iuse::oxytorch(player *p, item *it, bool, point)
{
    int dirx, diry;
    if (!(p->has_amount("goggles_welding", 1) || p->is_wearing("goggles_welding") ||
          p->is_wearing("rm13_armor_on") || p->has_bionic("bio_sunglasses"))) {
        add_msg(m_info, _("You need welding goggles to do that."));
        return 0;
    }
    if (!choose_adjacent(_("Cut up metal where?"), dirx, diry)) {
        return 0;
    }

    if (dirx == p->posx && diry == p->posy) {
        add_msg(m_info, _("Yuck.  Acetylene gas smells weird."));
        return 0;
    }


    if (g->m.furn(dirx, diry) == f_rack) {
        p->moves -= 200;
        g->m.furn_set(dirx, diry, f_null);
        g->sound(dirx, diry, 10, _("hissssssssss!"));
        g->m.spawn_item(p->posx, p->posy, "steel_chunk", rng(2, 6));
        return it->type->charges_to_use();
    }

    const ter_id ter = g->m.ter( dirx, diry );
    if( ter == t_chainfence_v || ter == t_chainfence_h || ter == t_chaingate_c ||
        ter == t_chaingate_l) {
            p->moves -= 1000;
            g->m.ter_set(dirx, diry, t_dirt);
            g->sound(dirx, diry, 10, _("hissssssssss!"));
            g->m.spawn_item(dirx, diry, "pipe", rng(1, 4));
            g->m.spawn_item(dirx, diry, "wire", rng(4, 16));
    } else if( ter == t_chainfence_posts ) {
            p->moves -= 200;
            g->m.ter_set(dirx, diry, t_dirt);
            g->sound(dirx, diry, 10, _("hissssssssss!"));
            g->m.spawn_item(dirx, diry, "pipe", rng(1, 4));
    } else if( ter == t_chaingate_l || ter == t_chaingate_c ) {
            p->moves -= 200;
            g->m.ter_set(dirx, diry, t_dirt);
            g->sound(dirx, diry, 10, _("hissssssssss!"));
            g->m.spawn_item(dirx, diry, "pipe", rng(1, 4));
    } else if( ter == t_door_metal_locked || ter == t_door_metal_c || ter == t_door_bar_c ||
               ter == t_door_bar_locked || ter == t_door_metal_pickable) {
            p->moves -= 1500;
            g->m.ter_set(dirx, diry, t_mdoor_frame);
            g->sound(dirx, diry, 10, _("hissssssssss!"));
            g->m.spawn_item(dirx, diry, "steel_plate", rng(0, 1));
            g->m.spawn_item(dirx, diry, "steel_chunk", rng(1, 2));
    } else if( ter == t_window_enhanced || ter == t_window_empty ) {
            p->moves -= 500;
            g->m.ter_set(dirx, diry, t_window_reinforced_noglass);
            g->sound(dirx, diry, 10, _("hissssssssss!"));
            g->m.spawn_item(dirx, diry, "steel_plate", rng(0, 1));
    } else if( ter == t_bars ) {
            if (g->m.ter(dirx + 1, diry) == t_sewage || g->m.ter(dirx, diry + 1) == t_sewage ||
                g->m.ter(dirx - 1, diry) == t_sewage || g->m.ter(dirx, diry - 1) == t_sewage) {
                g->m.ter_set(dirx, diry, t_sewage);
                p->moves -= 1000;
                g->sound(dirx, diry, 10, _("hissssssssss!"));
                g->m.spawn_item(p->posx, p->posy, "pipe", rng(1, 2));
            } else {
                g->m.ter_set(dirx, diry, t_floor);
                p->moves -= 1000;
                g->sound(dirx, diry, 10, _("hissssssssss!"));
                g->m.spawn_item(p->posx, p->posy, "pipe", rng(1, 2));
            }
    } else if( ter == t_window_bars_alarm ) {
            p->moves -= 1000;
            g->m.ter_set(dirx, diry, t_window_empty);
            g->sound(dirx, diry, 10, _("hissssssssss!"));
            g->m.spawn_item(p->posx, p->posy, "pipe", rng(1, 2));
    } else {
            add_msg(m_info, _("You can't cut that."));
            return 0;
    }
    return it->type->charges_to_use();
}

int iuse::hacksaw(player *p, item *it, bool, point)
{
    int dirx, diry;
    if (!choose_adjacent(_("Cut up metal where?"), dirx, diry)) {
        return 0;
    }

    if (dirx == p->posx && diry == p->posy) {
        add_msg(m_info, _("Why would you do that?"));
        add_msg(m_info, _("You're not even chained to a boiler."));
        return 0;
    }


    if (g->m.furn(dirx, diry) == f_rack) {
        p->moves -= 500;
        g->m.furn_set(dirx, diry, f_null);
        g->sound(dirx, diry, 15, _("grnd grnd grnd"));
        g->m.spawn_item(p->posx, p->posy, "pipe", rng(1, 3));
        g->m.spawn_item(p->posx, p->posy, "steel_chunk");
        return it->type->charges_to_use();
    }

    const ter_id ter = g->m.ter( dirx, diry );
    if( ter == t_chainfence_v || ter == t_chainfence_h || ter == t_chaingate_c ||
        ter == t_chaingate_l) {
            p->moves -= 500;
            g->m.ter_set(dirx, diry, t_dirt);
            g->sound(dirx, diry, 15, _("grnd grnd grnd"));
            g->m.spawn_item(dirx, diry, "pipe", 6);
            g->m.spawn_item(dirx, diry, "wire", 20);
    } else if( ter == t_chainfence_posts ) {
            p->moves -= 500;
            g->m.ter_set(dirx, diry, t_dirt);
            g->sound(dirx, diry, 15, _("grnd grnd grnd"));
            g->m.spawn_item(dirx, diry, "pipe", 6);
    } else if( ter == t_bars ) {
            if (g->m.ter(dirx + 1, diry) == t_sewage || g->m.ter(dirx, diry + 1) == t_sewage ||
                g->m.ter(dirx - 1, diry) == t_sewage || g->m.ter(dirx, diry - 1) == t_sewage) {
                g->m.ter_set(dirx, diry, t_sewage);
                p->moves -= 1000;
                g->sound(dirx, diry, 15, _("grnd grnd grnd"));
                g->m.spawn_item(p->posx, p->posy, "pipe", 3);
            } else {
                g->m.ter_set(dirx, diry, t_floor);
                p->moves -= 500;
                g->sound(dirx, diry, 15, _("grnd grnd grnd"));
                g->m.spawn_item(p->posx, p->posy, "pipe", 3);
            }
    } else {
            add_msg(m_info, _("You can't cut that."));
            return 0;
    }
    return it->type->charges_to_use();
}

int iuse::portable_structure(player *p, item *it, bool, point)
{
    int radius = it->type->id == "large_tent_kit" ? 2 : 1;
    furn_id floor =
        it->type->id == "tent_kit"       ? f_groundsheet
      : it->type->id == "large_tent_kit" ? f_large_groundsheet
      :                                    f_skin_groundsheet;
    furn_id wall =
        it->type->id == "tent_kit"       ? f_canvas_wall
      : it->type->id == "large_tent_kit" ? f_large_canvas_wall
      :                                    f_skin_wall;
    furn_id door =
        it->type->id == "tent_kit"       ? f_canvas_door
      : it->type->id == "large_tent_kit" ? f_large_canvas_door
      :                                    f_skin_door;
    furn_id center_floor =
        it->type->id == "large_tent_kit" ? f_center_groundsheet
                                         : floor;

    int diam = 2*radius + 1;

    int dirx, diry;
    if (!choose_adjacent(
            string_format(_("Put up the %s where (%dx%d clear area)?"),
                it->tname().c_str(),
                diam, diam),
            dirx, diry)) {
        return 0;
    }

    // We place the center of the structure (radius + 1)
    // spaces away from the player.
    // First check there's enough room.
    int posx = radius * (dirx - p->posx) + dirx;
        //(radius + 1)*posx + p->posx;
    int posy = radius * (diry - p->posy) + diry;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            if (!g->m.has_flag("FLAT", posx + i, posy + j) ||
                 g->m.veh_at( posx + i, posy + j ) != nullptr ||
                !g->is_empty( posx + i, posy + j ) ||
                 g->critter_at( posx + i, posy + j ) != nullptr ||
                    g->m.has_furn(posx + i, posy + j)) {
                add_msg(m_info, _("There isn't enough space in that direction."));
                return 0;
            }
        }
    }
    // Make a square of floor surrounded by wall.
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            g->m.furn_set(posx + i, posy + j, wall);
        }
    }
    for (int i = -(radius - 1); i <= (radius - 1); i++) {
        for (int j = -(radius - 1); j <= (radius - 1); j++) {
            g->m.furn_set(posx + i, posy + j, floor);
        }
    }
    // Place the center floor and the door.
    g->m.furn_set(posx, posy, center_floor);
    g->m.furn_set(posx - radius*(dirx - p->posx), posy - radius*(diry - p->posy), door);
    add_msg(m_info, _("You set up the %s on the ground."), it->tname().c_str());
    add_msg(m_info, _("Examine the center square to pack it up again."), it->tname().c_str());
    return 1;
}


int iuse::torch_lit(player *p, item *it, bool t, point pos)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(_("The torch is extinguished."));
        it->make("torch");
        it->active = false;
        return 0;
    }
    if (t) {
        if (it->charges < it->type->charges_to_use()) {
            p->add_msg_if_player(_("The torch burns out."));
            it->make("torch_done");
            it->active = false;
        }
    } else if (it->charges <= 0) {
        p->add_msg_if_player(_("The %s winks out"), it->tname().c_str());
    } else { // Turning it off
        int choice = menu(true, _("torch (lit)"), _("extinguish"),
                          _("light something"), _("cancel"), NULL);
        switch (choice) {
            case 1: {
                p->add_msg_if_player(_("The torch is extinguished"));
                it->charges -= 1;
                it->make("torch");
                it->active = false;
                return 0;
            }
            break;
            case 2: {
                if( prep_firestarter_use(p, it, pos) ) {
                    p->moves -= 5;
                    resolve_firestarter_use(p, it, pos);
                    return it->type->charges_to_use();
                }
            }
        }
    }
    return it->type->charges_to_use();
}


int iuse::battletorch_lit(player *p, item *it, bool t, point pos)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(_("The Louisville Slaughterer is extinguished."));
        it->make("bat");
        it->active = false;
        return 0;
    }
    if (t) {
        if (it->charges < it->type->charges_to_use()) {
            p->add_msg_if_player(_("The Louisville Slaughterer burns out."));
            it->make("battletorch_done");
            it->active = false;
        }
    } else if (it->charges <= 0) {
        p->add_msg_if_player(_("The %s winks out"), it->tname().c_str());
    } else { // Turning it off
        int choice = menu(true, _("Louisville Slaughterer (lit)"), _("extinguish"),
                          _("light something"), _("cancel"), NULL);
        switch (choice) {
            case 1: {
                p->add_msg_if_player(_("The Louisville Slaughterer is extinguished"));
                it->charges -= 1;
                it->make("battletorch");
                it->active = false;
                return 0;
            }
            break;
            case 2: {
                if( prep_firestarter_use(p, it, pos) ) {
                    p->moves -= 5;
                    resolve_firestarter_use(p, it, pos);
                    return it->type->charges_to_use();
                }
            }
        }
    }
    return it->type->charges_to_use();
}

iuse::bullet_pulling_t iuse::bullet_pulling_recipes;

void iuse::reset_bullet_pulling()
{
    bullet_pulling_recipes.clear();
}

void iuse::load_bullet_pulling(JsonObject &jo)
{
    const std::string type = jo.get_string("bullet");
    result_list_t &recipe = bullet_pulling_recipes[type];
    // Allow mods that are later loaded to override previously loaded recipes
    recipe.clear();
    JsonArray ja = jo.get_array("items");
    while (ja.has_more()) {
        JsonArray itm = ja.next_array();
        recipe.push_back(result_t(itm.get_string(0), itm.get_int(1)));
    }
}

int iuse::bullet_puller(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    int inventory_index = g->inv(_("Disassemble what?"));
    item *pull = &(p->i_at(inventory_index));
    if (pull->is_null()) {
        add_msg(m_info, _("You do not have that item!"));
        return 0;
    }
    bullet_pulling_t::const_iterator a = bullet_pulling_recipes.find(pull->type->id);
    if (a == bullet_pulling_recipes.end()) {
        add_msg(m_info, _("You cannot disassemble that."));
        return 0;
    }
    if (p->skillLevel("gun") < 2) {
        add_msg(m_info, _("You need to be at least level 2 in the firearms skill before you can disassemble ammunition."));
        return 0;
    }
    const long multiply = std::min<long>(20, pull->charges);
    pull->charges -= multiply;
    if (pull->charges == 0) {
        p->i_rem(inventory_index);
    }
    const result_list_t &recipe = a->second;
    for( const auto &elem : recipe ) {
        int count = elem.second * multiply;
        item new_item( elem.first, calendar::turn );
        if (new_item.count_by_charges()) {
            new_item.charges = count;
            count = 1;
        }
        p->i_add_or_drop(new_item, count);
    }
    add_msg(_("You take apart the ammunition."));
    p->moves -= 500;
    p->practice("fabrication", rng(1, multiply / 5 + 1));
    return it->type->charges_to_use();
}

int iuse::boltcutters(player *p, item *it, bool, point)
{
    int dirx, diry;
    if (!choose_adjacent(_("Cut up metal where?"), dirx, diry)) {
        return 0;
    }

    if (dirx == p->posx && diry == p->posy) {
        p->add_msg_if_player(
            _("You neatly sever all of the veins and arteries in your body. Oh wait, Never mind."));
        return 0;
    }
    if (g->m.ter(dirx, diry) == t_chaingate_l) {
        p->moves -= 100;
        g->m.ter_set(dirx, diry, t_chaingate_c);
        g->sound(dirx, diry, 5, _("Gachunk!"));
        g->m.spawn_item(p->posx, p->posy, "scrap", 3);
    } else if (g->m.ter(dirx, diry) == t_chainfence_v || g->m.ter(dirx, diry) == t_chainfence_h) {
        p->moves -= 500;
        g->m.ter_set(dirx, diry, t_chainfence_posts);
        g->sound(dirx, diry, 5, _("Snick, snick, gachunk!"));
        g->m.spawn_item(dirx, diry, "wire", 20);
    } else {
        add_msg(m_info, _("You can't cut that."));
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::mop(player *p, item *it, bool, point)
{
    int dirx, diry;
    if (!choose_adjacent(_("Mop where?"), dirx, diry)) {
        return 0;
    }

    if (dirx == p->posx && diry == p->posy) {
        p->add_msg_if_player(_("You mop yourself up."));
        p->add_msg_if_player(_("The universe implodes and reforms around you."));
        return 0;
    }
    if (g->m.moppable_items_at(dirx, diry)) {
        g->m.mop_spills(dirx, diry);
        add_msg(_("You mop up the spill."));
        p->moves -= 15;
    } else {
        p->add_msg_if_player(m_info, _("There's nothing to mop there."));
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::rag(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (p->has_effect("bleed")) {
        if (use_healing_item(p, it, 0, 0, 0, 50, 0, 0, false) != num_hp_parts) {
            p->use_charges("rag", 1);
            it->make("rag_bloody");
        }
        return 0;
    } else {
        p->add_msg_if_player(m_info, _("You're not bleeding enough to need your %s."),
                             it->tname().c_str());
        return 0;
    }
}

int iuse::LAW(player *p, item *it, bool, point)
{
    p->add_msg_if_player(_("You pull the activating lever, readying the LAW to fire."));
    it->make("LAW");
    it->charges++;
    // When converting a tool to a gun, you need to set the current ammo type, this is usually done when a gun is reloaded.
    it->curammo = dynamic_cast<it_ammo *>( item( "66mm_HEAT", 0 ).type );
    return it->type->charges_to_use();
}

/* MACGUFFIN FUNCTIONS
 * These functions should refer to it->associated_mission for the particulars
 */
int iuse::mcg_note(player *, item *, bool, point)
{
    return 0;
}

int iuse::artifact(player *p, item *it, bool, point)
{
    if (!it->is_artifact()) {
        debugmsg("iuse::artifact called on a non-artifact item! %s",
                 it->tname().c_str());
        return 0;
    } else if (!it->is_tool()) {
        debugmsg("iuse::artifact called on a non-tool artifact! %s",
                 it->tname().c_str());
        return 0;
    }
    if (!p->is_npc()) {
        //~ %s is artifact name
        p->add_memorial_log(pgettext("memorial_male", "Activated the %s."),
                            pgettext("memorial_female", "Activated the %s."),
                            it->tname().c_str());
    }
    it_artifact_tool *art = dynamic_cast<it_artifact_tool *>(it->type);
    size_t num_used = rng(1, art->effects_activated.size());
    if (num_used < art->effects_activated.size()) {
        num_used += rng(1, art->effects_activated.size() - num_used);
    }

    std::vector<art_effect_active> effects = art->effects_activated;
    for (size_t i = 0; i < num_used; i++) {
        int index = rng(0, effects.size() - 1);
        art_effect_active used = effects[index];
        effects.erase(effects.begin() + index);

        switch (used) {
            case AEA_STORM: {
                g->sound(p->posx, p->posy, 10, _("Ka-BOOM!"));
                int num_bolts = rng(2, 4);
                for (int j = 0; j < num_bolts; j++) {
                    int xdir = 0, ydir = 0;
                    while (xdir == 0 && ydir == 0) {
                        xdir = rng(-1, 1);
                        ydir = rng(-1, 1);
                    }
                    int dist = rng(4, 12);
                    int boltx = p->posx, bolty = p->posy;
                    for (int n = 0; n < dist; n++) {
                        boltx += xdir;
                        bolty += ydir;
                        g->m.add_field(boltx, bolty, fd_electricity, rng(2, 3));
                        if (one_in(4)) {
                            if (xdir == 0) {
                                xdir = rng(0, 1) * 2 - 1;
                            } else {
                                xdir = 0;
                            }
                        }
                        if (one_in(4)) {
                            if (ydir == 0) {
                                ydir = rng(0, 1) * 2 - 1;
                            } else {
                                ydir = 0;
                            }
                        }
                    }
                }
            }
            break;

            case AEA_FIREBALL: {
                point fireball = g->look_around();
                if (fireball.x != -1 && fireball.y != -1) {
                    g->explosion(fireball.x, fireball.y, 8, 0, true);
                }
            }
            break;

            case AEA_ADRENALINE:
                p->add_msg_if_player(m_good, _("You're filled with a roaring energy!"));
                p->add_effect("adrenaline", rng(200, 250));
                break;

            case AEA_MAP: {
                const tripoint center = g->om_global_location();
                const bool new_map = overmap_buffer.reveal(
                                         point(center.x, center.y), 20, center.z);
                if (new_map) {
                    p->add_msg_if_player(m_warning, _("You have a vision of the surrounding area..."));
                    p->moves -= 100;
                }
            }
            break;

            case AEA_BLOOD: {
                bool blood = false;
                for (int x = p->posx - 4; x <= p->posx + 4; x++) {
                    for (int y = p->posy - 4; y <= p->posy + 4; y++) {
                        if (!one_in(4) && g->m.add_field(x, y, fd_blood, 3) &&
                            (blood || g->u_see(x, y))) {
                            blood = true;
                        }
                    }
                }
                if (blood) {
                    p->add_msg_if_player(m_warning, _("Blood soaks out of the ground and walls."));
                }
            }
            break;

            case AEA_FATIGUE: {
                p->add_msg_if_player(m_warning, _("The fabric of space seems to decay."));
                int x = rng(p->posx - 3, p->posx + 3), y = rng(p->posy - 3, p->posy + 3);
                g->m.add_field(x, y, fd_fatigue, rng(1, 2));
            }
            break;

            case AEA_ACIDBALL: {
                point acidball = g->look_around();
                if (acidball.x != -1 && acidball.y != -1) {
                    for (int x = acidball.x - 1; x <= acidball.x + 1; x++) {
                        for (int y = acidball.y - 1; y <= acidball.y + 1; y++) {
                            g->m.add_field(x, y, fd_acid, rng(2, 3));
                        }
                    }
                }
            }
            break;

            case AEA_PULSE:
                g->sound(p->posx, p->posy, 30, _("The earth shakes!"));
                for (int x = p->posx - 2; x <= p->posx + 2; x++) {
                    for (int y = p->posy - 2; y <= p->posy + 2; y++) {
                        g->m.bash(x, y, 40);
                        g->m.bash(x, y, 40);  // Multibash effect, so that doors &c will fall
                        g->m.bash(x, y, 40);
                        if (g->m.is_bashable(x, y) && rng(1, 10) >= 3) {
                            g->m.bash(x, y, 999, false, true);
                        }
                    }
                }
                break;

            case AEA_HEAL:
                p->add_msg_if_player(m_good, _("You feel healed."));
                p->healall(2);
                break;

            case AEA_CONFUSED:
                for (int x = p->posx - 8; x <= p->posx + 8; x++) {
                    for (int y = p->posy - 8; y <= p->posy + 8; y++) {
                        int mondex = g->mon_at(x, y);
                        if (mondex != -1) {
                            g->zombie(mondex).add_effect("stunned", rng(5, 15));
                        }
                    }
                }

            case AEA_ENTRANCE:
                for (int x = p->posx - 8; x <= p->posx + 8; x++) {
                    for (int y = p->posy - 8; y <= p->posy + 8; y++) {
                        int mondex = g->mon_at(x, y);
                        if (mondex != -1 && g->zombie(mondex).friendly == 0 &&
                            rng(0, 600) > g->zombie(mondex).hp) {
                            g->zombie(mondex).make_friendly();
                        }
                    }
                }
                break;

            case AEA_BUGS: {
                int roll = rng(1, 10);
                std::string bug = "mon_null";
                int num = 0;
                std::vector<point> empty;
                for (int x = p->posx - 1; x <= p->posx + 1; x++) {
                    for (int y = p->posy - 1; y <= p->posy + 1; y++) {
                        if (g->is_empty(x, y)) {
                            empty.push_back(point(x, y));
                        }
                    }
                }
                if (empty.empty() || roll <= 4) {
                    p->add_msg_if_player(m_warning, _("Flies buzz around you."));
                } else if (roll <= 7) {
                    p->add_msg_if_player(m_warning, _("Giant flies appear!"));
                    bug = "mon_fly";
                    num = rng(2, 4);
                } else if (roll <= 9) {
                    p->add_msg_if_player(m_warning, _("Giant bees appear!"));
                    bug = "mon_bee";
                    num = rng(1, 3);
                } else {
                    p->add_msg_if_player(m_warning, _("Giant wasps appear!"));
                    bug = "mon_wasp";
                    num = rng(1, 2);
                }
                if (bug != "mon_null") {
                    monster spawned(GetMType(bug));
                    spawned.friendly = -1;
                    for (int j = 0; j < num && !empty.empty(); j++) {
                        int index_inner = rng(0, empty.size() - 1);
                        point spawnp = empty[index_inner];
                        empty.erase(empty.begin() + index_inner);
                        spawned.spawn(spawnp.x, spawnp.y);
                        g->add_zombie(spawned);
                    }
                }
            }
            break;

            case AEA_TELEPORT:
                g->teleport(p);
                break;

            case AEA_LIGHT:
                p->add_msg_if_player(_("The %s glows brightly!"), it->tname().c_str());
                g->add_event(EVENT_ARTIFACT_LIGHT, int(calendar::turn) + 30);
                break;

            case AEA_GROWTH: {
                monster tmptriffid(GetMType("mon_null"), p->posx, p->posy);
                mattack tmpattack;
                tmpattack.growplants(&tmptriffid, -1);
            }
            break;

            case AEA_HURTALL:
                for (size_t j = 0; j < g->num_zombies(); j++) {
                    g->zombie(j).apply_damage( nullptr, bp_torso, rng( 0, 5 ) );
                }
                break;

            case AEA_RADIATION:
                add_msg(m_warning, _("Horrible gases are emitted!"));
                for (int x = p->posx - 1; x <= p->posx + 1; x++) {
                    for (int y = p->posy - 1; y <= p->posy + 1; y++) {
                        g->m.add_field(x, y, fd_nuke_gas, rng(2, 3));
                    }
                }
                break;

            case AEA_PAIN:
                p->add_msg_if_player(m_bad, _("You're wracked with pain!"));
                // OK, the Lovecraftian thingamajig can bring Deadened
                // masochists & Cenobites the stimulation they've been
                // craving ;)
                p->pain += rng(5, 15);
                break;

            case AEA_MUTATE:
                if (!one_in(3)) {
                    p->mutate();
                }
                break;

            case AEA_PARALYZE:
                p->add_msg_if_player(m_bad, _("You're paralyzed!"));
                p->moves -= rng(50, 200);
                break;

            case AEA_FIRESTORM: {
                p->add_msg_if_player(m_bad, _("Fire rains down around you!"));
                std::vector<point> ps = closest_points_first(3, p->posx, p->posy);
                for (auto p_it : ps) {
                    if (!one_in(3)) {
                        g->m.add_field(p_it, fd_fire, 1 + rng(0, 1) * rng(0, 1), 30);
                    }
                }
                break;
            }

            case AEA_ATTENTION:
                p->add_msg_if_player(m_warning, _("You feel like your action has attracted attention."));
                p->add_effect("attention", 600 * rng(1, 3));
                break;

            case AEA_TELEGLOW:
                p->add_msg_if_player(m_warning, _("You feel unhinged."));
                p->add_effect("teleglow", 100 * rng(3, 12));
                break;

            case AEA_NOISE:
                p->add_msg_if_player(m_bad, _("Your %s emits a deafening boom!"), it->tname().c_str());
                g->sound(p->posx, p->posy, 100, "");
                break;

            case AEA_SCREAM:
                p->add_msg_if_player(m_warning, _("Your %s screams disturbingly."), it->tname().c_str());
                g->sound(p->posx, p->posy, 40, "");
                p->add_morale(MORALE_SCREAM, -10, 0, 300, 5);
                break;

            case AEA_DIM:
                p->add_msg_if_player(_("The sky starts to dim."));
                g->add_event(EVENT_DIM, int(calendar::turn) + 50);
                break;

            case AEA_FLASH:
                p->add_msg_if_player(_("The %s flashes brightly!"), it->tname().c_str());
                g->flashbang(p->posx, p->posy);
                break;

            case AEA_VOMIT:
                p->add_msg_if_player(m_bad, _("A wave of nausea passes through you!"));
                p->vomit();
                break;

            case AEA_SHADOWS: {
                int num_shadows = rng(4, 8);
                monster spawned(GetMType("mon_shadow"));
                int num_spawned = 0;
                for (int j = 0; j < num_shadows; j++) {
                    int tries = 0, monx, mony, junk;
                    do {
                        if (one_in(2)) {
                            monx = rng(p->posx - 5, p->posx + 5);
                            mony = (one_in(2) ? p->posy - 5 : p->posy + 5);
                        } else {
                            monx = (one_in(2) ? p->posx - 5 : p->posx + 5);
                            mony = rng(p->posy - 5, p->posy + 5);
                        }
                    } while (tries < 5 && !g->is_empty(monx, mony) &&
                             !g->m.sees(monx, mony, p->posx, p->posy, 10, junk));
                    if (tries < 5) {
                        num_spawned++;
                        spawned.reset_special_rng(0);
                        spawned.spawn(monx, mony);
                        g->add_zombie(spawned);
                    }
                }
                if (num_spawned > 1) {
                    p->add_msg_if_player(m_warning, _("Shadows form around you."));
                } else if (num_spawned == 1) {
                    p->add_msg_if_player(m_warning, _("A shadow forms nearby."));
                }
            }
            break;

            case AEA_SPLIT: // TODO
                break;

            case AEA_NULL: // BUG
            case NUM_AEAS:
            default:
                debugmsg("iuse::artifact(): wrong artifact type (%d)", used);
                break;
        }
    }
    return it->type->charges_to_use();
}

int iuse::spray_can(player *p, item *it, bool, point)
{
    bool ismarker = (it->type->id == "permanent_marker" || it->type->id == "survival_marker");
    if (ismarker) {
        int ret = menu(true, _("Write on what?"), _("The ground"), _("An item"), _("Cancel"), NULL);

        if (ret == 2) {
            // inscribe_item returns false if the action fails or is canceled somehow.
            bool canceled_inscription = !inscribe_item(p, _("Write"), _("Written"), false);
            if (canceled_inscription) {
                return 0;
            }
            return it->type->charges_to_use();
        } else if (ret != 1) { // User chose cancel or some other undefined key.
            return 0;
        }
    }

    std::string message = string_input_popup(ismarker ? _("Write what?") : _("Spray what?"),
                          0, "", "", "graffiti");

    if (message.empty()) {
        return 0;
    } else {
        g->m.set_graffiti( p->posx, p->posy, message );
            add_msg(
                ismarker ?
                _("You write a message on the ground.") :
                _("You spray a message on the ground.")
            );
            p->moves -= 2 * message.length();
    }
    return it->type->charges_to_use();
}

/**
 * Heats up a food item.
 * @return 1 if an item was heated, false if nothing was heated.
 */
static bool heat_item(player *p)
{
    int inventory_index = g->inv_for_filter( _("Heat up what?"), []( const item & itm ) {
        return itm.is_food() && itm.has_flag("EATEN_HOT");
    } );
    item *heat = &( p->i_at(inventory_index ) );
    if (heat->type->id == "null") {
        add_msg(m_info, _("You do not have that item!"));
        return false;
    }
    item *target = heat->is_food_container() ? &(heat->contents[0]) : heat;
    if ((target->is_food()) && (target->has_flag("EATEN_HOT"))) {
        p->moves -= 300;
        add_msg(_("You heat up the food."));
        target->item_tags.insert("HOT");
        target->active = true;
        target->item_counter = 600; // sets the hot food flag for 60 minutes
        return true;
    }
    add_msg(m_info, _("You can't heat that up!"));
    return false;
}

int iuse::heatpack(player *p, item *it, bool, point)
{
    if (heat_item(p)) {
        it->make("heatpack_used");
    }
    return 0;
}

int iuse::hotplate(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_info, _("The %s's batteries are dead."), it->tname().c_str());
        return 0;
    }

    int choice = 1;
    if ((p->has_effect("bite") || p->has_effect("bleed") || p->has_trait("MASOCHIST") ||
         p->has_trait("MASOCHIST_MED") || p->has_trait("CENOBITE")) && !p->is_underwater()) {
        //Might want to cauterize
        choice = menu(true, _("Using hotplate:"), _("Heat food"), _("Cauterize wound"), _("Cancel"), NULL);
    }

    if (choice == 1) {
        if (heat_item(p)) {
            return it->type->charges_to_use();
        }
    } else if (choice == 2) {
        return cauterize_elec(p, it);
    }
    return 0;
}

int iuse::quiver(player *p, item *it, bool, point)
{
    int choice = -1;
    if (!(it->contents.empty()) && it->contents[0].charges > 0) {
        choice = menu(true, _("Do what with quiver?"), _("Store more arrows"),
                      _("Empty quiver"), _("Cancel"), NULL);

        // empty quiver
        if (choice == 2) {
            item &arrows = it->contents[0];
            int arrowsRemoved = arrows.charges;
            p->add_msg_if_player(ngettext("You remove the %s from the %s.", "You remove the %s from the %s.",
                                          arrowsRemoved),
                                 arrows.type_name( arrowsRemoved ).c_str(), it->tname().c_str());
            p->inv.assign_empty_invlet(arrows, false);
            p->i_add(arrows);
            it->contents.erase(it->contents.begin());
            return it->type->charges_to_use();
        }
    }

    // if quiver is empty or storing more arrows, pull up menu asking what to store
    if (it->contents.empty() || choice == 1) {
        int inventory_index = g->inv_for_filter( _("Store which arrows?"), []( const item & itm ) {
            return itm.is_ammo() && (itm.ammo_type() == "arrow" || itm.ammo_type() == "bolt");
        } );
        item *put = &( p->i_at(inventory_index ) );
        if (put == NULL || put->is_null()) {
            p->add_msg_if_player(_("Never mind."));
            return 0;
        }

        if (!(put->is_ammo() && (put->ammo_type() == "arrow" || put->ammo_type() == "bolt"))) {
            p->add_msg_if_player(m_info, _("Those aren't arrows!"));
            return 0;
        }

        int maxArrows = it->max_charges_from_flag("QUIVER");
        if (maxArrows == 0) {
            debugmsg("Tried storing arrows in quiver without a QUIVER_n tag (iuse::quiver)");
            return 0;
        }

        int arrowsStored = 0;

        // not empty so adding more arrows
        if (!(it->contents.empty()) && it->contents[0].charges > 0) {
            if (it->contents[0].type->id != put->type->id) {
                p->add_msg_if_player(m_info, _("Those aren't the same arrows!"));
                return 0;
            }
            if (it->contents[0].charges >= maxArrows) {
                p->add_msg_if_player(m_info, _("That %s is already full!"), it->tname().c_str());
                return 0;
            }
            arrowsStored = it->contents[0].charges;
            it->contents[0].charges += put->charges;
            p->i_rem(put);

            // empty, putting in new arrows
        } else {
            it->put_in(p->i_rem(put));
        }

        // handle overflow
        if (it->contents[0].charges > maxArrows) {
            int toomany = it->contents[0].charges - maxArrows;
            it->contents[0].charges -= toomany;
            item clone = it->contents[0].clone();
            clone.charges = toomany;
            p->i_add(clone);
        }

        arrowsStored = it->contents[0].charges - arrowsStored;
        p->add_msg_if_player(ngettext("You store %d %s in your %s.", "You store %d %s in your %s.",
                                      arrowsStored),
                             arrowsStored, it->contents[0].type_name( arrowsStored ).c_str(), it->tname().c_str());
        p->moves -= 10 * arrowsStored;
    } else {
        p->add_msg_if_player(_("Never mind."));
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::holster_gun(player *p, item *it, bool, point)
{
    // if holster is empty, pull up menu asking what to holster
    if (it->contents.empty()) {
        int inventory_index = g->inv_type(_("Holster what?"), IC_GUN); // only show guns
        item *put = &(p->i_at(inventory_index));
        if (put == NULL || put->is_null()) {
            p->add_msg_if_player(_("Never mind."));
            return 0;
        }

        // make sure we're holstering a pistol
        if (!(put->is_gun())) {
//            it_gun *gun = dynamic_cast<it_gun *>(put->type);
//            if (!(gun->skill_used == Skill::skill("pistol") || put->type->id == "shotgun_sawn")) {
//                p->add_msg_if_player(m_info, _("The %s is not a pistol or sawn-off weapon!"), put->tname().c_str());
//                return 0;
//            }
//        } else {
            p->add_msg_if_player(m_info, _("That isn't a gun!"), put->tname().c_str());
            return 0;
        }

        it_gun *gun = dynamic_cast<it_gun *>(put->type);

        int maxvol = 5;
        if (it->type->id == "bootstrap") { // bootstrap can't hold as much as holster
            maxvol = 3;
        }

        // only allow guns smaller than a certain size
        if (put->volume() > maxvol) {
            p->add_msg_if_player(m_info, _("That holster is too small to hold your %s!"),
                                 put->tname().c_str());
            return 0;
        }

        int lvl = p->skillLevel(gun->skill_used);
        std::string message;
        if (lvl < 2) {
            message = _("You clumsily holster your %s.");
        } else if (lvl >= 7) {
            message = _("You deftly holster your %s.");
        } else  {
            message = _("You holster your %s.");
        }

        p->add_msg_if_player(message.c_str(), put->tname().c_str());
        p->store(it, put, gun->skill_used->ident(), 14);

        // else draw the holstered pistol and have the player wield it
    } else {
        if (!p->is_armed() || p->wield(NULL)) {
            item &gun = it->contents[0];
            it_gun *t_gun = dynamic_cast<it_gun *>(gun.type);
            int lvl = p->skillLevel(t_gun->skill_used);
            std::string message;
            if (lvl < 2) {
                message = _("You clumsily draw your %s from the %s.");
            } else if (lvl >= 7) {
                message = _("You quickly draw your %s from the %s.");
            } else {
                message = _("You draw your %s from the %s.");
            }

            p->add_msg_if_player(message.c_str(), gun.tname().c_str(), it->tname().c_str());
            p->wield_contents(it, true, t_gun->skill_used->ident(), 13);
        }
    }
    return it->type->charges_to_use();
}

int iuse::sheath_knife(player *p, item *it, bool, point)
{
    // if sheath is empty, pull up menu asking what to sheathe
    if (it->contents.empty()) {
        // only show SHEATH_KNIFE items
        int inventory_index = g->inv_for_flag("SHEATH_KNIFE", _("Sheathe what?"), false);
        item *put = &(p->i_at(inventory_index));
        if (put == NULL || put->is_null()) {
            p->add_msg_if_player(_("Never mind."));
            return 0;
        }

        if (!put->has_flag("SHEATH_KNIFE")) {
            if (put->has_flag("SHEATH_SWORD")) {
                p->add_msg_if_player(m_info, _("You need a scabbard to sheathe a sword!"),
                                     put->tname().c_str());
            } else {
                p->add_msg_if_player(m_info, _("You can't sheathe your %s!"), put->tname().c_str());
            }
            return 0;
        }

        int maxvol = 5;
        if (it->type->id == "bootstrap") { // bootstrap can't hold as much as sheath
            maxvol = 3;
        }

        // only allow knives smaller than a certain size
        if (put->volume() > maxvol) {
            p->add_msg_if_player(m_info, _("That sheath is too small to hold your %s!"), put->tname().c_str());
            return 0;
        }

        int lvl = p->skillLevel("cutting");
        std::string message;
        if (lvl < 2) {
            message = _("You clumsily shove your %s into the %s.");
        } else if (lvl >= 5) {
            message = _("You deftly insert your %s into the %s.");
        } else {
            message = _("You put your %s into the %s.");
        }

        p->add_msg_if_player(message.c_str(), put->tname().c_str(), it->tname().c_str());
        p->store(it, put, "cutting", 14);

        // else unsheathe a sheathed weapon and have the player wield it
    } else {
        if (!p->is_armed() || p->wield(NULL)) {
            p->wield_contents(it, true, "cutting", 13);

            int lvl = p->skillLevel("cutting");
            std::string message;
            if (lvl < 2) {
                message = _("You clumsily draw your %s from the %s.");
            } else if (lvl >= 5) {
                message = _("You deftly draw your %s from the %s.");
            } else {
                message = _("You draw your %s from the %s.");
            }

            p->add_msg_if_player(message.c_str(), p->weapon.tname().c_str(), it->tname().c_str());

            // diamond knives glimmer in the sunlight
            if (g->is_in_sunlight(p->posx, p->posy) && (p->weapon.made_of("diamond") ||
                    p->weapon.type->id == "foon" || p->weapon.type->id == "spork")) {
                p->add_msg_if_player(_("The %s glimmers magnificently in the sunlight."),
                                     p->weapon.tname().c_str());
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::sheath_sword(player *p, item *it, bool, point)
{
    // if sheath is empty, pull up menu asking what to sheathe
    if (it->contents.empty()) {
        // only show SHEATH_SWORD items
        int inventory_index = g->inv_for_flag("SHEATH_SWORD", _("Sheathe what?"), false);
        item *put = &(p->i_at(inventory_index));
        if (put == NULL || put->is_null()) {
            p->add_msg_if_player(_("Never mind."));
            return 0;
        }

        if (!put->has_flag("SHEATH_SWORD")) {
            if (put->has_flag("SHEATH_KNIFE")) {
                p->add_msg_if_player(m_info, _("You need a knife sheath for that!"), put->tname().c_str());
            } else {
                p->add_msg_if_player(m_info, _("You can't sheathe your %s!"), put->tname().c_str());
            }
            return 0;
        }

        int lvl = p->skillLevel("cutting");
        std::string message;
        if (lvl < 2) {
            message = _("You clumsily sheathe your %s.");
        } else if (lvl >= 7) {
            message = _("You deftly sheathe your %s.");
        } else {
            message = _("You sheathe your %s.");
        }

        p->add_msg_if_player(message.c_str(), put->tname().c_str());
        p->store(it, put, "cutting", 14);

        // else unsheathe a sheathed weapon and have the player wield it
    } else {
        if (!p->is_armed() || p->wield(NULL)) {
            int lvl = p->skillLevel("cutting");
            p->wield_contents(it, true, "cutting", 13);

            // in order to perform iaijutsu, have to pass a roll based on level
            bool iaijutsu =
                lvl >= 7 &&
                p->weapon.has_flag("IAIJUTSU") &&
                one_in(12 - lvl);

            // iaijutsu! slash an enemy as you draw your sword
            if (iaijutsu) {
                // check for adjacent enemies before asking to slash
                int mon_num = -1;
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        mon_num = g->mon_at(p->posx + i, p->posy + j);
                        if (mon_num != -1) {
                            break; // break at first found enemy
                        }
                    }
                    if (mon_num != -1) {
                        break;
                    }
                }

                // if there's an adjacent enemy, ask which one to slash
                // if a spot without an enemy is chosen, defaults to the first enemy found above
                if (mon_num != -1) {
                    int slashx, slashy;
                    if (choose_adjacent(_("Slash where?"), slashx, slashy)) {
                        const int mon_hit = g->mon_at(slashx, slashy);
                        if (mon_hit != -1) {
                            mon_num = mon_hit;
                        }
                    }
                    monster &zed = g->zombie(mon_num);
                    p->add_msg_if_player(m_good, _("You slash at the %s as you draw your %s."),
                                         zed.name().c_str(), p->weapon.tname().c_str());
                    p->melee_attack(zed, true);
                } else {
                    // no adjacent monsters, draw sword normally
                    iaijutsu = false;
                }
            }

            // draw sword normally
            if (!iaijutsu) {
                std::string message;
                if (lvl < 2) {
                    message = _("You clumsily draw your %s.");
                } else if (lvl >= 7) {
                    message = _("You masterfully draw your %s.");
                } else {
                    message = _("You draw your %s.");
                }

                p->add_msg_if_player(message.c_str(), p->weapon.tname().c_str());
            }

            // diamond swords glimmer in the sunlight
            if (g->is_in_sunlight(p->posx, p->posy) && p->weapon.made_of("diamond")) {
                p->add_msg_if_player(_("The %s glimmers magnificently in the sunlight."),
                                     p->weapon.tname().c_str());
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::holster_ankle(player *p, item *it, bool b, point pos)
{
    int choice = -1;
    // ask whether to store a knife or a pistol
    if (it->contents.empty()) {
        choice = menu(true, _("Using ankle holster:"), _("Holster a pistol"),
                      _("Sheathe a knife"), _("Cancel"), NULL);
        if (choice == 1) {
            holster_gun(p, it, b, pos);
        } else if (choice == 2) {
            sheath_knife(p, it, b, pos);
        }
        // unsheathe knife or draw pistol
    } else {
        if (!p->is_armed() || p->wield(NULL)) {
            item &stored = it->contents[0];
            if (stored.has_flag("SHEATH_KNIFE")) {
                sheath_knife(p, it, b, pos);
            } else {
                holster_gun(p, it, b, pos);
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::survivor_belt(player *p, item *it, bool b, point pos)
{
    int choice = -1;

    choice = menu( true,
                   _( "Using survivor belt:" ),
                   it->contents.empty() ? _( "Sheathe a knife" ) : _( "Unsheathe a knife" ),
                   _( "Use hammer" ),
                   _( "Use hacksaw" ),
                   _( "Use wood saw" ),
                   _( "Cancel" ),
                   NULL );

    switch ( choice ) {
        case 1:
            return sheath_knife( p, it, b, pos );
        case 2:
            return hammer( p, it, b, pos );
        case 3:
            return hacksaw( p, it, b, pos );
        case 4:
            return lumber( p, it, b, pos );
    }
    return 0;
}

int iuse::boots(player *p, item *it, bool, point)
{
    int choice = -1;
    if (it->contents.empty()) {
        choice = menu(true, _("Using boots:"), _("Put a knife in the boot"), _("Cancel"), NULL);
    } else if (it->contents.size() == 1) {
        choice = menu(true, _("Take what:"), it->contents[0].tname().c_str(), _("Put a knife in the boot"),
                      _("Cancel"), NULL);
    } else {
        choice = menu(true, _("Take what:"), it->contents[0].tname().c_str(),
                      it->contents[1].tname().c_str(), _("Cancel"), NULL);
    }

    if ((it->contents.size() > 0 && choice == 1) || // Pull 1st
        (it->contents.size() > 1 && choice == 2)) {  // Pull 2nd
        p->moves -= 15;
        item &knife = it->contents[choice - 1];
        if (!p->is_armed() || p->wield(NULL)) {
            p->inv.assign_empty_invlet(knife, true);  // force getting an invlet.
            p->wield(&(p->i_add(knife)));
            it->contents.erase(it->contents.begin() + choice - 1);
        }
    } else if ((it->contents.empty() && choice == 1) || // Put 1st
               (it->contents.size() == 1 && choice == 2)) { // Put 2st
        int inventory_index = g->inv_for_flag("SHEATH_KNIFE", _("Put what?"), false);
        item *put = &(p->i_at(inventory_index));
        if (put == NULL || put->is_null()) {
            p->add_msg_if_player(m_info, _("You do not have that item!"));
            return 0;
        }
        if (!put->type->can_use("KNIFE")) {
            p->add_msg_if_player(m_info, _("That isn't a knife!"));
            return 0;
        }
        if (put->volume() > 5) {
            p->add_msg_if_player(m_info, _("That item does not fit in your boot!"));
            return 0;
        }
        p->moves -= 30;
        p->add_msg_if_player(_("You put the %s in your boot."), put->tname().c_str());
        it->put_in(p->i_rem(inventory_index));
    }
    return it->type->charges_to_use();
}

int iuse::towel(player *p, item *it, bool t, point)
{
    if( t ) {
        // Continuous usage, do nothing as not initiated by the player, this is for
        // wet towels only as they are active items.
        return 0;
    }
    bool towelUsed = false;

    // can't use an already wet towel!
    if (it->has_flag("WET")) {
        p->add_msg_if_player(m_info, _("That %s is too wet to soak up any more liquid!"),
                             it->tname().c_str());
    }

    // dry off from being wet
    else if (abs(p->has_morale(MORALE_WET))) {
        p->rem_morale(MORALE_WET);
        for (int i = 0; i < num_bp; ++i) {
            p->body_wetness[i] = 0;
        }
        p->add_msg_if_player(_("You use the %s to dry off, saturating it with water!"),
                             it->tname().c_str());

        towelUsed = true;
        it->item_counter = 300;
    }

    // clean off slime
    else if (p->has_effect("slimed")) {
        p->remove_effect("slimed");
        p->add_msg_if_player(_("You use the %s to clean yourself off, saturating it with slime!"),
                             it->tname().c_str());

        towelUsed = true;
        it->item_counter = 450; // slime takes a bit longer to dry
    }

    // default message
    else {
        p->add_msg_if_player(_("You are already dry, the %s does nothing."), it->tname().c_str());
    }

    // towel was used
    if (towelUsed) {
        p->moves -= 50;
        // change "towel" to a "towel_wet" (different flavor text/color)
        if (it->type->id == "towel") {
            it->make("towel_wet");
        }

        // WET, active items have their timer decremented every turn
        it->item_tags.erase("ABSORBENT");
        it->item_tags.insert("WET");
        it->active = true;
    }
    return it->type->charges_to_use();
}

int iuse::unfold_generic(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    vehicle *veh = g->m.add_vehicle("none", p->posx, p->posy, 0, 0, 0, false);
    if (veh == NULL) {
        p->add_msg_if_player(m_info, _("There's no room to unfold the %s."), it->tname().c_str());
        return 0;
    }
    veh->name = it->item_vars["vehicle_name"];
    if (!veh->restore(it->item_vars["folding_bicycle_parts"])) {
        g->m.destroy_vehicle(veh);
        return 0;
    }
    g->m.update_vehicle_cache(veh, true);

    std::string unfold_msg = it->item_vars["unfold_msg"];
    if (unfold_msg.size() == 0) {
        unfold_msg = _("You painstakingly unfold the %s and make it ready to ride.");
    } else {
        unfold_msg = _(unfold_msg.c_str());
    }
    p->add_msg_if_player(unfold_msg.c_str(), veh->name.c_str());

    std::string smoves = it->item_vars["moves"];
    int moves = 500;
    if (smoves.size() > 0) {
        std::istringstream( smoves ) >> moves;
    }
    p->moves -= moves;
    return 1;
}

int iuse::adrenaline_injector(player *p, item *it, bool, point)
{
    p->moves -= 100;
    p->add_msg_if_player(_("You inject yourself with adrenaline."));

    item syringe( "syringe", it->bday );
    p->i_add( syringe );
    p->add_effect("adrenaline", 200);
    if (p->has_effect("adrenaline")) {
        //Massively boost stimulant level, risking death on an extended chain
        p->stim += 80;
    }

    if (p->has_effect("asthma")) {
        p->remove_effect("asthma");
        p->add_msg_if_player(m_good, _("The adrenaline causes your asthma to clear."));
    }
    return it->type->charges_to_use();
}

int iuse::jet_injector(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_info, _("The jet injector is empty."), it->tname().c_str());
        return 0;
    } else {
        p->add_msg_if_player(_("You inject yourself with the jet injector."));
        // Intensity is 2 here because intensity = 1 is the comedown
        p->add_effect("jetinjector", 200, num_bp, false, 2);
        p->pkill += 20;
        p->stim += 10;
        p->remove_effect("infected");
        p->remove_effect("bite");
        p->remove_effect("bleed");
        p->remove_effect("fungus");
        p->remove_effect("dermatik");
        p->radiation += 4;
        p->healall(20);
    }

    if (p->has_effect("jetinjector")) {
        if (p->get_effect_dur("jetinjector") > 200) {
            p->add_msg_if_player(m_warning, _("Your heart is beating alarmingly fast!"));
        }
    }
    return it->type->charges_to_use();
}

int iuse::radglove(player *p, item *it, bool, point)
{
    if (p->get_item_position(it) >= -1) {
        p->add_msg_if_player(m_info,
                             _("You must wear the radiation biomonitor before you can activate it."));
        return 0;
    } else if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_info, _("The radiation biomonitor needs batteries to function."));
        return 0;
    } else {
        p->add_msg_if_player(_("You activate your radiation biomonitor."));
        if (p->radiation >= 1) {
            p->add_msg_if_player(m_warning, _("You are currently irradiated."));
            add_msg(m_info, _("Your radiation level: %d"), p->radiation);
        } else {
            p->add_msg_if_player(m_info, _("You are not currently irradiated."));
        }
        p->add_msg_if_player(_("Have a nice day!"));
    }
    return it->type->charges_to_use();
}


int iuse::contacts(player *p, item *it, bool, point)
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    int duration = rng(80640, 120960); // Around 7 days.
    if (p->has_effect("contacts")) {
        if (query_yn(_("Replace your current lenses?"))) {
            p->moves -= 200;
            p->add_msg_if_player(_("You replace your current %s."), it->tname().c_str());
            p->remove_effect("contacts");
            p->add_effect("contacts", duration);
            return it->type->charges_to_use();
        } else {
            p->add_msg_if_player(_("You don't do anything with your %s."), it->tname().c_str());
            return 0;
        }
    } else if (p->has_trait("HYPEROPIC") || p->has_trait("MYOPIC") || p->has_trait("URSINE_EYE")) {
        p->moves -= 200;
        p->add_msg_if_player(_("You put the %s in your eyes."), it->tname().c_str());
        p->add_effect("contacts", duration);
        return it->type->charges_to_use();
    } else {
        p->add_msg_if_player(m_info, _("Your vision is fine already."));
        return 0;
    }
}

int iuse::talking_doll(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_info, _("The %s's batteries are dead."), it->tname().c_str());
        return 0;
    }

    std::string label;

    if (it->type->id == "talking_doll") {
        label = "doll";
    } else {
        label = "creepy_doll";
    }

    const SpeechBubble speech = get_speech(label);

    g->ambient_sound(p->posx, p->posy, speech.volume, speech.text);

    return it->type->charges_to_use();
}

int iuse::gun_repair(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (p->skillLevel("mechanics") < 2) {
        p->add_msg_if_player(m_info, _("You need a mechanics skill of 2 to use this repair kit."));
        return 0;
    }
    int inventory_index = g->inv(_("Select the firearm to repair."));
    item *fix = &(p->i_at(inventory_index));
    if (fix == NULL || fix->is_null()) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    if (!fix->is_gun()) {
        p->add_msg_if_player(m_info, _("That isn't a firearm!"));
        return 0;
    }
    if (fix->damage == -1) {
        p->add_msg_if_player(m_info, _("You cannot improve your %s any more this way."),
                             fix->tname().c_str());
        return 0;
    }
    if ((fix->damage == 0) && p->skillLevel("mechanics") < 8) {
        p->add_msg_if_player(m_info, _("Your %s is already in peak condition."), fix->tname().c_str());
        p->add_msg_if_player(m_info, _("With a higher mechanics skill, you might be able to improve it."));
        return 0;
    }
    if ((fix->damage == 0) && p->skillLevel("mechanics") >= 8) {
        p->add_msg_if_player(m_good, _("You accurize your %s."), fix->tname().c_str());
        g->sound(p->posx, p->posy, 6, "");
        p->moves -= 2000 * p->fine_detail_vision_mod();
        p->practice("mechanics", 10);
        fix->damage--;
    } else if (fix->damage >= 2) {
        p->add_msg_if_player(m_good, _("You repair your %s!"), fix->tname().c_str());
        g->sound(p->posx, p->posy, 8, "");
        p->moves -= 1000 * p->fine_detail_vision_mod();
        p->practice("mechanics", 10);
        fix->damage--;
    } else {
        p->add_msg_if_player(m_good, _("You repair your %s completely!"),
                             fix->tname().c_str());
        g->sound(p->posx, p->posy, 8, "");
        p->moves -= 500 * p->fine_detail_vision_mod();
        p->practice("mechanics", 10);
        fix->damage = 0;
    }
    return it->type->charges_to_use();
}

int iuse::misc_repair(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (p->skillLevel("fabrication") < 1) {
        p->add_msg_if_player(m_info, _("You need a fabrication skill of 1 to use this repair kit."));
        return 0;
    }
    int inventory_index = g->inv_for_filter( _("Select the item to repair."), []( const item & itm ) {
        return !itm.is_gun() && (itm.made_of("wood") || itm.made_of("plastic") || 
                                 itm.made_of("bone") || itm.made_of("chitin") ) ;
    } );
    item *fix = &( p->i_at(inventory_index ) );
    if (fix == NULL || fix->is_null()) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    if (fix->is_gun()) {
        p->add_msg_if_player(m_info, _("That requires gunsmithing tools."));
        return 0;
    }
    if (!(fix->made_of("wood") || fix->made_of("plastic") || fix->made_of("bone") ||
          fix->made_of("chitin"))) {
        p->add_msg_if_player(m_info, _("That isn't made of wood, bone, or chitin!"));
        return 0;
    }
    if (fix->damage == -1) {
        p->add_msg_if_player(m_info, _("You cannot improve your %s any more this way."),
                             fix->tname().c_str());
        return 0;
    }
    if (fix->damage == 0) {
        p->add_msg_if_player(m_good, _("You reinforce your %s."), fix->tname().c_str());
        p->moves -= 1000 * p->fine_detail_vision_mod();
        p->practice("fabrication", 10);
        fix->damage--;
    } else if (fix->damage >= 2) {
        p->add_msg_if_player(m_good, _("You repair your %s!"), fix->tname().c_str());
        p->moves -= 500 * p->fine_detail_vision_mod();
        p->practice("fabrication", 10);
        fix->damage--;
    } else {
        p->add_msg_if_player(m_good, _("You repair your %s completely!"), fix->tname().c_str());
        p->moves -= 250 * p->fine_detail_vision_mod();
        p->practice("fabrication", 10);
        fix->damage = 0;
    }
    return it->type->charges_to_use();
}

int iuse::bell(player *p, item *it, bool, point)
{
    if (it->type->id == "cow_bell") {
        g->sound(p->posx, p->posy, 12, _("Clank! Clank!"));
        if (!p->is_deaf()) {
            const int cow_factor = 1 + (p->mutation_category_level.find("MUTCAT_CATTLE") ==
                                        p->mutation_category_level.end() ?
                                        0 :
                                        (p->mutation_category_level.find("MUTCAT_CATTLE")->second) / 8
                                       );
            if (x_in_y(cow_factor, 1 + cow_factor)) {
                p->add_morale(MORALE_MUSIC, 1, 15 * (cow_factor > 10 ? 10 : cow_factor));
            }
        }
    } else {
        g->sound(p->posx, p->posy, 4, _("Ring! Ring!"));
    }
    return it->type->charges_to_use();
}

int iuse::seed(player *, item *it, bool, point)
{
    if (query_yn(_("Sure you want to eat the %s? You could plant it in a mound of dirt."),
                 it->tname().c_str())) {
        return it->type->charges_to_use(); //This eats the seed object.
    }
    return 0;
}

int iuse::robotcontrol(player *p, item *it, bool, point)
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(_("The %s's batteries are dead."), it->tname().c_str());
        return 0;

    }
    if (p->has_trait("ILLITERATE")) {
        p->add_msg_if_player(_("You cannot read a computer screen."));
        return 0;
    }

    int choice = menu(true, _("Welcome to hackPRO!:"), _("Override IFF protocols"),
                      _("Set friendly robots to passive mode"),
                      _("Set friendly robots to combat mode"), _("Cancel"), NULL);
    switch( choice ) {
    case 1: { // attempt to make a robot friendly
        uimenu pick_robot;
        pick_robot.text = _("Choose an endpoint to hack.");
        // Build a list of all unfriendly robots in range.
        for( size_t i = 0; i < g->num_zombies(); ++i ) {
            monster &candidate = g->zombie( i );
            if( candidate.type->in_species( "ROBOT" ) && candidate.friendly == 0 &&
                rl_dist( p->xpos(), p->ypos(), candidate.xpos(), candidate.ypos() <= 10 ) ) {
                pick_robot.addentry( i, true, -1, candidate.name() );
            }
        }
        if( pick_robot.entries.empty() ) {
            p->add_msg_if_player( m_info, _("No enemy robots in range.") );
            return it->type->charges_to_use();
        }
        pick_robot.addentry( INT_MAX, true, -1, _( "Cancel" ) );
        pick_robot.query();
        const size_t mondex = pick_robot.ret;
        if( mondex >= g->num_zombies() ) {
            p->add_msg_if_player(m_info, _("Never mind"));
            return it->type->charges_to_use();
        }
            monster *z = &(g->zombie(mondex));
            p->add_msg_if_player(_("You start reprogramming the %s into an ally."), z->name().c_str());
            p->moves -= 1000 - p->int_cur * 10 - p->skillLevel("computer") * 10;
            float success = p->skillLevel("computer") - 1.5 * (z->type->difficulty) /
                            ((rng(2, p->int_cur) / 2) + (p->skillLevel("computer") / 2));
            if (success >= 0) {
                p->add_msg_if_player(_("You successfully override the %s's IFF protocols!"),
                                     z->name().c_str());
                z->friendly = -1;
            } else if (success >= -2) { //A near success
                p->add_msg_if_player(_("The %s short circuits as you attempt to reprogram it!"),
                                     z->name().c_str());
                z->apply_damage( p, bp_torso, rng( 1, 10 ) ); //damage it a little
                if( z->is_dead() ) {
                    p->practice("computer", 10);
                    return it->type->charges_to_use(); // Do not do the other effects if the robot died
                }
                if (one_in(3)) {
                    p->add_msg_if_player(_("...and turns friendly!"));
                    if (one_in(3)) { //did the robot became friendly permanently?
                        z->friendly = -1; //it did
                    } else {
                        z->friendly = rng(5, 40); // it didn't
                    }
                }
            } else {
                p->add_msg_if_player(_("...but the robot refuses to acknowledge you as an ally!"));
            }
            p->practice("computer", 10);
            return it->type->charges_to_use();
        }
        case 2: { //make all friendly robots stop their purposeless extermination of (un)life.
            p->moves -= 100;
            int f = 0; //flag to check if you have robotic allies
            for (size_t i = 0; i < g->num_zombies(); i++) {
                if (g->zombie(i).friendly != 0 && g->zombie(i).type->in_species("ROBOT")) {
                    p->add_msg_if_player(_("A following %s goes into passive mode."),
                                         g->zombie(i).name().c_str());
                    g->zombie(i).add_effect("docile", 1, num_bp, true);
                    f = 1;
                }
            }
            if (f == 0) {
                p->add_msg_if_player(_("You are not commanding any robots."));
                return 0;
            }
            return it->type->charges_to_use();
            break;
        }
        case 3: { //make all friendly robots terminate (un)life with extreme prejudice
            p->moves -= 100;
            int f = 0; //flag to check if you have robotic allies
            for (size_t i = 0; i < g->num_zombies(); i++) {
                if (g->zombie(i).friendly != 0 && g->zombie(i).has_flag(MF_ELECTRONIC)) {
                    p->add_msg_if_player(_("A following %s goes into combat mode."),
                                         g->zombie(i).name().c_str());
                    g->zombie(i).remove_effect("docile");
                    f = 1;
                }
            }
            if (f == 0) {
                p->add_msg_if_player(_("You are not commanding any robots."));
                return 0;
            }
            return it->type->charges_to_use();
            break;
        }

    }
    return 0;
}

void init_memory_card_with_random_stuff(player *, item *it)
{

    if (it->has_flag("MC_MOBILE") && (it->has_flag("MC_RANDOM_STUFF") ||
                                      it->has_flag("MC_SCIENCE_STUFF")) && !(it->has_flag("MC_USED") ||
                                              it->has_flag("MC_HAS_DATA"))) {

        it->item_tags.insert("MC_HAS_DATA");

        bool encrypted = false;

        if (it->has_flag("MC_MAY_BE_ENCRYPTED") && one_in(8)) {
            it->make(it->type->id + "_encrypted");
        }

        //some special cards can contain "MC_ENCRYPTED" flag
        if (it->has_flag("MC_ENCRYPTED")) {
            encrypted = true;
        }

        int data_chance = 2;

        //encrypted memory cards often contain data
        if (encrypted && !one_in(3)) {
            data_chance--;
        }

        //just empty memory card
        if (!one_in(data_chance)) {
            return;
        }

        //add someone's personal photos
        if (one_in(data_chance)) {

            //decrease chance to more data
            data_chance++;

            if (encrypted && one_in(3)) {
                data_chance--;
            }

            const int duckfaces_count = rng(5, 30);
            it->item_vars["MC_PHOTOS"] = string_format("%d", duckfaces_count);
        }
        //decrease chance to music and other useful data
        data_chance++;
        if (encrypted && one_in(2)) {
            data_chance--;
        }

        if (one_in(data_chance)) {
            data_chance++;

            if (encrypted && one_in(3)) {
                data_chance--;
            }

            const int new_songs_count = rng(5, 15);
            it->item_vars["MC_MUSIC"] = string_format("%d", new_songs_count);
        }
        data_chance++;
        if (encrypted && one_in(2)) {
            data_chance--;
        }

        if (one_in(data_chance)) {
            it->item_vars["MC_RECIPE"] = "SIMPLE";
        }

        if (it->has_flag("MC_SCIENCE_STUFF")) {
            it->item_vars["MC_RECIPE"] = "SCIENCE";
        }
    }
}

bool einkpc_download_memory_card(player *p, item *eink, item *mc)
{
    bool something_downloaded = false;
    if (mc->item_vars["MC_PHOTOS"] != "") {
        something_downloaded = true;

        int new_photos = atoi(mc->item_vars["MC_PHOTOS"].c_str());
        mc->item_vars["MC_PHOTOS"] = "";

        p->add_msg_if_player(m_good, string_format(
                                 ngettext("You download %d new photo into internal memory.",
                                          "You download %d new photos into internal memory.", new_photos)).c_str());

        int old_photos = 0;
        if (eink->item_vars["EIPC_PHOTOS"] != "") {
            old_photos = atoi(eink->item_vars["EIPC_PHOTOS"].c_str());
        }

        eink->item_vars["EIPC_PHOTOS"] = string_format("%d", old_photos + new_photos);
    }

    if (mc->item_vars["MC_MUSIC"] != "") {
        something_downloaded = true;

        int new_songs = atoi(mc->item_vars["MC_MUSIC"].c_str());
        mc->item_vars["MC_MUSIC"] = "";

        p->add_msg_if_player(m_good, string_format(
                                 ngettext("You download %d new song into internal memory.",
                                          "You download %d new songs into internal memory.", new_songs)).c_str());

        int old_songs = 0;
        if (eink->item_vars["EIPC_MUSIC"] != "") {
            old_songs = atoi(eink->item_vars["EIPC_MUSIC"].c_str());
        }

        eink->item_vars["EIPC_MUSIC"] = string_format("%d", old_songs + new_songs);
    }

    if (mc->item_vars["MC_RECIPE"] != "") {
        const bool science = mc->item_vars["MC_RECIPE"] == "SCIENCE";

        mc->item_vars["MC_RECIPE"] = "";

        std::vector<const recipe *> candidates;

        for( auto &recipe : recipes ) {
            for( auto &elem : recipe.second ) {

                const int dif = ( elem )->difficulty;

                if (science) {
                    if( ( elem )->cat != "CC_NONCRAFT" ) {
                        if (dif >= 3 && one_in(dif + 1)) {
                            candidates.push_back( elem );
                        }
                    }
                } else {
                    if( ( elem )->cat == "CC_FOOD" ) {
                        if (dif <= 3 && one_in(dif)) {
                            candidates.push_back( elem );
                        }
                    }

                }


            }
        }

        if (candidates.size() > 0) {

            const recipe *r = candidates[rng(0, candidates.size() - 1)];
            const std::string rident = r->ident;

            const item dummy(r->result, 0);

            if (eink->item_vars["EIPC_RECIPES"] == "") {
                something_downloaded = true;
                eink->item_vars["EIPC_RECIPES"] = "," + rident + ",";

                p->add_msg_if_player(m_good, _("You download a recipe for %s into the tablet's memory."),
                                     dummy.type_name().c_str());
            } else {
                if (eink->item_vars["EIPC_RECIPES"].find("," + rident + ",") == std::string::npos) {
                    something_downloaded = true;
                    eink->item_vars["EIPC_RECIPES"] += rident + ",";

                    p->add_msg_if_player(m_good, _("You download a recipe for %s into the tablet's memory."),
                                         dummy.type_name().c_str());
                } else {
                    p->add_msg_if_player(m_good, _("Your tablet already has a recipe for %s."),
                                         dummy.type_name().c_str());
                }
            }
        }
    }

    if (mc->item_vars["MC_MONSTER_PHOTOS"] != "") {
        something_downloaded = true;
        p->add_msg_if_player(m_good, _("You have updated your monster collection."));

        if (eink->item_vars["EINK_MONSTER_PHOTOS"] == "") {
            eink->item_vars["EINK_MONSTER_PHOTOS"] = mc->item_vars["MC_MONSTER_PHOTOS"];
        } else {
            std::istringstream f(mc->item_vars["MC_MONSTER_PHOTOS"]);
            std::string s;
            while (getline(f, s, ',')) {

                if (s.size() == 0) {
                    continue;
                }

                const std::string mtype = s;
                getline(f, s, ',');
                char *chq = &s[0];
                const int quality = atoi(chq);

                const size_t eink_strpos = eink->item_vars["EINK_MONSTER_PHOTOS"].find("," + mtype + ",");

                if (eink_strpos == std::string::npos) {
                    eink->item_vars["EINK_MONSTER_PHOTOS"] += mtype + "," + string_format("%d", quality) + ",";
                } else {

                    const size_t strqpos = eink_strpos + mtype.size() + 2;
                    char *chq = &eink->item_vars["EINK_MONSTER_PHOTOS"][strqpos];
                    const int old_quality = atoi(chq);

                    if (quality > old_quality) {
                        chq = &string_format("%d", quality)[0];
                        eink->item_vars["EINK_MONSTER_PHOTOS"][strqpos] = *chq;
                    }
                }

            }
        }
    }

    if (mc->has_flag("MC_TURN_USED")) {
        mc->item_tags.clear();
        mc->item_vars.clear();
        mc->make("mobile_memory_card_used");
    }

    if (!something_downloaded) {
        p->add_msg_if_player(m_info, _("This memory card does not contain any new data."));
        return false;
    }

    return true;

}

const std::string photo_quality_names[] = { _("awful"), _("bad"), _("not bad"), _("good"), _("fine"), _("exceptional") };

int iuse::einktabletpc(player *p, item *it, bool t, point pos)
{
    if (t) {
        if (it->item_vars["EIPC_MUSIC_ON"] != "") {

            if (calendar::turn % 50 == 0) {
                it->charges--;
            }

            //the more varied music, the better max mood.
            const int songs = atoi(it->item_vars["EIPC_MUSIC"].c_str());

            //if user can hear this music and not already hear music
            if (g->sound(pos.x, pos.y, 8, "") && !p->has_effect("music")) {

                p->add_effect("music", 1);
                p->add_morale(MORALE_MUSIC, 1, std::min(100, songs), 5, 2);

                if (int(calendar::turn) % 50 == 0) { // Every 5 minutes, describe the music
                    const std::string sound = get_random_music_description(p);
                    g->sound(pos.x, pos.y, 8, sound);
                }
            }
        }

        return 0;

    } else {

        enum {
            ei_cancel, ei_photo, ei_music, ei_recipe, ei_monsters, ei_download, ei_decrypt
        };

        if (p->is_underwater()) {
            p->add_msg_if_player(m_info, _("You can't do that while underwater."));
            return 0;
        }
        if (p->has_trait("ILLITERATE")) {
            add_msg(m_info, _("You cannot read a computer screen."));
            return 0;
        }
        if (p->has_trait("HYPEROPIC") && !p->is_wearing("glasses_reading")
            && !p->is_wearing("glasses_bifocal") && !p->has_effect("contacts")) {
            add_msg(m_info, _("You'll need to put on reading glasses before you can see the screen."));
            return 0;
        }

        uimenu amenu;

        amenu.selected = 0;
        amenu.text = _("Choose menu option:");
        amenu.addentry(ei_cancel, true, 'q', _("Cancel"));

        if (it->item_vars["EIPC_PHOTOS"] != "") {
            const int photos = atoi(it->item_vars["EIPC_PHOTOS"].c_str());
            amenu.addentry(ei_photo, true, 'p', _("Photos [%d]"), photos);
        } else {
            amenu.addentry(ei_photo, false, 'p', _("No photos on device"));
        }

        if (it->item_vars["EIPC_MUSIC"] != "") {
            if (it->active) {
                amenu.addentry(ei_music, true, 'm', _("Turn music off"));
            } else {
                const int songs = atoi(it->item_vars["EIPC_MUSIC"].c_str());
                amenu.addentry(ei_music, true, 'm', _("Turn music on [%d]"), songs);
            }
        } else {
            amenu.addentry(ei_music, false, 'm', _("No music on device"));
        }

        if (it->item_vars["RECIPE"] != "") {
            const item dummy(it->item_vars["RECIPE"], 0);
            amenu.addentry(0, false, -1, _("Recipe: %s"), dummy.tname().c_str());
        }

        if (it->item_vars["EIPC_RECIPES"] != "") {
            amenu.addentry(ei_recipe, true, 'r', _("View recipe on E-ink screen"));
        }

        if (it->item_vars["EINK_MONSTER_PHOTOS"] != "") {
            amenu.addentry(ei_monsters, true, 'y', _("Your collection of monsters"));
        } else {
            amenu.addentry(ei_monsters, false, 'y', _("Collection of monsters is empty"));
        }

        amenu.addentry(ei_download, true, 'w', _("Download data from memory card"));

        if (p->skillLevel("computer") > 2) {
            amenu.addentry(ei_decrypt, true, 'd', _("Decrypt memory card"));
        } else {
            amenu.addentry(ei_decrypt, false, 'd', _("Decrypt memory card (low skill)"));
        }

        amenu.query();

        const int choice = amenu.ret;

        if (ei_cancel == choice) {
            return 0;
        }

        if (ei_photo == choice) {

            const int photos = atoi(it->item_vars["EIPC_PHOTOS"].c_str());
            const int viewed = std::min(photos, int(rng(10, 30)));
            const int count = photos - viewed;
            if (count == 0) {
                it->item_vars["EIPC_PHOTOS"] = "";
            } else {
                it->item_vars["EIPC_PHOTOS"] = string_format("%d", count);
            }

            p->moves -= rng(3, 7) * 100;

            if (p->has_trait("PSYCHOPATH")) {
                p->add_msg_if_player(m_info, _("Wasted time, these pictures do not provoke your senses."));
            } else {
                p->add_morale(MORALE_PHOTOS, rng(15, 30), 100);

                const int random_photo = rng(1, 20);
                switch (random_photo) {
                    case 1:
                        p->add_msg_if_player(m_good, _("You used to have a dog like this..."));
                        break;
                    case 2:
                        p->add_msg_if_player(m_good, _("Ha-ha! An amusing cat photo."));
                        break;
                    case 3:
                        p->add_msg_if_player(m_good, _("Excellent pictures of nature."));
                        break;
                    case 4:
                        p->add_msg_if_player(m_good, _("Food photos...your stomach rumbles!"));
                        break;
                    case 5:
                        p->add_msg_if_player(m_good, _("Some very interesting travel photos."));
                        break;
                    case 6:
                        p->add_msg_if_player(m_good, _("Pictures of a concert of popular band."));
                        break;
                    case 7:
                        p->add_msg_if_player(m_good, _("Photos of someone's luxurious house."));
                        break;
                    default:
                        p->add_msg_if_player(m_good, _("You feel nostalgic as you stare at the photo."));
                        break;
                }
            }

            return it->type->charges_to_use();
        }

        if (ei_music == choice) {

            p->moves -= 30;

            if (it->active) {
                it->active = false;
                it->item_vars["EIPC_MUSIC_ON"] = "";

                p->add_msg_if_player(m_info, _("You turned off music on your %s."), it->tname().c_str());
            } else {
                it->active = true;
                it->item_vars["EIPC_MUSIC_ON"] = "1";

                p->add_msg_if_player(m_info, _("You turned on music on your %s."), it->tname().c_str());

            }

            return it->type->charges_to_use();
        }

        if (ei_recipe == choice) {
            p->moves -= 50;

            uimenu rmenu;

            rmenu.selected = 0;
            rmenu.text = _("Choose recipe to view:");
            rmenu.addentry(0, true, 'q', _("Cancel"));

            std::vector<std::string> candidate_recipes;
            std::istringstream f(it->item_vars["EIPC_RECIPES"]);
            std::string s;
            int k = 1;
            while (getline(f, s, ',')) {

                if (s.size() == 0) {
                    continue;
                }

                candidate_recipes.push_back(s);

                auto recipe = find_recipe( s );
                if( recipe ) {
                    rmenu.addentry( k++, true, -1, item::nname( recipe->result ) );
                }
            }

            rmenu.query();

            const int rchoice = rmenu.ret;
            if (0 == rchoice) {
                return it->type->charges_to_use();
            } else {
                it->item_tags.insert("HAS_RECIPE");
                it->item_vars["RECIPE"] = candidate_recipes[rchoice - 1];

                auto recipe = find_recipe( it->item_vars["RECIPE"] );
                if( recipe ) {
                    p->add_msg_if_player(m_info,
                        _("You change the e-ink screen to show a recipe for %s."),
                                         item::nname( recipe->result ).c_str());
                }
            }

            return it->type->charges_to_use();
        }

        if (ei_monsters == choice) {

            uimenu pmenu;

            pmenu.selected = 0;
            pmenu.text = _("Your collection of monsters:");
            pmenu.addentry(0, true, 'q', _("Cancel"));

            std::vector<std::string> monster_photos;

            std::istringstream f(it->item_vars["EINK_MONSTER_PHOTOS"]);
            std::string s;
            int k = 1;
            while (getline(f, s, ',')) {
                if (s.size() == 0) {
                    continue;
                }
                monster_photos.push_back(s);
                std::string menu_str;
                const monster dummy(GetMType(s));
                menu_str = dummy.name();
                getline(f, s, ',');
                char *chq = &s[0];
                const int quality = atoi(chq);
                menu_str += " [" + photo_quality_names[quality] + "]";
                pmenu.addentry(k++, true, -1, menu_str.c_str());
            }

            int choice;
            do {
                pmenu.query();
                choice = pmenu.ret;

                if (0 == choice) {
                    break;
                }

                const monster dummy(GetMType(monster_photos[choice - 1]));
                popup(dummy.type->description.c_str());
            } while (true);
            return it->type->charges_to_use();
        }

        if (ei_download == choice) {

            p->moves -= 200;

            const int inventory_index = g->inv_for_flag("MC_MOBILE", _("Insert memory card"), false);
            item *mc = &(p->i_at(inventory_index));

            if (mc == NULL || mc->is_null()) {
                p->add_msg_if_player(m_info, _("You do not have that item!"));
                return it->type->charges_to_use();
            }
            if (!mc->has_flag("MC_MOBILE")) {
                p->add_msg_if_player(m_info, _("This is not a compatible memory card."));
                return it->type->charges_to_use();
            }

            init_memory_card_with_random_stuff(p, mc);

            if (mc->has_flag("MC_ENCRYPTED")) {
                p->add_msg_if_player(m_info, _("This memory card is encrypted."));
                return it->type->charges_to_use();
            }
            if (!mc->has_flag("MC_HAS_DATA")) {
                p->add_msg_if_player(m_info, _("This memory card does not contain any new data."));
                return it->type->charges_to_use();
            }

            einkpc_download_memory_card(p, it, mc);

            return it->type->charges_to_use();
        }

        if (ei_decrypt == choice) {
            p->moves -= 200;
            const int inventory_index = g->inv_for_flag("MC_MOBILE", _("Insert memory card"), false);
            item *mc = &(p->i_at(inventory_index));

            if (mc == NULL || mc->is_null()) {
                p->add_msg_if_player(m_info, _("You do not have that item!"));
                return it->type->charges_to_use();
            }
            if (!mc->has_flag("MC_MOBILE")) {
                p->add_msg_if_player(m_info, _("This is not a compatible memory card."));
                return it->type->charges_to_use();
            }

            init_memory_card_with_random_stuff(p, mc);

            if (!mc->has_flag("MC_ENCRYPTED")) {
                p->add_msg_if_player(m_info, _("This memory card is not encrypted."));
                return it->type->charges_to_use();
            }

            p->practice("computer", rng(2, 5));

            const int success = p->skillLevel("computer") * rng(1, p->skillLevel("computer")) *
                rng(1, p->int_cur) - rng(30, 80);
            if (success > 0) {
                p->practice("computer", rng(5, 10));
                p->add_msg_if_player(m_good, _("You successfully decrypted content on %s!"),
                                     mc->tname().c_str());
                einkpc_download_memory_card(p, it, mc);
            } else {
                if (success > -10 || one_in(5)) {
                    p->add_msg_if_player(m_neutral, _("You failed to decrypt the %s."), mc->tname().c_str());
                } else {
                    p->add_msg_if_player(m_bad, _("You tripped the firmware protection, and the card deleted its data!"));
                    mc->item_tags.clear();
                    mc->item_vars.clear();
                    mc->make("mobile_memory_card_used");
                }
            }
            return it->type->charges_to_use();
        }
    }
    return 0;
}

int iuse::camera(player *p, item *it, bool, point)
{
    enum {c_cancel, c_shot, c_photos, c_upload};

    uimenu amenu;

    amenu.selected = 0;
    amenu.text = _("What to do with camera?");
    amenu.addentry(c_shot, true, 'p', _("Take a photo"));
    if (it->item_vars["CAMERA_MONSTER_PHOTOS"] != "") {
        amenu.addentry(c_photos, true, 'l', _("List photos"));
        amenu.addentry(c_upload, true, 'u', _("Upload photos to memory card"));
    } else {
        amenu.addentry(c_photos, false, 'l', _("No photos in memory"));
    }

    amenu.addentry(c_cancel, true, 'q', _("Cancel"));

    amenu.query();
    const int choice = amenu.ret;

    if (c_cancel == choice) {
        return 0;
    }

    if (c_shot == choice) {

        point aim_point = g->look_around();

        if (aim_point.x == -1 || aim_point.y == -1) {
            p->add_msg_if_player(_("Never mind."));
            return 0;
        }

        if (aim_point.x == p->posx && aim_point.y == p->posy) {
            p->add_msg_if_player(_("You decide not to flash yourself."));
            return 0;
        }

        const int sel_zid = g->mon_at(aim_point.x, aim_point.y);
        const int sel_npcID = g->npc_at(aim_point.x, aim_point.y);

        if (sel_zid == -1 && sel_npcID == -1) {
            p->add_msg_if_player(_("There's nothing particularly interesting there."));
            return 0;
        }

        std::vector <point> trajectory = line_to(p->posx, p->posy, aim_point.x, aim_point.y, 0);
        trajectory.push_back(aim_point);

        p->moves -= 50;
        g->sound(p->posx, p->posy, 8, _("Click."));

        for (auto &i : trajectory) {
            int tx = i.x;
            int ty = i.y;

            int zid = g->mon_at(tx, ty);
            int npcID = g->npc_at(tx, ty);

            if (zid != -1 || npcID != -1) {
                int dist = rl_dist(p->posx, p->posy, tx, ty);

                int camera_bonus = it->has_flag("CAMERA_PRO") ? 10 : 0;
                int photo_quality = 20 - rng(dist, dist * 2) * 2 + rng(camera_bonus / 2, camera_bonus);
                if (photo_quality > 5) {
                    photo_quality = 5;
                }
                if (photo_quality < 0) {
                    photo_quality = 0;
                }

                const std::string quality_name = photo_quality_names[photo_quality];

                if (zid != -1) {
                    monster &z = g->zombie(zid);

                    if (dist < 4 && one_in(dist + 2) && z.has_flag(MF_SEES)) {
                        p->add_msg_if_player(_("%s looks blinded."), z.name().c_str());
                        z.add_effect("blind", rng(5, 10));
                    }

                    if (zid != sel_zid && (z.type->size <= MS_SMALL || z.is_hallucination() || z.type->in_species("HALLUCINATION"))) {
                        continue;
                    }

                    if (zid != sel_zid) {
                        p->add_msg_if_player(m_warning, _("There's a %s in the way!"), z.name().c_str());
                        return it->type->charges_to_use();
                    }

                    if (z.is_hallucination() || z.type->in_species("HALLUCINATION")) {
                        p->add_msg_if_player(_("Strange...there's nothing in the picture?"));
                        return it->type->charges_to_use();
                    }

                    if (z.mission_id != -1) {
                        //quest processing...
                    }

                    p->add_msg_if_player(_("You took a %s photo of %s."), quality_name.c_str(),
                                         z.name().c_str());

                    const std::string mtype = z.type->id;

                    if (it->item_vars["CAMERA_MONSTER_PHOTOS"] == "") {
                        it->item_vars["CAMERA_MONSTER_PHOTOS"] = "," + mtype + "," + string_format("%d",
                                photo_quality) + ",";
                    } else {

                        const size_t strpos = it->item_vars["CAMERA_MONSTER_PHOTOS"].find("," + mtype + ",");

                        if (strpos == std::string::npos) {
                            it->item_vars["CAMERA_MONSTER_PHOTOS"] += mtype + "," + string_format("%d", photo_quality) + ",";
                        } else {

                            const size_t strqpos = strpos + mtype.size() + 2;
                            char *chq = &it->item_vars["CAMERA_MONSTER_PHOTOS"][strqpos];
                            const int old_quality = atoi(chq);

                            if (photo_quality > old_quality) {
                                chq = &string_format("%d", photo_quality)[0];
                                it->item_vars["CAMERA_MONSTER_PHOTOS"][strqpos] = *chq;

                                p->add_msg_if_player(_("This photo is better than the previous one."));

                            }

                        }
                    }

                    return it->type->charges_to_use();

                } else {
                    npc *guy = g->active_npc[npcID];

                    if (dist < 4 && one_in(dist + 2)) {
                        p->add_msg_if_player(_("%s looks blinded."), guy->name.c_str());
                        guy->add_effect("blind", rng(5, 10));
                    }

                    if (npcID != sel_npcID) {
                        p->add_msg_if_player(m_warning, _("There's a %s in the way!"), guy->name.c_str());
                        return it->type->charges_to_use();
                    }

                    //just photo, no save. Maybe in the future we will need to create CAMERA_NPC_PHOTOS
                    p->add_msg_if_player(_("You took a %s photo of %s."), photo_quality_names[photo_quality].c_str(),
                                         guy->name.c_str());

                    return it->type->charges_to_use();
                }

                return it->type->charges_to_use();
            }

        }

        return it->type->charges_to_use();
    }

    if (c_photos == choice) {

        uimenu pmenu;

        pmenu.selected = 0;
        pmenu.text = _("Critter photos saved on camera:");
        pmenu.addentry(0, true, 'q', _("Cancel"));

        std::vector<std::string> monster_photos;

        std::istringstream f(it->item_vars["CAMERA_MONSTER_PHOTOS"]);
        std::string s;
        int k = 1;
        while (getline(f, s, ',')) {

            if (s.size() == 0) {
                continue;
            }

            monster_photos.push_back(s);

            std::string menu_str;

            const monster dummy(GetMType(s));
            menu_str = dummy.name();

            getline(f, s, ',');
            char *chq = &s[0];
            const int quality = atoi(chq);

            menu_str += " [" + photo_quality_names[quality] + "]";

            pmenu.addentry(k++, true, -1, menu_str.c_str());
        }

        int choice;
        do {
            pmenu.query();
            choice = pmenu.ret;

            if (0 == choice) {
                break;
            }

            const monster dummy(GetMType(monster_photos[choice - 1]));
            popup(dummy.type->description.c_str());

        } while (true);

        return it->type->charges_to_use();
    }

    if (c_upload == choice) {

        p->moves -= 200;

        const int inventory_index = g->inv_for_flag("MC_MOBILE", _("Insert memory card"), false);
        item *mc = &(p->i_at(inventory_index));

        if (mc == NULL || mc->is_null()) {
            p->add_msg_if_player(m_info, _("You do not have that item!"));
            return it->type->charges_to_use();
        }
        if (!mc->has_flag("MC_MOBILE")) {
            p->add_msg_if_player(m_info, _("This is not a compatible memory card."));
            return it->type->charges_to_use();
        }

        init_memory_card_with_random_stuff(p, mc);

        if (mc->has_flag("MC_ENCRYPTED")) {
            if (!query_yn(_("This memory card is encrypted. Format and clear data?"))) {
                return it->type->charges_to_use();
            }
        }
        if (mc->has_flag("MC_HAS_DATA")) {
            if (!query_yn(_("Are you sure you want to clear the old data on the card?"))) {
                return it->type->charges_to_use();
            }
        }

        mc->make("mobile_memory_card");
        mc->item_tags.clear();
        mc->item_vars.clear();
        mc->item_tags.insert("MC_HAS_DATA");

        mc->item_vars["MC_MONSTER_PHOTOS"] = it->item_vars["CAMERA_MONSTER_PHOTOS"];
        p->add_msg_if_player(m_info, _("You upload monster photos to memory card."));

        return it->type->charges_to_use();
    }

    return it->type->charges_to_use();
}

int iuse::ehandcuffs(player *p, item *it, bool t, point pos)
{
    if (t) {

        if (g->m.has_flag("SWIMMABLE", pos.x, pos.y)) {
            it->item_tags.erase("NO_UNWIELD");
            it->charges = 0;
            it->active = false;
            add_msg(m_good, _("%s automatically turned off!"), it->tname().c_str());
            return it->type->charges_to_use();
        }

        if (it->charges == 0) {

            g->sound(pos.x, pos.y, 2, "Click.");
            it->item_tags.erase("NO_UNWIELD");
            it->active = false;

            if (p->has_item(it) && p->weapon.type->id == "e_handcuffs") {
                add_msg(m_good, _("%s on your hands opened!"), it->tname().c_str());
            }

            return it->type->charges_to_use();
        }

        if (p->has_item(it)) {
            if (p->has_active_bionic("bio_shock") && p->power_level >= 2 && one_in(5)) {
                p->power_level -= 2;

                it->item_tags.erase("NO_UNWIELD");
                it->charges = 0;
                it->active = false;
                add_msg(m_good, _("The %s crackle with electricity from your bionic, then come off your hands!"), it->tname().c_str());

                return it->type->charges_to_use();
            }
        }

        if (calendar::turn % 10 == 0) {
            g->sound(pos.x, pos.y, 10, _("a police siren, whoop WHOOP."));
        }

        const int x = atoi(it->item_vars["HANDCUFFS_X"].c_str());
        const int y = atoi(it->item_vars["HANDCUFFS_Y"].c_str());

        if ((it->charges > it->type->maximum_charges() - 1000) && (x != pos.x || y != pos.y)) {

            if (p->has_item(it) && p->weapon.type->id == "e_handcuffs") {

                if (p->has_artifact_with(AEP_RESIST_ELECTRICITY) ||
                    p->has_active_bionic("bio_faraday")) { //Artifact or bionic stops electricity.
                    add_msg(_("The electricity flows around you."));
                } else if (p->worn_with_flag("ELECTRIC_IMMUNE")) { //Artifact or bionic stops electricity.
                    add_msg(_("Your armor safely grounds the electrical discharge."));
                } else {

                    add_msg(m_bad, _("Ouch, the cuffs shock you!"));

                    p->apply_damage(nullptr, bp_arm_l, rng(0, 2));
                    p->apply_damage(nullptr, bp_arm_r, rng(0, 2));
                    p->mod_pain(rng(2, 5));

                }

            } else {
                add_msg(m_bad, _("The %s spark with electricity!"), it->tname().c_str());
            }

            it->charges -= 50;
            if (it->charges < 1) {
                it->charges = 1;
            }

            it->item_vars["HANDCUFFS_X"] = string_format("%d", pos.x);
            it->item_vars["HANDCUFFS_Y"] = string_format("%d", pos.y);

            return it->type->charges_to_use();

        }

        return it->type->charges_to_use();

    }

    if (it->active) {
        add_msg("The %s are clamped tightly on your wrists.  You can't take them off.",
                it->tname().c_str());
    } else {
        add_msg("The %s have discharged and can be taken off.", it->tname().c_str());
    }

    return it->type->charges_to_use();
}

int iuse::radiocar(player *p, item *it, bool, point)
{
    int choice = -1;
    if (it->contents.empty()) {
        choice = menu(true, _("Using RC car:"), _("Turn on"),
                      _("Put a bomb to car"), _("Cancel"), NULL);
    } else if (it->contents.size() == 1) {
        choice = menu(true, _("Using RC car:"), _("Turn on"),
                      it->contents[0].tname().c_str(), _("Cancel"), NULL);
    }
    if (choice == 3) {
        return 0;
    }

    if (choice == 1) { //Turn car ON
        if( it->charges <= 0 ) {
            p->add_msg_if_player(_("The RC car's batteries seem to be dead."));
            return 0;
        }

        item bomb;

        if( !it->contents.empty() ) {
            bomb = it->contents[0];
        }

        it->make("radio_car_on");

        it->active = true;

        if( !(bomb.is_null()) ) {
            it->put_in(bomb);
        }

        p->add_msg_if_player(
            _("You turned on your RC car, now place it on ground, and use radiocontrol to play."));

        return 0;
    }

    if (choice == 2) {

        if( it->contents.empty() ) { //arming car with bomb
            int inventory_index = g->inv_for_flag("RADIOCARITEM", _("Arm what?"), false);
            item *put = &(p->i_at(inventory_index));
            if (put == NULL || put->is_null()) {
                p->add_msg_if_player(m_info, _("You do not have that item!"));
                return 0;
            }

            if (put->has_flag("RADIOCARITEM")) {
                p->moves -= 300;
                p->add_msg_if_player(_("You armed your RC car with %s."),
                                     put->tname().c_str());
                it->put_in(p->i_rem(inventory_index));
            } else {
                p->add_msg_if_player(_("RC car with %s ? How?"),
                                     put->tname().c_str());
            }
        } else { // Disarm the car
            p->moves -= 150;
            item &bomb = it->contents[0];

            p->inv.assign_empty_invlet(bomb, true); // force getting an invlet.
            p->i_add(bomb);
            it->contents.erase(it->contents.begin());

            p->add_msg_if_player(_("You disarmed your RC car"));
        }
    }

    return it->type->charges_to_use();
}

int iuse::radiocaron(player *p, item *it, bool t, point pos)
{
    if (t) {
        g->sound(pos.x, pos.y, 6, "buzzz...");

        return it->type->charges_to_use();
    } else if ( it->charges <= 0 ) {
        // Deactivate since other mode has an iuse too.
        it->active = false;
        return 0;
    }

    int choice = menu(true, _("What do with activated RC car:"), _("Turn off"),
                      _("Cancel"), NULL);

    if (choice == 2) {
        return it->type->charges_to_use();
    }

    if (choice == 1) {
        item bomb;

        if (!it->contents.empty()) {
            bomb = it->contents[0];
        }

        it->make("radio_car");
        it->active = false;

        if (!(bomb.is_null())) {
            it->put_in(bomb);
        }

        p->add_msg_if_player(_("You turned off your RC car"));
        return it->type->charges_to_use();
    }

    return it->type->charges_to_use();
}

void sendRadioSignal(player *p, std::string signal)
{
    for (size_t i = 0; i < p->inv.size(); i++) {
        item &it = p->inv.find_item(i);

        if (it.has_flag("RADIO_ACTIVATION") && it.has_flag(signal)) {
            g->sound(p->posx, p->posy, 6, "beep.");

            it_tool *tmp = dynamic_cast<it_tool *>(it.type);
            tmp->invoke(p, &it, false, p->pos());
        }
    }

    g->m.trigger_rc_items( signal );
}

int iuse::radiocontrol(player *p, item *it, bool t, point)
{
    if (t) {
        if (it->charges == 0) {
            it->active = false;
            p->remove_value( "remote_controlling" );
        } else if( p->get_value( "remote_controlling" ) == "" ) {
            it->active = false;
        }

        return it->type->charges_to_use();
    }

    int choice = -1;
    const char *car_action = NULL;

    if (!it->active) {
        car_action = _("Take control of RC car.");
    } else {
        car_action = _("Stop controlling RC car.");
    }

    choice = menu(true, _("What do with radiocontrol:"), _("Nothing"), car_action,
                  _("Press red button"), _("Press blue button"), _("Press green button"), NULL);

    if (choice == 1) {
        return 0;
    } else if (choice == 2) {
        if( it->active ) {
            it->active = false;
            p->remove_value( "remote_controlling" );
        } else {
            std::list<std::pair<tripoint, item *>> rc_pairs = g->m.get_rc_items();
            tripoint rc_item_location = {999, 999, 999};
            // TODO: grab the closest car or similar?
            for( auto &rc_pairs_rc_pair : rc_pairs ) {
                if( rc_pairs_rc_pair.second->type->id == "radio_car_on" &&
                    rc_pairs_rc_pair.second->active ) {
                    rc_item_location = rc_pairs_rc_pair.first;
                }
            }
            if( rc_item_location.x == 999 ) {
                p->add_msg_if_player(_("No active RC cars on ground and in range."));
                return it->type->charges_to_use();
            } else {
                std::stringstream car_location_string;
                // Populate with the point and stash it.
                car_location_string << rc_item_location.x << ' ' <<
                    rc_item_location.y << ' ' << rc_item_location.z;
                p->add_msg_if_player(m_good, _("You take control of the RC car."));

                p->set_value( "remote_controlling", car_location_string.str() );
                it->active = true;
            }
        }
    } else if (choice > 2) {
        std::string signal = "RADIOSIGNAL_";
        //red button
        if (choice == 3) {
            auto item_list = p->get_radio_items();
            for( auto &elem : item_list ) {
                if( ( elem )->has_flag( "BOMB" ) ) {
                    p->add_msg_if_player( m_warning,
                        _("You might want to place that radio bomb somewhere before detonating it.") );
                    return 0;
                }
            }
        }

        std::stringstream choice_str;
        choice_str << (choice - 2);
        signal += choice_str.str();

        p->add_msg_if_player(_("Click."));
        sendRadioSignal(p, signal);
        p->moves -= 150;
    }

    return it->type->charges_to_use();
}

static bool hackveh(player *p, item *it, vehicle *veh)
{
    if( !veh->is_locked || !veh->has_security_working() ) {
        return true;
    }
    bool advanced = veh->all_parts_with_feature( "REMOTE_CONTROLS", true ).size() > 0;
    if( advanced && veh->is_locked && veh->is_alarm_on ) {
        p->add_msg_if_player( m_bad, _("This vehicle's security system has locked you out!") );
        return false;
    }

    int roll = dice( p->skillLevel( "computer" ) + 2, p->int_cur ) - ( advanced ? 50 : 25 );
    int effort = 0;
    bool success = false;
    if( roll < -20 ) { // Really bad rolls will trigger the alarm before you know it exists
        effort = 1;
        p->add_msg_if_player( m_bad, _("You trigger the alarm!") );
        veh->is_alarm_on = true;
    } else if( roll >= 20 ) { // Don't bother the player if it's trivial
        effort = 1;
        p->add_msg_if_player( m_good, _("You quickly bypass the security system!") );
        success = true;
    }

    if( effort == 0 && !query_yn( _("Try to hack this car's security system?") ) ) {
        // Scanning for security systems isn't free
        p->moves -= 100;
        it->charges -= 1;
        return false;
    }

    p->practice( "computer", advanced ? 10 : 3 );
    if( roll < -10 ) {
        effort = rng( 4, 8 );
        p->add_msg_if_player( m_bad, _("You waste some time, but fail to affect the security system.") );
    } else if( roll < 0 ) {
        effort = 1;
        p->add_msg_if_player( m_bad, _("You fail to affect the security system.") );
    } else if( roll < 20 ) {
        effort = rng( 2, 8 );
        p->add_msg_if_player( m_mixed, _("You take some time, but manage to bypass the security system!") );
        success = true;
    } 
    
    p->moves -= effort * 100;
    it->charges -= effort;
    if( success && advanced ) { // Unlock controls, but only if they're drive-by-wire
        veh->is_locked = false;
    }
    return success;
}

int iuse::remoteveh(player *p, item *it, bool t, point)
{
    if (t) {
        if (it->charges == 0) {
            it->active = false;
            p->remove_value( "remote_controlling_vehicle" );
        } else if( g->remoteveh() == nullptr ) {
            p->add_msg_if_player( _("You stop remotely controlling the vehicle.") );
            it->active = false;
        }

        return it->type->charges_to_use();
    }

    bool controlling = it->active && p->get_value( "remote_controlling_vehicle" ) != "";
    int choice = menu(true, _("What to do with remote vehicle control:"), _("Nothing"), 
                      controlling ? _("Stop controlling the vehicle.") : _("Take control of a vehicle."),
                      _("Execute one vehicle action"), NULL);

   if (choice < 2 || choice > 3 ) {
        return 0;
    }

    if( choice == 2 && controlling ) {
        it->active = false;
        p->remove_value( "remote_controlling_vehicle" );
        p->add_msg_if_player(m_good, _("You stop remotely controlling the vehicle."));
        return 0;
    }

    int px = g->u.view_offset_x;
    int py = g->u.view_offset_y;
    
    point target = g->look_around();
    vehicle* veh = g->m.veh_at( target.x, target.y );
    
    if( veh == nullptr ) {
        popup( _("No vehicles here!") );
        return 0;
    }

    if( veh->fuel_left( "battery", true ) == 0 ) {
        popup( _("This vehicle has no power!") );
    }

    if( !hackveh( p, it, veh ) ) {
        return 0;
    }

    if( veh->all_parts_with_feature( "CONTROLS", true ).size() == 0 ) {
        popup( _("This vehicle has no working controls.") );
        return 0;
    } else if( choice == 2 ) {
        if( veh->all_parts_with_feature( "REMOTE_CONTROLS", true ).size() == 0 ) {
            popup( _("This vehicle has only mechanical controls and can't be driven remotely.") );
            return 0;
        }
        std::stringstream car_location_string;
        // Copypaste from RC car.
        car_location_string << veh->global_x() << ' ' << veh->global_y() << ' ';
        p->add_msg_if_player(m_good, _("You take control of the vehicle."));
        p->set_value( "remote_controlling_vehicle", car_location_string.str() );
        it->active = true;
    } else if( choice == 3 ) {
        veh->use_controls();
    } else {
        return 0;
    }

    g->u.view_offset_x = px;
    g->u.view_offset_y = py;
    return it->type->charges_to_use();
}

bool multicooker_hallu(player *p)
{
    p->moves -= 200;
    const int random_hallu = rng(1, 7);
    std::vector<point> points;
    switch (random_hallu) {

        case 1:
            add_msg(m_info, _("And when you gaze long into a screen, the screen also gazes into you."));
            return true;

        case 2:
            add_msg(m_bad, _("The multi-cooker boiled your head!"));
            return true;

        case 3:
            add_msg(m_info, _("The characters on the screen display an obscene joke.  Strange humor."));
            return true;

        case 4:
            //~ Single-spaced & lowercase are intentional, conveying hurried speech-KA101
            add_msg(m_warning, _("Are you sure?! the multi-cooker wants to poison your food!"));
            return true;

        case 5:
            add_msg(m_info,
                    _("The multi-cooker argues with you about the taste preferences.  You don't want to deal with it."));
            return true;

        case 6:

            for (int x = p->posx - 1; x <= p->posx + 1; x++)
                for (int y = p->posy - 1; y <= p->posy + 1; y++) {
                    if (g->is_empty(x, y)) {
                        points.push_back(point(x, y));
                    }
                }

            if (!one_in(5)) {
                add_msg(m_warning, _("The multi-cooker runs away!"));
                const point random_point = points[rng(0, points.size() - 1)];

                monster m(GetMType("mon_hallu_multicooker"));
                m.hallucination = true;
                m.add_effect("run", 1, num_bp, true);
                m.spawn(random_point.x, random_point.y);
                g->add_zombie(m);
            } else {
                add_msg(m_bad, _("You're surrounded by aggressive multi-cookers!"));

                for( auto &point : points ) {
                    monster m(GetMType("mon_hallu_multicooker"));
                    m.hallucination = true;
                    m.spawn( point.x, point.y );
                    g->add_zombie(m);
                }
            }
            return true;

        default:
            return false;
    }

}

int iuse::multicooker(player *p, item *it, bool t, point pos)
{
    if (t) {
        if (it->charges == 0) {
            it->active = false;
            return 0;
        }

        int cooktime = atoi(it->item_vars["COOKTIME"].c_str());
        cooktime -= 100;

        if (cooktime >= 300 && cooktime < 400) {
            //Smart or good cook or careful
            if (p->int_cur + p->skillLevel("cooking") + p->skillLevel("survival") > 16) {
                item dummy(it->item_vars["DISH"], 0);
                add_msg(m_info, _("The multi-cooker should be finishing shortly..."));
            }
        }

        if (cooktime <= 0) {
            it->active = false;

            item meal(it->item_vars["DISH"], calendar::turn);
            meal.active = true;

            if (meal.has_flag("EATEN_HOT")) {
                meal.item_tags.insert("HOT");
                meal.item_counter = 600;
            }

            it->put_in(meal);
            it->item_vars["DISH"] = "";

            //~ sound of a multi-cooker finishing its cycle!
            g->sound(pos.x, pos.y, 8, _("ding!"));

            return 0;
        } else {
            it->item_vars["COOKTIME"] = string_format("%d", cooktime);
            return 0;
        }

    } else {
        enum {
            mc_cancel, mc_start, mc_stop, mc_take, mc_upgrade
        };

        if (p->is_underwater()) {
            p->add_msg_if_player(m_info, _("You can't do that while underwater."));
            return false;
        }

        if (p->has_trait("ILLITERATE")) {
            add_msg(m_info, _("You cannot read, and don't understand the screen or the buttons!"));
            return 0;
        }

        if (p->has_effect("hallu") || p->has_effect("visuals")) {
            if (multicooker_hallu(p)) {
                return 0;
            }
        }

        if (p->has_trait("HYPEROPIC") && !p->is_wearing("glasses_reading")
            && !p->is_wearing("glasses_bifocal") && !p->has_effect("contacts")) {
            add_msg(m_info, _("You'll need to put on reading glasses before you can see the screen."));
            return 0;
        }

        uimenu menu;
        menu.selected = 0;
        menu.text = _("Welcome to the RobotChef3000. Choose option:");

        menu.addentry(mc_cancel, true, 'q', _("Cancel"));

        if (it->active) {
            menu.addentry(mc_stop, true, 's', _("Stop cooking"));
        } else {
            if (it->contents.empty()) {
                if (it->charges < 50) {
                    p->add_msg_if_player(_("Batteries are low."));
                    return 0;
                }
                menu.addentry(mc_start, true, 's', _("Start cooking"));

                if (p->skillLevel("electronics") > 3 && p->skillLevel("fabrication") > 3) {
                    if (it->item_vars["MULTI_COOK_UPGRADE"] == "") {
                        menu.addentry(mc_upgrade, true, 'u', _("Upgrade multi-cooker"));
                    } else {
                        if (it->item_vars["MULTI_COOK_UPGRADE"] == "UPGRADE") {
                            menu.addentry(mc_upgrade, false, 'u', _("Multi-cooker already upgraded"));
                        } else {
                            menu.addentry(mc_upgrade, false, 'u', _("Multi-cooker unable to upgrade"));
                        }
                    }
                }
            } else {
                menu.addentry(mc_take, true, 't', _("Take out dish"));
            }
        }

        menu.query();
        int choice = menu.ret;

        if (mc_cancel == choice) {
            return 0;
        }

        if (mc_stop == choice) {
            if (query_yn(_("Really stop cooking?"))) {
                it->active = false;
                it->item_vars["DISH"] = "";
            }
            return 0;
        }

        if (mc_take == choice) {
            item &dish = it->contents[0];

            if (dish.has_flag("HOT")) {
                p->add_msg_if_player(m_good, _("You got the dish from the multi-cooker.  The %s smells delicious."),
                                     dish.tname(dish.charges, false).c_str());
            } else {
                p->add_msg_if_player(m_good, _("You got the %s from the multi-cooker."),
                                     dish.tname(dish.charges, false).c_str());
            }

            p->i_add(dish);
            it->contents.clear();

            return 0;
        }

        if (mc_start == choice) {
            enum {
                d_cancel
            };

            uimenu dmenu;
            dmenu.selected = 0;
            dmenu.text = _("Choose desired meal:");

            dmenu.addentry(d_cancel, true, 'q', _("Cancel"));

            std::vector<const recipe *> dishes;

            inventory crafting_inv = g->u.crafting_inventory();
            //add some tools and qualities. we can't add this qualities to json, because multicook must be used only by activating, not as component other crafts.
            crafting_inv.push_back(item("hotplate", 0)); //hotplate inside
            crafting_inv.push_back(item("tongs", 0)); //some recipes requires tongs
            crafting_inv.push_back(item("toolset", 0)); //toolset with CUT and other qualities inside
            crafting_inv.push_back(item("pot", 0)); //good COOK, BOIL, CONTAIN qualities inside

            int counter = 1;

            for( auto &recipe : recipes ) {
                for( auto &elem : recipe.second ) {
                    if( ( elem )->cat == "CC_FOOD" && ( ( elem )->subcat == "CSC_FOOD_MEAT" ||
                                                        ( elem )->subcat == "CSC_FOOD_VEGGI" ||
                                                        ( elem )->subcat == "CSC_FOOD_PASTA" ) ) {

                        if( p->knows_recipe( ( elem ) ) ) {
                            dishes.push_back( elem );
                            const bool can_make = ( elem )->can_make_with_inventory( crafting_inv );
                            item dummy( ( elem )->result, 0 );

                            dmenu.addentry(counter++, can_make, -1, dummy.display_name());
                        }
                    }
                }
            }

            dmenu.query();

            int choice = dmenu.ret;

            if (d_cancel == choice) {
                return 0;
            } else {
                const recipe *meal = dishes[choice - 1];
                int mealtime;
                if (it->item_vars["MULTI_COOK_UPGRADE"] == "UPGRADE") {
                    mealtime = meal->time;
                } else {
                    mealtime = meal->time * 2 ;
                }

                it_tool *tmp = dynamic_cast<it_tool *>(it->type);
                const int all_charges = 50 + mealtime / (tmp->turns_per_charge * 100);

                if (it->charges < all_charges) {

                    p->add_msg_if_player(m_warning,
                                         _("The multi-cooker needs %d charges to cook this dish."),
                                         all_charges);

                    return 0;
                }

                for (auto it : meal->requirements.components) {
                    p->consume_items(it);
                }

                it->item_vars["DISH"] = meal->result;
                it->item_vars["COOKTIME"] = string_format("%d", mealtime);

                p->add_msg_if_player(m_good ,
                                     _("The screen flashes blue symbols and scales as the multi-cooker begins to shake."));

                it->active = true;
                it->charges -= 50;

                p->practice("cooking", meal->difficulty * 3); //little bonus

                return 0;
            }
        }

        if (mc_upgrade == choice) {

            if (p->morale_level() < MIN_MORALE_CRAFT) { // See morale.h
                add_msg(m_info, _("Your morale is too low to craft..."));
                return false;
            }

            bool has_tools = true;

            const inventory &cinv = g->u.crafting_inventory();

            if (!cinv.has_amount("soldering_iron", 1)) {
                p->add_msg_if_player(m_warning, _("You need a %s."), item::nname( "soldering_iron" ).c_str());
                has_tools = false;
            }

            if( !cinv.has_items_with_quality( "SCREW_FINE", 1, 1 ) ) {
                p->add_msg_if_player(m_warning, _("You need an item with %s of 1 or more to disassemble this."), quality::get_name( "SCREW_FINE" ).c_str() );
                has_tools = false;
            }

            if (!has_tools) {
                return 0;
            }

            p->practice("electronics", rng(5, 10));
            p->practice("fabrication", rng(5, 10));

            p->moves -= 700;

            if (p->skillLevel("electronics") + p->skillLevel("fabrication") + p->int_cur > rng(20, 35)) {

                p->practice("electronics", rng(5, 20));
                p->practice("fabrication", rng(5, 20));

                p->add_msg_if_player(m_good,
                                     _("You've successfully upgraded the multi-cooker, master tinkerer!  Now it cooks faster!"));

                it->item_vars["MULTI_COOK_UPGRADE"] = "UPGRADE";

                return 0;

            } else {

                if (!one_in(5)) {
                    p->add_msg_if_player(m_neutral,
                                         _("You sagely examine and analyze the multi-cooker, but don't manage to accomplish anything."));
                } else {
                    p->add_msg_if_player(m_bad,
                                         _("Your tinkering nearly breaks the multi-cooker!  Fortunately, it still works, but best to stop messing with it."));
                    it->item_vars["MULTI_COOK_UPGRADE"] = "DAMAGED";
                }

                return 0;

            }

        }

    }

    return 0;
}

int iuse::cable_attach(player *p, item *it, bool, point)
{
    if(it->item_vars.count("state") < 1) {
        it->item_vars["state"] = "attach_first";
    }

    std::string initial_state = it->item_vars["state"];

    if(initial_state == "attach_first") {
        int posx, posy;
        if(!choose_adjacent(_("Attach cable to vehicle where?"),posx,posy)) {
            return 0;
        }
        auto veh = g->m.veh_at(posx, posy);
        if (veh == nullptr) {
            p->add_msg_if_player(_("There's no vehicle there."));
            return 0;
        } else {
            point abspos = g->m.getabs(posx, posy);
            it->active = true;
            it->item_vars["state"] = "pay_out_cable";
            it->item_vars["source_x"] = string_format("%d", abspos.x);
            it->item_vars["source_y"] = string_format("%d", abspos.y);
            it->item_vars["source_z"] = string_format("%d", g->levz);
            it->process( p, p->pos(), false );
        }
        p->moves -= 15;
    }
    else if(initial_state == "pay_out_cable") {
        int choice = -1;
        uimenu kmenu;
        kmenu.selected = 0;
        kmenu.text = _("Using cable:");
        kmenu.addentry(0, true, -1, _("Attach loose end of the cable"));
        kmenu.addentry(1, true, -1, _("Detach and re-spool the cable"));
        kmenu.addentry(-1, true, 'q', _("Cancel"));
        kmenu.query();
        choice = kmenu.ret;

        if(choice == -1) {
            return 0; // we did nothing.
        } else if(choice == 1) {
            it->reset_cable(p);
            return 0;
        }

        int posx, posy;
        if(!choose_adjacent(_("Attach cable to vehicle where?"),posx,posy)) {
            return 0;
        }
        auto target_veh = g->m.veh_at(posx, posy);
        if (target_veh == nullptr) {
            p->add_msg_if_player(_("There's no vehicle there."));
            return 0;
        } else {
            point source_global(atoi(it->item_vars["source_x"].c_str()),
                                atoi(it->item_vars["source_y"].c_str()));
            point source_local = g->m.getlocal(source_global);
            auto source_veh = g->m.veh_at(source_local.x, source_local.y);

            if(source_veh == target_veh) {
                if (p != nullptr && p->has_item(it)) {
                    p->add_msg_if_player(m_warning, _("The %s already has access to its own electric system!"),
                                        source_veh->name.c_str());
                }
                return 0;
            }

            point target_global = g->m.getabs(posx, posy);
            point target_local(posx, posy);

            if(source_veh == nullptr) {
                if( p != nullptr && p->has_item(it) ) {
                    p->add_msg_if_player(m_bad, _("You notice the cable has come loose!"));
                }
                it->reset_cable(p);
                return 0;
            }

            point vcoords = g->m.veh_part_coordinates(source_local.x, source_local.y);
            vehicle_part source_part(it->typeId(), vcoords.x, vcoords.y, it);
            source_part.target.first = target_global;
            source_part.target.second = target_veh->real_global_pos();
            source_veh->install_part(vcoords.x, vcoords.y, source_part);

            vcoords = g->m.veh_part_coordinates(target_local.x, target_local.y);
            vehicle_part target_part(it->typeId(), vcoords.x, vcoords.y, it);
            target_part.target.first = source_global;
            target_part.target.second = source_veh->real_global_pos();
            target_veh->install_part(vcoords.x, vcoords.y, target_part);

            if( p != nullptr && p->has_item(it) ) {
                p->add_msg_if_player(m_good, _("You link up the electric systems of the %s and the %s."),
                                     source_veh->name.c_str(), target_veh->name.c_str());
            }

            return 1; // Let the cable be destroyed.
        }
    }

    return 0;
}

int iuse::weather_tool(player *p, item *it, bool, point)
{
    w_point weatherPoint = g->weatherGen.get_weather(p->pos(), calendar::turn);

    if (it->type->id == "weather_reader") {
        p->add_msg_if_player(m_neutral, _("The %s's monitor slowly outputs the data..."), it->tname().c_str());
    }
    if (it->has_flag("THERMOMETER")) {        
        if (it->type->id == "thermometer") {
            p->add_msg_if_player(m_neutral, _("The %s reads %s."), it->tname().c_str(), print_temperature(g->get_temperature()).c_str());
        } else {
            p->add_msg_if_player(m_neutral, _("Temperature: %s."), print_temperature(g->get_temperature()).c_str());
        }
    }
    if (it->has_flag("HYGROMETER")) {
        if (it->type->id == "hygrometer") {
            p->add_msg_if_player(m_neutral, _("The %s reads %s."), it->tname().c_str(), print_humidity(get_local_humidity(weatherPoint.humidity, g->weather, g->is_sheltered(g->u.posx, g->u.posy))).c_str());
        } else {
            p->add_msg_if_player(m_neutral, _("Relative Humidity: %s."), print_humidity(get_local_humidity(weatherPoint.humidity, g->weather, g->is_sheltered(g->u.posx, g->u.posy))).c_str());
        }
    }
    if (it->has_flag("BAROMETER")) {
        if (it->type->id == "barometer") {
            p->add_msg_if_player(m_neutral, _("The %s reads %s."), it->tname().c_str(), print_pressure((int)weatherPoint.pressure).c_str());
        } else {
            p->add_msg_if_player(m_neutral, _("Pressure: %s."), print_pressure((int)weatherPoint.pressure).c_str());
        }
    }

    if (it->type->id == "weather_reader") {
        int vpart = -1;
        vehicle *veh = g->m.veh_at( p->posx, p->posy, vpart );
        int vehwindspeed = 0;
        if( veh ) {
            vehwindspeed = abs(veh->velocity / 100); // For mph
        }
        const oter_id &cur_om_ter = overmap_buffer.ter(g->om_global_location());
        std::string omtername = otermap[cur_om_ter].name;
        int windpower = vehwindspeed + get_local_windpower(weatherPoint.windpower, omtername, g->is_sheltered(g->u.posx, g->u.posy));

        p->add_msg_if_player(m_neutral, _("Wind Speed: %s."), print_windspeed((float)windpower).c_str());
        p->add_msg_if_player(m_neutral, _("Feels Like: %s."), print_temperature(get_local_windchill(weatherPoint.temperature, weatherPoint.humidity, windpower) + g->get_temperature()).c_str());
    }
    
    return 0;
}
