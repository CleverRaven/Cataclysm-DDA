#include "iuse.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "output.h"
#include "debug.h"
#include "options.h"
#include "rng.h"
#include "line.h"
#include "mutation.h"
#include "player.h"
#include "vehicle.h"
#include "uistate.h"
#include "action.h"
#include "monstergenerator.h"
#include "speech.h"
#include "overmapbuffer.h"
#include "json.h"
#include "messages.h"
#include "crafting.h"
#include "recipe_dictionary.h"
#include "sounds.h"
#include "monattack.h"
#include "trap.h"
#include "iuse_actor.h" // For firestarter
#include "mongroup.h"
#include "translations.h"
#include "morale.h"
#include "input.h"
#include "npc.h"
#include "event.h"
#include "artifact.h"
#include "overmap.h"
#include "ui.h"
#include "mtype.h"
#include "field.h"
#include "weather_gen.h"
#include "weather.h"
#include "cata_utility.h"
#include "map_iterator.h"

#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <set>
#include <cstdlib>

#define RADIO_PER_TURN 25 // how many characters per turn of radio

#include "iuse_software.h"

const mtype_id mon_bee( "mon_bee" );
const mtype_id mon_blob( "mon_blob" );
const mtype_id mon_cat( "mon_cat" );
const mtype_id mon_dog( "mon_dog" );
const mtype_id mon_fly( "mon_fly" );
const mtype_id mon_hallu_multicooker( "mon_hallu_multicooker" );
const mtype_id mon_shadow( "mon_shadow" );
const mtype_id mon_spore( "mon_spore" );
const mtype_id mon_vortex( "mon_vortex" );
const mtype_id mon_wasp( "mon_wasp" );

const skill_id skill_firstaid( "firstaid" );
const skill_id skill_tailor( "tailor" );
const skill_id skill_survival( "survival" );
const skill_id skill_cooking( "cooking" );
const skill_id skill_mechanics( "mechanics" );
const skill_id skill_archery( "archery" );
const skill_id skill_computer( "computer" );
const skill_id skill_cutting( "cutting" );
const skill_id skill_carpentry( "carpentry" );
const skill_id skill_fabrication( "fabrication" );
const skill_id skill_electronics( "electronics" );
const skill_id skill_melee( "melee" );

const species_id ROBOT( "ROBOT" );
const species_id HALLUCINATION( "HALLUCINATION" );
const species_id ZOMBIE( "ZOMBIE" );
const species_id FUNGUS( "FUNGUS" );
const species_id INSECT( "INSECT" );

static bool is_firearm(const item &it);

void remove_double_ammo_mod( item &it, player &p )
{
    if( !it.item_tags.count( "DOUBLE_AMMO" ) || it.item_tags.count( "DOUBLE_REACTOR" )) {
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

void remove_double_plut_mod( item &it, player &p )
{
    if( !it.item_tags.count( "DOUBLE_AMMO" ) && !it.item_tags.count( "DOUBLE_REACTOR" ) ) {
        return;
    }
    p.add_msg_if_player( _( "You remove the double plutonium capacity mod of your %s!" ),
                         it.tname().c_str() );
    item mod( "double_plutonium_core", calendar::turn );
    p.i_add_or_drop( mod, 1 );
    it.item_tags.erase( "DOUBLE_AMMO" );
    it.item_tags.erase( "DOUBLE_REACTOR" );
    // Easier to remove all cells than to check for the actual real maximum
    if( it.charges > 0 ) {
        item cells( "plut_cell", calendar::turn );
        cells.charges = it.charges / 500;
        it.charges = 0;
        p.i_add_or_drop( cells, 1 );
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
    p.add_msg_if_player( _( "You remove the plutonium cells from your %s!" ), it.tname().c_str() );
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
    p.add_msg_if_player( _( "You remove the UPS Conversion Pack from your %s!" ), it.tname().c_str() );
    item mod( "battery_ups", calendar::turn );
    p.i_add_or_drop( mod, 1 );
    it.charges = 0;
    it.item_tags.erase( "USE_UPS" );
    it.item_tags.erase( "NO_UNLOAD" );
    it.item_tags.erase( "NO_RELOAD" );
}

void remove_radio_mod( item &it, player &p )
{
    if( !it.has_flag( "RADIO_MOD" ) ) {
        return;
    }
    p.add_msg_if_player( _( "You remove the radio modification from your %s!" ), it.tname().c_str() );
    item mod( "radio_mod", calendar::turn );
    p.i_add_or_drop( mod, 1 );
    it.item_tags.erase( "RADIO_ACTIVATION" );
    it.item_tags.erase( "RADIO_MOD" );
    it.item_tags.erase( "RADIOSIGNAL_1" );
    it.item_tags.erase( "RADIOSIGNAL_2" );
    it.item_tags.erase( "RADIOSIGNAL_3" );
    it.item_tags.erase( "RADIOCARITEM" );
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

    const bool hasnote = cut->has_var( "item_note" );
    std::string message = "";
    std::string messageprefix = string_format(hasnote ? _("(To delete, input one '.')\n") : "") +
                                string_format(_("%1$s on the %2$s is: "),
                                        gerund.c_str(), cut->type_name().c_str());
    message = string_input_popup(string_format(_("%s what?"), verb.c_str()), 64,
                                 (hasnote ? cut->get_var( "item_note" ) : message),
                                 messageprefix, "inscribe_item", 128);

    if (!message.empty()) {
        if (hasnote && message == ".") {
            cut->erase_var( "item_note" );
            cut->erase_var( "item_note_type" );
            cut->erase_var( "item_note_typez" );
        } else {
            cut->set_var( "item_note", message );
            cut->set_var( "item_note_type", gerund );
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
// They must also be passable.
std::vector<tripoint> points_for_gas_cloud(const tripoint &center, int radius)
{
    const std::vector<tripoint> gas_sources = closest_tripoints_first( radius, center );
    std::vector<tripoint> result;
    for( const auto &p : gas_sources ) {
        if (g->m.impassable( p )) {
            continue;
        }
        if( p != center ) {
            if (!g->m.clear_path( center, p, radius, 1, 100)) {
                // Can not splatter gas from center to that point, something is in the way
                continue;
            }
        }
        result.push_back( p );
    }
    return result;
}

/* iuse methods return the number of charges expended, which is usually it->charges_to_use().
 * Some items that don't normally use charges return 1 to indicate they're used up.
 * Regardless, returning 0 indicates the item has not been used up,
 * though it may have been successfully activated.
 */
int iuse::sewage(player *p, item *it, bool, const tripoint& )
{
    if( !p->query_yn( _("Are you sure you want to drink... this?") ) ) {
        return 0;
    }

    p->add_memorial_log(pgettext("memorial_male", "Ate a sewage sample."),
                        pgettext("memorial_female", "Ate a sewage sample."));
    p->vomit();
    if (one_in(4)) {
        p->mutate();
    }
    return it->type->charges_to_use();
}

int iuse::honeycomb(player *p, item *it, bool, const tripoint& )
{
    g->m.spawn_item( p->pos(), "wax", 2 );
    return it->type->charges_to_use();
}

int iuse::royal_jelly(player *p, item *it, bool, const tripoint& )
{
    p->add_effect("cureall", 1);
    return it->type->charges_to_use();
}

int iuse::xanax(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(_("You take some %s."), it->tname().c_str());
    p->add_effect("took_xanax", 900);
    return it->type->charges_to_use();
}

int iuse::caff(player *p, item *it, bool, const tripoint& )
{
    const auto food = dynamic_cast<const it_comest *> (it->type);
    p->fatigue -= food->stim * 3;
    return it->type->charges_to_use();
}

int iuse::atomic_caff(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(m_good, _("Wow!  This %s has a kick."), it->tname().c_str());
    const auto food = dynamic_cast<const it_comest *> (it->type);
    p->fatigue -= food->stim * 12;
    p->radiation += 8;
    return it->type->charges_to_use();
}

struct parasite_chances {
    int tapeworm;
    int bloodworms;
    int brainworms;
    int paincysts;
};

int raw_food(player *p, item *it, const struct parasite_chances &pcs)
{
    if( p->is_npc() ) {
        // NPCs don't need to eat, so they don't need to eat raw food
        return 0;
    }
    if (p->has_bionic("bio_digestion") || p->has_trait("PARAIMMUNE")) {
        return it->type->charges_to_use();
    }
    if (pcs.tapeworm > 0 && one_in(pcs.tapeworm) && !(p->has_effect("tapeworm")
            || p->has_trait("EATHEALTH"))) {
           // Hyper-Metabolism digests the thing before it can set up shop.
        p->add_effect("tapeworm", 1, num_bp, true);
    }
    if (pcs.bloodworms > 0 && one_in(pcs.bloodworms) && !(p->has_effect("bloodworms")
           || p->has_trait("ACIDBLOOD"))) {
           // The worms can't survive in acidic blood.
        p->add_effect("bloodworms", 1, num_bp, true);
    }
    if (pcs.brainworms > 0 && one_in(pcs.brainworms) && !p->has_effect("brainworms")) {
        p->add_effect("brainworms", 1, num_bp, true);
    }
    if (pcs.paincysts > 0 && one_in(pcs.paincysts) && !p->has_effect("paincysts")) {
        p->add_effect("paincysts", 1, num_bp, true);
    }
    return it->type->charges_to_use();
}

int iuse::raw_meat(player *p, item *it, bool, const tripoint& )
{
    struct parasite_chances pcs = {0, 0, 0, 0};
    pcs.tapeworm = 32;
    pcs.bloodworms = 64;
    pcs.brainworms = 128;
    pcs.paincysts = 64;
    return raw_food(p, it, pcs);
}

int iuse::raw_fat(player *p, item *it, bool, const tripoint& )
{
    struct parasite_chances pcs = {0, 0, 0, 0};
    pcs.tapeworm = 64;
    pcs.bloodworms = 128;
    pcs.brainworms = 128;
    return raw_food(p, it, pcs);
}

int iuse::raw_bone(player *p, item *it, bool, const tripoint& )
{
    struct parasite_chances pcs = {0, 0, 0, 0};
    pcs.bloodworms = 128;
    return raw_food(p, it, pcs);
}

int iuse::raw_fish(player *p, item *it, bool, const tripoint& )
{
    struct parasite_chances pcs = {0, 0, 0, 0};
    pcs.tapeworm = 256;
    pcs.bloodworms = 256;
    pcs.brainworms = 256;
    pcs.paincysts = 256;
    return raw_food(p, it, pcs);
}

int iuse::raw_wildveg(player *p, item *it, bool, const tripoint& )
{
    struct parasite_chances pcs = {0, 0, 0, 0};
    pcs.tapeworm = 512;
    pcs.bloodworms = 256;
    pcs.brainworms = 512;
    pcs.paincysts = 128;
    return raw_food(p, it, pcs);
}

#define STR(weak, medium, strong) (strength == 0 ? (weak) : strength == 1 ? (medium) : (strong))
int alcohol(player *p, item *it, int strength)
{
    // Weaker characters are cheap drunks
    ///\EFFECT_STR_MAX reduces drunkenness duration
    int duration = STR(340, 680, 900) - (STR(6, 10, 12) * p->str_max);
    const auto food = dynamic_cast<const it_comest *> (it->type);
    if (p->has_trait("ALCMET")) {
        duration = STR(90, 180, 250) - (STR(6, 10, 10) * p->str_max);
        // Metabolizing the booze improves the nutritional value;
        // might not be healthy, and still causes Thirst problems, though
        p->mod_hunger(-(abs(food->stim)));
        // Metabolizing it cancels out the depressant
        p->stim += (abs(food->stim));
    } else if (p->has_trait("TOLERANCE")) {
        duration -= STR(120, 300, 450);
    } else if (p->has_trait("LIGHTWEIGHT")) {
        duration += STR(120, 300, 450);
    }
    if (!(p->has_trait("ALCMET"))) {
        p->pkill += STR(4, 8, 12);
    }
    p->add_effect("drunk", duration);
    return it->type->charges_to_use();
}
#undef STR

int iuse::alcohol_weak(player *p, item *it, bool, const tripoint& )
{
    return alcohol(p, it, 0);
}

int iuse::alcohol_medium(player *p, item *it, bool, const tripoint& )
{
    return alcohol(p, it, 1);
}

int iuse::alcohol_strong(player *p, item *it, bool, const tripoint& )
{
    return alcohol(p, it, 2);
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
int iuse::smoking(player *p, item *it, bool, const tripoint& )
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
        p->mod_hunger(-3);
    } else if (it->type->id == "handrolled_cig") {
        // This transforms the hand-rolled into a normal cig, which isn't exactly
        // what I want, but leaving it for now.
        cig = item("cig_lit", int(calendar::turn));
        cig.item_counter = 40;
        p->thirst += 2;
        p->mod_hunger(-3);
    } else if (it->type->id == "cigar") {
        cig = item("cigar_lit", int(calendar::turn));
        cig.item_counter = 120;
        p->thirst += 3;
        p->mod_hunger(-4);
    } else if (it->type->id == "joint") {
        cig = item("joint_lit", int(calendar::turn));
        cig.item_counter = 40;
        p->mod_hunger(4);
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


int iuse::ecig(player *p, item *it, bool, const tripoint& )
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
    p->mod_hunger(-1);
    p->add_effect("cig", 100);
    if (p->get_effect_dur("cig") > (100 * (p->addiction_level(ADD_CIG) + 1))) {
        p->add_msg_if_player(m_bad, _("Ugh, too much nicotine... you feel nasty."));
    }
    return it->type->charges_to_use();
}

int iuse::antibiotic(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_player_or_npc( m_neutral,
        _("You take some antibiotics."),
        _("<npcname> takes some antibiotics.") );
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

int iuse::eyedrops(player *p, item *it, bool, const tripoint& )
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

int iuse::fungicide(player *p, item *it, bool, const tripoint& )
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }

    const bool has_fungus = p->has_effect("fungus");
    const bool has_spores = p->has_effect("spores");

    if( p->is_npc() && !has_fungus && !has_spores ) {
        return 0;
    }

    p->add_msg_player_or_npc( _("You use your fungicide."), _("<npcname> uses some fungicide") );
    if (has_fungus && (one_in(3))) {
        p->remove_effect("fungus");
        p->add_msg_if_player(m_warning,
                             _("You feel a burning sensation under your skin that quickly fades away."));
    }
    if (has_spores && (one_in(2))) {
        if (!p->has_effect("fungus")) {
            p->add_msg_if_player(m_warning, _("Your skin grows warm for a moment."));
        }
        p->remove_effect("spores");
        int spore_count = rng(1, 6);
        if (spore_count > 0) {
            for (int i = p->posx() - 1; i <= p->posx() + 1; i++) {
                for (int j = p->posy() - 1; j <= p->posy() + 1; j++) {
                    tripoint dest( i, j, p->posz() );
                    if (spore_count == 0) {
                        break;
                    }
                    if (i == p->posx() && j == p->posy()) {
                        continue;
                    }
                    if (g->m.passable(i, j) && x_in_y(spore_count, 8)) {
                        const int zid = g->mon_at(dest);
                        if (zid >= 0) {  // Spores hit a monster
                            if (g->u.sees(i, j) &&
                                !g->zombie(zid).type->in_species( FUNGUS )) {
                                add_msg(m_warning, _("The %s is covered in tiny spores!"),
                                        g->zombie(zid).name().c_str());
                            }
                            monster &critter = g->zombie( zid );
                            if( !critter.make_fungus() ) {
                                critter.die( p ); // counts as kill by player
                            }
                        } else {
                            g->summon_mon(mon_spore, dest);
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

int iuse::antifungal(player *p, item *it, bool, const tripoint& )
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

int iuse::antiparasitic(player *p, item *it, bool, const tripoint& )
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
        p->mod_hunger(-1);  // You just digested the tapeworm.
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
    if (p->has_effect("brainworms")) {
        p->remove_effect("brainworms");
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

int iuse::anticonvulsant(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(_("You take some anticonvulsant medication."));
    ///\EFFECT_STR reduces duration of anticonvulsant medication
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

int iuse::weed_brownie(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(_("You scarf down the delicious brownie.  It tastes a little funny though..."));
    int duration = 120;
    if (p->has_trait("TOLERANCE")) {
        duration = 90;
    }
    if (p->has_trait("LIGHTWEIGHT")) {
        duration = 150;
    }
    p->mod_hunger(2);
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

int iuse::coke(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(_("You snort a bump of coke."));
    ///\EFFECT_STR reduces duration of coke
    int duration = 21 - p->str_cur + rng(0, 10);
    if (p->has_trait("TOLERANCE")) {
        duration -= 10; // Symmetry would cause problems :-/
    }
    if (p->has_trait("LIGHTWEIGHT")) {
        duration += 20;
    }
    p->mod_hunger(-8);
    p->add_effect("high", duration);
    return it->type->charges_to_use();
}

int iuse::grack(player *p, item *it, bool, const tripoint& )
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
        p->mod_hunger(-10);
        p->add_effect("grack", duration);
        return it->type->charges_to_use();
    }
    return 0;
}

int iuse::meth(player *p, item *it, bool, const tripoint& )
{
    ///\EFFECT_STR reduces duration of meth
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
            g->m.add_field({p->posx() + int(rng(-2, 2)), p->posy() + int(rng(-2, 2)),p->posz()}, fd_methsmoke, 2,0);
        }
    } else {
        p->add_msg_if_player(_("You snort some crystal meth."));
        p->fatigue -= 300;
    }
    if (!p->has_effect("meth")) {
        duration += 600;
    }
    if (duration > 0) {
        // meth actually inhibits hunger, weaker characters benefit more
        ///\EFFECT_STR_MAX >4 experiences less hunger benefit from meth
        int hungerpen = (p->str_max < 5 ? 35 : 40 - ( 2 * p->str_max ));
        if (hungerpen>0) {
            p->mod_hunger(-hungerpen);
        }
        p->add_effect("meth", duration);
    }
    return it->type->charges_to_use();
}

int iuse::vitamins(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(_("You take some vitamins."));
    p->mod_healthy_mod(50, 50);
    return it->type->charges_to_use();
}

int iuse::vaccine(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(_("You inject the vaccine."));
    p->add_msg_if_player(m_good, _("You feel tough."));
    p->mod_healthy_mod(200, 200);
    p->mod_pain(3);
    item syringe( "syringe", it->bday );
    p->i_add( syringe );
    return it->type->charges_to_use();
}

int iuse::flu_vaccine(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(_("You inject the vaccine."));
    p->add_msg_if_player(m_good, _("You no longer need to fear the flu."));
    p->add_effect("flushot", 1, num_bp, true);
    p->mod_pain(3);
    item syringe( "syringe", it->bday );
    p->i_add( syringe );
    return it->type->charges_to_use();
}

int iuse::poison(player *p, item *it, bool, const tripoint& )
{
    if ((p->has_trait("EATDEAD"))) {
        return it->type->charges_to_use();
    }

    // NPCs have a magical sense of what is inedible
    // Players can abuse the crafting menu instead...
    if( !it->has_flag( "HIDDEN_POISON" ) &&
        ( p->is_npc() ||
          !p->query_yn( _("Are you sure you want to eat this? It looks poisonous...") ) ) ) {
        return 0;
    }
    ///\EFFECT_STR increases EATPOISON trait effectiveness (50-90%)
    if ((p->has_trait("EATPOISON")) && (!(one_in(p->str_cur / 2)))) {
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
int iuse::fun_hallu(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // NPCs hallucinating doesn't work yet!
        return 0;
    }
    const auto comest = dynamic_cast<const it_comest *>(it->type);

   //Fake a normal food morale effect
    if (p->has_trait("SPIRITUAL")) {
        p->add_morale(MORALE_FOOD_GOOD, 36, 72, 120, 60, false, comest);
    } else {
            p->add_morale(MORALE_FOOD_GOOD, 18, 36, 60, 30, false, comest);
      }
    if (!p->has_effect("hallu")) {
        p->add_effect("hallu", 3600);
    }
    return it->type->charges_to_use();
}

int iuse::meditate(player *p, item *it, bool, const tripoint& )
{
    if (p->has_trait("SPIRITUAL")) {
        p->moves -= 2000;
        p->add_msg_if_player(m_good, _("You pause to engage in spiritual contemplation."));
        p->add_morale(MORALE_FEELING_GOOD, 5, 10);
    } else {
            p->add_msg_if_player(_("This %s probably meant a lot to someone at one time."), it->tname().c_str());
      }
    return it->type->charges_to_use();
}

int iuse::thorazine(player *p, item *it, bool, const tripoint& )
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

int iuse::prozac(player *p, item *it, bool, const tripoint& )
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

int iuse::sleep(player *p, item *it, bool, const tripoint& )
{
    p->fatigue += 40;
    p->add_msg_if_player(m_warning, _("You feel very sleepy..."));
    return it->type->charges_to_use();
}

int iuse::iodine(player *p, item *it, bool, const tripoint& )
{
    p->add_effect("iodine", 1200);
    p->add_msg_if_player(_("You take an iodine tablet."));
    return it->type->charges_to_use();
}

int iuse::datura(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        return 0;
    }

    const auto comest = dynamic_cast<const it_comest *>(it->type);

    p->add_effect("datura", rng(2000, 8000));
    p->add_msg_if_player(_("You eat the datura seed."));
    if (p->has_trait("SPIRITUAL")) {
        p->add_morale(MORALE_FOOD_GOOD, 36, 72, 120, 60, false, comest);
    }
    return it->type->charges_to_use();
}

int iuse::flumed(player *p, item *it, bool, const tripoint& )
{
    p->add_effect("took_flumed", 6000);
    p->add_msg_if_player(_("You take some %s"), it->tname().c_str());
    return it->type->charges_to_use();
}

int iuse::flusleep(player *p, item *it, bool, const tripoint& )
{
    p->add_effect("took_flumed", 7200);
    p->fatigue += 30;
    p->add_msg_if_player(_("You take some %s"), it->tname().c_str());
    p->add_msg_if_player(m_warning, _("You feel very sleepy..."));
    return it->type->charges_to_use();
}

int iuse::inhaler(player *p, item *it, bool, const tripoint& )
{
    p->remove_effect("asthma");
    p->add_msg_if_player(m_neutral, _("You take a puff from your inhaler."));
    if (one_in(50)) {  // adverse reaction
        p->add_msg_if_player(m_bad, _("Your heart begins to race."));
        p->fatigue -= 10;
    }
    return it->type->charges_to_use();
}

int iuse::oxygen_bottle(player *p, item *it, bool, const tripoint& )
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
    p->remove_effect("winded");
    p->pkill += 2;
    return it->type->charges_to_use();
}

int iuse::blech(player *p, item *it, bool, const tripoint& )
{
    // TODO: Add more effects?
    if( it->made_of( LIQUID ) ) {
        if (!p->query_yn(_("This looks unhealthy, sure you want to drink it?"))) {
            return 0;
        }
    } else { //Assume that if a blech consumable isn't a drink, it will be eaten.
        if (!p->query_yn(_("This looks unhealthy, sure you want to eat it?"))) {
            return 0;
        }
    }
    p->add_msg_if_player(m_bad, _("Blech, that burns your throat!"));
    if (it->type->id != "soap") { // soap burns but doesn't make you throw up
        p->vomit();
    }
    return it->type->charges_to_use();
}

int iuse::plantblech(player *p, item *it, bool, const tripoint &pos)
{
    if (p->has_trait("THRESH_PLANT")) {
        double multiplier = -1;
        if (p->has_trait("CHLOROMORPH")) {
            multiplier = -3;
            p->add_msg_if_player(m_good, _("The meal is revitalizing."));
        } else{
            p->add_msg_if_player(m_good, _("Oddly enough, this doesn't taste so bad."));
        }
        const auto food = dynamic_cast<const it_comest*>(it->type);
        //reverses the harmful values of drinking fertilizer
        p->mod_hunger(p->nutrition_for(food) * multiplier);
        p->thirst -= food->quench * multiplier;
        p->mod_healthy_mod(food->healthy * multiplier, food->healthy * multiplier);
        p->add_morale(MORALE_FOOD_GOOD, -10 * multiplier, 60, 60, 30, false, food);
        return it->type->charges_to_use();
    } else {
        return blech( p, it, true, pos );
    }
}

int iuse::chew(player *p, item *it, bool, const tripoint& )
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
        p->add_msg_if_player( m_warning,
            _("The %s sears your insides white-hot, and you collapse to the ground!"),
            _("<npcname> writhes in agony and collapses to the ground!"),
            it->tname().c_str());
        p->vomit();
        p->mod_pain(35);
        // Lose a significant amount of HP, probably about 25-33%
        p->hurtall(rng(20, 35), nullptr);
        // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        p->fall_asleep((3000 - p->int_cur * 10));
        // Marloss is too thoroughly into your body to be dislodged by orals.
        p->set_mutation("MUTAGEN_AVOID");
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
            g->m.ter_set(p->posx(), p->posy(), t_fungus);
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
        p->add_msg_if_player( m_warning,
            _("The %s sears your insides white-hot, and you collapse to the ground!"),
            _("<npcname> writhes in agony and collapses to the ground!"),
            it->tname().c_str());
        p->vomit();
        p->mod_pain(55);
        // Lose a significant amount of HP, probably about 25-33%
        p->hurtall(rng(30, 45), nullptr);
         // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        ///\EFFECT_INT slightly reduces sleep duration when eating mycus+goo
        p->fall_asleep((4000 - p->int_cur * 10));
        // Injection does the trick.  Burn the fungus out.
        p->unset_mutation("THRESH_MARLOSS");
        p->set_mutation("MUTAGEN_AVOID");
        //~ Recall that Marloss mutations made you feel warmth spreading throughout your body.  That's gone.
        p->add_msg_if_player(m_warning, _("You feel a cold burn inside, as though everything warm has left you."));
        if( it->type->can_use("PURIFY_IV") ) {
            p->add_msg_if_player(m_warning, _("It was probably that marloss -- how did you know to call it \"marloss\" anyway?"));
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
            g->m.ter_set( p->pos(), t_fungus );
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

int iuse::mutagen(player *p, item *it, bool, const tripoint& )
{
    if (p->has_trait("MUTAGEN_AVOID")) {
         //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time?  uh-uh.  You're not drinking that chemical stuff."));
        return 0;
    }

    if( !(p->has_trait("THRESH_MYCUS")) ) {
        p->add_memorial_log(pgettext("memorial_male", "Consumed mutagen."),
                            pgettext("memorial_female", "Consumed mutagen."));
    }

    if( marloss_reject_mutagen( p, it ) ) {
        return it->type->charges_to_use();
    }

    if (p->has_trait("MUT_JUNKIE")) {
        p->add_msg_if_player(m_good, _("You quiver with anticipation..."));
        p->add_morale(MORALE_MUTAGEN, 5, 50);
    }
    bool downed = false;
    std::string mutation_category;
    // Generic "mutagen".
    if (it->has_flag("MUTAGEN_STRONG")) {
        mutation_category = "";
        p->mutate();
        p->mod_pain(2 * rng(1, 5));
        p->mod_hunger(10);
        p->fatigue += 5;
        p->thirst += 10;
        if (!one_in(3)) {
            p->mutate();
            p->mod_pain(2 * rng(1, 5));
            p->mod_hunger(10);
            p->fatigue += 5;
            p->thirst += 10;
            if (one_in(4)) {
                downed = true;
            }
        }
        if (one_in(2)) {
            p->mutate();
            p->mod_pain(2 * rng(1, 5));
            p->mod_hunger(10);
            p->fatigue += 5;
            p->thirst += 10;
            p->add_msg_player_or_npc( m_bad,
                _("Oops.  You must've blacked out for a minute there."),
                _("<npcname> suddenly collapses!") );
            //Should be about 3 min, less 6 sec/IN point.
            ///\EFFECT_INT reduces sleep duration when using mutagen
            p->fall_asleep((30 - p->int_cur));
        }
    }
    if (it->has_flag("MUTAGEN_WEAK")) {
        mutation_category = "";
        // Stuff like the limbs, the tainted tornado, etc.
        if (!one_in(3)) {
            p->mutate();
            p->mod_pain(2 * rng(1, 5));
            p->mod_hunger(10);
            p->fatigue += 5;
            p->thirst += 10;
            if (one_in(4)) {
                downed = true;
            }
        }
    } else {
        // Categorized/targeted mutagens go here.
        for (auto& iter : mutation_category_traits){
            mutation_category_trait m_category = iter.second;
            if (it->has_flag("MUTAGEN_" + m_category.category)) {
                mutation_category = "MUTCAT_" + m_category.category;
                p->add_msg_if_player(m_category.mutagen_message.c_str());
                p->mutate_category(mutation_category);
                p->mod_pain(m_category.mutagen_pain * rng(1, 5));
                p->mod_hunger(m_category.mutagen_hunger);
                p->fatigue += m_category.mutagen_fatigue;
                p->thirst += m_category.mutagen_thirst;
                break;
            }
        }
        // Yep, orals take a bit out of you too
        if (one_in(4)) {
            downed = true;
        }
    }

    // Don't print downed message for sleeping player
    if( downed && !p->in_sleep_state() ) {
        p->add_msg_player_or_npc( m_bad,
            _("You suddenly feel dizzy, and collapse to the ground."),
            _("<npcname> suddenly collapses to the ground!") );
        p->add_effect("downed", 1, num_bp, false, 0, true );
    }
    return it->type->charges_to_use();
}

static void test_crossing_threshold(player *p, const mutation_category_trait &m_category) {
    // Threshold-check.  You only get to cross once!
    if (!p->crossed_threshold()) {
        std::string mutation_category = "MUTCAT_" + m_category.category;
        std::string mutation_thresh = "THRESH_" + m_category.category;
        int total = 0;
        for (auto& iter : mutation_category_traits){
            total += p->mutation_category_level["MUTCAT_" + iter.second.category];
        }
        // Threshold-breaching
        std::string primary = p->get_highest_category();
        // Only if you were pushing for more in your primary category.
        // You wanted to be more like it and less human.
        // That said, you're required to have hit third-stage dreams first.
        if ((mutation_category == primary) && (p->mutation_category_level[primary] > 50)) {
            // Little help for the categories that have a lot of crossover.
            // Starting with Ursine as that's... a bear to get.  8-)
            // Alpha is similarly eclipsed by other mutation categories.
            // Will add others if there's serious/demonstrable need.
            int booster = 0;
            if (mutation_category == "MUTCAT_URSINE"  || mutation_category == "MUTCAT_ALPHA") {
                booster = 50;
            }
            int breacher = (p->mutation_category_level[primary]) + booster;
            if (x_in_y(breacher, total)) {
                p->add_msg_if_player(m_good,
                                   _("Something strains mightily for a moment...and then..you're...FREE!"));
                p->set_mutation(mutation_thresh);
                p->add_memorial_log(pgettext("memorial_male", m_category.memorial_message.c_str()),
                                    pgettext("memorial_female", m_category.memorial_message.c_str()));
                if (mutation_category == "MUTCAT_URSINE") {
                    // Manually removing Carnivore, since it tends to creep in
                    // This is because carnivore is a prereq for the
                    // predator-style post-threshold mutations.
                    if (p->has_trait("CARNIVORE")) {
                        p->unset_mutation("CARNIVORE");
                        p->add_msg_if_player(_("Your appetite for blood fades."));
                    }
                }
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

int iuse::mut_iv(player *p, item *it, bool, const tripoint& )
{
    if (p->has_trait("MUTAGEN_AVOID")) {
         //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time?  uh-uh.  You're not injecting that chemical stuff."));
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
        // TODO: Make MUT_JUNKIE NPCs like the player for giving them some of that stuff
        if (p->has_trait("MUT_JUNKIE")) {
            p->add_msg_if_player(m_good, _("Oh, yeah! That's the stuff!"));
            ///\EFFECT_STR increases volume of shouting with strong mutagen
            sounds::sound(p->pos(), 15 + 3 * p->str_cur, _("YES!  YES!  YESSS!!!"));
        } else if (p->has_trait("NOPAIN")) {
            p->add_msg_if_player(_("You inject yourself."));
        } else {
            p->add_msg_if_player(m_bad, _("You inject yoursel-arRGH!"));
            ///\EFFECT_STR increases volume of painful shouting with strong mutagen
            std::string scream = p->is_player() ?
                _("You scream in agony!!") :
                _("an agonized scream!");
            sounds::sound( p->pos(), 15 + 3 * p->str_cur, scream.c_str() );
        }
        p->mutate();
        p->mod_pain(1 * rng(1, 4));
        //Standard IV-mutagen effect: 10 hunger/thirst & 5 Fatigue *per mutation*.
        // Numbers may vary based on mutagen.
        p->mod_hunger(10);
        p->fatigue += 5;
        p->thirst += 10;
        p->mutate();
        p->mod_pain(2 * rng(1, 3));
        p->mod_hunger(10);
        p->fatigue += 5;
        p->thirst += 10;
        p->mutate();
        p->mod_hunger(10);
        p->fatigue += 5;
        p->thirst += 10;
        p->mod_pain(3 * rng(1, 2));
        if (!one_in(4)) {
            p->mutate();
            p->mod_hunger(10);
            p->fatigue += 5;
            p->thirst += 10;
        }
        if (!one_in(3)) {
            p->mutate();
            p->mod_hunger(10);
            p->fatigue += 5;
            p->thirst += 10;
            p->add_msg_if_player( m_bad,
                _("You writhe and collapse to the ground."),
                _("<npcname> writhes and collapses to the ground.") );
            p->add_effect("downed", rng( 1, 4 ), num_bp, false, 0, true );
        }
        if (!one_in(3)) {
            //Jackpot! ...kinda, don't wanna go unconscious in dangerous territory
            p->mutate();
            p->mod_hunger(10);
            p->fatigue += 5;
            p->thirst += 10;
            p->add_msg_if_player( m_bad,
                _("It all goes dark..."),
                _("<npcname> suddenly falls over!") );
            //Should be about 40 min, less 30 sec/IN point.
            ///\EFFECT_INT decreases sleep duration with IV mutagen
            p->fall_asleep((400 - p->int_cur * 5));
        }
    } else {
        mutation_category_trait m_category;
        std::string mutation_thresh;
        for (auto& iter : mutation_category_traits){
            if (it->has_flag("MUTAGEN_" + iter.second.category)) {
                m_category = iter.second;
                mutation_category = "MUTCAT_" + m_category.category;
                mutation_thresh = "THRESH_" + m_category.category;

                // try to cross the threshold to be able to get post-threshold mutations this iv.
                test_crossing_threshold(p, m_category);

                if (p->has_trait("MUT_JUNKIE")) {
                    p->add_msg_if_player(m_category.junkie_message.c_str());
                } else if (!(p->has_trait("MUT_JUNKIE"))) {
                    //there is only the one case, so no json, unless there is demand for it.
                    p->add_msg_if_player(m_category.iv_message.c_str());
                }
                // TODO: Remove the "is_player" part, implement NPC screams
                if( p->is_player() && !(p->has_trait("NOPAIN")) && m_category.iv_sound ) {
                    p->mod_pain(m_category.iv_pain);
                    ///\EFFECT_STR increases volume of painful shouting when using IV mutagen
                    sounds::sound(p->pos(), m_category.iv_noise + p->str_cur, m_category.iv_sound_message);
                }
                for (int i=0; i < m_category.iv_min_mutations; i++){
                    p->mutate_category(mutation_category);
                    p->mod_pain(m_category.iv_pain * rng(1, 5));
                    p->mod_hunger(m_category.iv_hunger);
                    p->fatigue += m_category.iv_fatigue;
                    p->thirst += m_category.iv_thirst;
                }
                for (int i=0; i < m_category.iv_additional_mutations; i++){
                    if (!one_in(m_category.iv_additional_mutations_chance)) {
                        p->mutate_category(mutation_category);
                        p->mod_pain(m_category.iv_pain * rng(1, 5));
                        p->mod_hunger(m_category.iv_hunger);
                        p->fatigue += m_category.iv_fatigue;
                        p->thirst += m_category.iv_thirst;
                    }
                }
                if (m_category.category == "CHIMERA"){
                     p->add_morale(MORALE_MUTAGEN_CHIMERA, m_category.iv_morale, m_category.iv_morale_max);
                } else if (m_category.category == "ELFA"){
                     p->add_morale(MORALE_MUTAGEN_ELF, m_category.iv_morale, m_category.iv_morale_max);
                } else if(m_category.iv_morale > 0){
                    p->add_morale(MORALE_MUTAGEN_MUTATION, m_category.iv_morale, m_category.iv_morale_max);
                }

                if (m_category.iv_sleep && !one_in(3)){
                    p->add_msg_if_player(m_bad, m_category.iv_sleep_message.c_str());
                    ///\EFFECT_INT reduces sleep duration when using IV mutagen
                    p->fall_asleep(m_category.iv_sleep_dur - p->int_cur * 5);
                }
                // try crossing again after getting new in-category mutations.
                test_crossing_threshold(p, m_category);
            }
        }
    }
    return it->type->charges_to_use();
}

// Helper to handle the logic of removing some random mutations.
static void do_purify( player *p )
{
    std::vector<std::string> valid; // Which flags the player has
    for( auto &traits_iter : mutation_branch::get_all() ) {
        if( p->has_trait( traits_iter.first ) && !p->has_base_trait( traits_iter.first ) ) {
            //Looks for active mutation
            valid.push_back( traits_iter.first );
        }
    }
    if( valid.empty() ) {
        p->add_msg_if_player(_("You feel cleansed."));
        return;
    }
    int num_cured = rng( 1, valid.size() );
    num_cured = std::min( 4, num_cured );
    for( int i = 0; i < num_cured && !valid.empty(); i++ ) {
        const std::string id = random_entry_removed( valid );
        if( p->purifiable( id ) ) {
            p->remove_mutation( id );
        } else {
            p->add_msg_if_player(m_warning, _("You feel a slight itching inside, but it passes."));
        }
    }
}

int iuse::purifier(player *p, item *it, bool, const tripoint& )
{
    if (p->has_trait("MUTAGEN_AVOID")) {
         //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time?  uh-uh.  You're not drinking that chemical stuff."));
        return 0;
    }

    if (!p->is_npc() && !(p->has_trait("THRESH_MYCUS"))) {
        p->add_memorial_log(pgettext("memorial_male", "Consumed purifier."),
                            pgettext("memorial_female", "Consumed purifier."));
    }

    if( marloss_reject_mutagen( p, it ) ) {
        it->type->charges_to_use();
    }
    do_purify( p );
    return it->type->charges_to_use();
}

int iuse::purify_iv(player *p, item *it, bool, const tripoint& )
{
    if (p->has_trait("MUTAGEN_AVOID")) {
         //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time?  uh-uh.  You're not injecting that chemical stuff."));
        return 0;
    }

    if( !(p->has_trait("THRESH_MYCUS")) ) {
        p->add_memorial_log(pgettext("memorial_male", "Injected purifier."),
                            pgettext("memorial_female", "Injected purifier."));
    }

    if( marloss_reject_mut_iv( p, it ) ) {
        return it->type->charges_to_use();
    }

    std::vector<std::string> valid; // Which flags the player has
    for( auto &traits_iter : mutation_branch::get_all() ) {
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
        const std::string id = random_entry_removed( valid );
        if (p->purifiable( id )) {
            p->remove_mutation( id );
        } else {
            p->add_msg_if_player(m_warning, _("You feel a distinct burning inside, but it passes."));
        }
        if (!(p->has_trait("NOPAIN"))) {
            p->mod_pain(2 * num_cured); //Hurts worse as it fixes more
            p->add_msg_if_player(m_warning, _("Feels like you're on fire, but you're OK."));
        }
        p->thirst += 2 * num_cured;
        p->mod_hunger(2 * num_cured);
        p->fatigue += 2 * num_cured;
    }
    return it->type->charges_to_use();
}

void spawn_spores( const player &p ) {
    int spores_spawned = 0;
    for( const tripoint &dest : closest_tripoints_first( 4, p.pos() ) ) {
        if( g->m.impassable( dest ) ) {
            continue;
        }
        float dist = trig_dist( dest, p.pos() );
        if( x_in_y( 1, dist ) ) {
            g->m.marlossify( dest );
        }
        if( g->critter_at(dest) != nullptr ) {
            continue;
        }
        if( one_in( 10 + 5 * dist ) && one_in( spores_spawned * 2 ) ) {
            if( g->summon_mon( mon_spore, dest ) ) {
                monster *spore = g->monster_at( dest );
                spore->friendly = -1;
                spores_spawned++;
            }
        }
    }
}

int iuse::marloss(player *p, item *it, bool t, const tripoint &pos)
{
    if (p->is_npc()) {
        return it->type->charges_to_use();
    }
    if (p->has_trait("MARLOSS_AVOID")) {
        //~"Uh-uh" is a sound used for "nope", "no", etc.
        p->add_msg_if_player(m_warning, _("After what happened that last time?  uh-uh.  You're not eating that alien poison sac."));
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
        p->set_hunger(-100);
        spawn_spores(*p);

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
     * 9-16 - Give Marloss mutation
     */
    int effect = rng(1, 16);
    if (effect <= 3) {
        p->add_msg_if_player(_("This berry tastes extremely strange!"));
        p->mutate();
        // Gruss dich, mutation drain, missed you!
        p->mod_pain(2 * rng(1, 5));
        p->mod_hunger(10);
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
        p->set_hunger(-100);
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
        p->hurtall(rng(40, 65), nullptr);// No good way to say "lose half your current HP"
        ///\EFFECT_INT slightly reduces sleep duration when eating mycus+goo
        p->fall_asleep((6000 - p->int_cur * 10)); // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        p->unset_mutation("MARLOSS_BLUE");
        p->unset_mutation("MARLOSS");
        p->set_mutation("MARLOSS_AVOID"); // And if you survive it's etched in your RNA, so you're unlikely to repeat the experiment.
        p->rem_addiction(ADD_MARLOSS_R);
        p->rem_addiction(ADD_MARLOSS_B);
        p->rem_addiction(ADD_MARLOSS_Y);
    } else if ( (p->has_trait("MARLOSS_BLUE") && p->has_trait("MARLOSS_YELLOW")) && (!p->has_trait("MARLOSS")) ) {
        p->add_msg_if_player(m_bad, _("You feel a familiar warmth, but suddenly it surges into painful burning as you convulse and collapse to the ground..."));
        p->fall_asleep((400 - p->int_cur * 5));
        p->unset_mutation("MARLOSS_BLUE");
        p->unset_mutation("MARLOSS_YELLOW");
        p->set_mutation("THRESH_MARLOSS");
        p->rem_addiction(ADD_MARLOSS_R);
        g->m.ter_set(p->posx(), p->posy(), t_marloss);
        p->add_memorial_log(pgettext("memorial_male", "Opened the Marloss Gateway."),
                        pgettext("memorial_female", "Opened the Marloss Gateway."));
        p->add_msg_if_player(m_good, _("You wake up in a marloss bush.  Almost *cradled* in it, actually, as though it grew there for you."));
        //~ Beginning to hear the Mycus while conscious: that's it speaking
        p->add_msg_if_player(m_good, _("unity.  together we have reached the door.  we provide the final key.  now to pass through..."));
    } else if (!p->has_trait("MARLOSS")) {
        p->add_msg_if_player(_("You feel a strange warmth spreading throughout your body..."));
        p->set_mutation("MARLOSS");
        p->add_addiction(ADD_MARLOSS_B, 60);
        p->add_addiction(ADD_MARLOSS_Y, 60);
        p->rem_addiction(ADD_MARLOSS_R);
    }
    return it->type->charges_to_use();
}

int iuse::marloss_seed(player *p, item *it, bool t, const tripoint &pos)
{
    if (p->is_npc()) {
        return it->type->charges_to_use();
    }
    if (p->has_trait("MARLOSS_AVOID")) {
        //~"Uh-uh" is a sound used for "nope", "no", etc.  "Drek" is a borrowed synonym for "shit".
        p->add_msg_if_player(m_warning, _("After what happened that last time?  uh-uh.  You're not eating that alien drek."));
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
        p->set_hunger(-100);
        spawn_spores(*p);

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
        p->mod_hunger(10);
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
        p->set_hunger(-100);
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
        p->hurtall(rng(40, 65), nullptr);// No good way to say "lose half your current HP"
        ///\EFFECT_INT slightly reduces sleep duration when eating mycus+goo
        p->fall_asleep((6000 - p->int_cur * 10)); // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        p->unset_mutation("MARLOSS_BLUE");
        p->unset_mutation("MARLOSS");
        p->set_mutation("MARLOSS_AVOID"); // And if you survive it's etched in your RNA, so you're unlikely to repeat the experiment.
        p->rem_addiction(ADD_MARLOSS_R);
        p->rem_addiction(ADD_MARLOSS_B);
        p->rem_addiction(ADD_MARLOSS_Y);
    } else if ( (p->has_trait("MARLOSS") && p->has_trait("MARLOSS_YELLOW")) && (!p->has_trait("MARLOSS_BLUE")) ) {
        p->add_msg_if_player(m_bad, _("You feel a familiar warmth, but suddenly it surges into painful burning as you convulse and collapse to the ground..."));
        ///\EFFECT_INT reduces sleep duration when eating wrong color marloss
        p->fall_asleep((400 - p->int_cur * 5));
        p->unset_mutation("MARLOSS");
        p->unset_mutation("MARLOSS_YELLOW");
        p->set_mutation("THRESH_MARLOSS");
        p->rem_addiction(ADD_MARLOSS_B);
        g->m.ter_set(p->posx(), p->posy(), t_marloss);
        p->add_memorial_log(pgettext("memorial_male", "Opened the Marloss Gateway."),
                        pgettext("memorial_female", "Opened the Marloss Gateway."));
        p->add_msg_if_player(m_good, _("You wake up in a marloss bush.  Almost *cradled* in it, actually, as though it grew there for you."));
        //~ Beginning to hear the Mycus while conscious: that's it speaking
        p->add_msg_if_player(m_good, _("unity.  together we have reached the door.  we provide the final key.  now to pass through..."));
    } else if (!p->has_trait("MARLOSS_BLUE")) {
        p->add_msg_if_player(_("You feel a strange warmth spreading throughout your body..."));
        p->set_mutation("MARLOSS_BLUE");
        p->add_addiction(ADD_MARLOSS_R, 60);
        p->add_addiction(ADD_MARLOSS_Y, 60);
        p->rem_addiction(ADD_MARLOSS_B);
    }
    return it->type->charges_to_use();
}

int iuse::marloss_gel(player *p, item *it, bool t, const tripoint &pos)
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
        p->set_hunger(-100);
        spawn_spores(*p);

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
        p->mod_hunger(10);
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
        p->set_hunger(-100);
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
        p->hurtall(rng(40, 65), nullptr);// No good way to say "lose half your current HP"
        ///\EFFECT_INT slightly reduces sleep duration when eating mycus+goo
        p->fall_asleep((6000 - p->int_cur * 10)); // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        p->unset_mutation("MARLOSS_BLUE");
        p->unset_mutation("MARLOSS");
        p->set_mutation("MARLOSS_AVOID"); // And if you survive it's etched in your RNA, so you're unlikely to repeat the experiment.
        p->rem_addiction(ADD_MARLOSS_R);
        p->rem_addiction(ADD_MARLOSS_B);
        p->rem_addiction(ADD_MARLOSS_Y);
    } else if ( (p->has_trait("MARLOSS_BLUE") && p->has_trait("MARLOSS")) && (!p->has_trait("MARLOSS_YELLOW")) ) {
        p->add_msg_if_player(m_bad, _("You feel a familiar warmth, but suddenly it surges into painful burning as you convulse and collapse to the ground..."));
        ///\EFFECT_INT slightly reduces sleep duration when eating wrong color marloss
        p->fall_asleep((400 - p->int_cur * 5));
        p->unset_mutation("MARLOSS_BLUE");
        p->unset_mutation("MARLOSS");
        p->rem_addiction(ADD_MARLOSS_Y);
        p->set_mutation("THRESH_MARLOSS");
        g->m.ter_set(p->posx(), p->posy(), t_marloss);
        p->add_memorial_log(pgettext("memorial_male", "Opened the Marloss Gateway."),
                        pgettext("memorial_female", "Opened the Marloss Gateway."));
        p->add_msg_if_player(m_good, _("You wake up in a marloss bush.  Almost *cradled* in it, actually, as though it grew there for you."));
        //~ Beginning to hear the Mycus while conscious: that's it speaking
        p->add_msg_if_player(m_good, _("unity.  together we have reached the door.  we provide the final key.  now to pass through..."));
    } else if (!p->has_trait("MARLOSS_YELLOW")) {
        p->add_msg_if_player(_("You feel a strange warmth spreading throughout your body..."));
        p->set_mutation("MARLOSS_YELLOW");
        p->add_addiction(ADD_MARLOSS_R, 60);
        p->add_addiction(ADD_MARLOSS_B, 60);
        p->rem_addiction(ADD_MARLOSS_Y);
    }
    return it->type->charges_to_use();
}

int iuse::mycus(player *p, item *it, bool t, const tripoint &pos)
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
        ///\EFFECT_INT slightly reduces sleep duration when eating mycus
        p->fall_asleep((3000 - p->int_cur * 10));
        p->unset_mutation("THRESH_MARLOSS");
        p->set_mutation("THRESH_MYCUS");
        //~ The Mycus does not use the term (or encourage the concept of) "you".  The PC is a local/native organism, but is now the Mycus.
        //~ It still understands the concept, but uninitelligent fungaloids and mind-bent symbiotes should not need it.
        //~ We are the Mycus.
        p->add_msg_if_player(m_good, _("We welcome into us.  We have endured long in this forbidding world."));
        p->add_msg_if_player(m_good, _("The natives have a saying: \"E Pluribus Unum\"  Out of many, one."));
        p->add_msg_if_player(m_good, _("We welcome the union of our lines in our local guide.  We will prosper, and unite this world."));
        p->add_msg_if_player(m_good, _("Even now, our fruits adapt to better serve local physiology."));
        p->add_msg_if_player(m_good, _("As, in time, shall we adapt to better welcome those who have not received us."));
        for (int x = p->posx() - 3; x <= p->posx() + 3; x++) {
            for (int y = p->posy() - 3; y <= p->posy() + 3; y++) {
                g->m.marlossify( tripoint( x, y, p->posz() ) );
            }
        }
        p->rem_addiction(ADD_MARLOSS_R);
        p->rem_addiction(ADD_MARLOSS_B);
        p->rem_addiction(ADD_MARLOSS_Y);
    }
    else if (p->has_trait("THRESH_MYCUS") && !p->has_trait("M_DEPENDENT")) { // OK, now set the hook.
        if (!one_in(3)) {
            p->mutate_category("MUTCAT_MYCUS");
            p->mod_hunger(10);
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
        p->mod_hunger(10);
        p->fatigue += 5;
        p->thirst += 10;
        p->vomit(); // no hunger/quench benefit for you
        p->mod_healthy_mod(-8, -50);
    }
    return it->type->charges_to_use();
}

// TOOLS below this point!

int petfood(player *p, item *it, bool is_dogfood)
{
    tripoint dirp;
    if (!choose_adjacent(string_format(_("Put the %s where?"), it->tname().c_str()), dirp)) {
        return 0;
    }
    p->moves -= 15;
    int mon_dex = g->mon_at( dirp, true );
    if (mon_dex != -1) {
        if (g->zombie(mon_dex).type->id == (is_dogfood ? mon_dog : mon_cat)) {
            p->add_msg_if_player(m_good, is_dogfood
              ? _("The dog seems to like you!")
              : _("The cat seems to like you!  Or maybe it just tolerates your presence better.  It's hard to tell with cats."));
            g->zombie(mon_dex).friendly = -1;
            if (is_dogfood) {
                g->zombie(mon_dex).add_effect("pet", 1, num_bp, true);
            }
        } else {
            p->add_msg_if_player(_("The %s seems quite unimpressed!"),
                                 g->zombie(mon_dex).name().c_str());
        }
    } else {
        p->add_msg_if_player(m_bad, _("You spill the %s all over the ground."), it->tname().c_str());
    }
    return 1;
}

int iuse::dogfood(player *p, item *it, bool, const tripoint& )
{
    return petfood(p, it, true);
}

int iuse::catfood(player *p, item *it, bool, const tripoint& )
{
    return petfood(p, it, false);
}

static bool is_firearm(const item &it)
{
    return it.is_gun() && !it.has_flag("PRIMITIVE_RANGED_WEAPON");
}

int iuse::sew_advanced(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        return 0;
    }

    if( p->is_underwater() ) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }

    if( p->fine_detail_vision_mod() > 4 ) {
        add_msg(m_info, _("You can't see to sew!"));
        return 0;
    }

    int pos = g->inv_for_filter( _("Enhance what?"), []( const item & itm ) {
            return itm.made_of( "cotton" ) ||
            itm.made_of( "leather" ) ||
            itm.made_of( "fur" ) ||
            itm.made_of( "nomex" ) ||
            itm.made_of( "plastic" ) ||
            itm.made_of( "kevlar" ) ||
            itm.made_of( "wool" );
        } );
    item *mod = &(p->i_at(pos));
    if (mod == NULL || mod->is_null()) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }

    if( !mod->is_armor() ) {
        p->add_msg_if_player(m_info, _("You can only tailor your clothes!"));
        return 0;
    }
    if (is_firearm(*mod)){
        p->add_msg_if_player(m_info, _("You can't use a tailor's kit on a firearm!"));
        return 0;
    }
    if (mod->is_power_armor()){
        p->add_msg_if_player(m_info, _("You can't modify your power armor!"));
        return 0;
    }

    std::vector<std::string> plurals;
    std::vector<itype_id> repair_items;
    std::string plural = "";
    //translation note: add <plural> tag to keep them unique

    // Little helper to cut down some surplus redundancy and repetition
    const auto add_material = [&]( const itype_id &material,
                                   const itype_id &mat_item,
                                   const std::string &plural ) {
        if( mod->made_of( material ) ) {
            repair_items.push_back( mat_item );
            plurals.push_back( rm_prefix( plural.c_str() ) );
        }
    };

    add_material( "cotton", "rag", _( "<plural>rags" ) );
    add_material( "leather", "leather", _( "<plural>leather" ) );
    add_material( "fur", "fur", _( "<plural>fur" ) );
    add_material( "nomex", "nomex", _( "<plural>Nomex" ) );
    add_material( "plastic", "plastic_chunk", _( "<plural>plastic" ) );
    add_material( "kevlar", "kevlar_plate", _( "<plural>Kevlar" ) );
    add_material( "wool", "felt_patch", _( "<plural>wool" ) );
    if (repair_items.empty()) {
        p->add_msg_if_player(m_info, _("Your %s is not made of fabric, leather, fur, Kevlar, wool or plastic."),
                             mod->tname().c_str());
        return 0;
    }
    if( mod == it || std::find(repair_items.begin(), repair_items.end(),
                               mod->typeId()) != repair_items.end()) {
        p->add_msg_if_player(m_info, _("This can be used to repair or modify other items, not itself."));
        return 0;
    }

    // Gives us an item with the mod added or removed (toggled)
    const auto modded_copy = []( const item *proto, const std::string &mod_type ) {
        item mcopy = *proto;
        if( mcopy.item_tags.count( mod_type ) == 0 ) {
            mcopy.item_tags.insert( mod_type );
        } else {
            mcopy.item_tags.erase( mod_type );
        }

        return mcopy;
    };

    // TODO: Wrap all the mods into structs, maybe even json-able
    // All possible mods here
    std::array<std::string, 4> clothing_mods{
        { "wooled", "furred", "leather_padded", "kevlar_padded" }
    };

    // Materials those mods use
    std::array<std::string, 4> mod_materials{
        { "felt_patch", "fur", "leather", "kevlar_plate" }
    };

    // Cache available materials
    std::map< itype_id, bool > has_enough;
    const int items_needed = ( ( ( mod->volume() ) / 3 ) + 1 );
    const inventory &crafting_inv = p->crafting_inventory();
    // Go through all discovered repair items and see if we have any of them available
    for( auto &material : mod_materials ) {
        has_enough[material] = crafting_inv.has_amount( material, items_needed );
    }

    const int mod_count = mod->item_tags.count("wooled") + mod->item_tags.count("furred") +
                          mod->item_tags.count("leather_padded") + mod->item_tags.count("kevlar_padded");

    // We need extra thread to lose it on bad rolls
    const int thread_needed = mod->volume() * 2 + 10;
    // Returns true if the item already has the mod or if we have enough materials and thread to add it
    const auto can_add_mod = [&]( const std::string &new_mod, const itype_id &mat_item ) {
        return mod->item_tags.count( new_mod ) > 0 ||
            ( it->charges >= thread_needed && has_enough[mat_item] );
    };

    uimenu tmenu;
    // TODO: Tell how much thread will we use
    if( it->charges >= thread_needed ) {
        tmenu.text = _("How do you want to modify it?");
    } else {
        tmenu.text = _("Not enough thread to modify. Which modification do you want to remove?");
    }

    // TODO: The supremely ugly block of code below looks better than 200 line boilerplate
    // that was there before, but it can probably be moved into a helper somehow

    // TODO 2: List how much material we have and how much we need
    item temp_item = modded_copy( mod, "wooled" );
    // Can we perform this addition or removal
    bool enab = can_add_mod( "wooled", "felt_patch" );
    tmenu.addentry( 0, enab, MENU_AUTOASSIGN, _("%s (Warmth: %d->%d, Encumbrance: %d->%d)"),
        mod->item_tags.count("wooled") == 0 ? _("Line it with wool") : _("Destroy wool lining"),
        mod->get_warmth(), temp_item.get_warmth(), mod->get_encumber(), temp_item.get_encumber() );

    temp_item = modded_copy( mod, "furred" );
    enab = can_add_mod( "furred", "fur" );
    tmenu.addentry( 1, enab, MENU_AUTOASSIGN, _("%s (Warmth: %d->%d, Encumbrance: %d->%d)"),
        mod->item_tags.count("furred") == 0 ? _("Line it with fur") : _("Destroy fur lining"),
        mod->get_warmth(), temp_item.get_warmth(), mod->get_encumber(), temp_item.get_encumber() );

    temp_item = modded_copy( mod, "leather_padded" );
    enab = can_add_mod( "leather_padded", "leather" );
    tmenu.addentry( 2, enab, MENU_AUTOASSIGN, _("%s (Bash/Cut: %d/%d->%d/%d, Encumbrance: %d->%d)"),
        mod->item_tags.count("leather_padded") == 0 ? _("Pad with leather") : _("Destroy leather padding"),
        mod->bash_resist(), mod->cut_resist(), temp_item.bash_resist(), temp_item.cut_resist(),
        mod->get_encumber(), temp_item.get_encumber() );

    temp_item = modded_copy( mod, "kevlar_padded" );
    enab = can_add_mod( "kevlar_padded", "kevlar_plate" );
    tmenu.addentry( 3, enab, MENU_AUTOASSIGN, _("%s (Bash/Cut: %d/%d->%d/%d, Encumbrance: %d->%d)"),
        mod->item_tags.count("kevlar_padded") == 0 ? _("Pad with Kevlar") : _("Destroy Kevlar padding"),
        mod->bash_resist(), mod->cut_resist(), temp_item.bash_resist(), temp_item.cut_resist(),
        mod->get_encumber(), temp_item.get_encumber() );

    tmenu.addentry( 4, true, 'q', _("Cancel") );

    tmenu.query();
    const int choice = tmenu.ret;

    if( choice < 0 || choice > 3 ) {
        return 0;
    }

    // The mod player picked
    const std::string &the_mod = clothing_mods[choice];

    // If the picked mod already exists, player wants to destroy it
    if( mod->item_tags.count( the_mod ) ) {
        if( query_yn( _("Are you sure?  You will not gain any materials back.") ) ) {
            mod->item_tags.erase( the_mod );
        }

        return 0;
    }

    // Get the id of the material used
    const auto &repair_item = mod_materials[choice];

    std::vector<item_comp> comps;
    comps.push_back( item_comp( repair_item, items_needed ) );
    p->moves -= 500 * p->fine_detail_vision_mod();
    p->practice( skill_tailor, items_needed * 3 + 3 );
    ///\EFFECT_TAILOR randomly improves clothing modifiation efforts
    int rn = dice( 3, 2 + p->skillLevel( skill_tailor ) ); // Skill
    ///\EFFECT_DEX randomly improves clothing modification efforts
    rn += rng( 0, p->dex_cur / 2 );                    // Dexterity
    ///\EFFECT_PER randomly improves clothing modification efforts
    rn += rng( 0, p->per_cur / 2 );                    // Perception
    rn -= mod_count * 10;                              // Other mods

    if( rn <= 8 ) {
        p->add_msg_if_player(m_bad, _("You damage your %s trying to modify it!"),
                             mod->tname().c_str());
        mod->damage++;
        if( mod->damage >= 5 ) {
            p->add_msg_if_player(m_bad, _("You destroy it!"));
            p->i_rem_keep_contents( pos );
        }
        return thread_needed / 2;
    } else if( rn <= 10 ) {
        p->add_msg_if_player( m_bad,
                              _("You fail to modify the clothing, and you waste thread and materials.") );
        p->consume_items( comps );
        return thread_needed;
    } else if( rn <= 14 ) {
        p->add_msg_if_player( m_mixed, _("You modify your %s, but waste a lot of thread."),
                              mod->tname().c_str() );
        p->consume_items( comps );
        mod->item_tags.insert( the_mod );
        return thread_needed;
    }

    p->add_msg_if_player( m_good, _("You modify your %s!"), mod->tname().c_str() );
    mod->item_tags.insert( the_mod );
    p->consume_items( comps );
    return thread_needed / 2;
}

void remove_battery_mods( item &modded, player &p )
{
    remove_atomic_mod( modded, p );
    remove_recharge_mod( modded, p );
    remove_ups_mod( modded, p );
    remove_double_ammo_mod( modded, p );
    remove_double_plut_mod( modded, p );
}

int iuse::extra_battery(player *p, item *, bool, const tripoint& )
{
    int inventory_index = g->inv_for_filter( _("Modify what?"), []( const item & itm ) {
        const auto tl = dynamic_cast<const it_tool *>(itm.type);
        return tl != nullptr && tl->ammo_id == "battery";
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

    const auto tool = dynamic_cast<const it_tool *>(modded->type);
    if (tool->ammo_id != "battery") {
        p->add_msg_if_player(m_info, _("That item does not use batteries!"));
        return 0;
    }

    if (modded->has_flag("DOUBLE_AMMO")) {
        p->add_msg_if_player(m_info, _("That item has already had its battery capacity doubled."));
        return 0;
    }

    remove_battery_mods( *modded, *p );

    p->add_msg_if_player( _( "You double the battery capacity of your %s!" ), modded->tname().c_str() );
    modded->item_tags.insert("DOUBLE_AMMO");
    return 1;
}

int iuse::double_reactor(player *p, item *, bool, const tripoint& )
{
    int inventory_index = g->inv_for_filter( _("Modify what?"), []( const item & itm ) {
        const auto tl = dynamic_cast<const it_tool *>(itm.type);
        return tl != nullptr && tl->ammo_id == "plutonium";
    } );
    item *modded = &( p->i_at( inventory_index ) );

    if (modded == NULL || modded->is_null()) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    if (!modded->is_tool()) {
        p->add_msg_if_player(m_info, _("This device can only be used on tools."));
        return 0;
    }

    const auto tool = dynamic_cast<const it_tool *>(modded->type);
    if (tool->ammo_id != "plutonium") {
        p->add_msg_if_player(m_info, _("That item does not use plutonium!"));
        return 0;
    }

    p->add_msg_if_player( _( "You double the plutonium capacity of your %s!" ), modded->tname().c_str() );
    modded->item_tags.insert("DOUBLE_AMMO");
    modded->item_tags.insert("DOUBLE_REACTOR");   //This flag lets the remove_ functions know that this is a plutonium tool without taking extra steps.
    return 1;
}

int iuse::rechargeable_battery(player *p, item *it, bool, const tripoint& )
{
    int inventory_index = g->inv_for_filter( _("Modify what?"), []( const item & itm ) {
        const auto tl = dynamic_cast<const it_tool *>(itm.type);
        return tl != nullptr && tl->ammo_id == "battery";
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

    const auto tool = dynamic_cast<const it_tool *>(modded->type);
    if (tool->ammo_id != "battery") {
        p->add_msg_if_player(m_info, _("That item does not use batteries!"));
        return 0;
    }

    if (modded->has_flag("RECHARGE")) {
        p->add_msg_if_player(m_info, _("That item already has a rechargeable battery pack."));
        return 0;
    }

    remove_battery_mods( *modded, *p );
    remove_ammo( modded, *p ); // remove batteries, replaced by charges from mod

    p->add_msg_if_player( _( "You replace the battery compartment of your %s with a rechargeable battery pack!" ), modded->tname().c_str() );
    modded->charges = it->charges;
    modded->item_tags.insert("RECHARGE");
    modded->item_tags.insert("NO_UNLOAD");
    modded->item_tags.insert("NO_RELOAD");
    return 1;
}

int iuse::atomic_battery(player *p, item *it, bool, const tripoint& )
{
    int inventory_index = g->inv_for_filter( _("Modify what?"), []( const item & itm ) {
        const auto tl = dynamic_cast<const it_tool *>(itm.type);
        return tl != nullptr && tl->ammo_id == "battery";
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

    const auto tool = dynamic_cast<const it_tool *>(modded->type);
    if (modded->has_flag("ATOMIC_AMMO")) {
        p->add_msg_if_player(m_info,
                             _("That item has already had its battery modified to accept plutonium cells."));
        return 0;
    }
    if (tool->ammo_id != "battery") {
        p->add_msg_if_player(m_info, _("That item does not use batteries!"));
        return 0;
    }


    remove_battery_mods( *modded, *p );
    remove_ammo( modded, *p ); // remove batteries, item::charges is now plutonium

    p->add_msg_if_player( _( "You modify your %s to run off plutonium cells!" ), modded->tname().c_str() );
    modded->item_tags.insert("ATOMIC_AMMO");
    modded->item_tags.insert("RADIOACTIVE");
    modded->item_tags.insert("LEAK_DAM");
    modded->item_tags.insert("NO_UNLOAD");
    modded->charges = it->charges;
    return 1;
}
int iuse::ups_battery(player *p, item *, bool, const tripoint& )
{
    int inventory_index = g->inv_for_filter( _("Modify what?"), []( const item & itm ) {
        const auto tl = dynamic_cast<const it_tool *>(itm.type);
        return tl != nullptr && tl->ammo_id == "battery";
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

    const auto tool = dynamic_cast<const it_tool *>(modded->type);
    if (tool->ammo_id != "battery") {
        p->add_msg_if_player(_("That item does not use batteries!"));
        return 0;
    }

    if (modded->has_flag("USE_UPS")) {
        p->add_msg_if_player(_("That item has already had its battery modified to use a UPS!"));
        return 0;
    }
    if( modded->typeId() == "UPS_off" || modded->typeId() == "adv_UPS_off" ) {
        p->add_msg_if_player( _( "You want to power a UPS with another UPS?  Very clever." ) );
        return 0;
    }

    remove_battery_mods( *modded, *p );
    remove_ammo( modded, *p );

    p->add_msg_if_player( _( "You modify your %s to run off a UPS!" ), modded->tname().c_str() );
    modded->item_tags.insert("USE_UPS");
    modded->item_tags.insert("NO_UNLOAD");
    modded->item_tags.insert("NO_RELOAD");
    //Perhaps keep the modded charges at 1 or 0?
    modded->charges = 0;
    return 1;
}

int iuse::radio_mod( player *p, item *, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // Now THAT would be kinda cruel
        return 0;
    }

    int inventory_index = g->inv_for_filter( _("Modify what?"), []( const item & itm ) {
        return itm.is_tool() && itm.has_flag( "RADIO_MODABLE" );
    } );
    item &modded = p->i_at( inventory_index );

    if( modded.is_null() ) {
        p->add_msg_if_player(_("You do not have that item!"));
        return 0;
    }
    if( !modded.is_tool() || !modded.has_flag( "RADIO_MODABLE" ) ) {
        p->add_msg_if_player(_("This item can't be made modified this way."));
        return 0;
    }

    int choice = menu( true, _("Which signal should activate the item?:"), _("\"Red\""),
                      _("\"Blue\""), _("\"Green\""), _("Cancel"), nullptr );

    std::string newtag;
    std::string colorname;
    switch( choice ) {
        case 1:
            newtag = "RADIOSIGNAL_1";
            colorname = _("\"Red\"");
            break;
        case 2:
            newtag = "RADIOSIGNAL_2";
            colorname = _("\"Blue\"");
            break;
        case 3:
            newtag = "RADIOSIGNAL_3";
            colorname = _("\"Green\"");
            break;
        default:
            return 0;
    }

    if( modded.has_flag( "RADIO_MOD" ) && modded.has_flag( newtag ) ) {
        p->add_msg_if_player(_("This item has been modified this way already."));
        return 0;
    }

    remove_radio_mod( modded, *p );

    p->add_msg_if_player( _( "You modify your %1$s to listen for %2$s activation signal on the radio." ),
                          modded.tname().c_str(), colorname.c_str() );
    modded.item_tags.insert( "RADIO_ACTIVATION" );
    modded.item_tags.insert( "RADIOCARITEM" );
    modded.item_tags.insert( "RADIO_MOD" );
    modded.item_tags.insert( newtag );
    return 1;
}

int iuse::remove_all_mods(player *p, item *, bool, const tripoint& )
{
    int inventory_index = g->inv_for_filter( _( "Detach power mods from what?" ), []( const item & itm ) {
        const auto tl = dynamic_cast<const it_tool *>(itm.type);
        return tl != nullptr && ( itm.has_flag("DOUBLE_AMMO") || itm.has_flag("RECHARGE") ||
                                  itm.has_flag("USE_UPS") || itm.has_flag("ATOMIC_AMMO") );
    } );
    item *modded = &( p->i_at( inventory_index ) );
    if (modded == NULL || modded->is_null()) {
        p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
        return 0;
    }

    if (!modded->is_tool()) {
        p->add_msg_if_player( m_info, _( "Only power mods for tools can be removed this way." ) );
        return 0;
    }

    remove_battery_mods( *modded, *p );
    remove_radio_mod( *modded, *p );
    return 0;
}

int iuse::fishing_rod(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // Long actions - NPCs don't like those yet
        return 0;
    }

    int dirx, diry;

    if (!choose_adjacent(_("Fish where?"), dirx, diry)) {
        return 0;
    }

    if (!g->m.has_flag("FISHABLE", dirx, diry)) {
        p->add_msg_if_player(m_info, _("You can't fish there!"));
        return 0;
    }
    point op = overmapbuffer::ms_to_omt_copy( g->m.getabs( dirx, diry ) );
    if( !otermap[overmap_buffer.ter(op.x, op.y, g->get_levz())].has_flag(river_tile) ) {
        p->add_msg_if_player(m_info, _("That water does not contain any fish.  Try a river instead."));
        return 0;
    }
    std::vector<monster*> fishables = g->get_fishable(60);
    if ( fishables.size() < 1){
        p->add_msg_if_player(m_info, _("There are no fish around.  Try another spot.")); // maybe let the player find that out by himself?
        return 0;
    }
    p->rooted_message();

    p->add_msg_if_player(_("You cast your line and wait to hook something..."));

    p->assign_activity(ACT_FISH, 30000, 0, p->get_item_position(it), it->tname());

    return 0;
}

int iuse::fish_trap(player *p, item *it, bool t, const tripoint &pos)
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
            p->add_msg_if_player(_("Fish are not foolish enough to go in here without bait."));
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
        if( !otermap[overmap_buffer.ter(op.x, op.y, g->get_levz())].has_flag(river_tile) ) {
            p->add_msg_if_player(m_info, _("That water does not contain any fish, try a river instead."));
            return 0;
        }
        std::vector<monster*> fishables = g->get_fishable(60);
        if ( fishables.size() < 1){
            p->add_msg_if_player(m_info, _("There is no fish around.  Try another spot.")); // maybe let the player find that out by himself?
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

            if (!g->m.has_flag("FISHABLE", pos)) {
                return 0;
            }
            point op = overmapbuffer::ms_to_omt_copy( g->m.getabs( pos.x, pos.y ) );
           if( !otermap[overmap_buffer.ter(op.x, op.y, g->get_levz())].has_flag(river_tile) ) {
                return 0;
            }
            int success = -50;
            const int surv = p->skillLevel( skill_survival );
            const int attempts = rng(it->charges, it->charges * it->charges);
            for (int i = 0; i < attempts; i++) {
                ///\EFFECT_SURVIVAL randomly increases number of fish caught in fishing trap
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
                p->practice( skill_survival, rng(5, 15));

                return 0;
            }
            std::vector<monster*> fishables = g->get_fishable(60); //get the fishables around the trap's spot
            for (int i = 0; i < fishes; i++) {
                p->practice( skill_survival, rng(3, 10));
                if (fishables.size() > 1){
                    g->catch_a_monster(fishables, pos, p, 180000); //catch the fish! 180000 is the time spent fishing.
                } else {
                    //there will always be a chance that the player will get lucky and catch a fish
                    //not existing in the fishables vector. (maybe it was in range, but wandered off)
                    //lets say it is a 5% chance per fish to catch
                    if (one_in(20)) {
                        item fish;
                        const std::vector<mtype_id> fish_group = MonsterGroupManager::GetMonstersFromGroup( mongroup_id( "GROUP_FISH" ) );
                        const mtype_id& fish_mon = fish_group[rng(1, fish_group.size()) - 1];
                        fish.make_corpse( fish_mon, it->bday + rng(0, 1800)); //we don't know when it was caught. its random
                        //Yes, we can put fishes in the trap like knives in the boot,
                        //and then get fishes via activation of the item,
                        //but it's not as comfortable as if you just put fishes in the same tile with the trap.
                        //Also: corpses and comestibles do not rot in containers like this, but on the ground they will rot.
                        g->m.add_item_or_charges( pos, fish );
                        break; //this can happen only once
                    }
                }
            }
        }
        return 0;
    }
}

int iuse::extinguisher(player *p, item *it, bool, const tripoint& )
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    g->draw();
    tripoint dest;
    // If anyone other than the player wants to use one of these,
    // they're going to need to figure out how to aim it.
    if (!choose_adjacent(_("Spray where?"), dest)) {
        return 0;
    }

    p->moves -= 140;

    // Reduce the strength of fire (if any) in the target tile.
    g->m.adjust_field_strength(dest, fd_fire, 0 - rng(2, 3));

    // Also spray monsters in that tile.
    int mondex = g->mon_at( dest, true );
    if (mondex != -1) {
        g->zombie(mondex).moves -= 150;
        bool blind = false;
        if (one_in(2) && g->zombie(mondex).has_flag(MF_SEES)) {
            blind = true;
            g->zombie(mondex).add_effect("blind", rng(10, 20));
        }
        if (g->u.sees(g->zombie(mondex))) {
            p->add_msg_if_player(_("The %s is sprayed!"), g->zombie(mondex).name().c_str());
            if(blind) {
                p->add_msg_if_player(_("The %s looks blinded."), g->zombie(mondex).name().c_str());
            }
        }
        if (g->zombie(mondex).made_of(LIQUID)) {
            if (g->u.sees(g->zombie(mondex))) {
                p->add_msg_if_player(_("The %s is frozen!"), g->zombie(mondex).name().c_str());
            }
            monster &critter = g->zombie( mondex );
            critter.apply_damage( p, bp_torso, rng( 20, 60 ) );
            critter.set_speed_base( critter.get_speed_base() / 2 );
        }
    }

    // Slightly reduce the strength of fire immediately behind the target tile.
    if (g->m.passable(dest)) {
        dest.x += (dest.x - p->posx());
        dest.y += (dest.y - p->posy());

        g->m.adjust_field_strength(dest, fd_fire, std::min(0 - rng(0, 1) + rng(0, 1), 0L));
    }

    return it->type->charges_to_use();
}

int iuse::rm13armor_off(player *p, item *it, bool, const tripoint& )
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

int iuse::rm13armor_on(player *p, item *it, bool t, const tripoint& )
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

int iuse::unpack_item(player *p, item *it, bool, const tripoint& )
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

int iuse::pack_item(player *p, item *it, bool t, const tripoint& )
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

static int cauterize_elec(player *p, item *it)
{
    if (it->charges == 0) {
        p->add_msg_if_player(m_info, _("You need batteries to cauterize wounds."));
        return 0;
    } else if (!p->has_effect("bite") && !p->has_effect("bleed") && !p->is_underwater()) {
        if ((p->has_trait("MASOCHIST") || p->has_trait("MASOCHIST_MED") || p->has_trait("CENOBITE")) &&
            p->query_yn(_("Cauterize yourself for fun?"))) {
            return cauterize_actor::cauterize_effect(p, it, true) ? it->type->charges_to_use() : 0;
        } else {
            p->add_msg_if_player(m_info,
                                 _("You are not bleeding or bitten, there is no need to cauterize yourself."));
            return 0;
        }
    } else if (p->is_npc() || query_yn(_("Cauterize any open wounds?"))) {
        return cauterize_actor::cauterize_effect(p, it, true) ? it->type->charges_to_use() : 0;
    }
    return 0;
}

int iuse::water_purifier(player *p, item *it, bool, const tripoint& )
{
    auto loc = g->inv_map_splice( []( const item & itm ) {
        return !itm.contents.empty() &&
               ( itm.contents[0].type->id == "water" ||
                 itm.contents[0].type->id == "salt_water" );
    }, _( "Purify what?" ), 1 );

    item *target = loc.get_item();
    if( target == nullptr ) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    if( target->contents.empty() ) {
        p->add_msg_if_player(m_info, _("You can only purify water."));
        return 0;
    }

    item *pure = &target->contents[0];
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

int iuse::two_way_radio(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // Getting NPC to talk to the radio could be cool.
        // But it isn't yet.
        return 0;
    }

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
            npc *coming = random_entry( in_range );
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

int iuse::radio_off(player *p, item *it, bool, const tripoint& )
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

int iuse::directional_antenna(player *p, item *it, bool, const tripoint& )
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
    const auto tref = overmap_buffer.find_radio_station( radio.frequency );
    if( !tref ) {
        add_msg(m_info, _("You can't find the direction if your radio isn't tuned."));
        return 0;
    }
    // Report direction.
    const auto player_pos = p->global_sm_location();
    direction angle = direction_from( player_pos.x, player_pos.y,
                                      tref.abs_sm_pos.x, tref.abs_sm_pos.y );
    add_msg(_("The signal seems strongest to the %s."), direction_name(angle).c_str());
    return it->type->charges_to_use();
}

int iuse::radio_on(player *p, item *it, bool t, const tripoint &pos)
{
    if (t) {
        // Normal use
        std::string message = _("Radio: Kssssssssssssh.");
        const auto tref = overmap_buffer.find_radio_station( it->frequency );
        if( tref ) {
            const auto selected_tower = tref.tower;
            if (selected_tower->type == MESSAGE_BROADCAST) {
                message = selected_tower->message;
            } else if (selected_tower->type == WEATHER_RADIO) {
                message = weather_forecast( tref.abs_sm_pos );
            }
            for( auto &elem : message ) {
                int signal_roll = dice(10, tref.signal_strength * 3);
                int static_roll = dice(10, 100);
                if (static_roll > signal_roll) {
                    if (static_roll < signal_roll * 1.1 && one_in(4)) {
                        elem = char( rng( 'a', 'z' ) );
                    } else {
                        elem = '#';
                    }
                }
            }

            std::vector<std::string> segments = foldstring(message, RADIO_PER_TURN);
            int index = calendar::once_every(segments.size());
            std::stringstream messtream;
            messtream << string_format(_("radio: %s"), segments[index].c_str());
            message = messtream.str();
        }
        sounds::ambient_sound(pos, 6, message.c_str());
    } else { // Activated
        int ch = 2;
        if (it->charges > 0) {
            ch = menu(true, _("Radio:"), _("Scan"), _("Turn off"), NULL);
        }

        switch (ch) {
            case 1: {
                const int old_frequency = it->frequency;
                const radio_tower *lowest_tower = nullptr;
                const radio_tower *lowest_larger_tower = nullptr;
                for( auto &tref : overmap_buffer.find_all_radio_stations() ) {
                    const auto new_frequency = tref.tower->frequency;
                    if( new_frequency == old_frequency ) {
                        continue;
                    }
                    if( new_frequency > old_frequency &&
                        ( lowest_larger_tower == nullptr || new_frequency < lowest_larger_tower->frequency)) {
                        lowest_larger_tower = tref.tower;
                    } else if( lowest_tower == nullptr || new_frequency < lowest_tower->frequency ) {
                        lowest_tower = tref.tower;
                    }
                }
                if( lowest_larger_tower != nullptr ) {
                    it->frequency = lowest_larger_tower->frequency;
                } else if( lowest_tower != nullptr ) {
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

int iuse::noise_emitter_off(player *p, item *it, bool, const tripoint& )
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

int iuse::noise_emitter_on(player *p, item *it, bool t, const tripoint &pos)
{
    if (t) { // Normal use
        //~ the sound of a noise emitter when turned on
        sounds::ambient_sound(pos, 30, _("KXSHHHHRRCRKLKKK!"));
    } else { // Turning it off
        p->add_msg_if_player(_("The infernal racket dies as the noise emitter turns off."));
        it->make("noise_emitter");
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::ma_manual(player *p, item *it, bool, const tripoint& )
{
    // [CR] - should NPCs just be allowed to learn this stuff? Just like that?

    // strip "manual_" from the start of the item id, add the rest to "style_"
    // TODO: replace this terrible hack to rely on the item name matching the style name, it's terrible.
    const matype_id style_to_learn( "style_" + it->type->id.substr(7) );

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
    p->practice( skill_carpentry, 1, 1);
    p->moves -= 500;
    g->m.spawn_item(p->posx(), p->posy(), "nail", 0, nails);
    g->m.spawn_item(p->posx(), p->posy(), "2x4", boards);
    g->m.ter_set(dirx, diry, newter);
    return true;
}

int iuse::hammer(player *p, item *it, bool, const tripoint& )
{
    g->draw();
    int x, y;
    // If anyone other than the player wants to use one of these,
    // they're going to need to figure out how to aim it.
    if (!choose_adjacent(_("Pry where?"), x, y)) {
        return 0;
    }

    if (x == p->posx() && y == p->posy()) {
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

int iuse::crowbar(player *p, item *it, bool, const tripoint &pos)
{
    // TODO: Make this 3D now that NPCs get to use items
    tripoint dirp = pos;
    if( pos == p->pos3() ) {
        if( !choose_adjacent(_("Pry where?"), dirp ) ) {
            return 0;
        }
    } // else it is already set to pos in the line above if

    int &dirx = dirp.x;
    int &diry = dirp.y;

    if( dirx == p->posx() && diry == p->posy() ) {
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
    } else if (type == t_window_domestic || type == t_curtains || type == t_window_no_curtains) {
        succ_action = _("You pry open the window.");
        fail_action = _("You pry, but cannot pry open the window.");
        new_type = (type == t_window_no_curtains) ? t_window_no_curtains_open : t_window_open;
        noisy = true;
        difficulty = 6;
    } else if (pry_nails(p, type, dirx, diry)) {
        return it->type->charges_to_use();
    } else {
        p->add_msg_if_player(m_info, _("There's nothing to pry there."));
        return 0;
    }

    p->practice( skill_mechanics, 1);
    ///\EFFECT_STR speeds up crowbar prying attempts

    ///\EFFECT_MECHANICS speeds up crowbar prying attempts
    p->moves -= std::max( 25, ( difficulty * 25 ) - ( ( p->str_cur + p->skillLevel( skill_mechanics ) ) * 5 ) );
    ///\EFFECT_STR increases chance of crowbar prying success

    ///\EFFECT_MECHANICS increases chance of crowbar prying success
    if (dice(4, difficulty) < dice(2, p->skillLevel( skill_mechanics )) + dice(2, p->str_cur)) {
        p->practice( skill_mechanics, 1);
        p->add_msg_if_player(m_good, succ_action);
        if (g->m.furn(dirx, diry) == f_crate_c) {
            g->m.furn_set(dirx, diry, f_crate_o);
        } else {
            g->m.ter_set(dirx, diry, new_type);
        }
        if (noisy) {
            sounds::sound(dirp, 12, _("crunch!"));
        }
        if (type == t_manhole_cover) {
            g->m.spawn_item(dirx, diry, "manhole_cover");
        }
        if (type == t_door_locked_alarm) {
            p->add_memorial_log(pgettext("memorial_male", "Set off an alarm."),
                                  pgettext("memorial_female", "Set off an alarm."));
            sounds::sound(p->pos(), 40, _("An alarm sounds!"));
            if (!g->event_queued(EVENT_WANTED)) {
                g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, p->global_sm_location());
            }
        }
    } else {
        if (type == t_window_domestic || type == t_curtains) {
            //chance of breaking the glass if pry attempt fails
            ///\EFFECT_STR reduces chance of breaking window with crowbar

            ///\EFFECT_MECHANICS reduces chance of breaking window with crowbar
            if (dice(4, difficulty) > dice(2, p->skillLevel( skill_mechanics )) + dice(2, p->str_cur)) {
                p->add_msg_if_player(m_mixed, _("You break the glass."));
                sounds::sound(dirp, 24, _("glass breaking!"));
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

int iuse::makemound(player *p, item *it, bool, const tripoint& )
{
    if (g->m.has_flag("DIGGABLE", p->pos()) && !g->m.has_flag("PLANT", p->pos())) {
        p->add_msg_if_player(_("You churn up the earth here."));
        p->moves = -300;
        g->m.ter_set(p->pos(), t_dirtmound);
        return it->type->charges_to_use();
    } else {
        p->add_msg_if_player(_("You can't churn up this ground."));
        return 0;
    }
}

//TODO remove this?
int iuse::dig(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(m_info, _("You can dig a pit via the construction menu -- hit *"));
    return it->type->charges_to_use();
}

void act_vehicle_siphon(vehicle *); // veh_interact.cpp

int iuse::siphon(player *p, item *it, bool, const tripoint& )
{
    tripoint posp;
    if (!choose_adjacent(_("Siphon from where?"), posp)) {
        return 0;
    }

    vehicle *veh = g->m.veh_at(posp);
    if (veh == NULL) {
        p->add_msg_if_player(m_info, _("There's no vehicle there."));
        return 0;
    }
    act_vehicle_siphon( veh );
    return it->type->charges_to_use();
}

int toolweapon_off( player *p, item *it, bool fast_startup,
                    bool condition, int volume,
                    const char *msg_success, const char *msg_failure )
{
    p->moves -= fast_startup ? 60 : 80;
    if (condition && it->charges > 0) {
        if( it->typeId() == "chainsaw_off" ) {
            sfx::play_variant_sound( "chainsaw_cord", "chainsaw_on", sfx::get_heard_volume(p->pos()));
            sfx::play_variant_sound( "chainsaw_start", "chainsaw_on", sfx::get_heard_volume(p->pos()));
            sfx::play_ambient_variant_sound("chainsaw_idle", "chainsaw_on", sfx::get_heard_volume(p->pos()), 18, 1000);
            sfx::play_ambient_variant_sound("weapon_theme", "chainsaw", sfx::get_heard_volume(p->pos()), 19, 3000);
        }
        sounds::sound(p->pos(), volume, msg_success);
        it->make(
            it->type->id.substr(0, it->type->id.size() - 4) +
              // 4 is the length of "_off".
            "_on");
        it->active = true;
    } else {
        if( it->typeId() == "chainsaw_off" ) {
            sfx::play_variant_sound( "chainsaw_cord", "chainsaw_on", sfx::get_heard_volume(p->pos()));
        }
        p->add_msg_if_player(msg_failure);
    }
    return it->type->charges_to_use();
}

int iuse::combatsaw_off(player *p, item *it, bool, const tripoint& )
{
    return toolweapon_off(p, it,
        true,
        !p->is_underwater(),
        30, _("With a snarl, the combat chainsaw screams to life!"),
        _("You yank the cord, but nothing happens."));
}

int iuse::chainsaw_off(player *p, item *it, bool, const tripoint& )
{
    return toolweapon_off(p, it,
        false,
        rng(0, 10) - it->damage > 5 && !p->is_underwater(),
        20, _("With a roar, the chainsaw leaps to life!"),
        _("You yank the cord, but nothing happens."));
}

int iuse::elec_chainsaw_off(player *p, item *it, bool, const tripoint& )
{
    return toolweapon_off(p, it,
        false,
        rng(0, 10) - it->damage > 5 && !p->is_underwater(),
        20, _("With a roar, the electric chainsaw leaps to life!"),
        _("You flip the switch, but nothing happens."));
}

int iuse::cs_lajatang_off(player *p, item *it, bool, const tripoint& )
{
    return toolweapon_off(p, it,
        false,
        rng(0, 10) - it->damage > 5 && it->charges > 1 && !p->is_underwater(),
        40, _("With a roar, the chainsaws leap to life!"),
        _("You yank the cords, but nothing happens."));
}

int iuse::carver_off(player *p, item *it, bool, const tripoint& )
{
    return toolweapon_off(p, it,
        false,
        true,
        20, _("The electric carver's serrated blades start buzzing!"),
        _("You pull the trigger, but nothing happens."));
}

int iuse::trimmer_off(player *p, item *it, bool, const tripoint& )
{
    return toolweapon_off(p, it,
        false,
        rng(0, 10) - it->damage > 3,
        15, _("With a roar, the hedge trimmer leaps to life!"),
        _("You yank the cord, but nothing happens."));
}

int toolweapon_on( player *p, item *it, bool t,
                   const char *tname, bool works_underwater,
                   int sound_chance, int volume,
                   const char *sound, bool double_charge_cost = false )
{
    std::string off_type =
        it->type->id.substr(0, it->type->id.size() - 3) +
          // 3 is the length of "_on".
        "_off";
    if (t) { // Effects while simply on
        if (double_charge_cost && it->charges > 0) {
            it->charges--;
        }
        if (!works_underwater && p->is_underwater()) {
            p->add_msg_if_player(_("Your %s gurgles in the water and stops."), tname);
            it->make(off_type);
            it->active = false;
        } else if (one_in(sound_chance)) {
            sounds::ambient_sound(p->pos(), volume, sound);
        }
    } else { // Toggling
        if( it->typeId() == "chainsaw_on" ) {
            sfx::play_variant_sound( "chainsaw_stop", "chainsaw_on", sfx::get_heard_volume(p->pos()));
            sfx::fade_audio_channel(18, 100);
            sfx::fade_audio_channel(19, 3000);
        }
        p->add_msg_if_player(_("Your %s goes quiet."), tname);
        it->make(off_type);
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::combatsaw_on(player *p, item *it, bool t, const tripoint& )
{
    return toolweapon_on(p, it, t, _("combat chainsaw"),
        false,
        12, 18, _("Your combat chainsaw growls."));
}

int iuse::chainsaw_on(player *p, item *it, bool t, const tripoint& )
{
    return toolweapon_on(p, it, t, _("chainsaw"),
        false,
        15, 12, _("Your chainsaw rumbles."));
}

int iuse::elec_chainsaw_on(player *p, item *it, bool t, const tripoint& )
{
    return toolweapon_on(p, it, t, _("electric chainsaw"),
        false,
        15, 12, _("Your electric chainsaw rumbles."));
}

int iuse::cs_lajatang_on(player *p, item *it, bool t, const tripoint& )
{
    return toolweapon_on(p, it, t, _("chainsaw lajatang"),
        false,
        15, 12, _("Your chainsaws rumble."),
        true);
          // The chainsaw lajatang drains 2 charges per turn, since
          // there are two chainsaws.
}

int iuse::carver_on(player *p, item *it, bool t, const tripoint& )
{
    return toolweapon_on(p, it, t, _("electric carver"),
        true,
        10, 8, _("Your electric carver buzzes."));
}

int iuse::trimmer_on(player *p, item *it, bool t, const tripoint& )
{
    return toolweapon_on(p, it, t, _("hedge trimmer"),
        true,
        15, 10, _("Your hedge trimmer rumbles."));
}

int iuse::circsaw_on(player *p, item *it, bool t, const tripoint& )
{
    return toolweapon_on(p, it, t, _("circular saw"),
        true,
        15, 7, _("Your circular saw buzzes."));
}

int iuse::jackhammer(player *p, item *it, bool, const tripoint &pos )
{
    bool normal_language = it->type->id != "jacqueshammer";
      // Jacqueshammers function the same as ordinary
      // jackhammers, except they print messages in French for
      // comic effect.
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, normal_language
          ? _("You can't do that while underwater.")
          //~ (jacqueshammer) "You can't do that while underwater."
          : _("Vous ne pouvez pas faire que sous l'eau."));
        return 0;
    }
    tripoint dirp = pos;
    if (!choose_adjacent(
          normal_language
            ? _("Drill where?")
            //~ (jacqueshammer) "Drill where?"
            : _("Percer dans quelle direction?"),
          dirp)) {
        return 0;
    }

    int &dirx = dirp.x;
    int &diry = dirp.y;

    if (dirx == p->posx() && diry == p->posy()) {
        p->add_msg_if_player(normal_language
          ? _("My god! Let's talk it over OK?")
          //~ (jacqueshammer) "My god! Let's talk it over OK?"
          : _("Mon dieu!  Nous allons en parler OK?"));
        p->add_msg_if_player(normal_language
          ? _("Don't do anything rash.")
          //~ (jacqueshammer) "Don't do anything rash."
          : _("Ne pas faire eruption rien."));
        return 0;
    }

    if (
           (g->m.is_bashable(dirx, diry) && (g->m.has_flag("SUPPORTS_ROOF", dirx, diry) || g->m.has_flag("MINEABLE", dirx, diry))&&
                g->m.ter(dirx, diry) != t_tree) ||
           (g->m.move_cost(dirx, diry) == 2 && g->get_levz() != -1 &&
                g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass)) {
        g->m.destroy( dirp, true );
        p->moves -= 500;
        sounds::sound(dirp, 45, normal_language
          //~ the sound of a jackhammer
          ? _("TATATATATATATAT!")
          //~ the sound of a "jacqueshammer"
          : _("OHOHOHOHOHOHOHOHO!"));
    } else {
        p->add_msg_if_player(m_info, normal_language
          ? _("You can't drill there.")
          //~ (jacqueshammer) "You can't drill there."
          : _("Vous ne pouvez pas percer la-bas."));
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::pickaxe(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // Long action
        return 0;
    }

    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    int dirx, diry;
    if (!choose_adjacent(_("Mine where?"), dirx, diry)) {
        return 0;
    }

    if (dirx == p->posx() && diry == p->posy()) {
        p->add_msg_if_player(_("Mining the depths of your experience,"));
        p->add_msg_if_player(_("you realize that it's best not to dig"));
        p->add_msg_if_player(_("yourself into a hole.  You stop digging."));
        return 0;
    }
    int turns;
    if (g->m.is_bashable(dirx, diry) && (g->m.has_flag("SUPPORTS_ROOF", dirx, diry) || g->m.has_flag("MINEABLE", dirx, diry)) &&
        g->m.ter(dirx, diry) != t_tree) {
        // Takes about 100 minutes (not quite two hours) base time.  Construction skill can speed this: 3 min off per level.
        ///\EFFECT_CARPENTRY speeds up mining with a pickaxe
        turns = (100000 - 3000 * p->skillLevel( skill_carpentry ));
    } else if (g->m.move_cost(dirx, diry) == 2 && g->get_levz() == 0 &&
               g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
        turns = 20000;
    } else {
        p->add_msg_if_player(m_info, _("You can't mine there."));
        return 0;
    }
    p->assign_activity(ACT_PICKAXE, turns, -1, p->get_item_position(it));
    p->activity.placement = tripoint(dirx, diry, p->posz()); // TODO: Z
    p->add_msg_if_player(_("You attack the %1$s with your %2$s."),
                         g->m.tername(dirx, diry).c_str(), it->tname().c_str());
    return 0; // handled when the activity finishes
}

int iuse::set_trap(player *p, item *it, bool, const tripoint& )
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

    int posx = dirx;
    int posy = diry;
    tripoint tr_loc( posx, posy, p->posz() );
    if (g->m.move_cost(posx, posy) != 2) {
        p->add_msg_if_player(m_info, _("You can't place a %s there."), it->tname().c_str());
        return 0;
    }

    const trap &existing_trap = g->m.tr_at( tr_loc );
    if( !existing_trap.is_null() ) {
        if( existing_trap.can_see( tr_loc, *p )) {
            p->add_msg_if_player(m_info, _("You can't place a %s there.  It contains a trap already."),
                                 it->tname().c_str());
        } else {
            p->add_msg_if_player(m_bad, _("You trigger a %s!"), existing_trap.name.c_str());
            existing_trap.trigger( tr_loc, p );
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
        buried = (p->has_items_with_quality( "DIG", 3, 1 ) &&
                  g->m.has_flag("DIGGABLE", posx, posy) &&
                  query_yn(_("Bury the beartrap?")));
        type = (buried ? tr_beartrap_buried : tr_beartrap);
        message << (buried ? _("You bury the beartrap.") : _("You set the beartrap."));
        practice = (buried ? 7 : 4);
    } else if (it->type->id == "board_trap") {
        message << string_format(_("You set the board trap on the %s, nails facing up."),
                                 g->m.tername(posx, posy).c_str());
        type = tr_nailboard;
        practice = 2;
    } else if (it->type->id == "caltrops") {
        message << string_format(_("You scatter the caltrops on the %s."),
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
        posx = (dirx - p->posx()) * 2 + p->posx(); //math correction for blade trap
        posy = (diry - p->posy()) * 2 + p->posy();
        tr_loc = tripoint( posx, posy, p->posz() );
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
        buried = (p->has_items_with_quality( "DIG", 3, 1 ) &&
                  g->m.has_flag("DIGGABLE", posx, posy) &&
                  query_yn(_("Bury the land mine?")));
        type = (buried ? tr_landmine_buried : tr_landmine);
        message << (buried ? _("You bury the land mine.") : _("You set the land mine."));
        practice = (buried ? 7 : 4);
    } else {
        p->add_msg_if_player(_("Tried to set a trap.  But got confused! %s"), it->tname().c_str());
    }

    const trap *tr = &type.obj();
    if (dirx == p->posx() && diry == p->posy() && !tr->is_benign()) {
        p->add_msg_if_player(m_info, _("Yeah.  Place the %s at your feet."), it->tname().c_str());
        p->add_msg_if_player(m_info, _("Real damn smart move."));
        return 0;
    }

    if (buried) {
        if (!p->has_amount("shovel", 1) && !p->has_amount("e_tool", 1)) {
            p->add_msg_if_player(m_info, _("You need a shovel."));
            return 0;
        } else if (!g->m.has_flag("DIGGABLE", posx, posy)) {
            p->add_msg_if_player(m_info, _("You can't dig in that %s."), g->m.tername(posx, posy).c_str());
            return 0;
        }
    }

    p->add_msg_if_player(message.str().c_str());
    p->practice( skill_id( "traps" ), practice);
    g->m.add_trap( tr_loc, type );
    if( !tr->can_see( tr_loc, *p ) ) {
        p->add_known_trap( tr_loc, *tr );
    }
    p->moves -= 100 + practice * 25;
    if (type == tr_engine) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i != 0 || j != 0) {
                    g->m.add_trap({posx + i, posy + j, tr_loc.z}, tr_blade);
                }
            }
        }
    }
    return 1;
}

int iuse::geiger(player *p, item *it, bool t, const tripoint &pos)
{
    if (t) { // Every-turn use when it's on
        const int rads = g->m.get_radiation( pos );
        if (rads == 0) {
            return it->type->charges_to_use();
        }
        sounds::sound( pos, 6, "" );
        if( !p->can_hear( pos, 6 ) ) {
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
    const auto type = dynamic_cast<const it_tool *>(it->type);
    bool is_on = (type->id == "geiger_on");
    if (is_on) {
        add_msg(_("The geiger counter's SCANNING LED turns off."));
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
                                 g->m.get_radiation( p->pos3() ) );
            break;
        case 3:
            p->add_msg_if_player(_("The geiger counter's scan LED turns on."));
            it->make("geiger_on");
            it->active = true;
            break;
        case 4:
            return 0;
    }
    return it->type->charges_to_use();
}

int iuse::teleport(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // That would be evil
        return 0;
    }

    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    p->moves -= 100;
    g->teleport(p);
    return it->type->charges_to_use();
}

int iuse::can_goo(player *p, item *it, bool, const tripoint& )
{
    it->make("canister_empty");
    int tries = 0;
    tripoint goop;
    goop.z = p->posz();
    do {
        goop.x = p->posx() + rng(-2, 2);
        goop.y = p->posy() + rng(-2, 2);
        tries++;
    } while (g->m.impassable(goop) && tries < 10);
    if (tries == 10) {
        return 0;
    }
    int mondex = g->mon_at(goop);
    if (mondex != -1) {
        auto &critter = g->zombie( mondex );
        if (g->u.sees(goop)) {
            add_msg(_("Black goo emerges from the canister and envelopes a %s!"),
                    critter.name().c_str());
        }
        critter.poly( mon_blob );

        critter.set_speed_base( critter.get_speed_base() - rng(5, 25) );
        critter.set_hp( critter.get_speed() );
    } else {
        if (g->u.sees(goop)) {
            add_msg(_("Living black goo emerges from the canister!"));
        }
        if (g->summon_mon(mon_blob, goop)) {
            monster *goo = g->monster_at(goop);
            goo->friendly = -1;
        }
    }
    tries = 0;
    while (!one_in(4) && tries < 10) {
        tries = 0;
        do {
            goop.x = p->posx() + rng(-2, 2);
            goop.y = p->posy() + rng(-2, 2);
            tries++;
        } while (g->m.impassable(goop) &&
                 g->m.tr_at(goop).is_null() && tries < 10);
        if (tries < 10) {
            if (g->u.sees(goop)) {
                add_msg(m_warning, _("A nearby splatter of goo forms into a goo pit."));
            }
            g->m.add_trap(goop, tr_goo);
        } else {
            return 0;
        }
    }
    return it->type->charges_to_use();
}

int iuse::throwable_extinguisher_act(player *, item *it, bool, const tripoint &pos)
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
                tripoint dest( pos.x + x, pos.y + y, pos.z );
                if (g->m.passable(dest) && (x == 0 || y == 0)) {
                    g->m.adjust_field_strength(dest, fd_fire, 0 - rng(0, 1));
                }
            }
        }
        return 1;
    }
    it->active = false;
    return 0;
}

int iuse::pipebomb_act(player *, item *it, bool t, const tripoint &pos)
{
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }
    if (t) { // Simple timer effects
        //~ the sound of a lit fuse
        sounds::sound(pos, 0, _("ssss...")); // Vol 0 = only heard if you hold it
    } else if (it->charges > 0) {
        add_msg(m_info, _("You've already lit the %s, try throwing it instead."), it->tname().c_str());
        return 0;
    } else { // The timer has run down
        if (one_in(10)) {
            // Fizzled, but we may not have seen it to know that
            if (g->u.sees( pos )) {
                add_msg(_("The pipe bomb fizzles out."));
            }
        } else {
            g->explosion( pos, rng(10, 24), 0.6, rng(0, 4), false);
        }
    }
    return 0;
}

int iuse::granade(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(_("You pull the pin on the Granade."));
    it->make("granade_act");
    it->charges = 5;
    it->active = true;
    return it->type->charges_to_use();
}

int iuse::granade_act(player *, item *it, bool t, const tripoint &pos)
{
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }
    if (t) { // Simple timer effects
        sounds::sound(pos, 0, _("Merged!"));  // Vol 0 = only heard if you hold it
    } else if (it->charges > 0) {
        add_msg(m_info, _("You've already pulled the %s's pin, try throwing it instead."),
                it->tname().c_str());
        return 0;
    } else { // When that timer runs down...
        int explosion_radius = 3;
        int effect_roll = rng(1, 5);
        auto buff_stat = [&](int &current_stat, int modify_by) {
          auto modified_stat = current_stat + modify_by;
          current_stat = std::max(current_stat, std::min(15, modified_stat));
        };
        switch (effect_roll) {
            case 1:
                sounds::sound(pos, 100, _("BUGFIXES!!"));
                g->draw_explosion( pos, explosion_radius, c_ltcyan );
                for (int i = -explosion_radius; i <= explosion_radius; i++) {
                    for (int j = -explosion_radius; j <= explosion_radius; j++) {
                        tripoint dest( pos.x + i, pos.y + j, pos.z );
                        const int zid = g->mon_at( dest, true );
                        if (zid != -1 &&
                            (g->zombie(zid).type->in_species( INSECT ) ||
                             g->zombie(zid).is_hallucination())) {
                            g->zombie( zid ).die_in_explosion( nullptr );
                        }
                    }
                }
                break;

            case 2:
                sounds::sound(pos, 100, _("BUFFS!!"));
                g->draw_explosion( pos, explosion_radius, c_green );
                for (int i = -explosion_radius; i <= explosion_radius; i++) {
                    for (int j = -explosion_radius; j <= explosion_radius; j++) {
                        tripoint dest( pos.x + i, pos.y + j, pos.z );
                        const int mon_hit = g->mon_at(dest);
                        if (mon_hit != -1) {
                            auto &critter = g->zombie( mon_hit );
                            critter.set_speed_base(
                                critter.get_speed_base() * rng_float(1.1, 2.0) );
                            critter.set_hp( critter.get_hp() * rng_float( 1.1, 2.0 ) );
                        } else if (g->npc_at(dest) != -1) {
                            int npc_hit = g->npc_at(dest);
                            ///\EFFECT_STR_MAX increases possible granade str buff for NPCs
                            buff_stat(g->active_npc[npc_hit]->str_max, rng(0, g->active_npc[npc_hit]->str_max / 2));
                            ///\EFFECT_DEX_MAX increases possible granade dex buff for NPCs
                            buff_stat(g->active_npc[npc_hit]->dex_max, rng(0, g->active_npc[npc_hit]->dex_max / 2));
                            ///\EFFECT_INT_MAX increases possible granade int buff for NPCs
                            buff_stat(g->active_npc[npc_hit]->int_max, rng(0, g->active_npc[npc_hit]->int_max / 2));
                            ///\EFFECT_PER_MAX increases possible granade per buff for NPCs
                            buff_stat(g->active_npc[npc_hit]->per_max, rng(0, g->active_npc[npc_hit]->per_max / 2));
                        } else if (g->u.posx() == pos.x + i && g->u.posy() == pos.y + j) {
                            ///\EFFECT_STR_MAX increases possible granade str buff
                            buff_stat(g->u.str_max, rng(0, g->u.str_max / 2));
                            ///\EFFECT_DEX_MAX increases possible granade dex buff
                            buff_stat(g->u.dex_max, rng(0, g->u.dex_max / 2));
                            ///\EFFECT_INT_MAX increases possible granade int buff
                            buff_stat(g->u.int_max, rng(0, g->u.int_max / 2));
                            ///\EFFECT_PER_MAX increases possible granade per buff
                            buff_stat(g->u.per_max, rng(0, g->u.per_max / 2));
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
                sounds::sound(pos, 100, _("NERFS!!"));
                g->draw_explosion( pos, explosion_radius, c_red);
                for (int i = -explosion_radius; i <= explosion_radius; i++) {
                    for (int j = -explosion_radius; j <= explosion_radius; j++) {
                        tripoint dest( pos.x + i, pos.y + j, pos.z );
                        const int mon_hit = g->mon_at(dest);
                        if (mon_hit != -1) {
                            auto &critter = g->zombie( mon_hit );
                            critter.set_speed_base(
                                rng( 0, critter.get_speed_base() ) );
                            critter.set_hp( rng( 1, critter.get_hp() ) );
                        } else if (g->npc_at(dest) != -1) {
                            int npc_hit = g->npc_at(dest);
                            ///\EFFECT_STR_MAX increases possible granade str debuff for NPCs (NEGATIVE)
                            g->active_npc[npc_hit]->str_max -= rng(0, g->active_npc[npc_hit]->str_max / 2);
                            ///\EFFECT_DEX_MAX increases possible granade dex debuff for NPCs (NEGATIVE)
                            g->active_npc[npc_hit]->dex_max -= rng(0, g->active_npc[npc_hit]->dex_max / 2);
                            ///\EFFECT_INT_MAX increases possible granade int debuff for NPCs (NEGATIVE)
                            g->active_npc[npc_hit]->int_max -= rng(0, g->active_npc[npc_hit]->int_max / 2);
                            ///\EFFECT_PER_MAX increases possible granade per debuff for NPCs (NEGATIVE)
                            g->active_npc[npc_hit]->per_max -= rng(0, g->active_npc[npc_hit]->per_max / 2);
                        } else if (g->u.posx() == pos.x + i && g->u.posy() == pos.y + j) {
                            ///\EFFECT_STR_MAX increases possible granade str debuff (NEGATIVE)
                            g->u.str_max -= rng(0, g->u.str_max / 2);
                            ///\EFFECT_DEX_MAX increases possible granade dex debuff (NEGATIVE)
                            g->u.dex_max -= rng(0, g->u.dex_max / 2);
                            ///\EFFECT_INT_MAX increases possible granade int debuff (NEGATIVE)
                            g->u.int_max -= rng(0, g->u.int_max / 2);
                            ///\EFFECT_PER_MAX increases possible granade per debuff (NEGATIVE)
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
                sounds::sound(pos, 100, _("REVERTS!!"));
                g->draw_explosion( pos, explosion_radius, c_pink);
                for (int i = -explosion_radius; i <= explosion_radius; i++) {
                    for (int j = -explosion_radius; j <= explosion_radius; j++) {
                        tripoint dest( pos.x + i, pos.y + j, pos.z );
                        const int mon_hit = g->mon_at(dest);
                        if (mon_hit != -1) {
                            auto &critter = g->zombie( mon_hit );
                            critter.set_speed_base( critter.type->speed );
                            critter.set_hp( critter.get_hp_max() );
                            critter.clear_effects();
                        } else if (g->npc_at(dest) != -1) {
                            int npc_hit = g->npc_at(dest);
                            g->active_npc[npc_hit]->environmental_revert_effect();
                        } else if (g->u.posx() == pos.x + i && g->u.posy() == pos.y + j) {
                            g->u.environmental_revert_effect();
                            do_purify( &(g->u) );
                        }
                    }
                }
                break;
            case 5:
                sounds::sound(pos, 100, _("BEES!!"));
                g->draw_explosion( pos, explosion_radius, c_yellow);
                for (int i = -explosion_radius; i <= explosion_radius; i++) {
                    for (int j = -explosion_radius; j <= explosion_radius; j++) {
                        tripoint dest( pos.x + i, pos.y + j, pos.z );
                        if (one_in(5) && -1 == g->mon_at(dest) &&
                            -1 == g->npc_at(dest)) {
                            g->m.add_field(dest, fd_bees, rng(1, 3), 0 );
                        }
                    }
                }
                break;
        }
    }
    return it->type->charges_to_use();
}

int iuse::c4(player *p, item *it, bool, const tripoint& )
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

int iuse::acidbomb_act(player *p, item *it, bool, const tripoint &pos)
{
    if (!p->has_item(it)) {
        tripoint tmp = pos;
        int &x = tmp.x;
        int &y = tmp.y;
        if (tmp.x == -999) {
            tmp = p->pos3();
        }
        it->charges = -1;
        for ( x = pos.x - 1; x <= pos.x + 1; x++) {
            for ( y = pos.y - 1; y <= pos.y + 1; y++) {
                g->m.add_field( tmp, fd_acid, 3, 0 );
            }
        }
    }
    return 0;
}

int iuse::grenade_inc_act(player *p, item *it, bool t, const tripoint &pos)
{
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }

    if (t) { // Simple timer effects
        sounds::sound(pos, 0, _("Tick!")); // Vol 0 = only heard if you hold it
    } else if (it->charges > 0) {
        p->add_msg_if_player(m_info, _("You've already released the handle, try throwing it instead."));
        return 0;
    } else {  // blow up
        int num_flames= rng(3,5);
        for (int current_flame = 0; current_flame < num_flames; current_flame++) {
            tripoint dest( pos.x + rng( -5, 5 ), pos.y + rng( -5, 5 ), pos.z );
            std::vector<tripoint> flames = line_to( pos, dest, 0, 0 );
            for( auto &flame : flames ) {
                g->m.add_field( flame, fd_fire, rng( 0, 2 ), 0 );
            }
        }
        g->explosion( pos, 8, 0.8, 0, true );
        for (int i = -2; i <= 2; i++) {
            for (int j = -2; j <= 2; j++) {
                g->m.add_field( { pos.x + i, pos.y + j, pos.z }, fd_incendiary, 3, 0 );
            }
        }

    }
    return 0;
}

int iuse::arrow_flamable(player *p, item *it, bool, const tripoint& )
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (!p->use_charges_if_avail("fire", 1)) {
        p->add_msg_if_player(m_info, _("You need a source of fire!"));
        return 0;
    }
    p->add_msg_if_player(_("You light the arrow!"));
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

int iuse::molotov_lit(player *p, item *it, bool t, const tripoint &pos)
{
    int age = int(calendar::turn) - it->bday;
    if (p->has_item(it)) {
        it->charges += 1;
        if (age >= 5) { // More than 5 turns old = chance of going out
            if (rng(1, 50) < age) {
                p->add_msg_if_player(_("Your lit Molotov goes out."));
                it->make("molotov");
                it->active = false;
            }
        }
    } else {
        if( !t ) {
            for( auto &&pt : g->m.points_in_radius( pos, 1, 0 ) ) {
                const int density = 1 + one_in( 3 ) + one_in( 5 );
                g->m.add_field( pt, fd_fire, density, 0 );
            }
        }
    }
    return 0;
}

int iuse::firecracker_pack(player *p, item *it, bool, const tripoint& )
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (!p->has_charges("fire", 1)) {
        p->add_msg_if_player(m_info, _("You need a source of fire!"));
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

int iuse::firecracker_pack_act(player *, item *it, bool, const tripoint &pos)
{
    int current_turn = calendar::turn;
    int timer = current_turn - it->bday;
    if (timer < 2) {
        sounds::sound(pos, 0, _("ssss..."));
        it->damage += 1;
    } else if (it->charges > 0) {
        int ex = rng(3, 5);
        int i = 0;
        if (ex > it->charges) {
            ex = it->charges;
        }
        for (i = 0; i < ex; i++) {
            sounds::sound(pos, 20, _("Bang!"));
        }
        it->charges -= ex;
    }
    if (it->charges == 0) {
        it->charges = -1;
    }
    return 0;
}

int iuse::firecracker(player *p, item *it, bool, const tripoint& )
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (!p->use_charges_if_avail("fire", 1)) {
        p->add_msg_if_player(m_info, _("You need a source of fire!"));
        return 0;
    }
    p->add_msg_if_player(_("You light the firecracker."));
    it->make("firecracker_act");
    it->charges = 2;
    it->active = true;
    return it->type->charges_to_use();
}

int iuse::firecracker_act(player *, item *it, bool t, const tripoint &pos)
{
    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }
    if (t) {// Simple timer effects
        sounds::sound(pos, 0, _("ssss..."));
    } else if (it->charges > 0) {
        add_msg(m_info, _("You've already lit the %s, try throwing it instead."), it->tname().c_str());
        return 0;
    } else { // When that timer runs down...
        sounds::sound(pos, 20, _("Bang!"));
    }
    return 0;
}

int iuse::mininuke(player *p, item *it, bool, const tripoint& )
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

int iuse::pheromone( player *p, item *it, bool, const tripoint &pos )
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }

    if (pos.x == -999 || pos.y == -999) {
        return 0;
    }

    p->add_msg_player_or_npc(_("You squeeze the pheromone ball..."),
                             _("<npcname> squeezes the pheromone ball..."));

    p->moves -= 15;

    int converts = 0;
    for (int x = pos.x - 4; x <= pos.x + 4; x++) {
        for (int y = pos.y - 4; y <= pos.y + 4; y++) {
            tripoint dest( x, y, pos.z );
            int mondex = g->mon_at( dest, true );
            if( mondex == -1 ) {
                continue;
            }
            monster &critter = g->zombie( mondex );
            if( critter.type->in_species( ZOMBIE ) && critter.friendly == 0 && rng( 0, 500 ) > critter.get_hp() ) {
                converts++;
                critter.make_friendly();
            }
        }
    }

    if (g->u.sees(*p)) {
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


int iuse::portal(player *p, item *it, bool, const tripoint& )
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    tripoint t(p->posx() + rng(-2, 2), p->posy() + rng(-2, 2), p->posz());
    g->m.add_trap(t, tr_portal);
    return it->type->charges_to_use();
}

int iuse::tazer(player *p, item *it, bool, const tripoint &pos )
{
    if( it->charges < it->type->charges_to_use() ) {
        return 0;
    }

    tripoint dirp = pos;
    if( p->pos() == pos && !choose_adjacent( _("Shock where?"), dirp ) ) {
        return 0;
    }

    if( dirp == p->pos() ) {
        p->add_msg_if_player(m_info, _("Umm.  No."));
        return 0;
    }

    Creature *target = g->critter_at( dirp, true );
    if( target == nullptr ) {
        p->add_msg_if_player(_("There's nothing to zap there!"));
        return 0;
    }

    // Hacky, there should be a method doing all that when the player willingly hurts someone
    npc *foe = dynamic_cast<npc *>( target );
    if( foe != nullptr && foe->attitude != NPCATT_KILL && foe->attitude != NPCATT_FLEE ) {
        if( !p->query_yn( _("Really shock %s"), target->disp_name().c_str() ) ) {
            return 0;
        }

        foe->attitude = NPCATT_KILL;
        foe->hit_by_player = true;
    }

    ///\EFFECT_DEX slightly increases chance of successfully using tazer

    ///\EFFECT_MELEE increases chance of successfully using a tazer
    int numdice = 3 + (p->dex_cur / 2.5) + p->skillLevel( skill_melee ) * 2;
    p->moves -= 100;

    ///\EFFECT_DODGE increases chance of dodging a tazer attack
    int target_dice = target->get_dodge();
    if( dice( numdice, 10 ) < dice( target_dice, 10 ) ) {
        // A miss!
        p->add_msg_player_or_npc( _("You attempt to shock %s, but miss."),
                                  _("<npcname> attempts to shock %s, but misses."),
                                  target->disp_name().c_str() );
        return it->type->charges_to_use();
    }

    // Maybe-TODO: Execute an attack and maybe zap something other than torso
    // Maybe, because it's torso (heart) that fails when zapped with electricity
    int dam = target->deal_damage( p, bp_torso, damage_instance( DT_ELECTRIC, rng( 5, 25 ) ) ).total_damage();
    if( dam > 0 ) {
        p->add_msg_player_or_npc( m_good,
                                  _("You shock %s!"),
                                  _("<npcname> shocks %s!"),
                                  target->disp_name().c_str() );
    } else {
        p->add_msg_player_or_npc( m_warning,
                                  _("You unsuccessfully attempt to shock %s!"),
                                  _("<npcname> unsuccessfully attempts to shock %s!"),
                                  target->disp_name().c_str() );
    }

    return it->type->charges_to_use();
}

int iuse::tazer2(player *p, item *it, bool b, const tripoint &pos )
{
    if( it->charges >= 100 ) {
        // Instead of having a ctrl+c+v of the function above, spawn a fake tazer and use it
        // Ugly, but less so than copied blocks
        item fake( "tazer", 0 );
        fake.charges = 100;
        return tazer( p, &fake, b, pos );
    } else {
        p->add_msg_if_player( m_info, _("Insufficient power") );
    }

    return 0;
}

int iuse::shocktonfa_off(player *p, item *it, bool t, const tripoint &pos)
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

int iuse::shocktonfa_on(player *p, item *it, bool t, const tripoint &pos)
{
    if (t) {  // Effects while simply on

    } else {
        if (it->charges <= 0) {
            p->add_msg_if_player(m_info, _("Your tactical tonfa is out of power."));
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
                    p->add_msg_if_player(_("You turn off the light."));
                    it->make("shocktonfa_off");
                    it->active = false;
                }
            }
        }
    }
    return 0;
}

int iuse::mp3(player *p, item *it, bool, const tripoint& )
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

// Some music descriptions affect the character additionally, e.g. increase player::stim, or
// increase the morale from the music.
struct music_description {
    std::string sound;
    int stim_bonus = 0;
    int morale_bonus = 0;
};
music_description get_music_description( const player & p )
{
    music_description result;
    switch( rng( 1, 10 ) ) {
        case 1:
            result.sound = _("a sweet guitar solo!");
            result.stim_bonus = 1;
            break;
        case 2:
            result.sound = _("a funky bassline.");
            break;
        case 3:
            result.sound = _("some amazing vocals.");
            break;
        case 4:
            result.sound = _("some pumping bass.");
            break;
        case 5:
            result.sound = _("dramatic classical music.");
            ///\EFFECT_INT increases possible morale benefit from listening to music
            if( p.int_cur >= 10 ) {
                result.morale_bonus = p.int_cur * 2;
            }
            break;
    }
    if (one_in(50)) {
        result.sound = _("some bass-heavy post-glam speed polka.");
    }
    return result;
}

void iuse::play_music( player * const p, const tripoint &source, int const volume, int const max_morale )
{
    // TODO: what about other "player", e.g. when a NPC is listening or when the PC is listening,
    // the other characters around should be able to profit as well.

    bool const do_effects = !p->has_effect( "music" ) && p->can_hear( source, volume );
    int morale_bonus = 0;
    std::string sound;
    if( calendar::once_every(MINUTES(5)) ) {
        // Every 5 minutes, describe the music
        auto const music = get_music_description( *p );
        if ( !music.sound.empty() ) {
            // return only music description by default
            sound = music.sound;
            // music source is on player's square
            if( p->pos() == source && volume != 0 ) {
                // generic stereo players without earphones
                sound = string_format( _("You listen to %s"), music.sound.c_str() );
            } else if ( p->pos() == source && volume == 0 && p->can_hear( source, volume)) {
                // in-ear music, such as mp3 player
                p->add_msg_if_player( _( "You listen to %s"), music.sound.c_str() );
            }
        }
        if( do_effects ) {
            p->stim += music.stim_bonus;
            morale_bonus += music.morale_bonus;
        }
    }
    // do not process mp3 player
    if ( volume != 0 ) {
            sounds::ambient_sound( source, volume, sound );
    }
    if( do_effects ) {
        p->add_effect("music", 1);
        p->add_morale(MORALE_MUSIC, 1, max_morale + morale_bonus, 5, 2);
        // mp3 player reduces hearing
        if ( volume == 0 ) {
             p->add_effect("earphones",1);
        }
    }
}

int iuse::mp3_on(player *p, item *it, bool t, const tripoint &pos)
{
    if (t) { // Normal use
        if (p->has_item(it)) {
            // mp3 player in inventory, we can listen
            play_music( p, pos, 0, 50 );
        }
    } else { // Turning it off
        p->add_msg_if_player(_("The mp3 player turns off."));
        it->make("mp3");
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::portable_game(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // Long action
        return 0;
    }

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
        as_m.entries.push_back(uimenu_entry(4, true, '4', _("Minesweeper")));
        as_m.entries.push_back(uimenu_entry(5, true, '5', _("Cancel")));
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
            case 4:
                loaded_software = "minesweeper_game";
                p->rooted_message();
                break;
            case 5: //Cancel
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

int iuse::vibe(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // Long action
        // Also, that would be creepy as fuck, seriously
        return 0;
    }

    if ((p->is_underwater()) && (!((p->has_trait("GILLS")) || (p->is_wearing("rebreather_on")) ||
                                   (p->is_wearing("rebreather_xl_on")) || (p->is_wearing("mask_h20survivor_on"))))) {
        p->add_msg_if_player(m_info, _("It's waterproof, but oxygen maybe?"));
        return 0;
    }
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_info, _("The %s's batteries are dead."), it->tname().c_str());
        return 0;
    }
    if (p->fatigue >= DEAD_TIRED) {
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

int iuse::vortex(player *p, item *it, bool, const tripoint& )
{
    std::vector<tripoint> spawn;
    auto empty_add = [&]( int x, int y ) {
        tripoint pt( x, y, p->posz() );
        if( g->is_empty( pt ) ) {
            spawn.push_back( pt );
        }
    };
    for (int i = -3; i <= 3; i++) {
        empty_add(p->posx() - 3, p->posy() + i);
        empty_add(p->posx() + 3, p->posy() + i);
        empty_add(p->posx() + i, p->posy() - 3);
        empty_add(p->posx() + i, p->posy() + 3);
    }
    if (spawn.empty()) {
        p->add_msg_if_player(m_warning, _("Air swirls around you for a moment."));
        it->make("spiral_stone");
        return it->type->charges_to_use();
    }

    p->add_msg_if_player(m_warning, _("Air swirls all over..."));
    p->moves -= 100;
    it->make("spiral_stone");
    monster mvortex( mon_vortex, random_entry( spawn ) );
    mvortex.friendly = -1;
    g->add_zombie(mvortex);
    return it->type->charges_to_use();
}

int iuse::dog_whistle(player *p, item *it, bool, const tripoint& )
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    p->add_msg_if_player(_("You blow your dog whistle."));
    for (size_t i = 0; i < g->num_zombies(); i++) {
        if (g->zombie(i).friendly != 0 && g->zombie(i).type->id == mon_dog) {
            bool u_see = g->u.sees(g->zombie(i));
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

int iuse::vacutainer(player *p, item *it, bool, const tripoint& )
{
    if (p->is_npc()) {
        return 0;    // No NPCs for now!
    }

    if (!it->contents.empty()) {
        p->add_msg_if_player(m_info, _("That %s is full!"), it->tname().c_str());
        return 0;
    }

    item blood("blood", calendar::turn);
    item acid("acid", calendar::turn);
    bool drew_blood = false;
    for( auto &map_it : g->m.i_at(p->posx(), p->posy()) ) {
        if( map_it.is_corpse() &&
            query_yn(_("Draw blood from %s?"), map_it.tname().c_str()) ) {
            blood.set_mtype( map_it.get_mtype() );
            drew_blood = true;
        }
    }

    if (!drew_blood && query_yn(_("Draw your own blood?"))) {
        drew_blood = true;
        if (p->has_trait ("ACIDBLOOD")) {
            it->put_in(acid);
            if (one_in(2) && it->damage <= 3){
                it->damage++;
                p->add_msg_if_player(m_info, _("Your acidic blood damages the %s!"), it->tname().c_str());
            }
            if (!one_in(4) && it->damage >= 4){
                p->add_msg_if_player(m_info, _("Your acidic blood melts the %s, destroying it!"), it->tname().c_str());
                p->inv.remove_item(it);
                return 0;
            }
            return it->type->charges_to_use();
        }
    }

    if (!drew_blood) {
        return it->type->charges_to_use();
    }

    it->put_in(blood);
    return it->type->charges_to_use();
}

void iuse::cut_log_into_planks(player *p)
{
    p->moves -= 300;
    p->add_msg_if_player(_("You cut the log into planks."));
    item plank("2x4", int(calendar::turn));
    item scrap("splinter", int(calendar::turn));
    ///\EFFECT_CARPENTRY increases number of planks cut from a log
    int planks = (rng(1, 3) + (p->skillLevel( skill_carpentry ) * 2));
    int scraps = 12 - planks;
    if (planks >= 12) {
        planks = 12;
    }
    if (scraps >= planks) {
        add_msg(m_bad, _("You waste a lot of the wood."));
    }
    p->i_add_or_drop(plank, planks);
    p->i_add_or_drop(scrap, scraps);
}

int iuse::lumber(player *p, item *it, bool, const tripoint& )
{
    // Check if player is standing on any lumber
    for (auto &i : g->m.i_at(p->pos())) {
        if (i.type->id == "log")
        {
            g->m.i_rem(p->pos(), &i);
            cut_log_into_planks( p );
            return it->type->charges_to_use();
        }
    }

    // If the player is not standing on a log, check inventory
    int pos = g->inv_for_filter( _("Cut up what?"), []( const item & itm ) {
        return itm.type->id == "log";
    } );
    item* cut = &( p->i_at( pos ) );

    if (cut->type->id == "null") {
        add_msg(m_info, _("You do not have that item!"));
        return 0;
    }
    if (cut->type->id == "log") {
        p->i_rem( cut );
        cut_log_into_planks(p);
        return it->type->charges_to_use();
    } else {
        add_msg(m_info, _("You can't cut that up!"));
        return 0;
    }
}


int iuse::oxytorch(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // Long action
        return 0;
    }

    int dirx, diry;
    if (!(p->has_amount("goggles_welding", 1) || p->is_wearing("goggles_welding") ||
          p->is_wearing("rm13_armor_on") || p->has_bionic("bio_sunglasses"))) {
        add_msg(m_info, _("You need welding goggles to do that."));
        return 0;
    }
    if (!choose_adjacent(_("Cut up metal where?"), dirx, diry)) {
        return 0;
    }

    if (dirx == p->posx() && diry == p->posy()) {
        add_msg(m_info, _("Yuck.  Acetylene gas smells weird."));
        return 0;
    }

    const ter_id ter = g->m.ter( dirx, diry );
    int moves;

    if( g->m.furn(dirx, diry) == f_rack || ter == t_chainfence_posts ) {
        moves = 200;
    } else if( ter == t_window_enhanced || ter == t_window_enhanced_noglass ) {
        moves = 500;
    } else if( ter == t_chainfence_v || ter == t_chainfence_h || ter == t_chaingate_c ||
               ter == t_chaingate_l  || ter == t_bars || ter == t_window_bars_alarm ) {
        moves = 1000;
    } else if( ter == t_door_metal_locked || ter == t_door_metal_c || ter == t_door_bar_c ||
               ter == t_door_bar_locked || ter == t_door_metal_pickable ) {
        moves = 1500;
    } else {
        add_msg( m_info, _("You can't cut that.") );
        return 0;
    }

    const int charges = moves / 100 * it->type->charges_to_use();

    if( charges > it->charges ) {
        add_msg( m_info, _("Your torch doesn't have enough acetylene to cut that.") );
        return 0;
    }

    // placing ter here makes resuming tasks work better
    p->assign_activity( ACT_OXYTORCH, moves, (int)ter, p->get_item_position( it ) );
    p->activity.placement = tripoint( dirx, diry, 0 );
    p->activity.values.push_back( charges );

    // charges will be consumed in oxytorch_do_turn, not here
    return 0;
}

int iuse::hacksaw(player *p, item *it, bool, const tripoint &pos )
{
    tripoint dirp = pos;
    if (!choose_adjacent(_("Cut up metal where?"), dirp)) {
        return 0;
    }
    int &dirx = dirp.x;
    int &diry = dirp.y;

    if (dirx == p->posx() && diry == p->posy()) {
        add_msg(m_info, _("Why would you do that?"));
        add_msg(m_info, _("You're not even chained to a boiler."));
        return 0;
    }


    if (g->m.furn(dirx, diry) == f_rack) {
        p->moves -= 500;
        g->m.furn_set(dirx, diry, f_null);
        sounds::sound(dirp, 15, _("grnd grnd grnd"));
        g->m.spawn_item(p->posx(), p->posy(), "pipe", rng(1, 3));
        g->m.spawn_item(p->posx(), p->posy(), "steel_chunk");
        return it->type->charges_to_use();
    }

    const ter_id ter = g->m.ter( dirx, diry );
    if( ter == t_chainfence_v || ter == t_chainfence_h || ter == t_chaingate_c ||
        ter == t_chaingate_l) {
        p->moves -= 500;
        g->m.ter_set(dirx, diry, t_dirt);
        sounds::sound(dirp, 15, _("grnd grnd grnd"));
        g->m.spawn_item(dirx, diry, "pipe", 6);
        g->m.spawn_item(dirx, diry, "wire", 20);
    } else if( ter == t_chainfence_posts ) {
        p->moves -= 500;
        g->m.ter_set(dirx, diry, t_dirt);
        sounds::sound(dirp, 15, _("grnd grnd grnd"));
        g->m.spawn_item(dirx, diry, "pipe", 6);
    } else if( ter == t_bars ) {
        if (g->m.ter(dirx + 1, diry) == t_sewage || g->m.ter(dirx, diry + 1) == t_sewage ||
            g->m.ter(dirx - 1, diry) == t_sewage || g->m.ter(dirx, diry - 1) == t_sewage) {
            g->m.ter_set(dirx, diry, t_sewage);
            p->moves -= 1000;
            sounds::sound(dirp, 15, _("grnd grnd grnd"));
            g->m.spawn_item(p->posx(), p->posy(), "pipe", 3);
        } else {
            g->m.ter_set(dirx, diry, t_floor);
            p->moves -= 500;
            sounds::sound(dirp, 15, _("grnd grnd grnd"));
            g->m.spawn_item(p->posx(), p->posy(), "pipe", 3);
        }
    } else {
        add_msg(m_info, _("You can't cut that."));
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::portable_structure(player *p, item *it, bool, const tripoint& )
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
    int posx = radius * (dirx - p->posx()) + dirx;
        //(radius + 1)*posx + p->posx();
    int posy = radius * (diry - p->posy()) + diry;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            tripoint dest( posx + i, posy + j, p->posz() );
            if (!g->m.has_flag("FLAT", dest) ||
                 g->m.veh_at( dest ) != nullptr ||
                !g->is_empty( dest ) ||
                 g->critter_at( dest ) != nullptr ||
                    g->m.has_furn(dest)) {
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
    g->m.furn_set(posx - radius*(dirx - p->posx()), posy - radius*(diry - p->posy()), door);
    add_msg(m_info, _("You set up the %s on the ground."), it->tname().c_str());
    add_msg(m_info, _("Examine the center square to pack it up again."), it->tname().c_str());
    return 1;
}


int iuse::torch_lit(player *p, item *it, bool t, const tripoint &pos)
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
        p->add_msg_if_player(_("The %s winks out."), it->tname().c_str());
    } else { // Turning it off
        int choice = menu(true, _("torch (lit)"), _("extinguish"),
                          _("light something"), _("cancel"), NULL);
        switch (choice) {
            case 1: {
                p->add_msg_if_player(_("The torch is extinguished."));
                it->charges -= 1;
                it->make("torch");
                it->active = false;
                return 0;
            }
            break;
            case 2: {
                tripoint temp = pos;
                if( firestarter_actor::prep_firestarter_use(p, it, temp) ) {
                    p->moves -= 5;
                    firestarter_actor::resolve_firestarter_use(p, it, temp);
                    return it->type->charges_to_use();
                }
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::battletorch_lit(player *p, item *it, bool t, const tripoint &pos)
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
                p->add_msg_if_player(_("The Louisville Slaughterer is extinguished."));
                it->charges -= 1;
                it->make("battletorch");
                it->active = false;
                return 0;
            }
            break;
            case 2: {
                tripoint temp = pos;
                if( firestarter_actor::prep_firestarter_use(p, it, temp) ) {
                    p->moves -= 5;
                    firestarter_actor::resolve_firestarter_use(p, it, temp);
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

int iuse::bullet_puller(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // Uses NPC inventory, disarms them
        return 0;
    }

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
    ///\EFFECTS_GUN >1 allows disassembling ammunition
    if (p->skillLevel( skill_id( "gun" ) ) < 2) {
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
    p->practice( skill_fabrication, rng(1, multiply / 5 + 1));
    return it->type->charges_to_use();
}

int iuse::boltcutters(player *p, item *it, bool, const tripoint &pos )
{
    tripoint dirp = pos;
    if (!choose_adjacent(_("Cut up metal where?"), dirp)) {
        return 0;
    }
    int &dirx = dirp.x;
    int &diry = dirp.y;

    if (dirx == p->posx() && diry == p->posy()) {
        p->add_msg_if_player(
            _("You neatly sever all of the veins and arteries in your body.  Oh wait, Never mind."));
        return 0;
    }
    if (g->m.ter(dirx, diry) == t_chaingate_l) {
        p->moves -= 100;
        g->m.ter_set(dirx, diry, t_chaingate_c);
        sounds::sound(dirp, 5, _("Gachunk!"));
        g->m.spawn_item(p->posx(), p->posy(), "scrap", 3);
    } else if (g->m.ter(dirx, diry) == t_chainfence_v || g->m.ter(dirx, diry) == t_chainfence_h) {
        p->moves -= 500;
        g->m.ter_set(dirx, diry, t_chainfence_posts);
        sounds::sound(dirp, 5, _("Snick, snick, gachunk!"));
        g->m.spawn_item(dirx, diry, "wire", 20);
    } else {
        add_msg(m_info, _("You can't cut that."));
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::mop(player *p, item *it, bool, const tripoint& )
{
    int dirx, diry;
    if (!choose_adjacent(_("Mop where?"), dirx, diry)) {
        return 0;
    }

    tripoint dirp( dirx, diry, p->posz() );
    if (dirx == p->posx() && diry == p->posy()) {
        p->add_msg_if_player(_("You mop yourself up."));
        p->add_msg_if_player(_("The universe implodes and reforms around you."));
        return 0;
    }
    if (p->has_effect("blind") || p->worn_with_flag("BLIND")) {
        add_msg(_("You move the mop around, unsure whether it's doing any good."));
        p->moves -= 15;
        if (one_in(3) && g->m.moppable_items_at( dirp )) {
            g->m.mop_spills( dirp );
        }
    } else {
        if (g->m.moppable_items_at( dirp )) {
            g->m.mop_spills( dirp );
            add_msg(_("You mop up the spill."));
            p->moves -= 15;
        } else {
            p->add_msg_if_player(m_info, _("There's nothing to mop there."));
            return 0;
        }
    }
    return it->type->charges_to_use();
}

int iuse::LAW(player *p, item *it, bool, const tripoint& )
{
    p->add_msg_if_player(_("You pull the activating lever, readying the LAW to fire."));
    it->make("LAW");
    // When converting a tool to a gun, you need to set the current ammo type, this is usually done when a gun is reloaded.
    it->set_curammo( "66mm_HEAT" );
    return it->type->charges_to_use();
}

/* MACGUFFIN FUNCTIONS
 * These functions should refer to it->associated_mission for the particulars
 */
int iuse::mcg_note(player *, item *, bool, const tripoint& )
{
    return 0;
}

int iuse::artifact(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // TODO: Allow this for trusting NPCs
        return 0;
    }

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
                            it->tname( 1, false ).c_str());
    }

    const auto art = it->type->artifact.get();
    size_t num_used = rng(1, art->effects_activated.size());
    if (num_used < art->effects_activated.size()) {
        num_used += rng(1, art->effects_activated.size() - num_used);
    }

    std::vector<art_effect_active> effects = art->effects_activated;
    for (size_t i = 0; i < num_used && !effects.empty(); i++) {
        const art_effect_active used = random_entry_removed( effects );

        switch (used) {
            case AEA_STORM: {
                sounds::sound(p->pos(), 10, _("Ka-BOOM!"));
                int num_bolts = rng(2, 4);
                for (int j = 0; j < num_bolts; j++) {
                    int xdir = 0, ydir = 0;
                    while (xdir == 0 && ydir == 0) {
                        xdir = rng(-1, 1);
                        ydir = rng(-1, 1);
                    }
                    int dist = rng(4, 12);
                    int boltx = p->posx(), bolty = p->posy();
                    for (int n = 0; n < dist; n++) {
                        boltx += xdir;
                        bolty += ydir;
                        g->m.add_field( {boltx, bolty, p->posz()}, fd_electricity, rng(2, 3), 0 );
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
                tripoint fireball = g->look_around();
                if( fireball != tripoint_min ) {
                    g->explosion( fireball, 24, 0.5, 0, true );
                }
            }
            break;

            case AEA_ADRENALINE:
                p->add_msg_if_player(m_good, _("You're filled with a roaring energy!"));
                p->add_effect("adrenaline", rng(200, 250));
                break;

            case AEA_MAP: {
                const tripoint center = p->global_omt_location();
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
                for (int x = p->posx() - 4; x <= p->posx() + 4; x++) {
                    for (int y = p->posy() - 4; y <= p->posy() + 4; y++) {
                        if (!one_in(4) && g->m.add_field({x, y, p->posz()}, fd_blood, 3, 0 ) &&
                            (blood || g->u.sees(x, y))) {
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
                int x = rng(p->posx() - 3, p->posx() + 3), y = rng(p->posy() - 3, p->posy() + 3);
                g->m.add_field({x, y, p->posz()}, fd_fatigue, rng(1, 2), 0);
            }
            break;

            case AEA_ACIDBALL: {
                tripoint acidball = g->look_around();
                if( acidball != tripoint_min ) {
                    for (int x = acidball.x - 1; x <= acidball.x + 1; x++) {
                        for (int y = acidball.y - 1; y <= acidball.y + 1; y++) {
                            g->m.add_field( tripoint( x, y, acidball.z ), fd_acid, rng(2, 3), 0 );
                        }
                    }
                }
            }
            break;

            case AEA_PULSE:
                sounds::sound(p->pos(), 30, _("The earth shakes!"));
                for (int x = p->posx() - 2; x <= p->posx() + 2; x++) {
                    for (int y = p->posy() - 2; y <= p->posy() + 2; y++) {
                        tripoint pt( x, y, p->posz() );
                        g->m.bash( pt, 40 );
                        g->m.bash( pt, 40 );  // Multibash effect, so that doors &c will fall
                        g->m.bash( pt, 40 );
                        if (g->m.is_bashable( pt ) && rng(1, 10) >= 3) {
                            g->m.bash( pt, 999, false, true );
                        }
                    }
                }
                break;

            case AEA_HEAL:
                p->add_msg_if_player(m_good, _("You feel healed."));
                p->healall(2);
                break;

            case AEA_CONFUSED:
                for (int x = p->posx() - 8; x <= p->posx() + 8; x++) {
                    for (int y = p->posy() - 8; y <= p->posy() + 8; y++) {
                        tripoint dest( x, y, p->posz() );
                        int mondex = g->mon_at( dest, true );
                        if (mondex != -1) {
                            g->zombie(mondex).add_effect("stunned", rng(5, 15));
                        }
                    }
                }

            case AEA_ENTRANCE:
                for (int x = p->posx() - 8; x <= p->posx() + 8; x++) {
                    for (int y = p->posy() - 8; y <= p->posy() + 8; y++) {
                        tripoint dest( x, y, p->posz() );
                        int mondex = g->mon_at( dest, true );
                        if (mondex != -1 && g->zombie(mondex).friendly == 0 &&
                            rng(0, 600) > g->zombie(mondex).get_hp()) {
                            g->zombie(mondex).make_friendly();
                        }
                    }
                }
                break;

            case AEA_BUGS: {
                int roll = rng(1, 10);
                mtype_id bug = NULL_ID;
                int num = 0;
                std::vector<tripoint> empty;
                for (int x = p->posx() - 1; x <= p->posx() + 1; x++) {
                    for (int y = p->posy() - 1; y <= p->posy() + 1; y++) {
                        tripoint dest(x, y, p->posz());
                        if (g->is_empty(dest)) {
                            empty.push_back(dest);
                        }
                    }
                }
                if (empty.empty() || roll <= 4) {
                    p->add_msg_if_player(m_warning, _("Flies buzz around you."));
                } else if (roll <= 7) {
                    p->add_msg_if_player(m_warning, _("Giant flies appear!"));
                    bug = mon_fly;
                    num = rng(2, 4);
                } else if (roll <= 9) {
                    p->add_msg_if_player(m_warning, _("Giant bees appear!"));
                    bug = mon_bee;
                    num = rng(1, 3);
                } else {
                    p->add_msg_if_player(m_warning, _("Giant wasps appear!"));
                    bug = mon_wasp;
                    num = rng(1, 2);
                }
                if( bug ) {
                    for (int j = 0; j < num && !empty.empty(); j++) {
                        const tripoint spawnp = random_entry_removed( empty );
                        if (g->summon_mon(bug, spawnp)) {
                            monster *b = g->monster_at(spawnp);
                            b->friendly = -1;
                        }
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
                monster tmptriffid( NULL_ID, p->pos3() );
                mattack::growplants(&tmptriffid);
            }
            break;

            case AEA_HURTALL:
                for (size_t j = 0; j < g->num_zombies(); j++) {
                    g->zombie(j).apply_damage( nullptr, bp_torso, rng( 0, 5 ) );
                }
                break;

            case AEA_RADIATION:
                add_msg(m_warning, _("Horrible gases are emitted!"));
                for (int x = p->posx() - 1; x <= p->posx() + 1; x++) {
                    for (int y = p->posy() - 1; y <= p->posy() + 1; y++) {
                        g->m.add_field({x, y, p->posz()}, fd_nuke_gas, rng(2, 3), 0 );
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
                std::vector<tripoint> ps = closest_tripoints_first( 3, p->pos() );
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
                sounds::sound(p->pos(), 100, "");
                break;

            case AEA_SCREAM:
                p->add_msg_if_player(m_warning, _("Your %s screams disturbingly."), it->tname().c_str());
                sounds::sound(p->pos(), 40, "");
                p->add_morale(MORALE_SCREAM, -10, 0, 300, 5);
                break;

            case AEA_DIM:
                p->add_msg_if_player(_("The sky starts to dim."));
                g->add_event(EVENT_DIM, int(calendar::turn) + 50);
                break;

            case AEA_FLASH:
                p->add_msg_if_player(_("The %s flashes brightly!"), it->tname().c_str());
                g->flashbang( p->pos3() );
                break;

            case AEA_VOMIT:
                p->add_msg_if_player(m_bad, _("A wave of nausea passes through you!"));
                p->vomit();
                break;

            case AEA_SHADOWS: {
                int num_shadows = rng(4, 8);
                int num_spawned = 0;
                for (int j = 0; j < num_shadows; j++) {
                    int tries = 0;
                    tripoint monp = p->pos();
                    do {
                        if (one_in(2)) {
                            monp.x = rng(p->posx() - 5, p->posx() + 5);
                            monp.y = (one_in(2) ? p->posy() - 5 : p->posy() + 5);
                        } else {
                            monp.x = (one_in(2) ? p->posx() - 5 : p->posx() + 5);
                            monp.y = rng(p->posy() - 5, p->posy() + 5);
                        }
                    } while (tries < 5 && !g->is_empty(monp) &&
                             !g->m.sees(monp, p->pos(), 10));
                    if (tries < 5) {
                        if (g->summon_mon(mon_shadow, monp)) {
                            num_spawned++;
                            monster *spawned = g->monster_at(monp);
                            spawned->reset_special_rng("DISAPPEAR");
                        }
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

int iuse::spray_can(player *p, item *it, bool, const tripoint& )
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

    return handle_ground_graffiti(p, it, ismarker ? _("Write what?") : _("Spray what?"));
}

int iuse::handle_ground_graffiti(player *p, item *it, const std::string prefix)
{
    std::string message = string_input_popup( prefix + " " + _("(To delete, input one '.')"),
                                              0, "", "", "graffiti" );

    if( message.empty() ) {
        return 0;
    } else {
        const auto where = p->pos3();
        int move_cost;
        if( message == "." ) {
            if( g->m.has_graffiti_at( where ) ) {
                move_cost = 3 * g->m.graffiti_at( where ).length();
                g->m.delete_graffiti( where );
                add_msg( _("You manage to get rid of the message on the ground.") );
            } else {
                add_msg( _("There isn't anything to erase here.") );
                return 0;
            }
        } else {
            g->m.set_graffiti( where, message );
            add_msg( _("You write a message on the ground.") );
            move_cost = 2 * message.length();
        }
        p->moves -= move_cost;
    }

    return it->type->charges_to_use();
}

/**
 * Heats up a food item.
 * @return 1 if an item was heated, false if nothing was heated.
 */
static bool heat_item(player *p)
{
   auto loc = g->inv_map_splice( []( const item & itm ) {
        return (itm.is_food() && itm.has_flag("EATEN_HOT")) ||
            (itm.is_food_container() && itm.contents[0].has_flag("EATEN_HOT"));
    }, _( "Heat up what?" ), 1 );

    item *heat = loc.get_item();
    if( heat == nullptr ) {
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

int iuse::heatpack(player *p, item *it, bool, const tripoint& )
{
    if (heat_item(p)) {
        it->make("heatpack_used");
    }
    return 0;
}

int iuse::hotplate(player *p, item *it, bool, const tripoint& )
{
    if ( it->type->id != "atomic_coffeepot" && (it->charges < it->type->charges_to_use()) ) {
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

int iuse::quiver(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ) {
        // Could cause player vs. NPC inventory conflict
        return 0;
    }

    int choice = -1;
    if (!(it->contents.empty()) && it->contents[0].charges > 0) {
        choice = menu(true, _("Do what with quiver?"), _("Store more arrows"),
                      _("Empty quiver"), _("Cancel"), NULL);

        // empty quiver
        if (choice == 2) {
            item &arrows = it->contents[0];
            int arrowsRemoved = arrows.charges;
            p->add_msg_if_player(ngettext("You remove the %1$s from the %2$s.", "You remove the %1$s from the %2$s.",
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
            item clone = it->contents[0];
            clone.charges = toomany;
            p->i_add(clone);
        }

        arrowsStored = it->contents[0].charges - arrowsStored;
        p->add_msg_if_player(ngettext("You store %1$d %2$s in your %3$s.", "You store %1$d %2$s in your %3$s.",
                                      arrowsStored),
                             arrowsStored, it->contents[0].type_name( arrowsStored ).c_str(), it->tname().c_str());
        p->moves -= 10 * arrowsStored;
    } else {
        p->add_msg_if_player(_("Never mind."));
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::towel(player *p, item *it, bool t, const tripoint& )
{
    if( t ) {
        // Continuous usage, do nothing as not initiated by the player, this is for
        // wet towels only as they are active items.
        return 0;
    }
    bool slime = p->has_effect("slimed");
    bool boom = p->has_effect("boomered");
    bool glow = p->has_effect("glowing");
    int mult = slime + boom + glow; // cleaning off more than one at once makes it take longer
    bool towelUsed = false;

    // can't use an already wet towel!
    if (it->has_flag("WET")) {
        p->add_msg_if_player(m_info, _("That %s is too wet to soak up any more liquid!"),
                             it->tname().c_str());


    // clean off the messes first, more important
    } else if (slime || boom || glow) {
        p->remove_effect("slimed");  // able to clean off all at once
        p->remove_effect("boomered");
        p->remove_effect("glowing");
        p->add_msg_if_player(_("You use the %s to clean yourself off, saturating it with slime!"),
                             it->tname().c_str());

        towelUsed = true;
        if (it->type->id == "towel") {
            it->make("towel_soiled");
        }

    // dry off from being wet
    } else if (abs(p->has_morale(MORALE_WET))) {
        p->rem_morale(MORALE_WET);
        for (int i = 0; i < num_bp; ++i) {
            p->body_wetness[i] = 0;
        }
        p->add_msg_if_player(_("You use the %s to dry off, saturating it with water!"),
                             it->tname().c_str());

        towelUsed = true;
        it->item_counter = 300;

    // default message
    } else {
        p->add_msg_if_player(_("You are already dry, the %s does nothing."), it->tname().c_str());
    }

    // towel was used
    if (towelUsed) {
        if ( mult == 0 ) {
            mult = 1;
        }
        p->moves -= 50 * mult;
        // change "towel" to a "towel_wet" (different flavor text/color)
        if (it->type->id == "towel") {
            it->make("towel_wet");
        }

        // WET, active items have their timer decremented every turn
        it->item_tags.insert("WET");
        it->active = true;
    }
    return it->type->charges_to_use();
}

int iuse::unfold_generic(player *p, item *it, bool, const tripoint& )
{
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    vehicle *veh = g->m.add_vehicle( vproto_id( "none" ), p->posx(), p->posy(), 0, 0, 0, false);
    if (veh == NULL) {
        p->add_msg_if_player(m_info, _("There's no room to unfold the %s."), it->tname().c_str());
        return 0;
    }
    veh->name = it->get_var( "vehicle_name" );
    if (!veh->restore(it->get_var( "folding_bicycle_parts" ))) {
        g->m.destroy_vehicle(veh);
        return 0;
    }
    g->m.add_vehicle_to_cache( veh );

    std::string unfold_msg = it->get_var( "unfold_msg" );
    if (unfold_msg.size() == 0) {
        unfold_msg = _("You painstakingly unfold the %s and make it ready to ride.");
    } else {
        unfold_msg = _(unfold_msg.c_str());
    }
    p->add_msg_if_player(unfold_msg.c_str(), veh->name.c_str());

    p->moves -= it->get_var( "moves", 500 );
    return 1;
}

int iuse::adrenaline_injector(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() && p->stim > 100 ) {
        return 0;
    }

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

int iuse::jet_injector(player *p, item *it, bool, const tripoint& )
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
        p->healall(20);
    }

    if (p->has_effect("jetinjector")) {
        if (p->get_effect_dur("jetinjector") > 200) {
            p->add_msg_if_player(m_warning, _("Your heart is beating alarmingly fast!"));
        }
    }
    return it->type->charges_to_use();
}

int iuse::stimpack(player *p, item *it, bool, const tripoint& )
{
    if (p->get_item_position(it) >= -1) {
        p->add_msg_if_player(m_info,
                             _("You must wear the stimulant delivery system before you can activate it."));
        return 0;
    }     if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(m_info, _("The stimulant delivery system is empty."), it->tname().c_str());
        return 0;
    } else {
        p->add_msg_if_player(_("You inject yourself with the stimulants."));
        // Intensity is 2 here because intensity = 1 is the comedown
        p->add_effect("stimpack", 250, num_bp, false, 2);
        p->pkill += 2;
        p->stim += 20;
        p->fatigue -= 100;
        p->stamina = p->get_stamina_max();
    }
    return it->type->charges_to_use();
}

int iuse::radglove(player *p, item *it, bool, const tripoint& )
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
    if( p->is_npc() ) {
        const npc *np = dynamic_cast<const npc *>( p );
        if( np->radiation > 0 ) {
            np->say( _("It says here that my radiation level is %d"), np->radiation );
        } else {
            np->say( _("It says I'm not irradiated") );
        }
    }
    return it->type->charges_to_use();
}


int iuse::contacts(player *p, item *it, bool, const tripoint& )
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

int iuse::talking_doll(player *p, item *it, bool, const tripoint& )
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

    sounds::ambient_sound(p->pos(), speech.volume, speech.text);

    return it->type->charges_to_use();
}

int iuse::gun_repair(player *p, item *it, bool, const tripoint& )
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    ///\EFFECT_MECHANICS >1 allows gun repair
    if (p->skillLevel( skill_mechanics ) < 2) {
        p->add_msg_if_player(m_info, _("You need a mechanics skill of 2 to use this repair kit."));
        return 0;
    }
    int inventory_index = g->inv(_("Select the firearm to repair"));
    item *fix = &(p->i_at(inventory_index));
    if (fix == NULL || fix->is_null()) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    if (!is_firearm(*fix)) {
        p->add_msg_if_player(m_info, _("That isn't a firearm!"));
        return 0;
    }
    if (fix->damage == -1) {
        p->add_msg_if_player(m_info, _("You cannot improve your %s any more this way."),
                             fix->tname().c_str());
        return 0;
    }
    if ((fix->damage == 0) && p->skillLevel( skill_mechanics ) < 8) {
        p->add_msg_if_player(m_info, _("Your %s is already in peak condition."), fix->tname().c_str());
        p->add_msg_if_player(m_info, _("With a higher mechanics skill, you might be able to improve it."));
        return 0;
    }
    ///\EFFECT_MECHANICS >7 allows accurizing ranged weapons
    if ((fix->damage == 0) && p->skillLevel( skill_mechanics ) >= 8) {
        p->add_msg_if_player(m_good, _("You accurize your %s."), fix->tname().c_str());
        sounds::sound(p->pos(), 6, "");
        p->moves -= 2000 * p->fine_detail_vision_mod();
        p->practice( skill_mechanics, 10);
        fix->damage--;
    } else if (fix->damage >= 2) {
        p->add_msg_if_player(m_good, _("You repair your %s!"), fix->tname().c_str());
        sounds::sound(p->pos(), 8, "");
        p->moves -= 1000 * p->fine_detail_vision_mod();
        p->practice( skill_mechanics, 10);
        fix->damage--;
    } else {
        p->add_msg_if_player(m_good, _("You repair your %s completely!"),
                             fix->tname().c_str());
        sounds::sound(p->pos(), 8, "");
        p->moves -= 500 * p->fine_detail_vision_mod();
        p->practice( skill_mechanics, 10);
        fix->damage = 0;
    }
    return it->type->charges_to_use();
}

int iuse::misc_repair(player *p, item *it, bool, const tripoint& )
{
    if (it->charges < it->type->charges_to_use()) {
        return 0;
    }
    if (p->is_underwater()) {
        p->add_msg_if_player(m_info, _("You can't do that while underwater."));
        return 0;
    }
    if (p->fine_detail_vision_mod() > 4) {
        add_msg(m_info, _("You can't see to repair!"));
        return 0;
    }
    ///\EFFECT_FABRICATION >0 allows use of repair kit
    if (p->skillLevel( skill_fabrication ) < 1) {
        p->add_msg_if_player(m_info, _("You need a fabrication skill of 1 to use this repair kit."));
        return 0;
    }
    int inventory_index = g->inv_for_filter( _("Select the item to repair."), []( const item & itm ) {
        return ( !is_firearm(itm) ) && (itm.made_of("wood") || itm.made_of("paper") ||
                                 itm.made_of("bone") || itm.made_of("chitin") ) ;
    } );
    item *fix = &( p->i_at(inventory_index ) );
    if (fix == NULL || fix->is_null()) {
        p->add_msg_if_player(m_info, _("You do not have that item!"));
        return 0;
    }
    if ( is_firearm(*fix) ) {
        p->add_msg_if_player(m_info, _("That requires gunsmithing tools."));
        return 0;
    }
    if (!(fix->made_of("wood") || fix->made_of("paper") || fix->made_of("bone") ||
          fix->made_of("chitin"))) {
        p->add_msg_if_player(m_info, _("That isn't made of wood, paper, bone, or chitin!"));
        return 0;
    }
    if (fix->damage == -1) {
        p->add_msg_if_player(m_info, _("You cannot improve your %s any more this way."),
                             fix->tname().c_str());
        return 0;
    }
    if (fix->damage == 0 && fix->has_flag( "PRIMITIVE_RANGED_WEAPON" )) {
        p->add_msg_if_player(m_info, _("You cannot improve your %s any more this way."),
                             fix->tname().c_str());
        return 0;
    }
    if (fix->damage == 0) {
        p->add_msg_if_player(m_good, _("You reinforce your %s."), fix->tname().c_str());
        p->moves -= 1000 * p->fine_detail_vision_mod();
        p->practice( skill_fabrication, 10);
        fix->damage--;
    } else if (fix->damage >= 2) {
        p->add_msg_if_player(m_good, _("You repair your %s!"), fix->tname().c_str());
        p->moves -= 500 * p->fine_detail_vision_mod();
        p->practice( skill_fabrication, 10);
        fix->damage--;
    } else {
        p->add_msg_if_player(m_good, _("You repair your %s completely!"), fix->tname().c_str());
        p->moves -= 250 * p->fine_detail_vision_mod();
        p->practice( skill_fabrication, 10);
        fix->damage = 0;
    }
    return it->type->charges_to_use();
}

int iuse::bell(player *p, item *it, bool, const tripoint& )
{
    if (it->type->id == "cow_bell") {
        sounds::sound(p->pos(), 12, _("Clank! Clank!"));
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
        sounds::sound(p->pos(), 4, _("Ring! Ring!"));
    }
    return it->type->charges_to_use();
}

int iuse::seed(player *p, item *it, bool, const tripoint& )
{
    if( p->is_npc() ||
        query_yn(_("Sure you want to eat the %s? You could plant it in a mound of dirt."),
            it->tname().c_str())) {
        return it->type->charges_to_use(); //This eats the seed object.
    }
    return 0;
}

int iuse::robotcontrol(player *p, item *it, bool, const tripoint& )
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(_("The %s's batteries are dead."), it->tname().c_str());
        return 0;

    }
    if( p->has_trait("ILLITERATE") ) {
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
            std::vector< monster* > mons;
            std::vector< tripoint > locations;
            int entry_num = 0;
            for( size_t i = 0; i < g->num_zombies(); ++i ) {
                monster &candidate = g->zombie( i );
                if( candidate.type->in_species( ROBOT ) && candidate.friendly == 0 &&
                    rl_dist( p->pos3(), candidate.pos3() ) <= 10 ) {
                    mons.push_back( &candidate );
                    pick_robot.addentry( entry_num++, true, MENU_AUTOASSIGN, candidate.name() );
                    tripoint seen_loc;
                    // Show locations of seen robots, center on player if robot is not seen
                    if( p->sees( candidate ) ) {
                        seen_loc = candidate.pos();
                    } else {
                        seen_loc = p->pos();
                    }
                    locations.push_back( seen_loc );
                }
            }
            if( mons.empty() ) {
                p->add_msg_if_player( m_info, _("No enemy robots in range.") );
                return it->type->charges_to_use();
            }
            pointmenu_cb callback( locations );
            pick_robot.callback = &callback;
            pick_robot.addentry( INT_MAX, true, -1, _( "Cancel" ) );
            pick_robot.query();
            const size_t mondex = pick_robot.ret;
            if( mondex >= mons.size() ) {
                p->add_msg_if_player(m_info, _("Never mind"));
                return it->type->charges_to_use();
            }
            monster *z = mons[mondex];
            p->add_msg_if_player(_("You start reprogramming the %s into an ally."), z->name().c_str());
            ///\EFFECT_INT speeds up robot reprogramming

            ///\EFFECT_COMPUTER speeds up robot reprogramming
            p->moves -= 1000 - p->int_cur * 10 - p->skillLevel( skill_computer ) * 10;
            ///\EFFECT_INT increases chance of successful robot reprogramming, vs difficulty

            ///\EFFECT_COMPUTER increases chance of successful robot reprogramming, vs difficulty
            float success = p->skillLevel( skill_computer ) - 1.5 * (z->type->difficulty) /
                            ((rng(2, p->int_cur) / 2) + (p->skillLevel( skill_computer ) / 2));
            if (success >= 0) {
                p->add_msg_if_player(_("You successfully override the %s's IFF protocols!"),
                                     z->name().c_str());
                z->friendly = -1;
            } else if (success >= -2) { //A near success
                p->add_msg_if_player(_("The %s short circuits as you attempt to reprogram it!"),
                                     z->name().c_str());
                z->apply_damage( p, bp_torso, rng( 1, 10 ) ); //damage it a little
                if( z->is_dead() ) {
                    p->practice( skill_computer, 10);
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
            p->practice( skill_computer, 10);
            return it->type->charges_to_use();
        }
        case 2: { //make all friendly robots stop their purposeless extermination of (un)life.
            p->moves -= 100;
            int f = 0; //flag to check if you have robotic allies
            for (size_t i = 0; i < g->num_zombies(); i++) {
                if (g->zombie(i).friendly != 0 && g->zombie(i).type->in_species( ROBOT )) {
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
            it->set_var( "MC_PHOTOS", duckfaces_count );
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
            it->set_var( "MC_MUSIC", new_songs_count );
        }
        data_chance++;
        if (encrypted && one_in(2)) {
            data_chance--;
        }

        if (one_in(data_chance)) {
            it->set_var( "MC_RECIPE", "SIMPLE" );
        }

        if (it->has_flag("MC_SCIENCE_STUFF")) {
            it->set_var( "MC_RECIPE", "SCIENCE" );
        }
    }
}

bool einkpc_download_memory_card(player *p, item *eink, item *mc)
{
    bool something_downloaded = false;
    if (mc->get_var( "MC_PHOTOS", 0 ) > 0) {
        something_downloaded = true;

        int new_photos = mc->get_var( "MC_PHOTOS", 0 );
        mc->erase_var( "MC_PHOTOS" );

        p->add_msg_if_player(m_good, string_format(
                                 ngettext("You download %d new photo into internal memory.",
                                          "You download %d new photos into internal memory.", new_photos),
                                                   new_photos).c_str());

        const int old_photos = eink->get_var( "EIPC_PHOTOS", 0 );
        eink->set_var( "EIPC_PHOTOS", old_photos + new_photos);
    }

    if (mc->get_var( "MC_MUSIC", 0 ) > 0) {
        something_downloaded = true;

        int new_songs = mc->get_var( "MC_MUSIC", 0 );
        mc->erase_var( "MC_MUSIC" );

        p->add_msg_if_player(m_good, string_format(
                                 ngettext("You download %d new song into internal memory.",
                                          "You download %d new songs into internal memory.", new_songs),
                                                   new_songs).c_str());

        const int old_songs = eink->get_var( "EIPC_MUSIC", 0 );
        eink->set_var( "EIPC_MUSIC", old_songs + new_songs);
    }

    if (!mc->get_var( "MC_RECIPE" ).empty()) {
        const bool science = mc->get_var( "MC_RECIPE" ) == "SCIENCE";

        mc->erase_var( "MC_RECIPE" );

        std::vector<const recipe *> candidates;

        for( auto &elem : recipe_dict ) {

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

        if (candidates.size() > 0) {

            const recipe *r = random_entry( candidates );
            const std::string rident = r->ident;

            const item dummy(r->result, 0);

            const auto old_recipes = eink->get_var( "EIPC_RECIPES" );
            if( old_recipes.empty() ) {
                something_downloaded = true;
                eink->set_var( "EIPC_RECIPES", "," + rident + "," );

                p->add_msg_if_player(m_good, _("You download a recipe for %s into the tablet's memory."),
                                     dummy.type_name().c_str());
            } else {
                if (old_recipes.find("," + rident + ",") == std::string::npos) {
                    something_downloaded = true;
                    eink->set_var( "EIPC_RECIPES", old_recipes + rident + "," );

                    p->add_msg_if_player(m_good, _("You download a recipe for %s into the tablet's memory."),
                                         dummy.type_name().c_str());
                } else {
                    p->add_msg_if_player(m_good, _("Your tablet already has a recipe for %s."),
                                         dummy.type_name().c_str());
                }
            }
        }
    }

    const auto monster_photos = mc->get_var( "MC_MONSTER_PHOTOS" );
    if( !monster_photos.empty() ) {
        something_downloaded = true;
        p->add_msg_if_player(m_good, _("You have updated your monster collection."));

        auto photos = eink->get_var( "EINK_MONSTER_PHOTOS" );
        if( photos.empty() ) {
            eink->set_var( "EINK_MONSTER_PHOTOS", monster_photos );
        } else {
            std::istringstream f(monster_photos);
            std::string s;
            while (getline(f, s, ',')) {

                if (s.size() == 0) {
                    continue;
                }

                const std::string mtype = s;
                getline(f, s, ',');
                char *chq = &s[0];
                const int quality = atoi(chq);

                const size_t eink_strpos = photos.find("," + mtype + ",");

                if (eink_strpos == std::string::npos) {
                    photos += mtype + "," + string_format("%d", quality) + ",";
                } else {

                    const size_t strqpos = eink_strpos + mtype.size() + 2;
                    char *chq = &photos[strqpos];
                    const int old_quality = atoi(chq);

                    if (quality > old_quality) {
                        chq = &string_format("%d", quality)[0];
                        photos[strqpos] = *chq;
                    }
                }

            }
            eink->set_var( "EINK_MONSTER_PHOTOS", photos );
        }
    }

    if (mc->has_flag("MC_TURN_USED")) {
        mc->clear_vars();
        mc->unset_flags();
        mc->make("mobile_memory_card_used");
    }

    if (!something_downloaded) {
        p->add_msg_if_player(m_info, _("This memory card does not contain any new data."));
        return false;
    }

    return true;

}

static const std::string &photo_quality_name( const int index )
{
    static std::array<std::string, 6> const names { {
        { _("awful") }, { _("bad") }, { _("not bad") }, { _("good") }, { _("fine") }, { _("exceptional") } } };
    return names[index];
}


int iuse::einktabletpc(player *p, item *it, bool t, const tripoint &pos)
{
    if (t) {
        if( it->get_var( "EIPC_MUSIC_ON" ) != "" ) {
            if( calendar::once_every(MINUTES(5)) ) {
                it->charges--;
            }

            //the more varied music, the better max mood.
            const int songs = it->get_var( "EIPC_MUSIC", 0 );
            play_music( p, pos, 8, std::min( 100, songs ) );
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

        const int photos = it->get_var( "EIPC_PHOTOS", 0 );
        if( photos > 0 ) {
            amenu.addentry(ei_photo, true, 'p', _("Photos [%d]"), photos);
        } else {
            amenu.addentry(ei_photo, false, 'p', _("No photos on device"));
        }

        const int songs = it->get_var( "EIPC_MUSIC", 0 );
        if( songs > 0 ) {
            if (it->active) {
                amenu.addentry(ei_music, true, 'm', _("Turn music off"));
            } else {
                amenu.addentry(ei_music, true, 'm', _("Turn music on [%d]"), songs);
            }
        } else {
            amenu.addentry(ei_music, false, 'm', _("No music on device"));
        }

        if (it->get_var( "RECIPE" ) != "") {
            const item dummy(it->get_var( "RECIPE" ), 0);
            amenu.addentry(0, false, -1, _("Recipe: %s"), dummy.tname().c_str());
        }

        if (it->get_var( "EIPC_RECIPES" ) != "") {
            amenu.addentry(ei_recipe, true, 'r', _("View recipe on E-ink screen"));
        }

        if (it->get_var( "EINK_MONSTER_PHOTOS" ) != "") {
            amenu.addentry(ei_monsters, true, 'y', _("Your collection of monsters"));
        } else {
            amenu.addentry(ei_monsters, false, 'y', _("Collection of monsters is empty"));
        }

        amenu.addentry(ei_download, true, 'w', _("Download data from memory card"));

        ///\EFFECT_COMPUTER >2 allows decrypting memory cards more easily
        if (p->skillLevel( skill_computer ) > 2) {
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

            const int photos = it->get_var( "EIPC_PHOTOS", 0 );
            const int viewed = std::min(photos, int(rng(10, 30)));
            const int count = photos - viewed;
            if (count == 0) {
                it->erase_var( "EIPC_PHOTOS" );
            } else {
                it->set_var( "EIPC_PHOTOS", count );
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
                        p->add_msg_if_player(m_good, _("Ha-ha!  An amusing cat photo."));
                        break;
                    case 3:
                        p->add_msg_if_player(m_good, _("Excellent pictures of nature."));
                        break;
                    case 4:
                        p->add_msg_if_player(m_good, _("Food photos... your stomach rumbles!"));
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
                it->erase_var( "EIPC_MUSIC_ON" );

                p->add_msg_if_player(m_info, _("You turned off music on your %s."), it->tname().c_str());
            } else {
                it->active = true;
                it->set_var( "EIPC_MUSIC_ON", "1" );

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
            std::istringstream f(it->get_var( "EIPC_RECIPES" ));
            std::string s;
            int k = 1;
            while (getline(f, s, ',')) {

                if (s.size() == 0) {
                    continue;
                }

                candidate_recipes.push_back(s);

                auto recipe = recipe_by_name( s );
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
                const auto rec_id = candidate_recipes[rchoice - 1];
                it->set_var( "RECIPE", rec_id );

                auto recipe = recipe_by_name( rec_id );
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

            std::vector<mtype_id> monster_photos;

            std::istringstream f(it->get_var( "EINK_MONSTER_PHOTOS" ));
            std::string s;
            int k = 1;
            while (getline(f, s, ',')) {
                if (s.size() == 0) {
                    continue;
                }
                monster_photos.push_back( mtype_id( s ) );
                std::string menu_str;
                const monster dummy( monster_photos.back() );
                menu_str = dummy.name();
                getline(f, s, ',');
                char *chq = &s[0];
                const int quality = atoi(chq);
                menu_str += " [" + photo_quality_name( quality ) + "]";
                pmenu.addentry(k++, true, -1, menu_str.c_str());
            }

            int choice;
            do {
                pmenu.query();
                choice = pmenu.ret;

                if (0 == choice) {
                    break;
                }

                const monster dummy( monster_photos[choice - 1] );
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

            p->practice( skill_computer, rng(2, 5));

            ///\EFFECT_INT increases chance of safely decrypting memory card

            ///\EFFECT_COMPUTER increases chance of safely decrypting memory card
            const int success = p->skillLevel( skill_computer ) * rng(1, p->skillLevel( skill_computer )) *
                rng(1, p->int_cur) - rng(30, 80);
            if (success > 0) {
                p->practice( skill_computer , rng(5, 10));
                p->add_msg_if_player(m_good, _("You successfully decrypted content on %s!"),
                                     mc->tname().c_str());
                einkpc_download_memory_card(p, it, mc);
            } else {
                if (success > -10 || one_in(5)) {
                    p->add_msg_if_player(m_neutral, _("You failed to decrypt the %s."), mc->tname().c_str());
                } else {
                    p->add_msg_if_player(m_bad, _("You tripped the firmware protection, and the card deleted its data!"));
                    mc->clear_vars();
                    mc->unset_flags();
                    mc->make("mobile_memory_card_used");
                }
            }
            return it->type->charges_to_use();
        }
    }
    return 0;
}

int iuse::camera(player *p, item *it, bool, const tripoint& )
{
    enum {c_cancel, c_shot, c_photos, c_upload};

    uimenu amenu;

    amenu.selected = 0;
    amenu.text = _("What to do with camera?");
    amenu.addentry(c_shot, true, 'p', _("Take a photo"));
    if (it->get_var( "CAMERA_MONSTER_PHOTOS" ) != "") {
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

        tripoint aim_point = g->look_around();

        if( aim_point == tripoint_min ) {
            p->add_msg_if_player(_("Never mind."));
            return 0;
        }

        if( aim_point == p->pos() ) {
            p->add_msg_if_player(_("You decide not to flash yourself."));
            return 0;
        }

        const int sel_zid = g->mon_at( aim_point, true );
        const int sel_npcID = g->npc_at( aim_point );

        if (sel_zid == -1 && sel_npcID == -1) {
            p->add_msg_if_player(_("There's nothing particularly interesting there."));
            return 0;
        }

        std::vector<tripoint> trajectory = line_to( p->pos(), aim_point, 0, 0 );
        trajectory.push_back(aim_point);

        p->moves -= 50;
        sounds::sound( p->pos(), 8, _("Click.") );

        for (auto &i : trajectory) {

            int zid = g->mon_at( i, true );
            int npcID = g->npc_at(i);

            if (zid != -1 || npcID != -1) {
                int dist = rl_dist( p->pos(), i );

                int camera_bonus = it->has_flag("CAMERA_PRO") ? 10 : 0;
                int photo_quality = 20 - rng(dist, dist * 2) * 2 + rng(camera_bonus / 2, camera_bonus);
                if (photo_quality > 5) {
                    photo_quality = 5;
                }
                if (photo_quality < 0) {
                    photo_quality = 0;
                }
                if (p->has_effect("blind") || p->worn_with_flag("BLIND")) {
                    photo_quality /= 2;
                }

                const std::string quality_name = photo_quality_name( photo_quality );

                if (zid != -1) {
                    monster &z = g->zombie(zid);

                    if (dist < 4 && one_in(dist + 2) && z.has_flag(MF_SEES)) {
                        p->add_msg_if_player(_("%s looks blinded."), z.name().c_str());
                        z.add_effect("blind", rng(5, 10));
                    }

                    // shoot past small monsters and hallucinations
                    if (zid != sel_zid && (z.type->size <= MS_SMALL || z.is_hallucination() || z.type->in_species( HALLUCINATION ))) {
                        continue;
                    }

                    // get an empty photo if the target is a hallucination
                    if (zid == sel_zid && (z.is_hallucination() || z.type->in_species( HALLUCINATION ))) {
                        p->add_msg_if_player(_("Strange... there's nothing in the picture?"));
                        return it->type->charges_to_use();
                    }

                    if (z.mission_id != -1) {
                        //quest processing...
                    }

                    if (zid == sel_zid) {
                        // if the loop makes it to the target, take its photo
                        if (p->has_effect("blind") || p->worn_with_flag("BLIND")) {
                            p->add_msg_if_player(_("You took a photo of %s."), z.name().c_str());
                        } else {
                            p->add_msg_if_player(_("You took a %1$s photo of %2$s."), quality_name.c_str(),
                                             z.name().c_str());
                        }
                    } else {
                        // or take a photo of the monster that's in the way
                        p->add_msg_if_player(m_warning, _("A %s got in the way of your photo."), z.name().c_str());
                        photo_quality = 0;
                    }

                    const std::string mtype = z.type->id.str();

                    auto monster_photos = it->get_var( "CAMERA_MONSTER_PHOTOS" );
                    if (monster_photos == "") {
                        monster_photos = "," + mtype + "," + string_format("%d",
                                photo_quality) + ",";
                    } else {

                        const size_t strpos = monster_photos.find("," + mtype + ",");

                        if (strpos == std::string::npos) {
                            monster_photos += mtype + "," + string_format("%d", photo_quality) + ",";
                        } else {

                            const size_t strqpos = strpos + mtype.size() + 2;
                            char *chq = &monster_photos[strqpos];
                            const int old_quality = atoi(chq);

                            if (!p->has_effect("blind") && !p->worn_with_flag("BLIND")) {
                                if (photo_quality > old_quality) {
                                    chq = &string_format("%d", photo_quality)[0];
                                    monster_photos[strqpos] = *chq;

                                    p->add_msg_if_player(_("This photo is better than the previous one."));

                                }
                            }
                        }
                    }
                    it->set_var( "CAMERA_MONSTER_PHOTOS", monster_photos );

                    return it->type->charges_to_use();

                } else {
                    npc *guy = g->active_npc[npcID];

                    if (dist < 4 && one_in(dist + 2)) {
                        p->add_msg_if_player(_("%s looks blinded."), guy->name.c_str());
                        guy->add_effect("blind", rng(5, 10));
                    }

                    //just photo, no save. Maybe in the future we will need to create CAMERA_NPC_PHOTOS
                    if (npcID == sel_npcID) {
                        if (p->has_effect("blind") || p->worn_with_flag("BLIND")) {
                            p->add_msg_if_player(_("You took a photo of %s."), guy->name.c_str());
                        } else {
                            p->add_msg_if_player(_("You took a %1$s photo of %2$s."), quality_name.c_str(),
                                             guy->name.c_str());
                        }
                    } else {
                        p->add_msg_if_player(m_warning, _("%s got in the way of your photo."), guy->name.c_str());
                        photo_quality = 0;
                    }

                    return it->type->charges_to_use();
                }

                return it->type->charges_to_use();
            }

        }

        return it->type->charges_to_use();
    }

    if (c_photos == choice) {

        if (p->has_effect("blind") || p->worn_with_flag("BLIND")) {
            p->add_msg_if_player(_("You can't see the camera screen, you're blind."));
            return 0;
        }

        uimenu pmenu;

        pmenu.selected = 0;
        pmenu.text = _("Critter photos saved on camera:");
        pmenu.addentry(0, true, 'q', _("Cancel"));

        std::vector<mtype_id> monster_photos;

        std::istringstream f(it->get_var( "CAMERA_MONSTER_PHOTOS" ));
        std::string s;
        int k = 1;
        while (getline(f, s, ',')) {

            if (s.size() == 0) {
                continue;
            }

            monster_photos.push_back( mtype_id( s ) );

            std::string menu_str;

            const monster dummy( monster_photos.back() );
            menu_str = dummy.name();

            getline(f, s, ',');
            char *chq = &s[0];
            const int quality = atoi(chq);

            menu_str += " [" + photo_quality_name( quality ) + "]";

            pmenu.addentry(k++, true, -1, menu_str.c_str());
        }

        int choice;
        do {
            pmenu.query();
            choice = pmenu.ret;

            if (0 == choice) {
                break;
            }

            const monster dummy( monster_photos[choice - 1] );
            popup(dummy.type->description.c_str());

        } while (true);

        return it->type->charges_to_use();
    }

    if (c_upload == choice) {

        if (p->has_effect("blind") || p->worn_with_flag("BLIND")) {
            p->add_msg_if_player(_("You can't see the camera screen, you're blind."));
            return 0;
        }

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
            if (!query_yn(_("This memory card is encrypted.  Format and clear data?"))) {
                return it->type->charges_to_use();
            }
        }
        if (mc->has_flag("MC_HAS_DATA")) {
            if (!query_yn(_("Are you sure you want to clear the old data on the card?"))) {
                return it->type->charges_to_use();
            }
        }

        mc->make("mobile_memory_card");
        mc->clear_vars();
        mc->unset_flags();
        mc->item_tags.insert("MC_HAS_DATA");

        mc->set_var( "MC_MONSTER_PHOTOS", it->get_var( "CAMERA_MONSTER_PHOTOS" ) );
        p->add_msg_if_player(m_info, _("You upload monster photos to memory card."));

        return it->type->charges_to_use();
    }

    return it->type->charges_to_use();
}

int iuse::ehandcuffs(player *p, item *it, bool t, const tripoint &pos)
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

            sounds::sound(pos, 2, "Click.");
            it->item_tags.erase("NO_UNWIELD");
            it->active = false;

            if (p->has_item(it) && p->weapon.type->id == "e_handcuffs") {
                add_msg(m_good, _("%s on your hands opened!"), it->tname().c_str());
            }

            return it->type->charges_to_use();
        }

        if (p->has_item(it)) {
            if (p->has_active_bionic("bio_shock") && p->power_level >= 2 && one_in(5)) {
                p->charge_power(-2);

                it->item_tags.erase("NO_UNWIELD");
                it->charges = 0;
                it->active = false;
                add_msg(m_good, _("The %s crackle with electricity from your bionic, then come off your hands!"), it->tname().c_str());

                return it->type->charges_to_use();
            }
        }

        if( calendar::once_every(MINUTES(1)) ) {
            sounds::sound(pos, 10, _("a police siren, whoop WHOOP."));
        }

        const int x = it->get_var( "HANDCUFFS_X", 0 );
        const int y = it->get_var( "HANDCUFFS_Y", 0 );

        if ((it->charges > it->type->maximum_charges() - 1000) && (x != pos.x || y != pos.y)) {

            if (p->has_item(it) && p->weapon.type->id == "e_handcuffs") {

                if( p->is_elec_immune() ) {
                    if( one_in( 10 ) ) {
                        add_msg( m_good, _("The cuffs try to shock you, but you're protected from electrocution.") );
                    }
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

            it->set_var( "HANDCUFFS_X", pos.x );
            it->set_var( "HANDCUFFS_Y", pos.y );

            return it->type->charges_to_use();

        }

        return it->type->charges_to_use();

    }

    if (it->active) {
        add_msg(_("The %s are clamped tightly on your wrists.  You can't take them off."),
                it->tname().c_str());
    } else {
        add_msg(_("The %s have discharged and can be taken off."), it->tname().c_str());
    }

    return it->type->charges_to_use();
}

int iuse::radiocar(player *p, item *it, bool, const tripoint& )
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
            _("You turned on your RC car, now place it on ground, and use radio control to play."));

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

            if (put->has_flag("RADIOCARITEM") && ((put->volume() <= 5) || (put->weight() <= 2000))) {
                p->moves -= 300;
                p->add_msg_if_player(_("You armed your RC car with %s."),
                                     put->tname().c_str());
                it->put_in(p->i_rem(inventory_index));
            } else if (!put->has_flag("RADIOCARITEM")) {
                p->add_msg_if_player(_("RC car with %s ? How?"),
                put->tname().c_str());
            } else {
                p->add_msg_if_player(_("Your %s is too heavy or bulky for this RC car."),
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

int iuse::radiocaron(player *p, item *it, bool t, const tripoint &pos)
{
    if (t) {
        //~Sound of a radio controlled car moving around
        sounds::sound(pos, 6, _("buzzz..."));

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
            sounds::sound(p->pos(), 6, _("beep."));

            auto tmp = dynamic_cast<const it_tool *>(it.type);
            if( it.has_flag("RADIO_INVOKE_PROC") ) {
                // Invoke twice: first to transform, then later to proc
                tmp->invoke( p, &it, p->pos3() );
                it.charges = 0;
                // The type changed
                tmp = dynamic_cast<const it_tool *>(it.type);
            }

            tmp->invoke(p, &it, p->pos3());
        }
    }

    g->m.trigger_rc_items( signal );
}

int iuse::radiocontrol(player *p, item *it, bool t, const tripoint& )
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

    choice = menu(true, _("What do with radio control:"), _("Nothing"), car_action,
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
        std::stringstream choice_str;
        choice_str << (choice - 2);
        signal += choice_str.str();

        auto item_list = p->get_radio_items();
        for( auto &elem : item_list ) {
            if( ( elem )->has_flag( "BOMB" ) && ( elem )->has_flag( signal ) ) {
                p->add_msg_if_player( m_warning,
                    _("The %s in you inventory would explode on this signal.  Place it down before sending the signal."),
                    ( elem )->display_name().c_str() );
                return 0;
            }
        }

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

    ///\EFFECT_INT increases chance of bypassing vehicle security system

    ///\EFFECT_COMPUTER increases chance of bypassing vehicle security system
    int roll = dice( p->skillLevel( skill_computer ) + 2, p->int_cur ) - ( advanced ? 50 : 25 );
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

    p->practice( skill_computer, advanced ? 10 : 3 );
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

vehicle *pickveh( const tripoint& center, bool advanced )
{
    static const std::string ctrl = "CONTROLS";
    static const std::string advctrl = "REMOTE_CONTROLS";
    uimenu pmenu;
    pmenu.title = _("Select vehicle to access");
    std::vector< vehicle* > vehs;

    for( auto &veh : g->m.get_vehicles() ) {
        auto &v = veh.v;
        const auto gp = v->global_pos();
        if( rl_dist( center.x, center.y, gp.x, gp.y ) < 40 &&
            v->fuel_left( "battery", true ) > 0 &&
            ( v->all_parts_with_feature( advctrl, true ).size() > 0 ||
            ( !advanced && v->all_parts_with_feature( ctrl, true ).size() > 0 ) ) ) {
            vehs.push_back( v );
        }
    }
    std::vector<tripoint> locations;
    for( int i = 0; i < (int)vehs.size(); i++ ) {
        auto veh = vehs[i];
        locations.push_back( veh->global_pos3() );
        pmenu.addentry( i, true, MENU_AUTOASSIGN, veh->name.c_str() );
    }

    if( vehs.size() == 0 ) {
        add_msg( m_bad, _("No vehicle available.") );
        return nullptr;
    }

    pmenu.addentry( vehs.size(), true, 'q', _("Cancel") );
    pointmenu_cb callback( locations );
    pmenu.callback = &callback;
    pmenu.w_y = 0;
    pmenu.query();

    if( pmenu.ret < 0 || pmenu.ret >= (int)vehs.size() ) {
        return nullptr;
    } else {
        return vehs[pmenu.ret];
    }
}

int iuse::remoteveh(player *p, item *it, bool t, const tripoint &pos)
{
    vehicle *remote = g->remoteveh();
    if( t ) {
        bool stop = false;
        if( it->charges == 0 ) {
            p->add_msg_if_player( m_bad, _("The remote control's battery goes dead.") );
            stop = true;
        } else if( remote == nullptr ) {
            p->add_msg_if_player( _("Lost contact with the vehicle.") );
            stop = true;
        } else if( remote->fuel_left( "battery", true ) == 0 ) {
            p->add_msg_if_player( m_bad, _("The vehicle's battery died.") );
            stop = true;
        }
        if( stop ) {
            it->active = false;
            g->setremoteveh( nullptr );
        }

        return it->type->charges_to_use();
    }

    bool controlling = it->active && remote != nullptr;
    int choice = menu(true, _("What to do with remote vehicle control:"), _("Nothing"),
                      controlling ? _("Stop controlling the vehicle.") : _("Take control of a vehicle."),
                      _("Execute one vehicle action"), NULL);

   if (choice < 2 || choice > 3 ) {
        return 0;
    }

    if( choice == 2 && controlling ) {
        it->active = false;
        g->setremoteveh( nullptr );
        return 0;
    }

    int px = g->u.view_offset.x;
    int py = g->u.view_offset.y;

    vehicle* veh = pickveh( pos, choice == 2 );

    if( veh == nullptr ) {
        return 0;
    }

    if( !hackveh( p, it, veh ) ) {
        return 0;
    }

    if( choice == 2 ) {
        it->active = true;
        g->setremoteveh( veh );
        p->add_msg_if_player(m_good, _("You take control of the vehicle."));
        if( !veh->engine_on ) {
            veh->start_engines();
        }
    } else if( choice == 3 ) {
        veh->use_controls( pos );
    } else {
        return 0;
    }

    g->u.view_offset.x = px;
    g->u.view_offset.y = py;
    return it->type->charges_to_use();
}

bool multicooker_hallu(player *p)
{
    p->moves -= 200;
    const int random_hallu = rng(1, 7);
    std::vector<tripoint> points;
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

            for (int x = p->posx() - 1; x <= p->posx() + 1; x++)
                for (int y = p->posy() - 1; y <= p->posy() + 1; y++) {
                    tripoint pt(x, y, p->posz());
                    if (g->is_empty( pt )) {
                        points.push_back( pt );
                    }
                }

            if (!one_in(5)) {
                add_msg(m_warning, _("The multi-cooker runs away!"));
                const tripoint random_point = random_entry( points );
                if (g->summon_mon(mon_hallu_multicooker, random_point)) {
                    monster *m = g->monster_at(random_point);
                    m->hallucination = true;
                    m->add_effect("run", 1, num_bp, true);
                }
            } else {
                add_msg(m_bad, _("You're surrounded by aggressive multi-cookers!"));

                for( auto &point : points ) {
                    if (g->summon_mon(mon_hallu_multicooker, point )) {
                        monster *m = g->monster_at(point);
                        m->hallucination = true;
                    }
                }
            }
            return true;

        default:
            return false;
    }

}

int iuse::multicooker(player *p, item *it, bool t, const tripoint &pos)
{
    if (t) {
        if (it->charges == 0) {
            it->active = false;
            return 0;
        }

        int cooktime = it->get_var( "COOKTIME", 0 );
        cooktime -= 100;

        if (cooktime >= 300 && cooktime < 400) {
            //Smart or good cook or careful
            ///\EFFECT_INT increases chance of checking multi-cooker on time

            ///\EFFECT_SURVIVAL increases chance of checking multi-cooker on time
            if (p->int_cur + p->skillLevel( skill_cooking ) + p->skillLevel( skill_survival ) > 16) {
                add_msg(m_info, _("The multi-cooker should be finishing shortly..."));
            }
        }

        if (cooktime <= 0) {
            it->active = false;

            item meal(it->get_var( "DISH" ), calendar::turn);
            meal.active = true;

            if (meal.has_flag("EATEN_HOT")) {
                meal.item_tags.insert("HOT");
                meal.item_counter = 600;
            }

            it->put_in(meal);
            it->erase_var( "DISH" );
            it->erase_var( "COOKTIME" );

            //~ sound of a multi-cooker finishing its cycle!
            sounds::sound(pos, 8, _("ding!"));

            return 0;
        } else {
            it->set_var( "COOKTIME", cooktime );
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
        menu.text = _("Welcome to the RobotChef3000.  Choose option:");

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

                ///\EFFECT_ELECTRONICS >3 allows multicooker upgrade

                ///\EFFECT_FABRICATION >3 allows multicooker upgrade
                if (p->skillLevel( skill_electronics ) > 3 && p->skillLevel( skill_fabrication ) > 3) {
                    const auto upgr = it->get_var( "MULTI_COOK_UPGRADE" );
                    if (upgr == "" ) {
                        menu.addentry(mc_upgrade, true, 'u', _("Upgrade multi-cooker"));
                    } else {
                        if (upgr == "UPGRADE") {
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
                it->erase_var( "DISH" );
                it->erase_var( "COOKTIME" );
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

            for( auto &elem : recipe_dict ) {
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

            dmenu.query();

            int choice = dmenu.ret;

            if (d_cancel == choice) {
                return 0;
            } else {
                const recipe *meal = dishes[choice - 1];
                int mealtime;
                if (it->get_var( "MULTI_COOK_UPGRADE" ) == "UPGRADE") {
                    mealtime = meal->time;
                } else {
                    mealtime = meal->time * 2 ;
                }

                const auto tmp = dynamic_cast<const it_tool *>(it->type);
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

                it->set_var( "DISH", meal->result );
                it->set_var( "COOKTIME", mealtime );

                p->add_msg_if_player(m_good ,
                                     _("The screen flashes blue symbols and scales as the multi-cooker begins to shake."));

                it->active = true;
                it->charges -= 50;

                p->practice( skill_cooking, meal->difficulty * 3); //little bonus

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

            p->practice( skill_electronics, rng(5, 10));
            p->practice( skill_fabrication, rng(5, 10));

            p->moves -= 700;

            ///\EFFECT_INT increases chance to successfully upgrade multi-cooker

            ///\EFFECT_ELECTRONICS increases chance to successfully upgrade multi-cooker

            ///\EFFECT_FABRICATION increases chance to successfully upgrade multi-cooker
            if (p->skillLevel( skill_electronics ) + p->skillLevel( skill_fabrication ) + p->int_cur > rng(20, 35)) {

                p->practice( skill_electronics, rng(5, 20));
                p->practice( skill_fabrication, rng(5, 20));

                p->add_msg_if_player(m_good,
                                     _("You've successfully upgraded the multi-cooker, master tinkerer!  Now it cooks faster!"));

                it->set_var( "MULTI_COOK_UPGRADE", "UPGRADE" );

                return 0;

            } else {

                if (!one_in(5)) {
                    p->add_msg_if_player(m_neutral,
                                         _("You sagely examine and analyze the multi-cooker, but don't manage to accomplish anything."));
                } else {
                    p->add_msg_if_player(m_bad,
                                         _("Your tinkering nearly breaks the multi-cooker!  Fortunately, it still works, but best to stop messing with it."));
                    it->set_var( "MULTI_COOK_UPGRADE", "DAMAGED" );
                }

                return 0;

            }

        }

    }

    return 0;
}

int iuse::cable_attach(player *p, item *it, bool, const tripoint& )
{
    std::string initial_state = it->get_var( "state", "attach_first" );

    if(initial_state == "attach_first") {
        tripoint posp;
        if(!choose_adjacent(_("Attach cable to vehicle where?"),posp)) {
            return 0;
        }
        auto veh = g->m.veh_at( posp );
        auto ter = g->m.ter_at( posp );
        if( veh == nullptr && ter.id != "t_chainfence_h" && ter.id != "t_chainfence_v") {
            p->add_msg_if_player(_("There's no vehicle there."));
            return 0;
        } else {
            const auto abspos = g->m.getabs( posp );
            it->active = true;
            it->set_var( "state", "pay_out_cable" );
            it->set_var( "source_x", abspos.x );
            it->set_var( "source_y", abspos.y );
            it->set_var( "source_z", g->get_levz() );
            it->process( p, p->pos3(), false );
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

        tripoint vpos;
        if(!choose_adjacent(_("Attach cable to vehicle where?"), vpos)) {
            return 0;
        }
        auto target_veh = g->m.veh_at( vpos );
        if (target_veh == nullptr) {
            p->add_msg_if_player(_("There's no vehicle there."));
            return 0;
        } else {
            tripoint source_global( it->get_var( "source_x", 0 ),
                                    it->get_var( "source_y", 0 ),
                                    it->get_var( "source_z", 0 ) );
            tripoint source_local = g->m.getlocal(source_global);
            auto source_veh = g->m.veh_at( source_local );

            if(source_veh == target_veh) {
                if (p != nullptr && p->has_item(it)) {
                    p->add_msg_if_player(m_warning, _("The %s already has access to its own electric system!"),
                                        source_veh->name.c_str());
                }
                return 0;
            }

            tripoint target_global = g->m.getabs( vpos );
            tripoint target_local = vpos;

            if(source_veh == nullptr) {
                if( p != nullptr && p->has_item(it) ) {
                    p->add_msg_if_player(m_bad, _("You notice the cable has come loose!"));
                }
                it->reset_cable(p);
                return 0;
            }

            // TODO: make sure there is always a matching vpart id here. Maybe transform this into
            // a iuse_actor class, or add a check in item_factory.
            const vpart_str_id vpid( it->typeId() );

            point vcoords = g->m.veh_part_coordinates( source_local );
            vehicle_part source_part(vpid, vcoords.x, vcoords.y, it);
            source_part.target.first = target_global;
            source_part.target.second = target_veh->real_global_pos3();
            source_veh->install_part(vcoords.x, vcoords.y, source_part);

            vcoords = g->m.veh_part_coordinates( target_local );
            vehicle_part target_part(vpid, vcoords.x, vcoords.y, it);
            target_part.target.first = source_global;
            target_part.target.second = source_veh->real_global_pos3();
            target_veh->install_part(vcoords.x, vcoords.y, target_part);

            if( p != nullptr && p->has_item(it) ) {
                p->add_msg_if_player(m_good, _("You link up the electric systems of the %1$s and the %2$s."),
                                     source_veh->name.c_str(), target_veh->name.c_str());
            }

            return 1; // Let the cable be destroyed.
        }
    }

    return 0;
}

int iuse::shavekit(player *p, item *it, bool, const tripoint&)
{
    if (it->charges < it->type->charges_to_use()) {
        p->add_msg_if_player(_("You need soap to use this."));
    } else {
        p->add_msg_if_player(_("You open up your kit and shave."));
        p->moves -= 3000;
        p->add_morale(MORALE_SHAVE, 8, 8, 2400, 30);
    }
    return it->type->charges_to_use();
}

int iuse::hairkit(player *p, item *it, bool, const tripoint&)
{
        p->add_msg_if_player(_("You give your hair a trim."));
        p->moves -= 3000;
        p->add_morale(MORALE_HAIRCUT, 3, 3, 4800, 30);
    return it->type->charges_to_use();
}

int iuse::weather_tool( player *p, item *it, bool, const tripoint& )
{
    w_point const weatherPoint = g->weather_gen->get_weather( p->global_square_location(),
                                                              calendar::turn );

    if( it->type->id == "weather_reader" ) {
        p->add_msg_if_player( m_neutral, _( "The %s's monitor slowly outputs the data..." ),
                              it->tname().c_str() );
    }
    if( it->has_flag( "THERMOMETER" ) ) {
        if( it->type->id == "thermometer" ) {
            p->add_msg_if_player( m_neutral, _( "The %1$s reads %2$s." ), it->tname().c_str(),
                                  print_temperature( g->get_temperature() ).c_str() );
        } else {
            p->add_msg_if_player( m_neutral, _( "Temperature: %s." ),
                                  print_temperature( g->get_temperature() ).c_str() );
        }
    }
    if( it->has_flag( "HYGROMETER" ) ) {
        if( it->type->id == "hygrometer" ) {
            p->add_msg_if_player(
                m_neutral, _( "The %1$s reads %2$s." ), it->tname().c_str(),
                print_humidity( get_local_humidity( weatherPoint.humidity, g->weather,
                                                    g->is_sheltered( g->u.pos() ) ) ).c_str() );
        } else {
            p->add_msg_if_player(
                m_neutral, _( "Relative Humidity: %s." ),
                print_humidity( get_local_humidity( weatherPoint.humidity, g->weather,
                                                    g->is_sheltered( g->u.pos() ) ) ).c_str() );
        }
    }
    if( it->has_flag( "BAROMETER" ) ) {
        if( it->type->id == "barometer" ) {
            p->add_msg_if_player(
                m_neutral, _( "The %1$s reads %2$s." ), it->tname().c_str(),
                print_pressure( (int)weatherPoint.pressure ).c_str() );
        } else {
            p->add_msg_if_player( m_neutral, _( "Pressure: %s." ),
                                  print_pressure( (int)weatherPoint.pressure ).c_str() );
        }
    }

    if( it->type->id == "weather_reader" ) {
        int vpart = -1;
        vehicle *veh = g->m.veh_at( p->pos(), vpart );
        int vehwindspeed = 0;
        if( veh ) {
            vehwindspeed = abs( veh->velocity / 100 ); // For mph
        }
        const oter_id &cur_om_ter = overmap_buffer.ter( p->global_omt_location() );
        std::string omtername = otermap[cur_om_ter].name;
        /* windpower defined in internal velocity units (=.01 mph) */
        int windpower = int(100.0f * get_local_windpower( weatherPoint.windpower + vehwindspeed,
                                                          omtername, g->is_sheltered( g->u.pos() ) ) );

        p->add_msg_if_player( m_neutral, _( "Wind Speed: %.1f %s." ),
                              convert_velocity( windpower, VU_WIND ),
                              velocity_units( VU_WIND ) );
        p->add_msg_if_player(
            m_neutral, _( "Feels Like: %s." ),
            print_temperature(
                get_local_windchill( weatherPoint.temperature, weatherPoint.humidity, windpower) +
                g->get_temperature() ).c_str() );
    }

    return 0;
}

int iuse::capture_monster_act( player *p, item *it, bool, const tripoint &pos )
{
    if( it->has_var("contained_name") ) {
        tripoint target;
        if( g->is_empty(pos) ) {
            // It's been activated somewhere where there isn't a player or monster, good.
            target = pos;
        } else {
            if( it->has_flag("PLACE_RANDOMLY") ) {
                std::vector<tripoint> valid;
                for( const tripoint &dest : g->m.points_in_radius( p->pos(), 1 ) ) {
                    if( g->is_empty(dest) ) {
                        valid.push_back(dest);
                    }
                }
                if( valid.empty() ) {
                    p->add_msg_if_player(_("There is no place to put the %s."),
                                         it->get_var("contained_name","").c_str());
                    return 0;
                }
                target = random_entry( valid );
            } else {
                const std::string query = string_format(_("Place the %s where?"),
                                                        it->get_var("contained_name","").c_str());
                if( !choose_adjacent( query, target ) ) {
                    return 0;
                }
                if( !g->is_empty(target) ) {
                    p->add_msg_if_player(m_info,_("You cannot place the %s there!"),
                                         it->get_var("contained_name","").c_str());
                    return 0;
                }
            }
        }
        monster new_monster;
        try {
            new_monster.deserialize( it->get_var("contained_json","") );
        } catch( const JsonError &e ) {
            debugmsg( _("Error restoring monster: %s"), e.c_str() );
            return 0;
        }
        new_monster.spawn( target );
        g->add_zombie( new_monster );
        it->erase_var( "contained_name" );
        it->erase_var( "contained_json" );
        it->erase_var( "name" );
        it->erase_var( "weight" );
        return 0;
    } else {
        tripoint target = pos;
        const std::string query = string_format(_("Capture what with the %s?"), it->tname().c_str());
        if( !choose_adjacent( query, target ) ) {
            p->add_msg_if_player( m_info, _("You cannot use a %s there."), it->tname().c_str() );
            return 0;
        }
        // Capture the thing, if it's on the same square.
        int mon_dex = g->mon_at( target );
        if( mon_dex != -1 ) {
            monster f = g->zombie( mon_dex );

            if( !it->has_property("monster_size_capacity") ) {
                debugmsg( "%s has no monster_size_capacity.", it->tname().c_str() );
                return 0;
            }
            const std::string capacity = it->get_property_string( "monster_size_capacity" );
            if( Creature::size_map.count( capacity ) == 0 ) {
                debugmsg( "%s has invalid monster_size_capacity %s.",
                          it->tname().c_str(), capacity.c_str() );
                return 0;
            }
            if( f.get_size() > Creature::size_map.find( capacity )->second ) {
                p->add_msg_if_player( m_info, _("The %1$s is too big to put in your %2$s."),
                                      f.type->nname().c_str(), it->tname().c_str() );
                return 0;
            }
            // TODO: replace this with some kind of melee check.
            int chance = f.hp_percentage() / 10;
            // A weaker monster is easier to capture.
            // If the monster is friendly, then put it in the item
            // without checking if it rolled a success.
            if( f.friendly != 0 || one_in( chance ) ) {
                std::string serialized_monster;
                try {
                    serialized_monster = f.serialize();
                } catch( const JsonError &e ) {
                    debugmsg( _("Error serializing monster: %s"), e.c_str() );
                    return 0;
                }
                it->set_var( "contained_json", serialized_monster );
                it->set_var( "contained_name", f.type->nname() );
                it->set_var( "name", string_format(_("%s holding %s"), it->type->nname(1).c_str(),
                                                   f.type->nname().c_str()));
                m_size mon_size = f.get_size();
                int new_weight = 0;
                switch( mon_size ) {
                case MS_TINY:
                    new_weight = 1000;
                    break;
                case MS_SMALL:
                    new_weight = 40750;
                    break;
                case MS_MEDIUM:
                    new_weight = 81500;
                    break;
                case MS_LARGE:
                    new_weight = 120000;
                    break;
                case MS_HUGE:
                    new_weight = 200000;
                    break;
                }
                it->set_var( "weight", new_weight );
                g->remove_zombie( mon_dex );
                return 0;
            } else {
                p->add_msg_if_player( m_bad, _("The %1$s avoids your attempts to put it in the %2$s."),
                                      f.type->nname().c_str(), it->type->nname(1).c_str() );
            }
            p->moves -= 100;
        } else {
            add_msg(_("The %s can't capture nothing"),it->tname().c_str());
            return 0;
        }
    }
    return 0;
}

int iuse::ladder( player *p, item *, bool, const tripoint& )
{
    if( !g->m.has_zlevels() ) {
        debugmsg( "Ladder can't be used used in non-z-level mode" );
        return 0;
    }

    tripoint dirp;
    if( !choose_adjacent( _("Put the ladder where?"), dirp ) ) {
        return 0;
    }

    if( !g->is_empty( dirp ) || g->m.has_furn( dirp ) ) {
        p->add_msg_if_player( m_bad, _("Can't place it there."));
        return 0;
    }

    p->add_msg_if_player(_("You set down the ladder."));
    p->moves -= 500;
    g->m.furn_set( dirp, "f_ladder" );
    return 1;
}
