#include "iuse.h"

#include "coordinate_conversions.h"
#include "game.h"
#include "game_inventory.h"
#include "map.h"
#include "fungal_effects.h"
#include "mapdata.h"
#include "output.h"
#include "effect.h" // for weed_msg
#include "debug.h"
#include "options.h"
#include "iexamine.h"
#include "requirements.h"
#include "rng.h"
#include "calendar.h"
#include "string_formatter.h"
#include "line.h"
#include "mutation.h"
#include "player.h"
#include "vehicle.h"
#include "uistate.h"
#include "vpart_position.h"
#include "action.h"
#include "monstergenerator.h"
#include "speech.h"
#include "overmapbuffer.h"
#include "messages.h"
#include "crafting.h"
#include "recipe_dictionary.h"
#include "sounds.h"
#include "monattack.h"
#include "trap.h"
#include "iuse_actor.h" // For firestarter
#include "mongroup.h"
#include "translations.h"
#include "morale_types.h"
#include "input.h"
#include "npc.h"
#include "event.h"
#include "artifact.h"
#include "overmap.h"
#include "ui.h"
#include "mtype.h"
#include "field.h"
#include "weather.h"
#include "cata_utility.h"
#include "map_iterator.h"
#include "string_input_popup.h"
#include "inventory.h"

#include <vector>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <set>
#include <cstdlib>
#include <functional>

#define RADIO_PER_TURN 25 // how many characters per turn of radio

#include "iuse_software.h"

const mtype_id mon_bee( "mon_bee" );
const mtype_id mon_blob( "mon_blob" );
const mtype_id mon_cat( "mon_cat" );
const mtype_id mon_hologram( "mon_hologram" );
const mtype_id mon_dog( "mon_dog" );
const mtype_id mon_dog_thing( "mon_dog_thing" );
const mtype_id mon_fly( "mon_fly" );
const mtype_id mon_hallu_multicooker( "mon_hallu_multicooker" );
const mtype_id mon_shadow( "mon_shadow" );
const mtype_id mon_spore( "mon_spore" );
const mtype_id mon_vortex( "mon_vortex" );
const mtype_id mon_wasp( "mon_wasp" );
const mtype_id mon_cow( "mon_cow" );

const skill_id skill_firstaid( "firstaid" );
const skill_id skill_tailor( "tailor" );
const skill_id skill_survival( "survival" );
const skill_id skill_cooking( "cooking" );
const skill_id skill_mechanics( "mechanics" );
const skill_id skill_archery( "archery" );
const skill_id skill_computer( "computer" );
const skill_id skill_cutting( "cutting" );
const skill_id skill_fabrication( "fabrication" );
const skill_id skill_electronics( "electronics" );
const skill_id skill_melee( "melee" );

const species_id ROBOT( "ROBOT" );
const species_id HALLUCINATION( "HALLUCINATION" );
const species_id ZOMBIE( "ZOMBIE" );
const species_id FUNGUS( "FUNGUS" );
const species_id INSECT( "INSECT" );

const efftype_id effect_adrenaline( "adrenaline" );
const efftype_id effect_antibiotic( "antibiotic" );
const efftype_id effect_asthma( "asthma" );
const efftype_id effect_attention( "attention" );
const efftype_id effect_bite( "bite" );
const efftype_id effect_bleed( "bleed" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_bloodworms( "bloodworms" );
const efftype_id effect_boomered( "boomered" );
const efftype_id effect_brainworms( "brainworms" );
const efftype_id effect_cig( "cig" );
const efftype_id effect_contacts( "contacts" );
const efftype_id effect_cureall( "cureall" );
const efftype_id effect_datura( "datura" );
const efftype_id effect_dermatik( "dermatik" );
const efftype_id effect_docile( "docile" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_drunk( "drunk" );
const efftype_id effect_earphones( "earphones" );
const efftype_id effect_flushot( "flushot" );
const efftype_id effect_foodpoison( "foodpoison" );
const efftype_id effect_formication( "formication" );
const efftype_id effect_fungus( "fungus" );
const efftype_id effect_glowing( "glowing" );
const efftype_id effect_hallu( "hallu" );
const efftype_id effect_high( "high" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_jetinjector( "jetinjector" );
const efftype_id effect_meth( "meth" );
const efftype_id effect_music( "music" );
const efftype_id effect_paincysts( "paincysts" );
const efftype_id effect_panacea( "panacea" );
const efftype_id effect_pet( "pet" );
const efftype_id effect_poison( "poison" );
const efftype_id effect_recover( "recover" );
const efftype_id effect_run( "run" );
const efftype_id effect_shakes( "shakes" );
const efftype_id effect_slimed( "slimed" );
const efftype_id effect_smoke( "smoke" );
const efftype_id effect_spores( "spores" );
const efftype_id effect_stimpack( "stimpack" );
const efftype_id effect_strong_antibiotic( "strong_antibiotic" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_tapeworm( "tapeworm" );
const efftype_id effect_teleglow( "teleglow" );
const efftype_id effect_tetanus( "tetanus" );
const efftype_id effect_took_flumed( "took_flumed" );
const efftype_id effect_took_prozac( "took_prozac" );
const efftype_id effect_took_prozac_bad( "took_prozac_bad" );
const efftype_id effect_took_xanax( "took_xanax" );
const efftype_id effect_valium( "valium" );
const efftype_id effect_visuals( "visuals" );
const efftype_id effect_weak_antibiotic( "weak_antibiotic" );
const efftype_id effect_weed_high( "weed_high" );
const efftype_id effect_winded( "winded" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_ACIDPROOF( "ACIDPROOF" );
static const trait_id trait_ALCMET( "ALCMET" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_EATDEAD( "EATDEAD" );
static const trait_id trait_EATHEALTH( "EATHEALTH" );
static const trait_id trait_EATPOISON( "EATPOISON" );
static const trait_id trait_GILLS( "GILLS" );
static const trait_id trait_HYPEROPIC( "HYPEROPIC" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_LIGHTWEIGHT( "LIGHTWEIGHT" );
static const trait_id trait_MARLOSS_AVOID( "MARLOSS_AVOID" );
static const trait_id trait_MARLOSS_BLUE( "MARLOSS_BLUE" );
static const trait_id trait_MARLOSS( "MARLOSS" );
static const trait_id trait_MARLOSS_YELLOW( "MARLOSS_YELLOW" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_MYOPIC( "MYOPIC" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PARAIMMUNE( "PARAIMMUNE" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );
static const trait_id trait_THRESH_MARLOSS( "THRESH_MARLOSS" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );
static const trait_id trait_THRESH_PLANT( "THRESH_PLANT" );
static const trait_id trait_TOLERANCE( "TOLERANCE" );
static const trait_id trait_URSINE_EYE( "URSINE_EYE" );

static const quality_id AXE( "AXE" );
static const quality_id DIG( "DIG" );

void remove_radio_mod( item &it, player &p )
{
    if( !it.has_flag( "RADIO_MOD" ) ) {
        return;
    }
    p.add_msg_if_player( _( "You remove the radio modification from your %s!" ), it.tname().c_str() );
    item mod( "radio_mod" );
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

static bool item_inscription( player &/*p*/, item &cut, const std::string &verb,
                              const std::string &gerund,
                              bool carveable )
{
    if( !cut.made_of( SOLID ) ) {
        std::string lower_verb = verb;
        std::transform( lower_verb.begin(), lower_verb.end(), lower_verb.begin(), ::tolower );
        add_msg( m_info, _( "You can't %s an item that's not solid!" ), lower_verb.c_str() );
        return false;
    }
    if( carveable && !( cut.made_of( material_id( "wood" ) ) ||
                        cut.made_of( material_id( "plastic" ) ) ||
                        cut.made_of( material_id( "glass" ) ) || cut.made_of( material_id( "chitin" ) ) ||
                        cut.made_of( material_id( "iron" ) ) || cut.made_of( material_id( "steel" ) ) ||
                        cut.made_of( material_id( "silver" ) ) ) ) {
        std::string lower_verb = verb;
        std::transform( lower_verb.begin(), lower_verb.end(), lower_verb.begin(), ::tolower );
        add_msg( m_info, _( "You can't %1$s %2$s because of the material it is made of." ),
                 lower_verb.c_str(), cut.display_name().c_str() );
        return false;
    }

    const bool hasnote = cut.has_var( "item_note" );
    std::string messageprefix = string_format( hasnote ? _( "(To delete, input one '.')\n" ) : "" ) +
                                string_format( _( "%1$s on the %2$s is: " ),
                                        gerund.c_str(), cut.type_name().c_str() );
    string_input_popup popup;
    popup.title( string_format( _( "%s what?" ), verb.c_str() ) )
    .width( 64 )
    .text( hasnote ? cut.get_var( "item_note" ) : "" )
    .description( messageprefix )
    .identifier( "inscribe_item" )
    .max_length( 128 )
    .query();

    if( popup.canceled() ) {
        return false;
    }
    const std::string message = popup.text();
    if( hasnote && message == "." ) {
        cut.erase_var( "item_note" );
        cut.erase_var( "item_note_type" );
        cut.erase_var( "item_note_typez" );
    } else {
        cut.set_var( "item_note", message );
        cut.set_var( "item_note_type", gerund );
    }
    return true;
}

// Returns false if the inscription failed or if the player canceled the action. Otherwise, returns true.

static bool inscribe_item( player &p, const std::string &verb, const std::string &gerund,
                           bool carveable )
{
    //Note: this part still strongly relies on English grammar.
    //Although it can be easily worked around in language like Chinese,
    //but might need to be reworked for some European languages that have more verb forms
    int pos = g->inv_for_all( string_format( _( "%s on what?" ), verb.c_str() ) );
    //@todo the above gives an index in inventory of g->u, but p can be a reference to anybody, maybe use g->u directly?
    item &cut = p.i_at( pos );
    if( cut.is_null() ) {
        add_msg( m_info, _( "You do not have that item!" ) );
        return false;
    }
    return item_inscription( p, cut, verb, gerund, carveable );
}

// For an explosion (which releases some kind of gas), this function
// calculates the points around that explosion where to create those
// gas fields.
// Those points must have a clear line of sight and a clear path to
// the center of the explosion.
// They must also be passable.
std::vector<tripoint> points_for_gas_cloud( const tripoint &center, int radius )
{
    const std::vector<tripoint> gas_sources = closest_tripoints_first( radius, center );
    std::vector<tripoint> result;
    for( const auto &p : gas_sources ) {
        if( g->m.impassable( p ) ) {
            continue;
        }
        if( p != center ) {
            if( !g->m.clear_path( center, p, radius, 1, 100 ) ) {
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
int iuse::sewage( player *p, item *it, bool, const tripoint & )
{
    if( !p->query_yn( _( "Are you sure you want to drink... this?" ) ) ) {
        return 0;
    }

    p->add_memorial_log( pgettext( "memorial_male", "Ate a sewage sample." ),
                         pgettext( "memorial_female", "Ate a sewage sample." ) );
    p->vomit();
    if( one_in( 4 ) ) {
        p->mutate();
    }
    return it->type->charges_to_use();
}

int iuse::honeycomb( player *p, item *it, bool, const tripoint & )
{
    g->m.spawn_item( p->pos(), "wax", 2 );
    return it->type->charges_to_use();
}

int iuse::royal_jelly( player *p, item *it, bool, const tripoint & )
{
    p->add_effect( effect_cureall, 1_turns );
    return it->type->charges_to_use();
}

int iuse::xanax( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( _( "You take some %s." ), it->tname().c_str() );
    p->add_effect( effect_took_xanax, 90_minutes );
    return it->type->charges_to_use();
}

int iuse::caff( player *p, item *it, bool, const tripoint & )
{
    p->mod_fatigue( -( it->type->comestible ? it->type->comestible->stim : 0 ) * 3 );
    return it->type->charges_to_use();
}

int iuse::atomic_caff( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( m_good, _( "Wow!  This %s has a kick." ), it->tname().c_str() );
    p->mod_fatigue( -( it->type->comestible ? it->type->comestible->stim : 0 ) * 12 );
    p->radiation += 8;
    return it->type->charges_to_use();
}

constexpr static time_duration alc_strength( const int strength, const time_duration weak,
        const time_duration medium, const time_duration strong )
{
    return strength == 0 ? weak : strength == 1 ? medium : strong;
}

int alcohol( player &p, const item &it, const int strength )
{
    // Weaker characters are cheap drunks
    /** @EFFECT_STR_MAX reduces drunkenness duration */
    time_duration duration = alc_strength( strength, 34_minutes, 68_minutes,
                                           90_minutes ) - ( alc_strength( strength, 6_turns, 10_turns, 12_turns ) * p.str_max );
    if( p.has_trait( trait_ALCMET ) ) {
        duration = alc_strength( strength, 9_minutes, 18_minutes, 25_minutes ) - ( alc_strength( strength,
                   6_turns, 10_turns, 10_turns ) * p.str_max );
        // Metabolizing the booze improves the nutritional value;
        // might not be healthy, and still causes Thirst problems, though
        p.mod_hunger( -( abs( it.type->comestible ? it.type->comestible->stim : 0 ) ) );
        // Metabolizing it cancels out the depressant
        p.stim += abs( it.type->comestible ? it.type->comestible->stim : 0 );
    } else if( p.has_trait( trait_TOLERANCE ) ) {
        duration -= alc_strength( strength, 12_minutes, 30_minutes, 45_minutes );
    } else if( p.has_trait( trait_LIGHTWEIGHT ) ) {
        duration += alc_strength( strength, 12_minutes, 30_minutes, 45_minutes );
    }
    if( !( p.has_trait( trait_ALCMET ) ) ) {
        p.mod_painkiller( to_turns<int>( alc_strength( strength, 4_turns, 8_turns, 12_turns ) ) );
    }
    p.add_effect( effect_drunk, duration );
    return it.type->charges_to_use();
}

int iuse::alcohol_weak( player *p, item *it, bool, const tripoint & )
{
    return alcohol( *p, *it, 0 );
}

int iuse::alcohol_medium( player *p, item *it, bool, const tripoint & )
{
    return alcohol( *p, *it, 1 );
}

int iuse::alcohol_strong( player *p, item *it, bool, const tripoint & )
{
    return alcohol( *p, *it, 2 );
}

/**
 * Entry point for intentional bodily intake of smoke via paper wrapped one
 * time use items: cigars, cigarettes, etc.
 *
 * @param p Player doing the smoking
 * @param it the item to be smoked.
 * @return Charges used in item smoked
 */
int iuse::smoking( player *p, item *it, bool, const tripoint & )
{
    bool hasFire = ( p->has_charges( "fire", 1 ) );

    // make sure we're not already smoking something
    if( !check_litcig( *p ) ) {
        return 0;
    }

    if( !hasFire ) {
        p->add_msg_if_player( m_info, _( "You don't have anything to light it with!" ) );
        return 0;
    }

    item cig;
    if( it->typeId() == "cig" ) {
        cig = item( "cig_lit", calendar::turn );
        cig.item_counter = 40;
        p->mod_hunger( -3 );
        p->mod_thirst( 2 );
    } else if( it->typeId() == "handrolled_cig" ) {
        // This transforms the hand-rolled into a normal cig, which isn't exactly
        // what I want, but leaving it for now.
        cig = item( "cig_lit", calendar::turn );
        cig.item_counter = 40;
        p->mod_thirst( 2 );
        p->mod_hunger( -3 );
    } else if( it->typeId() == "cigar" ) {
        cig = item( "cigar_lit", calendar::turn );
        cig.item_counter = 120;
        p->mod_thirst( 3 );
        p->mod_hunger( -4 );
    } else if( it->typeId() == "joint" ) {
        cig = item( "joint_lit", calendar::turn );
        cig.item_counter = 40;
        p->mod_hunger( 4 );
        p->mod_thirst( 6 );
        if( p->get_painkiller() < 5 ) {
            p->set_painkiller( ( p->get_painkiller() + 3 ) * 2 );
        }
    } else {
        p->add_msg_if_player( m_bad,
                              _( "Please let the devs know you should be able to smoke a %s but the smoking code does not know how." ),
                              it->tname().c_str() );
        return 0;
    }
    // If we're here, we better have a cig to light.
    p->use_charges_if_avail( "fire", 1 );
    cig.active = true;
    p->inv.add_item( cig, false, true );
    p->add_msg_if_player( m_neutral, _( "You light a %s." ), cig.tname().c_str() );

    // Parting messages
    if( it->typeId() == "joint" ) {
        // Would group with the joint, but awkward to mutter before lighting up.
        if( one_in( 5 ) ) {
            weed_msg( *p );
        }
    }
    if( p->get_effect_dur( effect_cig ) > 100_turns * ( p->addiction_level( ADD_CIG ) + 1 ) ) {
        p->add_msg_if_player( m_bad, _( "Ugh, too much smoke... you feel nasty." ) );
    }

    return it->type->charges_to_use();
}


int iuse::ecig( player *p, item *it, bool, const tripoint & )
{
    if( it->typeId() == "ecig" ) {
        p->add_msg_if_player( m_neutral, _( "You take a puff from your electronic cigarette." ) );
    } else if( it->typeId() == "advanced_ecig" ) {
        if( p->has_charges( "nicotine_liquid", 1 ) ) {
            p->add_msg_if_player( m_neutral,
                                  _( "You inhale some vapor from your advanced electronic cigarette." ) );
            p->use_charges( "nicotine_liquid", 1 );
        } else {
            p->add_msg_if_player( m_info, _( "You don't have any nicotine liquid!" ) );
            return 0;
        }
    }

    p->mod_thirst( 1 );
    p->mod_hunger( -1 );
    p->add_effect( effect_cig, 10_minutes );
    if( p->get_effect_dur( effect_cig ) > 100_turns * ( p->addiction_level( ADD_CIG ) + 1 ) ) {
        p->add_msg_if_player( m_bad, _( "Ugh, too much nicotine... you feel nasty." ) );
    }
    return it->type->charges_to_use();
}

int iuse::antibiotic( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_player_or_npc( m_neutral,
                              _( "You take some antibiotics." ),
                              _( "<npcname> takes some antibiotics." ) );
    if( p->has_effect( effect_tetanus ) ) {
        if( one_in( 3 ) ) {
            p->remove_effect( effect_tetanus );
            p->add_msg_if_player( m_good, _( "The muscle spasms start to go away." ) );
        } else {
            p->add_msg_if_player( m_warning, _( "The medication does nothing to help the spasms." ) );
        }
    }
    if( p->has_effect( effect_infected ) && !p->has_effect( effect_antibiotic ) ) {
        p->add_msg_if_player( m_good,
                              _( "Maybe just placebo effect, but you feel a little better as the dose settles in." ) );
    }
    p->add_effect( effect_antibiotic, 12_hours );
    return it->type->charges_to_use();
}

int iuse::eyedrops( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    if( it->charges < it->type->charges_to_use() ) {
        p->add_msg_if_player( _( "You're out of %s." ), it->tname().c_str() );
        return 0;
    }
    p->add_msg_if_player( _( "You use your %s." ), it->tname().c_str() );
    p->moves -= 150;
    if( p->has_effect( effect_boomered ) ) {
        p->remove_effect( effect_boomered );
        p->add_msg_if_player( m_good, _( "You wash the slime from your eyes." ) );
    }
    return it->type->charges_to_use();
}

int iuse::fungicide( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }

    const bool has_fungus = p->has_effect( effect_fungus );
    const bool has_spores = p->has_effect( effect_spores );

    if( p->is_npc() && !has_fungus && !has_spores ) {
        return 0;
    }

    p->add_msg_player_or_npc( _( "You use your fungicide." ), _( "<npcname> uses some fungicide" ) );
    if( has_fungus && ( one_in( 3 ) ) ) {
        p->remove_effect( effect_fungus );
        p->add_msg_if_player( m_warning,
                              _( "You feel a burning sensation under your skin that quickly fades away." ) );
    }
    if( has_spores && ( one_in( 2 ) ) ) {
        if( !p->has_effect( effect_fungus ) ) {
            p->add_msg_if_player( m_warning, _( "Your skin grows warm for a moment." ) );
        }
        p->remove_effect( effect_spores );
        int spore_count = rng( 1, 6 );
        for( const tripoint &dest : g->m.points_in_radius( p->pos(), 1 ) ) {
            if( spore_count == 0 ) {
                break;
            }
            if( dest == p->pos() ) {
                continue;
            }
            if( g->m.passable( dest ) && x_in_y( spore_count, 8 ) ) {
                if( monster *const mon_ptr = g->critter_at<monster>( dest ) ) {
                    monster &critter = *mon_ptr;
                    if( g->u.sees( dest ) &&
                        !critter.type->in_species( FUNGUS ) ) {
                        add_msg( m_warning, _( "The %s is covered in tiny spores!" ),
                                 critter.name().c_str() );
                    }
                    if( !critter.make_fungus() ) {
                        critter.die( p ); // counts as kill by player
                    }
                } else {
                    g->summon_mon( mon_spore, dest );
                }
                spore_count--;
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::antifungal( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    p->add_msg_if_player( _( "You take some antifungal medication." ) );
    if( p->has_effect( effect_fungus ) ) {
        p->remove_effect( effect_fungus );
        p->add_msg_if_player( m_warning,
                              _( "You feel a burning sensation under your skin that quickly fades away." ) );
    }
    if( p->has_effect( effect_spores ) ) {
        if( !p->has_effect( effect_fungus ) ) {
            p->add_msg_if_player( m_warning, _( "Your skin grows warm for a moment." ) );
        }
    }
    return it->type->charges_to_use();
}

int iuse::antiparasitic( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    p->add_msg_if_player( _( "You take some antiparasitic medication." ) );
    if( p->has_effect( effect_dermatik ) ) {
        p->remove_effect( effect_dermatik );
        p->add_msg_if_player( m_good, _( "The itching sensation under your skin fades away." ) );
    }
    if( p->has_effect( effect_tapeworm ) ) {
        p->remove_effect( effect_tapeworm );
        p->mod_hunger( -1 ); // You just digested the tapeworm.
        if( p->has_trait( trait_NOPAIN ) ) {
            p->add_msg_if_player( m_good, _( "Your bowels clench as something inside them dies." ) );
        } else {
            p->add_msg_if_player( m_mixed, _( "Your bowels spasm painfully as something inside them dies." ) );
            p->mod_pain( rng( 8, 24 ) );
        }
    }
    if( p->has_effect( effect_bloodworms ) ) {
        p->remove_effect( effect_bloodworms );
        p->add_msg_if_player( _( "Your skin prickles and your veins itch for a few moments." ) );
    }
    if( p->has_effect( effect_brainworms ) ) {
        p->remove_effect( effect_brainworms );
        if( p->has_trait( trait_NOPAIN ) ) {
            p->add_msg_if_player( m_good, _( "The pressure inside your head feels better already." ) );
        } else {
            p->add_msg_if_player( m_mixed,
                                  _( "Your head pounds like a sore tooth as something inside of it dies." ) );
            p->mod_pain( rng( 8, 24 ) );
        }
    }
    if( p->has_effect( effect_paincysts ) ) {
        p->remove_effect( effect_paincysts );
        if( p->has_trait( trait_NOPAIN ) ) {
            p->add_msg_if_player( m_good, _( "The stiffness in your joints goes away." ) );
        } else {
            p->add_msg_if_player( m_good, _( "The pain in your joints goes away." ) );
        }
    }
    return it->type->charges_to_use();
}

int iuse::anticonvulsant( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( _( "You take some anticonvulsant medication." ) );
    /** @EFFECT_STR reduces duration of anticonvulsant medication */
    time_duration duration = 8_hours - p->str_cur * rng( 0_turns, 100_turns );
    if( p->has_trait( trait_TOLERANCE ) ) {
        duration -= 1_hours;
    }
    if( p->has_trait( trait_LIGHTWEIGHT ) ) {
        duration += 2_hours;
    }
    p->add_effect( effect_valium, duration );
    p->add_effect( effect_high, duration );
    if( p->has_effect( effect_shakes ) ) {
        p->remove_effect( effect_shakes );
        p->add_msg_if_player( m_good, _( "You stop shaking." ) );
    }
    return it->type->charges_to_use();
}

int iuse::weed_brownie( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player(
        _( "You start scarfing down the delicious brownie.  It tastes a little funny though..." ) );
    time_duration duration = 12_minutes;
    if( p->has_trait( trait_TOLERANCE ) ) {
        duration = 9_minutes;
    }
    if( p->has_trait( trait_LIGHTWEIGHT ) ) {
        duration = 15_minutes;
    }
    p->mod_hunger( 2 );
    p->mod_thirst( 6 );
    if( p->get_painkiller() < 5 ) {
        p->set_painkiller( ( p->get_painkiller() + 3 ) * 2 );
    }
    p->add_effect( effect_weed_high, duration );
    p->moves -= 100;
    if( one_in( 5 ) ) {
        weed_msg( *p );
    }
    return it->type->charges_to_use();
}

int iuse::coke( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( _( "You snort a bump of coke." ) );
    /** @EFFECT_STR reduces duration of coke */
    time_duration duration = 21_turns - 1_turns * p->str_cur + rng( 0_turns, 10_turns );
    if( p->has_trait( trait_TOLERANCE ) ) {
        duration -= 1_minutes; // Symmetry would cause problems :-/
    }
    if( p->has_trait( trait_LIGHTWEIGHT ) ) {
        duration += 2_minutes;
    }
    p->mod_hunger( -8 );
    p->add_effect( effect_high, duration );
    return it->type->charges_to_use();
}

int iuse::meth( player *p, item *it, bool, const tripoint & )
{
    /** @EFFECT_STR reduces duration of meth */
    time_duration duration = 1_minutes * ( 60 - p->str_cur );
    if( p->has_amount( "apparatus", 1 ) && p->use_charges_if_avail( "fire", 1 ) ) {
        p->add_msg_if_player( m_neutral, _( "You smoke your meth." ) );
        p->add_msg_if_player( m_good, _( "The world seems to sharpen." ) );
        p->mod_fatigue( -375 );
        if( p->has_trait( trait_TOLERANCE ) ) {
            duration *= 1.2;
        } else {
            duration *= ( p->has_trait( trait_LIGHTWEIGHT ) ? 1.8 : 1.5 );
        }
        // breathe out some smoke
        for( int i = 0; i < 3; i++ ) {
            g->m.add_field( {p->posx() + int( rng( -2, 2 ) ), p->posy() + int( rng( -2, 2 ) ), p->posz()},
                            fd_methsmoke, 2 );
        }
    } else {
        p->add_msg_if_player( _( "You snort some crystal meth." ) );
        p->mod_fatigue( -300 );
    }
    if( !p->has_effect( effect_meth ) ) {
        duration += 1_hours;
    }
    if( duration > 0_turns ) {
        // meth actually inhibits hunger, weaker characters benefit more
        /** @EFFECT_STR_MAX >4 experiences less hunger benefit from meth */
        int hungerpen = ( p->str_max < 5 ? 35 : 40 - ( 2 * p->str_max ) );
        if( hungerpen > 0 ) {
            p->mod_hunger( -hungerpen );
        }
        p->add_effect( effect_meth, duration );
    }
    return it->type->charges_to_use();
}

int iuse::vaccine( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( _( "You inject the vaccine." ) );
    p->add_msg_if_player( m_good, _( "You feel tough." ) );
    p->mod_healthy_mod( 200, 200 );
    p->mod_pain( 3 );
    item syringe( "syringe", it->birthday() );
    p->i_add( syringe );
    return it->type->charges_to_use();
}

int iuse::flu_vaccine( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( _( "You inject the vaccine." ) );
    p->add_msg_if_player( m_good, _( "You no longer need to fear the flu." ) );
    p->add_effect( effect_flushot, 1_turns, num_bp, true );
    p->mod_pain( 3 );
    item syringe( "syringe", it->birthday() );
    p->i_add( syringe );
    return it->type->charges_to_use();
}

int iuse::poison( player *p, item *it, bool, const tripoint & )
{
    if( ( p->has_trait( trait_EATDEAD ) ) ) {
        return it->type->charges_to_use();
    }

    // NPCs have a magical sense of what is inedible
    // Players can abuse the crafting menu instead...
    if( !it->has_flag( "HIDDEN_POISON" ) &&
        ( p->is_npc() ||
          !p->query_yn( _( "Are you sure you want to eat this? It looks poisonous..." ) ) ) ) {
        return 0;
    }
    /** @EFFECT_STR increases EATPOISON trait effectiveness (50-90%) */
    if( ( p->has_trait( trait_EATPOISON ) ) && ( !( one_in( p->str_cur / 2 ) ) ) ) {
        return it->type->charges_to_use();
    }
    p->add_effect( effect_poison, 1_hours );
    p->add_effect( effect_foodpoison, 3_hours );
    return it->type->charges_to_use();
}

/**
 * Hallucinogenic with a fun effect. Specifically used to have a comestible
 * give a morale boost without it being noticeable by examining the item (ie,
 * for magic mushrooms).
 */
int iuse::fun_hallu( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        // NPCs hallucinating doesn't work yet!
        return 0;
    }

    //Fake a normal food morale effect
    if( p->has_trait( trait_SPIRITUAL ) ) {
        p->add_morale( MORALE_FOOD_GOOD, 36, 72, 12_minutes, 6_minutes, false, it->type );
    } else {
        p->add_morale( MORALE_FOOD_GOOD, 18, 36, 6_minutes, 3_minutes, false, it->type );
    }
    if( !p->has_effect( effect_hallu ) ) {
        p->add_effect( effect_hallu, 6_hours );
    }
    return it->type->charges_to_use();
}

int iuse::meditate( player *p, item *it, bool t, const tripoint & )
{
    if( !p || t ) {
        return 0;
    }

    if( p->has_trait( trait_SPIRITUAL ) ) {
        p->assign_activity( activity_id( "ACT_MEDITATE" ), 2000 );
    } else {
        p->add_msg_if_player( _( "This %s probably meant a lot to someone at one time." ),
                              it->tname().c_str() );
    }
    return it->type->charges_to_use();
}

int iuse::thorazine( player *p, item *it, bool, const tripoint & )
{
    p->mod_fatigue( 5 );
    p->remove_effect( effect_hallu );
    p->remove_effect( effect_visuals );
    p->remove_effect( effect_high );
    if( !p->has_effect( effect_dermatik ) ) {
        p->remove_effect( effect_formication );
    }
    if( one_in( 50 ) ) { // adverse reaction
        p->add_msg_if_player( m_bad, _( "You feel completely exhausted." ) );
        p->mod_fatigue( 15 );
    } else {
        p->add_msg_if_player( m_warning, _( "You feel a bit wobbly." ) );
    }
    return it->type->charges_to_use();
}

int iuse::prozac( player *p, item *it, bool, const tripoint & )
{
    if( !p->has_effect( effect_took_prozac ) ) {
        p->add_effect( effect_took_prozac, 12_hours );
    } else {
        p->stim += 3;
    }
    if( one_in( 50 ) ) { // adverse reaction, same duration as prozac effect.
        p->add_msg_if_player( m_warning, _( "You suddenly feel hollow inside." ) );
        p->add_effect( effect_took_prozac_bad, p->get_effect_dur( effect_took_prozac ) );
    }
    return it->type->charges_to_use();
}

int iuse::sleep( player *p, item *it, bool, const tripoint & )
{
    p->mod_fatigue( 40 );
    p->add_msg_if_player( m_warning, _( "You feel very sleepy..." ) );
    return it->type->charges_to_use();
}

int iuse::datura( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        return 0;
    }

    p->add_effect( effect_datura, rng( 200_minutes, 800_minutes ) );
    p->add_msg_if_player( _( "You eat the datura seed." ) );
    if( p->has_trait( trait_SPIRITUAL ) ) {
        p->add_morale( MORALE_FOOD_GOOD, 36, 72, 12_minutes, 6_minutes, false, it->type );
    }
    return it->type->charges_to_use();
}

int iuse::flumed( player *p, item *it, bool, const tripoint & )
{
    p->add_effect( effect_took_flumed, 10_hours );
    p->add_msg_if_player( _( "You take some %s" ), it->tname().c_str() );
    return it->type->charges_to_use();
}

int iuse::flusleep( player *p, item *it, bool, const tripoint & )
{
    p->add_effect( effect_took_flumed, 12_hours );
    p->mod_fatigue( 30 );
    p->add_msg_if_player( _( "You take some %s" ), it->tname().c_str() );
    p->add_msg_if_player( m_warning, _( "You feel very sleepy..." ) );
    return it->type->charges_to_use();
}

int iuse::inhaler( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( m_neutral, _( "You take a puff from your inhaler." ) );
    if( !p->remove_effect( effect_asthma ) ) {
        p->mod_fatigue( -3 ); // if we don't have asthma can be used as stimulant
        if( one_in( 20 ) ) {   // with a small but significant risk of adverse reaction
            p->add_effect( effect_shakes, rng( 2_minutes, 5_minutes ) );
        }
    }
    p->remove_effect( effect_smoke );
    return it->type->charges_to_use();
}

int iuse::oxygen_bottle( player *p, item *it, bool, const tripoint & )
{
    p->moves -= 500;
    p->add_msg_if_player( m_neutral, _( "You breathe deeply from the %s" ), it->tname().c_str() );
    if( p->has_effect( effect_smoke ) ) {
        p->remove_effect( effect_smoke );
    } else if( p->has_effect( effect_asthma ) ) {
        p->remove_effect( effect_asthma );
    } else if( p->stim < 16 ) {
        p->stim += 8;
        p->mod_painkiller( 2 );
    }
    p->remove_effect( effect_winded );
    p->mod_painkiller( 2 );
    return it->type->charges_to_use();
}

int iuse::blech( player *p, item *it, bool, const tripoint & )
{
    // TODO: Add more effects?
    if( it->made_of( LIQUID ) ) {
        if( !p->query_yn( _( "This looks unhealthy, sure you want to drink it?" ) ) ) {
            return 0;
        }
    } else { //Assume that if a blech consumable isn't a drink, it will be eaten.
        if( !p->query_yn( _( "This looks unhealthy, sure you want to eat it?" ) ) ) {
            return 0;
        }
    }

    if( it->has_flag( "ACID" ) && ( p->has_trait( trait_ACIDPROOF ) ||
                                    p->has_trait( trait_ACIDBLOOD ) ) ) {
        p->add_msg_if_player( m_bad, _( "Blech, that tastes gross!" ) );
        //reverse the harmful values of drinking this acid.
        double multiplier = -1;
        p->mod_hunger( -p->nutrition_for( *it ) * multiplier );
        p->mod_thirst( -it->type->comestible->quench * multiplier );
        p->mod_thirst( -20 ); //acidproof people can drink acids like diluted water.
        p->mod_stomach_water( 20 );
        p->mod_healthy_mod( it->type->comestible->healthy * multiplier,
                            it->type->comestible->healthy * multiplier );
        p->add_morale( MORALE_FOOD_BAD, it->type->comestible->fun * multiplier, 60, 6_minutes, 3_minutes,
                       false, it->type );
    } else {
        p->add_msg_if_player( m_bad, _( "Blech, that burns your throat!" ) );
        p->mod_pain( rng( 32, 64 ) );
        p->add_effect( effect_poison, 1_hours );
        p->apply_damage( nullptr, bp_torso, rng( 4, 12 ) );
        p->vomit();
    }
    return it->type->charges_to_use();
}

int iuse::plantblech( player *p, item *it, bool, const tripoint &pos )
{
    if( p->has_trait( trait_THRESH_PLANT ) ) {
        double multiplier = -1;
        if( p->has_trait( trait_CHLOROMORPH ) ) {
            multiplier = -3;
            p->add_msg_if_player( m_good, _( "The meal is revitalizing." ) );
        } else {
            p->add_msg_if_player( m_good, _( "Oddly enough, this doesn't taste so bad." ) );
        }

        //reverses the harmful values of drinking fertilizer
        p->mod_hunger( p->nutrition_for( *it ) * multiplier );
        p->mod_thirst( -it->type->comestible->quench * multiplier );
        p->mod_healthy_mod( it->type->comestible->healthy * multiplier,
                            it->type->comestible->healthy * multiplier );
        p->add_morale( MORALE_FOOD_GOOD, -10 * multiplier, 60, 6_minutes, 3_minutes, false, it->type );
        return it->type->charges_to_use();
    } else {
        return blech( p, it, true, pos );
    }
}

int iuse::chew( player *p, item *it, bool, const tripoint & )
{
    // TODO: Add more effects?
    p->add_msg_if_player( _( "You chew your %s." ), it->tname().c_str() );
    return it->type->charges_to_use();
}

// Helper to handle the logic of removing some random mutations.
static void do_purify( player &p )
{
    std::vector<trait_id> valid; // Which flags the player has
    for( auto &traits_iter : mutation_branch::get_all() ) {
        if( p.has_trait( traits_iter.first ) && !p.has_base_trait( traits_iter.first ) ) {
            //Looks for active mutation
            valid.push_back( traits_iter.first );
        }
    }
    if( valid.empty() ) {
        p.add_msg_if_player( _( "You feel cleansed." ) );
        return;
    }
    int num_cured = rng( 1, valid.size() );
    num_cured = std::min( 4, num_cured );
    for( int i = 0; i < num_cured && !valid.empty(); i++ ) {
        const trait_id id = random_entry_removed( valid );
        if( p.purifiable( id ) ) {
            p.remove_mutation( id );
        } else {
            p.add_msg_if_player( m_warning, _( "You feel a slight itching inside, but it passes." ) );
        }
    }
}

int iuse::purifier( player *p, item *it, bool, const tripoint & )
{
    mutagen_attempt checks = mutagen_common_checks( *p, *it, false,
                             pgettext( "memorial_male", "Consumed purifier." ), pgettext( "memorial_female",
                                     "Consumed purifier." ) );
    if( !checks.allowed ) {
        return checks.charges_used;
    }

    do_purify( *p );
    return it->type->charges_to_use();
}

int iuse::purify_iv( player *p, item *it, bool, const tripoint & )
{
    mutagen_attempt checks = mutagen_common_checks( *p, *it, false,
                             pgettext( "memorial_male", "Injected purifier." ), pgettext( "memorial_female",
                                     "Injected purifier." ) );
    if( !checks.allowed ) {
        return checks.charges_used;
    }

    std::vector<trait_id> valid; // Which flags the player has
    for( auto &traits_iter : mutation_branch::get_all() ) {
        if( p->has_trait( traits_iter.first ) && !p->has_base_trait( traits_iter.first ) ) {
            //Looks for active mutation
            valid.push_back( traits_iter.first );
        }
    }
    if( valid.empty() ) {
        p->add_msg_if_player( _( "You feel cleansed." ) );
        return it->type->charges_to_use();
    }
    int num_cured = rng( 4,
                         valid.size() ); //Essentially a double-strength purifier, but guaranteed at least 4.  Double-edged and all
    if( num_cured > 8 ) {
        num_cured = 8;
    }
    for( int i = 0; i < num_cured && !valid.empty(); i++ ) {
        const trait_id id = random_entry_removed( valid );
        if( p->purifiable( id ) ) {
            p->remove_mutation( id );
        } else {
            p->add_msg_if_player( m_warning, _( "You feel a distinct burning inside, but it passes." ) );
        }
        if( !( p->has_trait( trait_NOPAIN ) ) ) {
            p->mod_pain( 2 * num_cured ); //Hurts worse as it fixes more
            p->add_msg_if_player( m_warning, _( "Feels like you're on fire, but you're OK." ) );
        }
        p->mod_hunger( 2 * num_cured );
        p->mod_thirst( 2 * num_cured );
        p->mod_fatigue( 2 * num_cured );
    }
    return it->type->charges_to_use();
}

void spawn_spores( const player &p )
{
    int spores_spawned = 0;
    fungal_effects fe( *g, g->m );
    for( const tripoint &dest : closest_tripoints_first( 4, p.pos() ) ) {
        if( g->m.impassable( dest ) ) {
            continue;
        }
        float dist = trig_dist( dest, p.pos() );
        if( x_in_y( 1, dist ) ) {
            fe.marlossify( dest );
        }
        if( g->critter_at( dest ) != nullptr ) {
            continue;
        }
        if( one_in( 10 + 5 * dist ) && one_in( spores_spawned * 2 ) ) {
            if( monster *const spore = g->summon_mon( mon_spore, dest ) ) {
                spore->friendly = -1;
                spores_spawned++;
            }
        }
    }
}

static void marloss_common( player &p, item &it, const trait_id &current_color )
{
    static const std::map<trait_id, add_type> mycus_colors = {{
            { trait_MARLOSS_BLUE, ADD_MARLOSS_B }, { trait_MARLOSS_YELLOW, ADD_MARLOSS_Y }, { trait_MARLOSS, ADD_MARLOSS_R }
        }
    };

    if( p.has_trait( current_color ) || p.has_trait( trait_THRESH_MARLOSS ) ) {
        p.add_msg_if_player( m_good,
                             _( "As you eat the %s, you have a near-religious experience, feeling at one with your surroundings..." ),
                             it.tname().c_str() );
        p.add_morale( MORALE_MARLOSS, 100, 1000 );
        for( const std::pair<trait_id, add_type> &pr : mycus_colors ) {
            if( pr.first != current_color ) {
                p.add_addiction( pr.second, 50 );
            }
        }

        p.set_hunger( -10 );
        spawn_spores( p );
        return;
    }

    int marloss_count = std::count_if( mycus_colors.begin(), mycus_colors.end(),
    [&p]( const std::pair<trait_id, add_type> &pr ) {
        return p.has_trait( pr.first );
    } );

    /* If we're not already carriers of current type of Marloss, roll for a random effect:
     * 1 - Mutate
     * 2 - Mutate
     * 3 - Mutate
     * 4 - Purify
     * 5 - Purify
     * 6 - Cleanse radiation + Purify
     * 7 - Fully satiate
     * 8 - Vomit
     * 9-12 - Give Marloss mutation
     */
    int effect = rng( 1, 12 );
    if( effect <= 3 ) {
        p.add_msg_if_player( _( "It tastes extremely strange!" ) );
        p.mutate();
        // Gruss dich, mutation drain, missed you!
        p.mod_pain( 2 * rng( 1, 5 ) );
        p.mod_hunger( 10 );
        p.mod_thirst( 10 );
        p.mod_fatigue( 5 );
    } else if( effect <= 6 ) { // Radiation cleanse is below
        p.add_msg_if_player( m_good, _( "You feel better all over." ) );
        p.mod_painkiller( 30 );
        iuse dummy;
        dummy.purifier( &p, &it, false, p.pos() );
        if( effect == 6 ) {
            p.radiation = 0;
        }
    } else if( effect == 7 ) {
        p.add_msg_if_player( m_good, _( "It is delicious, and very filling!" ) );
        p.set_hunger( -10 );
    } else if( effect == 8 ) {
        p.add_msg_if_player( m_bad, _( "You take one bite, and immediately vomit!" ) );
        p.vomit();
    } else if( p.crossed_threshold() ) {
        // Mycus Rejection.  Goo already present fights off the fungus.
        p.add_msg_if_player( m_bad,
                             _( "You feel a familiar warmth, but suddenly it surges into an excruciating burn as you convulse, vomiting, and black out..." ) );
        p.add_memorial_log( pgettext( "memorial_male", "Suffered Marloss Rejection." ),
                            pgettext( "memorial_female", "Suffered Marloss Rejection." ) );
        p.vomit();
        p.mod_pain( 90 );
        p.hurtall( rng( 40, 65 ), nullptr ); // No good way to say "lose half your current HP"
        /** @EFFECT_INT slightly reduces sleep duration when eating mycus+goo */
        p.fall_asleep( 10_hours - p.int_cur *
                       1_minutes ); // Hope you were eating someplace safe.  Mycus v. Goo in your guts is no joke.
        for( const std::pair<trait_id, add_type> &pr : mycus_colors ) {
            p.unset_mutation( pr.first );
            p.rem_addiction( pr.second );
        }
        p.set_mutation(
            trait_MARLOSS_AVOID ); // And if you survive it's etched in your RNA, so you're unlikely to repeat the experiment.
    } else if( marloss_count >= 2 ) {
        p.add_msg_if_player( m_bad,
                             _( "You feel a familiar warmth, but suddenly it surges into painful burning as you convulse and collapse to the ground..." ) );
        /** @EFFECT_INT reduces sleep duration when eating wrong color marloss */
        p.fall_asleep( 40_minutes - 1_minutes * p.int_cur / 2 );
        for( const std::pair<trait_id, add_type> &pr : mycus_colors ) {
            p.unset_mutation( pr.first );
            p.rem_addiction( pr.second );
        }

        p.set_mutation( trait_THRESH_MARLOSS );
        g->m.ter_set( p.pos(), t_marloss );
        p.add_memorial_log( pgettext( "memorial_male", "Opened the Marloss Gateway." ),
                            pgettext( "memorial_female", "Opened the Marloss Gateway." ) );
        p.add_msg_if_player( m_good,
                             _( "You wake up in a marloss bush.  Almost *cradled* in it, actually, as though it grew there for you." ) );
        //~ Beginning to hear the Mycus while conscious: that's it speaking
        p.add_msg_if_player( m_good,
                             _( "unity.  together we have reached the door.  we provide the final key.  now to pass through..." ) );
    } else {
        p.add_msg_if_player( _( "You feel a strange warmth spreading throughout your body..." ) );
        p.set_mutation( current_color );
        // Give us addictions to the other two colors, but cure one for current color
        for( const std::pair<trait_id, add_type> &pr : mycus_colors ) {
            if( pr.first == current_color ) {
                p.rem_addiction( pr.second );
            } else {
                p.add_addiction( pr.second, 60 );
            }
        }
    }
}

static bool marloss_prevented( const player &p )
{
    if( p.is_npc() ) {
        return true;
    }
    if( p.has_trait( trait_MARLOSS_AVOID ) ) {
        //~"Uh-uh" is a sound used for "nope", "no", etc.
        p.add_msg_if_player( m_warning,
                             _( "After what happened that last time?  uh-uh.  You're not eating that alien poison." ) );
        return true;
    }
    if( p.has_trait( trait_THRESH_MYCUS ) ) {
        p.add_msg_if_player( m_info,
                             _( "We no longer require this scaffolding.  We reserve it for other uses." ) );
        return true;
    }

    return false;
}

int iuse::marloss( player *p, item *it, bool, const tripoint & )
{
    if( marloss_prevented( *p ) ) {
        return 0;
    }

    p->add_memorial_log( pgettext( "memorial_male", "Ate a marloss berry." ),
                         pgettext( "memorial_female", "Ate a marloss berry." ) );

    marloss_common( *p, *it, trait_MARLOSS );
    return it->type->charges_to_use();
}

int iuse::marloss_seed( player *p, item *it, bool, const tripoint & )
{
    if( !query_yn( _( "Sure you want to eat the %s? You could plant it in a mound of dirt." ),
                   it->tname().c_str() ) ) {
        return 0; // Save the seed for later!
    }

    if( marloss_prevented( *p ) ) {
        return 0;
    }

    p->add_memorial_log( pgettext( "memorial_male", "Ate a marloss seed." ),
                         pgettext( "memorial_female", "Ate a marloss seed." ) );

    marloss_common( *p, *it, trait_MARLOSS_BLUE );
    return it->type->charges_to_use();
}

int iuse::marloss_gel( player *p, item *it, bool, const tripoint & )
{
    if( marloss_prevented( *p ) ) {
        return 0;
    }

    p->add_memorial_log( pgettext( "memorial_male", "Ate some marloss jelly." ),
                         pgettext( "memorial_female", "Ate some marloss jelly." ) );

    marloss_common( *p, *it, trait_MARLOSS_YELLOW );
    return it->type->charges_to_use();
}

int iuse::mycus( player *p, item *it, bool t, const tripoint &pos )
{
    if( p->is_npc() ) {
        return it->type->charges_to_use();
    }
    // Welcome our guide.  Welcome.  To. The Mycus.
    if( p->has_trait( trait_THRESH_MARLOSS ) ) {
        p->add_memorial_log( pgettext( "memorial_male", "Became one with the Mycus." ),
                             pgettext( "memorial_female", "Became one with the Mycus." ) );
        p->add_msg_if_player( m_neutral,
                              _( "The apple tastes amazing, and you finish it quickly, not even noticing the lack of any core or seeds." ) );
        p->add_msg_if_player( m_good, _( "You feel better all over." ) );
        p->mod_painkiller( 30 );
        this->purifier( p, it, t, pos ); // Clear out some of that goo you may have floating around
        p->radiation = 0;
        p->healall( 4 ); // Can't make you a whole new person, but not for lack of trying
        p->add_msg_if_player( m_good,
                              _( "As the apple settles in, you feel ecstasy radiating through every part of your body..." ) );
        p->add_morale( MORALE_MARLOSS, 1000, 1000 ); // Last time you'll ever have it this good.  So enjoy.
        p->add_msg_if_player( m_good,
                              _( "Your eyes roll back in your head.  Everything dissolves into a blissful haze..." ) );
        /** @EFFECT_INT slightly reduces sleep duration when eating mycus */
        p->fall_asleep( 5_hours - p->int_cur * 1_minutes );
        p->unset_mutation( trait_THRESH_MARLOSS );
        p->set_mutation( trait_THRESH_MYCUS );
        //~ The Mycus does not use the term (or encourage the concept of) "you".  The PC is a local/native organism, but is now the Mycus.
        //~ It still understands the concept, but uninitelligent fungaloids and mind-bent symbiotes should not need it.
        //~ We are the Mycus.
        p->add_msg_if_player( m_good,
                              _( "We welcome into us.  We have endured long in this forbidding world." ) );
        p->add_msg_if_player( m_good,
                              _( "The natives have a saying: \"E Pluribus Unum\"  Out of many, one." ) );
        p->add_msg_if_player( m_good,
                              _( "We welcome the union of our lines in our local guide.  We will prosper, and unite this world." ) );
        p->add_msg_if_player( m_good, _( "Even now, our fruits adapt to better serve local physiology." ) );
        p->add_msg_if_player( m_good,
                              _( "As, in time, shall we adapt to better welcome those who have not received us." ) );
        fungal_effects fe( *g, g->m );
        for( const tripoint &pos : g->m.points_in_radius( p->pos(), 3 ) ) {
            fe.marlossify( pos );
        }
        p->rem_addiction( ADD_MARLOSS_R );
        p->rem_addiction( ADD_MARLOSS_B );
        p->rem_addiction( ADD_MARLOSS_Y );
    } else if( p->has_trait( trait_THRESH_MYCUS ) &&
               !p->has_trait( trait_M_DEPENDENT ) ) { // OK, now set the hook.
        if( !one_in( 3 ) ) {
            p->mutate_category( "MYCUS" );
            p->mod_hunger( 10 );
            p->mod_thirst( 10 );
            p->mod_fatigue( 5 );
            p->add_morale( MORALE_MARLOSS, 25, 200 ); // still covers up mutation pain
        }
    } else if( p->has_trait( trait_THRESH_MYCUS ) ) {
        p->mod_painkiller( 5 );
        p->stim += 5;
    } else { // In case someone gets one without having been adapted first.
        // Marloss is the Mycus' method of co-opting humans.  Mycus fruit is for symbiotes' maintenance and development.
        p->add_msg_if_player(
            _( "This apple tastes really weird!  You're not sure it's good for you..." ) );
        p->mutate();
        p->mod_pain( 2 * rng( 1, 5 ) );
        p->mod_hunger( 10 );
        p->mod_thirst( 10 );
        p->mod_fatigue( 5 );
        p->vomit(); // no hunger/quench benefit for you
        p->mod_healthy_mod( -8, -50 );
    }
    return it->type->charges_to_use();
}

// Types of petfood for taming each different monster.
enum Petfood {
    DOGFOOD,
    CATFOOD,
    CATTLEFODDER,
    BIRDFOOD
};

int feedpet( player &p, monster &mon, m_flag food_flag, const char *message )
{
    if( mon.has_flag( food_flag ) ) {
        p.add_msg_if_player( m_good, message, mon.get_name().c_str() );
        mon.friendly = -1;
        mon.add_effect( effect_pet, 1_turns, num_bp, true );
        return 1;
    } else {
        p.add_msg_if_player( _( "The %s doesn't want that kind of food." ), mon.get_name().c_str() );
        return 0;
    }
}

int petfood( player &p, const item &it, Petfood animal_food_type )
{
    tripoint dirp;
    if( !choose_adjacent( string_format( _( "Put the %s where?" ), it.tname().c_str() ), dirp ) ) {
        return 0;
    }
    p.moves -= 15;

    // First a check to see if we are trying to feed a NPC dog food.
    if( animal_food_type == DOGFOOD && g->critter_at<npc>( dirp ) != NULL ) {
        if( npc *const person_ = g->critter_at<npc>( dirp ) ) {
            npc &person = *person_;
            if( query_yn( _( "Are you sure you want to feed a person the dog food?" ) ) ) {
                p.add_msg_if_player( _( "You put your %1$s into %2$s's mouth!" ), it.tname().c_str(),
                                     person.name.c_str() );
                if( person.is_friend() || x_in_y( 9, 10 ) ) {
                    person.say(
                        _( "Okay, but please, don't give me this again.  I don't want to eat dog food in the cataclysm all day." ) );
                    return 1;
                } else {
                    p.add_msg_if_player( _( "%s knocks it out from your hand!" ), person.name.c_str() );
                    person.make_angry();
                    return 1;
                }
            } else {
                p.add_msg_if_player( _( "Never mind." ) );
                return 0;
            }
        }
        // Then monsters.
    } else if( monster *const mon_ptr = g->critter_at<monster>( dirp, true ) ) {
        monster &mon = *mon_ptr;
        // This switch handles each petfood for each type of tameable monster.
        switch( animal_food_type ) {
            case DOGFOOD:
                if( mon.type->id == mon_dog_thing ) {
                    p.deal_damage( &mon, bp_hand_r, damage_instance( DT_CUT, rng( 1, 10 ) ) );
                    p.add_msg_if_player( m_bad, _( "You want to feed it the dog food, but it bites your fingers!" ) );
                    if( one_in( 5 ) ) {
                        p.add_msg_if_player(
                            _( "Apparently it's more interested in your flesh than the dog food in your hand!" ) );
                        return 1;
                    }
                } else
                    return feedpet( p, mon, MF_DOGFOOD,
                                    _( "The %s seems to like you!  It lets you pat its head and seems friendly." ) );
                break;
            case CATFOOD:
                return feedpet( p, mon, MF_CATFOOD,
                                _( "The %s seems to like you!  Or maybe it just tolerates your presence better.  It's hard to tell with felines." ) );
                break;
            case CATTLEFODDER:
                return feedpet( p, mon, MF_CATTLEFODDER,
                                _( "The %s seems to like you!  It lets you pat its head and seems friendly." ) );
                break;
            case BIRDFOOD:
                return feedpet( p, mon, MF_BIRDFOOD,
                                _( "The %s seems to like you!  It runs around your legs and seems friendly." ) );
                break;
        }

    } else {
        p.add_msg_if_player( _( "There is nothing to be fed here." ) );
        return 0;
    }

    return 1;
}


int iuse::dogfood( player *p, item *it, bool, const tripoint & )
{
    return petfood( *p, *it, DOGFOOD );
}

int iuse::catfood( player *p, item *it, bool, const tripoint & )
{
    return petfood( *p, *it, CATFOOD );
}

int iuse::feedcattle( player *p, item *it, bool, const tripoint & )
{
    return petfood( *p, *it, CATTLEFODDER );
}

int iuse::feedbird( player *p, item *it, bool, const tripoint & )
{
    return petfood( *p, *it, BIRDFOOD );
}

int iuse::sew_advanced( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        return 0;
    }

    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }

    if( p->fine_detail_vision_mod() > 4 ) {
        add_msg( m_info, _( "You can't see to sew!" ) );
        return 0;
    }

    int pos = g->inv_for_filter( _( "Enhance what?" ), []( const item & itm ) {
        return itm.made_of( material_id( "cotton" ) ) ||
               itm.made_of( material_id( "leather" ) ) ||
               itm.made_of( material_id( "fur" ) ) ||
               itm.made_of( material_id( "nomex" ) ) ||
               itm.made_of( material_id( "plastic" ) ) ||
               itm.made_of( material_id( "kevlar" ) ) ||
               itm.made_of( material_id( "wool" ) );
    } );
    item &mod = p->i_at( pos );
    if( mod.is_null() ) {
        p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
        return 0;
    }

    if( !mod.is_armor() ) {
        p->add_msg_if_player( m_info, _( "You can only tailor your clothes!" ) );
        return 0;
    }
    if( mod.is_firearm() ) {
        p->add_msg_if_player( m_info, _( "You can't use a tailor's kit on a firearm!" ) );
        return 0;
    }
    if( mod.is_power_armor() ) {
        p->add_msg_if_player( m_info, _( "You can't modify your power armor!" ) );
        return 0;
    }

    std::vector<itype_id> repair_items;

    // Little helper to cut down some surplus redundancy and repetition
    const auto add_material = [&]( const material_id & material,
    const itype_id & mat_item ) {
        if( mod.made_of( material ) ) {
            repair_items.push_back( mat_item );
        }
    };

    add_material( material_id( "cotton" ), "rag" );
    add_material( material_id( "leather" ), "leather" );
    add_material( material_id( "fur" ), "fur" );
    add_material( material_id( "nomex" ), "nomex" );
    add_material( material_id( "plastic" ), "plastic_chunk" );
    add_material( material_id( "kevlar" ), "kevlar_plate" );
    add_material( material_id( "wool" ), "felt_patch" );
    if( repair_items.empty() ) {
        p->add_msg_if_player( m_info,
                              _( "Your %s is not made of fabric, leather, fur, Kevlar, wool or plastic." ),
                              mod.tname().c_str() );
        return 0;
    }
    if( &mod == it || std::find( repair_items.begin(), repair_items.end(),
                                 mod.typeId() ) != repair_items.end() ) {
        p->add_msg_if_player( m_info,
                              _( "This can be used to repair or modify other items, not itself." ) );
        return 0;
    }

    // Gives us an item with the mod added or removed (toggled)
    const auto modded_copy = []( const item & proto, const std::string & mod_type ) {
        item mcopy = proto;
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
    const int items_needed = mod.volume() / 750_ml + 1;
    const inventory &crafting_inv = p->crafting_inventory();
    // Go through all discovered repair items and see if we have any of them available
    for( auto &material : mod_materials ) {
        has_enough[material] = crafting_inv.has_amount( material, items_needed );
    }

    const int mod_count = mod.item_tags.count( "wooled" ) + mod.item_tags.count( "furred" ) +
                          mod.item_tags.count( "leather_padded" ) + mod.item_tags.count( "kevlar_padded" );

    // We need extra thread to lose it on bad rolls
    const int thread_needed = mod.volume() / 125_ml + 10;
    // Returns true if the item already has the mod or if we have enough materials and thread to add it
    const auto can_add_mod = [&]( const std::string & new_mod, const itype_id & mat_item ) {
        return mod.item_tags.count( new_mod ) > 0 ||
               ( it->charges >= thread_needed && has_enough[mat_item] );
    };

    uimenu tmenu;
    // TODO: Tell how much thread will we use
    if( it->charges >= thread_needed ) {
        tmenu.text = _( "How do you want to modify it?" );
    } else {
        tmenu.text = _( "Not enough thread to modify. Which modification do you want to remove?" );
    }

    // TODO: The supremely ugly block of code below looks better than 200 line boilerplate
    // that was there before, but it can probably be moved into a helper somehow

    // TODO 2: List how much material we have and how much we need
    item temp_item = modded_copy( mod, "wooled" );
    // Can we perform this addition or removal
    bool enab = can_add_mod( "wooled", "felt_patch" );
    tmenu.addentry( 0, enab, MENU_AUTOASSIGN, _( "%s (Warmth: %d->%d, Encumbrance: %d->%d)" ),
                    mod.item_tags.count( "wooled" ) == 0 ? _( "Line it with wool" ) : _( "Destroy wool lining" ),
                    mod.get_warmth(), temp_item.get_warmth(), mod.get_encumber(), temp_item.get_encumber() );

    temp_item = modded_copy( mod, "furred" );
    enab = can_add_mod( "furred", "fur" );
    tmenu.addentry( 1, enab, MENU_AUTOASSIGN, _( "%s (Warmth: %d->%d, Encumbrance: %d->%d)" ),
                    mod.item_tags.count( "furred" ) == 0 ? _( "Line it with fur" ) : _( "Destroy fur lining" ),
                    mod.get_warmth(), temp_item.get_warmth(), mod.get_encumber(), temp_item.get_encumber() );

    temp_item = modded_copy( mod, "leather_padded" );
    enab = can_add_mod( "leather_padded", "leather" );
    tmenu.addentry( 2, enab, MENU_AUTOASSIGN, _( "%s (Bash/Cut: %d/%d->%d/%d, Encumbrance: %d->%d)" ),
                    mod.item_tags.count( "leather_padded" ) == 0 ? _( "Pad with leather" ) :
                    _( "Destroy leather padding" ),
                    mod.bash_resist(), mod.cut_resist(), temp_item.bash_resist(), temp_item.cut_resist(),
                    mod.get_encumber(), temp_item.get_encumber() );

    temp_item = modded_copy( mod, "kevlar_padded" );
    enab = can_add_mod( "kevlar_padded", "kevlar_plate" );
    tmenu.addentry( 3, enab, MENU_AUTOASSIGN, _( "%s (Bash/Cut: %d/%d->%d/%d, Encumbrance: %d->%d)" ),
                    mod.item_tags.count( "kevlar_padded" ) == 0 ? _( "Pad with Kevlar" ) :
                    _( "Destroy Kevlar padding" ),
                    mod.bash_resist(), mod.cut_resist(), temp_item.bash_resist(), temp_item.cut_resist(),
                    mod.get_encumber(), temp_item.get_encumber() );

    tmenu.addentry( 4, true, 'q', _( "Cancel" ) );

    tmenu.query();
    const int choice = tmenu.ret;

    if( choice < 0 || choice > 3 ) {
        return 0;
    }

    // The mod player picked
    const std::string &the_mod = clothing_mods[choice];

    // If the picked mod already exists, player wants to destroy it
    if( mod.item_tags.count( the_mod ) ) {
        if( query_yn( _( "Are you sure?  You will not gain any materials back." ) ) ) {
            mod.item_tags.erase( the_mod );
        }

        return 0;
    }

    // Get the id of the material used
    const auto &repair_item = mod_materials[choice];

    std::vector<item_comp> comps;
    comps.push_back( item_comp( repair_item, items_needed ) );
    p->moves -= 500 * p->fine_detail_vision_mod();
    p->practice( skill_tailor, items_needed * 3 + 3 );
    /** @EFFECT_TAILOR randomly improves clothing modification efforts */
    int rn = dice( 3, 2 + p->get_skill_level( skill_tailor ) ); // Skill
    /** @EFFECT_DEX randomly improves clothing modification efforts */
    rn += rng( 0, p->dex_cur / 2 );                    // Dexterity
    /** @EFFECT_PER randomly improves clothing modification efforts */
    rn += rng( 0, p->per_cur / 2 );                    // Perception
    rn -= mod_count * 10;                              // Other mods

    if( rn <= 8 ) {
        p->add_msg_if_player( m_bad, _( "You damage your %s trying to modify it!" ),
                              mod.tname().c_str() );
        if( mod.inc_damage() ) {
            p->add_msg_if_player( m_bad, _( "You destroy it!" ) );
            p->i_rem_keep_contents( pos );
        }
        return thread_needed / 2;
    } else if( rn <= 10 ) {
        p->add_msg_if_player( m_bad,
                              _( "You fail to modify the clothing, and you waste thread and materials." ) );
        p->consume_items( comps );
        return thread_needed;
    } else if( rn <= 14 ) {
        p->add_msg_if_player( m_mixed, _( "You modify your %s, but waste a lot of thread." ),
                              mod.tname().c_str() );
        p->consume_items( comps );
        mod.item_tags.insert( the_mod );
        return thread_needed;
    }

    p->add_msg_if_player( m_good, _( "You modify your %s!" ), mod.tname().c_str() );
    mod.item_tags.insert( the_mod );
    p->consume_items( comps );
    return thread_needed / 2;
}

int iuse::radio_mod( player *p, item *, bool, const tripoint & )
{
    if( p->is_npc() ) {
        // Now THAT would be kinda cruel
        return 0;
    }

    int inventory_index = g->inv_for_filter( _( "Modify what?" ), []( const item & itm ) {
        return itm.has_flag( "RADIO_MODABLE" );
    } );
    item &modded = p->i_at( inventory_index );

    if( modded.is_null() ) {
        p->add_msg_if_player( _( "You do not have that item!" ) );
        return 0;
    }

    int choice = menu( true, _( "Which signal should activate the item?:" ), _( "\"Red\"" ),
                       _( "\"Blue\"" ), _( "\"Green\"" ), _( "Cancel" ), nullptr );

    std::string newtag;
    std::string colorname;
    switch( choice ) {
        case 1:
            newtag = "RADIOSIGNAL_1";
            colorname = _( "\"Red\"" );
            break;
        case 2:
            newtag = "RADIOSIGNAL_2";
            colorname = _( "\"Blue\"" );
            break;
        case 3:
            newtag = "RADIOSIGNAL_3";
            colorname = _( "\"Green\"" );
            break;
        default:
            return 0;
    }

    if( modded.has_flag( "RADIO_MOD" ) && modded.has_flag( newtag ) ) {
        p->add_msg_if_player( _( "This item has been modified this way already." ) );
        return 0;
    }

    remove_radio_mod( modded, *p );

    p->add_msg_if_player(
        _( "You modify your %1$s to listen for %2$s activation signal on the radio." ),
        modded.tname().c_str(), colorname.c_str() );
    modded.item_tags.insert( "RADIO_ACTIVATION" );
    modded.item_tags.insert( "RADIOCARITEM" );
    modded.item_tags.insert( "RADIO_MOD" );
    modded.item_tags.insert( newtag );
    return 1;
}

int iuse::remove_all_mods( player *p, item *, bool, const tripoint & )
{
    if( !p ) {
        return 0;
    }

    auto loc = g->inv_map_splice( []( const item & e ) {
        return !e.toolmods().empty();
    },
    _( "Remove mods from tool?" ), 1,
    _( "You don't have any modified tools." ) );

    if( !loc ) {
        add_msg( m_info, _( "Never mind." ) );
        return 0;
    }

    if( !loc->ammo_remaining() || g->unload( *loc ) ) {
        auto mod = std::find_if( loc->contents.begin(), loc->contents.end(), []( const item & e ) {
            return e.is_toolmod();
        } );
        p->i_add_or_drop( *mod );
        loc->contents.erase( mod );

        remove_radio_mod( *loc, *p );
    }
    return 0;
}

int iuse::fishing_rod( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        // Long actions - NPCs don't like those yet
        return 0;
    }

    int dirx = 0;
    int diry = 0;

    if( !choose_adjacent( _( "Fish where?" ), dirx, diry ) ) {
        return 0;
    }

    if( !g->m.has_flag( "FISHABLE", dirx, diry ) ) {
        p->add_msg_if_player( m_info, _( "You can't fish there!" ) );
        return 0;
    }
    point op = ms_to_omt_copy( g->m.getabs( dirx, diry ) );
    if( !overmap_buffer.ter( op.x, op.y, g->get_levz() )->has_flag( river_tile ) ) {
        p->add_msg_if_player( m_info, _( "That water does not contain any fish.  Try a river instead." ) );
        return 0;
    }
    std::vector<monster *> fishables = g->get_fishable( 60 );
    if( fishables.empty() ) {
        p->add_msg_if_player( m_info,
                              _( "There are no fish around.  Try another spot." ) ); // maybe let the player find that out by himself?
        return 0;
    }

    p->add_msg_if_player( _( "You cast your line and wait to hook something..." ) );

    p->assign_activity( activity_id( "ACT_FISH" ), 30000, 0, p->get_item_position( it ), it->tname() );

    return 0;
}

int iuse::fish_trap( player *p, item *it, bool t, const tripoint &pos )
{
    if( !t ) {
        // Handle deploying fish trap.
        if( it->active ) {
            it->active = false;
            return 0;
        }

        if( it->charges < 0 ) {
            it->charges = 0;
            return 0;
        }

        if( p->is_underwater() ) {
            p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
            return 0;
        }

        if( it->charges == 0 ) {
            p->add_msg_if_player( _( "Fish are not foolish enough to go in here without bait." ) );
            return 0;
        }

        int dirx = 0;
        int diry = 0;

        if( !choose_adjacent( _( "Put fish trap where?" ), dirx, diry ) ) {
            return 0;
        }
        if( !g->m.has_flag( "FISHABLE", dirx, diry ) ) {
            p->add_msg_if_player( m_info, _( "You can't fish there!" ) );
            return 0;
        }
        point op = ms_to_omt_copy( g->m.getabs( dirx, diry ) );
        if( !overmap_buffer.ter( op.x, op.y, g->get_levz() )->has_flag( river_tile ) ) {
            p->add_msg_if_player( m_info, _( "That water does not contain any fish, try a river instead." ) );
            return 0;
        }
        std::vector<monster *> fishables = g->get_fishable( 60 );
        if( fishables.empty() ) {
            p->add_msg_if_player( m_info,
                                  _( "There is no fish around.  Try another spot." ) ); // maybe let the player find that out by himself?
            return 0;
        }
        it->active = true;
        it->set_age( 0 );
        g->m.add_item_or_charges( dirx, diry, *it );
        p->i_rem( it );
        p->add_msg_if_player( m_info,
                              _( "You place the fish trap, in three hours or so you may catch some fish." ) );

        return 0;

    } else {
        // Handle processing fish trap over time.
        if( it->charges == 0 ) {
            it->active = false;
            return 0;
        }
        if( it->age() > 3_hours ) {
            it->active = false;

            if( !g->m.has_flag( "FISHABLE", pos ) ) {
                return 0;
            }
            point op = ms_to_omt_copy( g->m.getabs( pos.x, pos.y ) );
            if( !overmap_buffer.ter( op.x, op.y, g->get_levz() )->has_flag( river_tile ) ) {
                return 0;
            }
            int success = -50;
            const int surv = p->get_skill_level( skill_survival );
            const int attempts = rng( it->charges, it->charges * it->charges );
            for( int i = 0; i < attempts; i++ ) {
                /** @EFFECT_SURVIVAL randomly increases number of fish caught in fishing trap */
                success += rng( surv, surv * surv );
            }

            it->charges = rng( -1, it->charges );
            if( it->charges < 0 ) {
                it->charges = 0;
            }

            int fishes = 0;

            if( success < 0 ) {
                fishes = 0;
            } else if( success < 300 ) {
                fishes = 1;
            } else if( success < 1500 ) {
                fishes = 2;
            } else {
                fishes = rng( 3, 5 );
            }

            if( fishes == 0 ) {
                it->charges = 0;
                p->practice( skill_survival, rng( 5, 15 ) );

                return 0;
            }
            std::vector<monster *> fishables = g->get_fishable( 60 ); //get the fishables around the trap's spot
            for( int i = 0; i < fishes; i++ ) {
                p->practice( skill_survival, rng( 3, 10 ) );
                if( fishables.size() > 1 ) {
                    g->catch_a_monster( fishables, pos, p, 180000 ); //catch the fish! 180000 is the time spent fishing.
                } else {
                    //there will always be a chance that the player will get lucky and catch a fish
                    //not existing in the fishables vector. (maybe it was in range, but wandered off)
                    //lets say it is a 5% chance per fish to catch
                    if( one_in( 20 ) ) {
                        const std::vector<mtype_id> fish_group = MonsterGroupManager::GetMonstersFromGroup(
                                    mongroup_id( "GROUP_FISH" ) );
                        const mtype_id &fish_mon = random_entry_ref( fish_group );
                        //Yes, we can put fishes in the trap like knives in the boot,
                        //and then get fishes via activation of the item,
                        //but it's not as comfortable as if you just put fishes in the same tile with the trap.
                        //Also: corpses and comestibles do not rot in containers like this, but on the ground they will rot.
                        //we don't know when it was caught so use a random turn
                        g->m.add_item_or_charges( pos, item::make_corpse( fish_mon, it->birthday() + rng( 0_turns,
                                                  3_hours ) ) );
                        break; //this can happen only once
                    }
                }
            }
        }
        return 0;
    }
}

int iuse::extinguisher( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        return 0;
    }
    g->draw();
    tripoint dest;
    // If anyone other than the player wants to use one of these,
    // they're going to need to figure out how to aim it.
    if( !choose_adjacent( _( "Spray where?" ), dest ) ) {
        return 0;
    }

    p->moves -= 140;

    // Reduce the strength of fire (if any) in the target tile.
    g->m.adjust_field_strength( dest, fd_fire, 0 - rng( 2, 3 ) );

    // Also spray monsters in that tile.
    if( monster *const mon_ptr = g->critter_at<monster>( dest, true ) ) {
        monster &critter = *mon_ptr;
        critter.moves -= 150;
        bool blind = false;
        if( one_in( 2 ) && critter.has_flag( MF_SEES ) ) {
            blind = true;
            critter.add_effect( effect_blind, rng( 1_minutes, 2_minutes ) );
        }
        if( g->u.sees( critter ) ) {
            p->add_msg_if_player( _( "The %s is sprayed!" ), critter.name().c_str() );
            if( blind ) {
                p->add_msg_if_player( _( "The %s looks blinded." ), critter.name().c_str() );
            }
        }
        if( critter.made_of( LIQUID ) ) {
            if( g->u.sees( critter ) ) {
                p->add_msg_if_player( _( "The %s is frozen!" ), critter.name().c_str() );
            }
            critter.apply_damage( p, bp_torso, rng( 20, 60 ) );
            critter.set_speed_base( critter.get_speed_base() / 2 );
        }
    }

    // Slightly reduce the strength of fire immediately behind the target tile.
    if( g->m.passable( dest ) ) {
        dest.x += ( dest.x - p->posx() );
        dest.y += ( dest.y - p->posy() );

        g->m.adjust_field_strength( dest, fd_fire, std::min( 0 - rng( 0, 1 ) + rng( 0, 1 ), 0L ) );
    }

    return it->type->charges_to_use();
}

int iuse::rm13armor_off( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( m_info, _( "The RM13 combat armor's fuel cells are dead." ) );
        return 0;
    } else {
        std::string oname = it->typeId() + "_on";
        p->add_msg_if_player( _( "You activate your RM13 combat armor." ) );
        p->add_msg_if_player( _( "Rivtech Model 13 RivOS v2.19:   ONLINE." ) );
        p->add_msg_if_player( _( "CBRN defense system:            ONLINE." ) );
        p->add_msg_if_player( _( "Acoustic dampening system:      ONLINE." ) );
        p->add_msg_if_player( _( "Thermal regulation system:      ONLINE." ) );
        p->add_msg_if_player( _( "Vision enhancement system:      ONLINE." ) );
        p->add_msg_if_player( _( "Electro-reactive armor system:  ONLINE." ) );
        p->add_msg_if_player( _( "All systems nominal." ) );
        it->convert( oname ).active = true;
        return it->type->charges_to_use();
    }
}

int iuse::rm13armor_on( player *p, item *it, bool t, const tripoint & )
{
    if( t ) { // Normal use
    } else { // Turning it off
        std::string oname = it->typeId();
        if( oname.length() > 3 && oname.compare( oname.length() - 3, 3, "_on" ) == 0 ) {
            oname.erase( oname.length() - 3, 3 );
        } else {
            debugmsg( "no item type to turn it into (%s)!", oname.c_str() );
            return 0;
        }
        p->add_msg_if_player( _( "RivOS v2.19 shutdown sequence initiated." ) );
        p->add_msg_if_player( _( "Shutting down." ) );
        p->add_msg_if_player( _( "Your RM13 combat armor turns off." ) );
        it->convert( oname ).active = false;
    }
    return it->type->charges_to_use();
}

int iuse::unpack_item( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    std::string oname = it->typeId() + "_on";
    p->moves -= 300;
    p->add_msg_if_player( _( "You unpack your %s for use." ), it->tname().c_str() );
    it->convert( oname ).active = false;
    return 0;
}

int iuse::pack_item( player *p, item *it, bool t, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    if( t ) { // Normal use
        // Numbers below -1 are reserved for worn items
    } else if( p->get_item_position( it ) < -1 ) {
        p->add_msg_if_player( m_info, _( "You can't pack your %s until you take it off." ),
                              it->tname().c_str() );
        return 0;
    } else { // Turning it off
        std::string oname = it->typeId();
        if( oname.length() > 3 && oname.compare( oname.length() - 3, 3, "_on" ) == 0 ) {
            oname.erase( oname.length() - 3, 3 );
        } else {
            debugmsg( "no item type to turn it into (%s)!", oname.c_str() );
            return 0;
        }
        p->moves -= 500;
        p->add_msg_if_player( _( "You pack your %s for storage." ), it->tname().c_str() );
        it->convert( oname ).active = false;
    }
    return 0;
}

static int cauterize_elec( player &p, item &it )
{
    if( it.charges == 0 && it.ammo_capacity() ) {
        p.add_msg_if_player( m_info, _( "You need batteries to cauterize wounds." ) );
        return 0;
    } else if( !p.has_effect( effect_bite ) && !p.has_effect( effect_bleed ) && !p.is_underwater() ) {
        if( ( p.has_trait( trait_MASOCHIST ) || p.has_trait( trait_MASOCHIST_MED ) ||
              p.has_trait( trait_CENOBITE ) ) &&
            p.query_yn( _( "Cauterize yourself for fun?" ) ) ) {
            return cauterize_actor::cauterize_effect( p, it, true ) ? it.type->charges_to_use() : 0;
        } else {
            p.add_msg_if_player( m_info,
                                 _( "You are not bleeding or bitten, there is no need to cauterize yourself." ) );
            return 0;
        }
    } else if( p.is_npc() || query_yn( _( "Cauterize any open wounds?" ) ) ) {
        return cauterize_actor::cauterize_effect( p, it, true ) ? it.type->charges_to_use() : 0;
    }
    return 0;
}

int iuse::water_purifier( player *p, item *it, bool, const tripoint & )
{
    auto obj = g->inv_map_splice( []( const item & e ) {
        return !e.contents.empty() && e.contents.front().typeId() == "water";
    }, _( "Purify what?" ), 1, _( "You don't have water to purify." ) );

    if( !obj ) {
        p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
        return 0;
    }

    item &liquid = obj->contents.front();
    if( !it->units_sufficient( *p, liquid.charges ) ) {
        p->add_msg_if_player( m_info, _( "That volume of water is too large to purify." ) );
        return 0;
    }

    p->moves -= 150;

    liquid.convert( "water_clean" ).poison = 0;
    return liquid.charges;
}

int iuse::radio_off( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( _( "It's dead." ) );
    } else {
        p->add_msg_if_player( _( "You turn the radio on." ) );
        it->convert( "radio_on" ).active = true;
    }
    return it->type->charges_to_use();
}

int iuse::directional_antenna( player *p, item *it, bool, const tripoint & )
{
    // Find out if we have an active radio
    auto radios = p->items_with( []( const item & it ) {
        return it.typeId() == "radio_on";
    } );
    if( radios.empty() ) {
        add_msg( m_info, _( "Must have an active radio to check for signal direction." ) );
        return 0;
    }
    const item radio = *radios.front();
    // Find the radio station its tuned to (if any)
    const auto tref = overmap_buffer.find_radio_station( radio.frequency );
    if( !tref ) {
        add_msg( m_info, _( "You can't find the direction if your radio isn't tuned." ) );
        return 0;
    }
    // Report direction.
    const auto player_pos = p->global_sm_location();
    direction angle = direction_from( player_pos.x, player_pos.y,
                                      tref.abs_sm_pos.x, tref.abs_sm_pos.y );
    add_msg( _( "The signal seems strongest to the %s." ), direction_name( angle ).c_str() );
    return it->type->charges_to_use();
}

int iuse::radio_on( player *p, item *it, bool t, const tripoint &pos )
{
    if( t ) {
        // Normal use
        std::string message = _( "Radio: Kssssssssssssh." );
        const auto tref = overmap_buffer.find_radio_station( it->frequency );
        if( tref ) {
            const auto selected_tower = tref.tower;
            if( selected_tower->type == MESSAGE_BROADCAST ) {
                message = selected_tower->message;
            } else if( selected_tower->type == WEATHER_RADIO ) {
                message = weather_forecast( tref.abs_sm_pos );
            }

            message = obscure_message( message, [&]()->int {
                int signal_roll = dice( 10, tref.signal_strength * 3 );
                int static_roll = dice( 10, 100 );
                if( static_roll > signal_roll )
                {
                    if( static_roll < signal_roll * 1.1 && one_in( 4 ) ) {
                        return 0;
                    } else {
                        return '#';
                    }
                } else
                {
                    return -1;
                }
            } );

            std::vector<std::string> segments = foldstring( message, RADIO_PER_TURN );
            int index = calendar::turn % segments.size();
            std::stringstream messtream;
            messtream << string_format( _( "radio: %s" ), segments[index].c_str() );
            message = messtream.str();
        }
        sounds::ambient_sound( pos, 6, message );
    } else { // Activated
        int ch = 2;
        if( it->ammo_remaining() > 0 ) {
            ch = menu( true, _( "Radio:" ), _( "Scan" ), _( "Turn off" ), NULL );
        }

        switch( ch ) {
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
                        ( lowest_larger_tower == nullptr || new_frequency < lowest_larger_tower->frequency ) ) {
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
                p->add_msg_if_player( _( "The radio dies." ) );
                it->convert( "radio" ).active = false;
                break;
            case 3:
                break;
        }
    }
    return it->type->charges_to_use();
}

int iuse::noise_emitter_off( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( _( "It's dead." ) );
    } else {
        p->add_msg_if_player( _( "You turn the noise emitter on." ) );
        it->convert( "noise_emitter_on" ).active = true;
    }
    return it->type->charges_to_use();
}

int iuse::noise_emitter_on( player *p, item *it, bool t, const tripoint &pos )
{
    if( t ) { // Normal use
        //~ the sound of a noise emitter when turned on
        sounds::ambient_sound( pos, 30, _( "KXSHHHHRRCRKLKKK!" ) );
    } else { // Turning it off
        p->add_msg_if_player( _( "The infernal racket dies as the noise emitter turns off." ) );
        it->convert( "noise_emitter" ).active = false;
    }
    return it->type->charges_to_use();
}

int iuse::ma_manual( player *p, item *it, bool, const tripoint & )
{
    // [CR] - should NPCs just be allowed to learn this stuff? Just like that?

    // strip "manual_" from the start of the item id, add the rest to "style_"
    // TODO: replace this terrible hack to rely on the item name matching the style name, it's terrible.
    const matype_id style_to_learn( "style_" + it->typeId().substr( 7 ) );

    if( p->has_martialart( style_to_learn ) ) {
        p->add_msg_if_player( m_info, _( "You already know all this book has to teach." ) );
        return 0;
    }

    p->ma_styles.push_back( style_to_learn );

    p->add_msg_if_player( m_good, _( "You learn the essential elements of the style." ) );

    return 1;
}

static bool pry_nails( player &p, const ter_id &type, const int dirx, const int diry )
{
    int nails = 0;
    int boards = 0;
    ter_id newter;
    if( type == t_fence ) {
        nails = 6;
        boards = 3;
        newter = t_fence_post;
        p.add_msg_if_player( _( "You pry out the fence post." ) );
    } else if( type == t_window_boarded ) {
        nails = 8;
        boards = 4;
        newter = t_window_frame;
        p.add_msg_if_player( _( "You pry the boards from the window." ) );
    } else if( type == t_window_boarded_noglass ) {
        nails = 8;
        boards = 4;
        newter = t_window_empty;
        p.add_msg_if_player( _( "You pry the boards from the window frame." ) );
    } else if( type == t_door_boarded || type == t_door_boarded_damaged ||
               type == t_rdoor_boarded || type == t_rdoor_boarded_damaged ||
               type == t_door_boarded_peep || type == t_door_boarded_damaged_peep ) {
        nails = 8;
        boards = 4;
        if( type == t_door_boarded ) {
            newter = t_door_c;
        } else if( type == t_door_boarded_damaged ) {
            newter = t_door_b;
        } else if( type == t_door_boarded_peep ) {
            newter = t_door_c_peep;
        } else if( type == t_door_boarded_damaged_peep ) {
            newter = t_door_b_peep;
        } else if( type == t_rdoor_boarded ) {
            newter = t_rdoor_c;
        } else { // if (type == t_rdoor_boarded_damaged)
            newter = t_rdoor_b;
        }
        p.add_msg_if_player( _( "You pry the boards from the door." ) );
    } else {
        return false;
    }
    p.practice( skill_fabrication, 1, 1 );
    p.moves -= 500;
    g->m.spawn_item( p.posx(), p.posy(), "nail", 0, nails );
    g->m.spawn_item( p.posx(), p.posy(), "2x4", boards );
    g->m.ter_set( dirx, diry, newter );
    return true;
}

int iuse::hammer( player *p, item *it, bool, const tripoint & )
{
    g->draw();
    int x = 0;
    int y = 0;
    // If anyone other than the player wants to use one of these,
    // they're going to need to figure out how to aim it.
    if( !choose_adjacent( _( "Pry where?" ), x, y ) ) {
        return 0;
    }

    if( x == p->posx() && y == p->posy() ) {
        p->add_msg_if_player( _( "You try to hit yourself with the hammer." ) );
        p->add_msg_if_player( _( "But you can't touch this." ) );
        return 0;
    }

    ter_id type = g->m.ter( x, y );
    if( pry_nails( *p, type, x, y ) ) {
        return it->type->charges_to_use();
    } else {
        p->add_msg_if_player( m_info, _( "There's nothing to pry there." ) );
    }
    return 0;
}

int iuse::crowbar( player *p, item *it, bool, const tripoint &pos )
{
    // TODO: Make this 3D now that NPCs get to use items
    tripoint dirp = pos;
    if( pos == p->pos() ) {
        if( !choose_adjacent( _( "Pry where?" ), dirp ) ) {
            return 0;
        }
    } // else it is already set to pos in the line above if

    int &dirx = dirp.x;
    int &diry = dirp.y;

    if( dirx == p->posx() && diry == p->posy() ) {
        p->add_msg_if_player( m_info, _( "You attempt to pry open your wallet" ) );
        p->add_msg_if_player( m_info, _( "but alas.  You are just too miserly." ) );
        return 0;
    }
    ter_id type = g->m.ter( dirx, diry );
    const char *succ_action;
    const char *fail_action;
    ter_id new_type = t_null;
    bool noisy;
    int difficulty;

    if( type == t_door_locked || type == t_door_locked_alarm || type == t_door_locked_interior ) {
        succ_action = _( "You pry open the door." );
        fail_action = _( "You pry, but cannot pry open the door." );
        new_type = t_door_o;
        noisy = true;
        difficulty = 6;
    } else if( type == t_door_locked_peep ) {
        succ_action = _( "You pry open the door." );
        fail_action = _( "You pry, but cannot pry open the door." );
        new_type = t_door_o_peep;
        noisy = true;
        difficulty = 6;
    } else if( type == t_door_c ) {
        p->add_msg_if_player( m_info, _( "You notice the door is unlocked, so you simply open it." ) );
        g->m.ter_set( dirx, diry, t_door_o );
        p->mod_moves( -100 );
        return 0;
    } else if( type == t_door_c_peep ) {
        p->add_msg_if_player( m_info, _( "You notice the door is unlocked, so you simply open it." ) );
        g->m.ter_set( dirx, diry, t_door_o_peep );
        p->mod_moves( -100 );
        return 0;
    } else if( type == t_door_bar_locked ) {
        succ_action = _( "You pry open the door." );
        fail_action = _( "You pry, but cannot pry open the door." );
        new_type = t_door_bar_o;
        noisy = false;
        difficulty = 10;
    } else if( type == t_manhole_cover ) {
        succ_action = _( "You lift the manhole cover." );
        fail_action = _( "You pry, but cannot lift the manhole cover." );
        new_type = t_manhole;
        noisy = false;
        difficulty = 12;
    } else if( g->m.furn( dirx, diry ) == f_crate_c ) {
        succ_action = _( "You pop open the crate." );
        fail_action = _( "You pry, but cannot pop open the crate." );
        noisy = true;
        difficulty = 6;
    } else if( g->m.furn( dirx, diry ) == f_coffin_c ) {
        succ_action = _( "You wedge open the coffin." );
        fail_action = _( "You pry, but the coffin remains closed." );
        noisy = true;
        difficulty = 5;
    } else if( type == t_window_domestic || type == t_curtains || type == t_window_no_curtains ) {
        succ_action = _( "You pry open the window." );
        fail_action = _( "You pry, but cannot pry open the window." );
        new_type = ( type == t_window_no_curtains ) ? t_window_no_curtains_open : t_window_open;
        noisy = true;
        difficulty = 6;
    } else if( pry_nails( *p, type, dirx, diry ) ) {
        return it->type->charges_to_use();
    } else {
        p->add_msg_if_player( m_info, _( "There's nothing to pry there." ) );
        return 0;
    }

    /** @EFFECT_STR speeds up crowbar prying attempts */

    p->mod_moves( -( std::max( 20, ( difficulty * 20 ) - ( ( p->str_cur ) * 5 ) ) ) );
    /** @EFFECT_STR increases chance of crowbar prying success */

    if( dice( 4, difficulty ) < dice( 4, p->str_cur ) ) {
        p->add_msg_if_player( m_good, succ_action );

        if( g->m.furn( dirx, diry ) == f_crate_c ) {
            g->m.furn_set( dirx, diry, f_crate_o );
        } else if( g->m.furn( dirx, diry ) == f_coffin_c ) {
            g->m.furn_set( dirx, diry, f_coffin_o );
        } else {
            g->m.ter_set( dirx, diry, new_type );
        }

        if( noisy ) {
            sounds::sound( dirp, 12, _( "crunch!" ) );
        }
        if( type == t_manhole_cover ) {
            g->m.spawn_item( dirx, diry, "manhole_cover" );
        }
        if( type == t_door_locked_alarm ) {
            p->add_memorial_log( pgettext( "memorial_male", "Set off an alarm." ),
                                 pgettext( "memorial_female", "Set off an alarm." ) );
            sounds::sound( p->pos(), 40, _( "an alarm sound!" ) );
            if( !g->events.queued( EVENT_WANTED ) ) {
                g->events.add( EVENT_WANTED, calendar::turn + 30_minutes, 0, p->global_sm_location() );
            }
        }
    } else {
        if( type == t_window_domestic || type == t_curtains ) {
            //chance of breaking the glass if pry attempt fails
            /** @EFFECT_STR reduces chance of breaking window with crowbar */

            /** @EFFECT_MECHANICS reduces chance of breaking window with crowbar */
            if( dice( 4, difficulty ) > dice( 2, p->get_skill_level( skill_mechanics ) ) + dice( 2,
                    p->str_cur ) ) {
                p->add_msg_if_player( m_mixed, _( "You break the glass." ) );
                sounds::sound( dirp, 24, _( "glass breaking!" ) );
                g->m.ter_set( dirx, diry, t_window_frame );
                g->m.spawn_item( dirx, diry, "sheet", 2 );
                g->m.spawn_item( dirx, diry, "stick" );
                g->m.spawn_item( dirx, diry, "string_36" );
                return it->type->charges_to_use();
            }
        }
        p->add_msg_if_player( fail_action );
    }
    return it->type->charges_to_use();
}

int iuse::makemound( player *p, item *it, bool t, const tripoint &pos )
{
    if( !p || t ) {
        return 0;
    }

    tripoint dirp = pos;
    if( !choose_adjacent( _( "Till soil where?" ), dirp ) ) {
        return 0;
    }

    if( dirp == p->pos() ) {
        add_msg( m_info, _( "You think about jumping on a shovel, but then change up your mind." ) );
        return 0;
    }

    if( g->m.has_flag( "DIGGABLE", dirp ) && !g->m.has_flag( "PLANT", dirp ) ) {
        p->add_msg_if_player( _( "You churn up the earth here." ) );
        p->moves = -300;
        g->m.ter_set( dirp, t_dirtmound );
        return it->type->charges_to_use();
    } else {
        p->add_msg_if_player( _( "You can't churn up this ground." ) );
        return 0;
    }
}

int iuse::dig( player *p, item *it, bool t, const tripoint &pos )
{
    if( !p || t ) {
        return 0;
    }

    tripoint dirp = pos;
    if( !choose_adjacent( _( "Dig pit where?" ), dirp ) ) {
        return 0;
    }

    if( dirp == p->pos() ) {
        add_msg( m_info, _( "You delve into yourself." ) );
        return 0;
    }

    int moves;

    if( g->m.has_flag( "DIGGABLE", dirp ) ) {
        if( g->m.ter( dirp ) == t_pit_shallow ) {
            if( p->crafting_inventory().has_quality( DIG, 2 ) ) {
                moves = MINUTES( 90 ) / it->get_quality( DIG ) * 100;
            } else {
                p->add_msg_if_player( _( "You can't deepen this pit without a proper shovel." ) );
                return 0;
            }
        } else {
            moves = MINUTES( 30 ) / it->get_quality( DIG ) * 100;
        }
    } else {
        p->add_msg_if_player( _( "You can't dig a pit on this ground." ) );
        return 0;
    }

    p->assign_activity( activity_id( "ACT_DIG" ), moves, -1, p->get_item_position( it ) );
    p->activity.placement = dirp;

    return it->type->charges_to_use();
}

int iuse::fill_pit( player *p, item *it, bool t, const tripoint &pos )
{
    if( !p || t ) {
        return 0;
    }

    tripoint dirp = pos;
    if( !choose_adjacent( _( "Fill which pit or mound?" ), dirp ) ) {
        return 0;
    }

    if( dirp == p->pos() ) {
        add_msg( m_info, _( "You decide not to bury yourself that early." ) );
        return 0;
    }

    int moves;

    if( g->m.ter( dirp ) == t_pit || g->m.ter( dirp ) == t_pit_spiked ||
        g->m.ter( dirp ) == t_pit_glass || g->m.ter( dirp ) == t_pit_corpsed ) {
        moves = MINUTES( 15 ) * 100;
    } else if( g->m.ter( dirp ) == t_pit_shallow ) {
        moves = MINUTES( 10 ) * 100;
    } else if( g->m.ter( dirp ) == t_dirtmound ) {
        moves = MINUTES( 5 ) * 100;
    } else {
        p->add_msg_if_player( _( "There is nothing to fill." ) );
        return 0;
    }

    p->assign_activity( activity_id( "ACT_FILL_PIT" ), moves, -1, p->get_item_position( it ) );
    p->activity.placement = dirp;

    return it->type->charges_to_use();
}

/**
 * Explanation of ACT_CLEAR_RUBBLE activity values:
 *
 * coords[0]: Where the rubble is.
 * index: The bonus, for calculating hunger and thirst penalties.
 */

int iuse::clear_rubble( player *p, item *it, bool, const tripoint &pos )
{
    for( const tripoint &pt : closest_tripoints_first( 1, pos ) ) {
        if( g->m.furn( pt ).obj().examine == iexamine::rubble ) {
            // costs per tile are (30 minutes/dig quality), with +66% hunger and +100% thirst, so:
            // DIG 2 = 150  seconds
            // DIG 3 = 75   seconds
            // DIG 4 = 50   seconds
            // DIG 5 = 37.5 seconds
            int bonus = std::max( it->get_quality( quality_id( "DIG" ) ) - 1, 1 );
            player_activity act( activity_id( "ACT_CLEAR_RUBBLE" ), 2500 / bonus, bonus );
            act.coords.push_back( pt );
            p->assign_activity( act );

            return it->type->charges_to_use();
        }
    }

    p->add_msg_if_player( m_bad, _( "There's no rubble to clear." ) );
    return 0;
}

void act_vehicle_siphon( vehicle * ); // veh_interact.cpp

int iuse::siphon( player *p, item *it, bool, const tripoint & )
{
    tripoint posp;
    if( !choose_adjacent( _( "Siphon from where?" ), posp ) ) {
        return 0;
    }

    const optional_vpart_position vp = g->m.veh_at( posp );
    if( !vp ) {
        p->add_msg_if_player( m_info, _( "There's no vehicle there." ) );
        return 0;
    }
    act_vehicle_siphon( &vp->vehicle() );
    return it->type->charges_to_use();
}

int toolweapon_off( player &p, item &it, const bool fast_startup,
                    const bool condition, const int volume,
                    const std::string &msg_success, const std::string &msg_failure )
{
    p.moves -= fast_startup ? 60 : 80;
    if( condition && it.ammo_remaining() > 0 ) {
        if( it.typeId() == "chainsaw_off" ) {
            sfx::play_variant_sound( "chainsaw_cord", "chainsaw_on", sfx::get_heard_volume( p.pos() ) );
            sfx::play_variant_sound( "chainsaw_start", "chainsaw_on", sfx::get_heard_volume( p.pos() ) );
            sfx::play_ambient_variant_sound( "chainsaw_idle", "chainsaw_on", sfx::get_heard_volume( p.pos() ),
                                             18, 1000 );
            sfx::play_ambient_variant_sound( "weapon_theme", "chainsaw", sfx::get_heard_volume( p.pos() ), 19,
                                             3000 );
        }
        sounds::sound( p.pos(), volume, msg_success );
        it.convert( it.typeId().substr( 0, it.typeId().size() - 4 ) + "_on" ); // 4 is the length of "_off".
        it.active = true;
    } else {
        if( it.typeId() == "chainsaw_off" ) {
            sfx::play_variant_sound( "chainsaw_cord", "chainsaw_on", sfx::get_heard_volume( p.pos() ) );
        }
        p.add_msg_if_player( msg_failure );
    }
    return it.type->charges_to_use();
}

int iuse::combatsaw_off( player *p, item *it, bool, const tripoint & )
{
    return toolweapon_off( *p, *it,
                           true,
                           !p->is_underwater(),
                           30, _( "With a snarl, the combat chainsaw screams to life!" ),
                           _( "You yank the cord, but nothing happens." ) );
}

int iuse::chainsaw_off( player *p, item *it, bool, const tripoint & )
{
    return toolweapon_off( *p, *it,
                           false,
                           rng( 0, 10 ) - it->damage() > 5 && !p->is_underwater(),
                           20, _( "With a roar, the chainsaw leaps to life!" ),
                           _( "You yank the cord, but nothing happens." ) );
}

int iuse::elec_chainsaw_off( player *p, item *it, bool, const tripoint & )
{
    return toolweapon_off( *p, *it,
                           false,
                           rng( 0, 10 ) - it->damage() > 5 && !p->is_underwater(),
                           20, _( "With a roar, the electric chainsaw leaps to life!" ),
                           _( "You flip the switch, but nothing happens." ) );
}

int iuse::cs_lajatang_off( player *p, item *it, bool, const tripoint & )
{
    return toolweapon_off( *p, *it,
                           false,
                           rng( 0, 10 ) - it->damage() > 5 && it->ammo_remaining() > 1 && !p->is_underwater(),
                           40, _( "With a roar, the chainsaws leap to life!" ),
                           _( "You yank the cords, but nothing happens." ) );
}

int iuse::carver_off( player *p, item *it, bool, const tripoint & )
{
    return toolweapon_off( *p, *it,
                           false,
                           true,
                           20, _( "The electric carver's serrated blades start buzzing!" ),
                           _( "You pull the trigger, but nothing happens." ) );
}

int iuse::trimmer_off( player *p, item *it, bool, const tripoint & )
{
    return toolweapon_off( *p, *it,
                           false,
                           rng( 0, 10 ) - it->damage() > 3,
                           15, _( "With a roar, the hedge trimmer leaps to life!" ),
                           _( "You yank the cord, but nothing happens." ) );
}

int toolweapon_on( player &p, item &it, const bool t,
                   const std::string &tname, const bool works_underwater,
                   const int sound_chance, const int volume,
                   const std::string &sound, const bool double_charge_cost = false )
{
    std::string off_type =
        it.typeId().substr( 0, it.typeId().size() - 3 ) +
        // 3 is the length of "_on".
        "_off";
    if( t ) { // Effects while simply on
        if( double_charge_cost && it.ammo_remaining() > 0 ) {
            it.ammo_consume( 1, p.pos() );
        }
        if( !works_underwater && p.is_underwater() ) {
            p.add_msg_if_player( _( "Your %s gurgles in the water and stops." ), tname );
            it.convert( off_type ).active = false;
        } else if( one_in( sound_chance ) ) {
            sounds::ambient_sound( p.pos(), volume, sound );
        }
    } else { // Toggling
        if( it.typeId() == "chainsaw_on" ) {
            sfx::play_variant_sound( "chainsaw_stop", "chainsaw_on", sfx::get_heard_volume( p.pos() ) );
            sfx::fade_audio_channel( 18, 100 );
            sfx::fade_audio_channel( 19, 3000 );
        }
        p.add_msg_if_player( _( "Your %s goes quiet." ), tname );
        it.convert( off_type ).active = false;
    }
    return it.type->charges_to_use();
}

int iuse::combatsaw_on( player *p, item *it, bool t, const tripoint & )
{
    return toolweapon_on( *p, *it, t, _( "combat chainsaw" ),
                          false,
                          12, 18, _( "Your combat chainsaw growls." ) );
}

int iuse::chainsaw_on( player *p, item *it, bool t, const tripoint & )
{
    return toolweapon_on( *p, *it, t, _( "chainsaw" ),
                          false,
                          15, 12, _( "Your chainsaw rumbles." ) );
}

int iuse::elec_chainsaw_on( player *p, item *it, bool t, const tripoint & )
{
    return toolweapon_on( *p, *it, t, _( "electric chainsaw" ),
                          false,
                          15, 12, _( "Your electric chainsaw rumbles." ) );
}

int iuse::cs_lajatang_on( player *p, item *it, bool t, const tripoint & )
{
    return toolweapon_on( *p, *it, t, _( "chainsaw lajatang" ),
                          false,
                          15, 12, _( "Your chainsaws rumble." ),
                          true );
    // The chainsaw lajatang drains 2 charges per turn, since
    // there are two chainsaws.
}

int iuse::carver_on( player *p, item *it, bool t, const tripoint & )
{
    return toolweapon_on( *p, *it, t, _( "electric carver" ),
                          true,
                          10, 8, _( "Your electric carver buzzes." ) );
}

int iuse::trimmer_on( player *p, item *it, bool t, const tripoint & )
{
    return toolweapon_on( *p, *it, t, _( "hedge trimmer" ),
                          true,
                          15, 10, _( "Your hedge trimmer rumbles." ) );
}

int iuse::circsaw_on( player *p, item *it, bool t, const tripoint & )
{
    return toolweapon_on( *p, *it, t, _( "circular saw" ),
                          true,
                          15, 7, _( "Your circular saw buzzes." ) );
}

int iuse::jackhammer( player *p, item *it, bool, const tripoint &pos )
{
    // use has_enough_charges to check for UPS availability
    // p is assumed to exist for iuse cases
    if( !p->has_enough_charges( *it, false ) ) {
        return 0;
    }

    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }

    tripoint dirp = pos;
    if( !choose_adjacent( _( "Drill where?" ), dirp ) ) {
        return 0;
    }

    if( dirp == p->pos() ) {
        p->add_msg_if_player( _( "My god! Let's talk it over OK?" ) );
        p->add_msg_if_player( _( "Don't do anything rash." ) );
        return 0;
    }

    const time_duration duration = 30_minutes;
    const bool mineable = g->m.is_bashable( dirp ) &&
                          ( g->m.has_flag( "SUPPORTS_ROOF", dirp ) || g->m.has_flag( "MINEABLE", dirp ) ) &&
                          !g->m.has_flag( "TREE", dirp );
    const bool not_dirt_or_grass = g->m.move_cost( dirp ) == 2 && g->get_levz() != -1 &&
                                   g->m.ter( dirp ) != t_dirt && g->m.ter( dirp ) != t_grass;

    if( !( mineable || not_dirt_or_grass ) ) {
        p->add_msg_if_player( m_info, _( "You can't drill there." ) );
        return 0;
    }

    p->assign_activity( activity_id( "ACT_JACKHAMMER" ), to_turns<int>( duration ) * 100, -1,
                        p->get_item_position( it ) );
    p->activity.placement = dirp;

    return it->type->charges_to_use();
}

int iuse::pickaxe( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        // Long action
        return 0;
    }

    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }

    int dirx = 0;
    int diry = 0;
    if( !choose_adjacent( _( "Mine where?" ), dirx, diry ) ) {
        return 0;
    }

    if( dirx == p->posx() && diry == p->posy() ) {
        p->add_msg_if_player( _( "Mining the depths of your experience," ) );
        p->add_msg_if_player( _( "you realize that it's best not to dig" ) );
        p->add_msg_if_player( _( "yourself into a hole.  You stop digging." ) );
        return 0;
    }

    int turns;
    if( g->m.is_bashable( dirx, diry ) && g->m.ter( dirx, diry ) != t_tree &&
        ( g->m.has_flag( "SUPPORTS_ROOF", dirx, diry ) || g->m.has_flag( "MINEABLE", dirx, diry ) ) ) {

        /** @EFFECT_STR speeds up mining with a pickaxe */
        turns = ( ( MAX_STAT + 4 ) - std::min( p->str_cur, MAX_STAT ) ) * MINUTES( 5 );
    } else if( g->m.move_cost( dirx, diry ) == 2 && g->get_levz() == 0 &&
               g->m.ter( dirx, diry ) != t_dirt && g->m.ter( dirx, diry ) != t_grass ) {

        turns = MINUTES( 20 );

    } else {
        p->add_msg_if_player( m_info, _( "You can't mine there." ) );
        return 0;
    }
    p->assign_activity( activity_id( "ACT_PICKAXE" ), turns * 100, -1, p->get_item_position( it ) );
    p->activity.placement = tripoint( dirx, diry, p->posz() ); // TODO: Z
    p->add_msg_if_player( _( "You attack the %1$s with your %2$s." ),
                          g->m.tername( dirx, diry ).c_str(), it->tname().c_str() );
    return 0; // handled when the activity finishes
}

int iuse::geiger( player *p, item *it, bool t, const tripoint &pos )
{
    if( t ) { // Every-turn use when it's on
        const int rads = g->m.get_radiation( pos );
        if( rads == 0 ) {
            return it->type->charges_to_use();
        }
        sounds::sound( pos, 6, "" );
        if( !p->can_hear( pos, 6 ) ) {
            // can not hear it, but may have alarmed other creatures
            return it->type->charges_to_use();
        }
        if( rads > 50 ) {
            add_msg( m_warning, _( "The geiger counter buzzes intensely." ) );
        } else if( rads > 35 ) {
            add_msg( m_warning, _( "The geiger counter clicks wildly." ) );
        } else if( rads > 25 ) {
            add_msg( m_warning, _( "The geiger counter clicks rapidly." ) );
        } else if( rads > 15 ) {
            add_msg( m_warning, _( "The geiger counter clicks steadily." ) );
        } else if( rads > 8 ) {
            add_msg( m_warning, _( "The geiger counter clicks slowly." ) );
        } else if( rads > 4 ) {
            add_msg( _( "The geiger counter clicks intermittently." ) );
        } else {
            add_msg( _( "The geiger counter clicks once." ) );
        }
        return it->type->charges_to_use();
    }
    // Otherwise, we're activating the geiger counter
    if( it->typeId() == "geiger_on" ) {
        add_msg( _( "The geiger counter's SCANNING LED turns off." ) );
        it->convert( "geiger_off" ).active = false;
        return 0;
    }

    int ch = menu( true, _( "Geiger counter:" ), _( "Scan yourself" ), _( "Scan the ground" ),
                   _( "Turn continuous scan on" ), _( "Cancel" ), NULL );
    switch( ch ) {
        case 1:
            p->add_msg_if_player( m_info, _( "Your radiation level: %d (%d from items)" ), p->radiation,
                                  p->leak_level( "RADIOACTIVE" ) );
            break;
        case 2:
            p->add_msg_if_player( m_info, _( "The ground's radiation level: %d" ),
                                  g->m.get_radiation( p->pos() ) );
            break;
        case 3:
            p->add_msg_if_player( _( "The geiger counter's scan LED turns on." ) );
            it->convert( "geiger_on" ).active = true;
            break;
        case 4:
            return 0;
    }
    return it->type->charges_to_use();
}

int iuse::teleport( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        // That would be evil
        return 0;
    }

    if( !it->ammo_sufficient() ) {
        return 0;
    }
    p->moves -= 100;
    g->teleport( p );
    return it->type->charges_to_use();
}

int iuse::can_goo( player *p, item *it, bool, const tripoint & )
{
    it->convert( "canister_empty" );
    int tries = 0;
    tripoint goop;
    goop.z = p->posz();
    do {
        goop.x = p->posx() + rng( -2, 2 );
        goop.y = p->posy() + rng( -2, 2 );
        tries++;
    } while( g->m.impassable( goop ) && tries < 10 );
    if( tries == 10 ) {
        return 0;
    }
    if( monster *const mon_ptr = g->critter_at<monster>( goop ) ) {
        monster &critter = *mon_ptr;
        if( g->u.sees( goop ) ) {
            add_msg( _( "Black goo emerges from the canister and envelopes a %s!" ),
                     critter.name().c_str() );
        }
        critter.poly( mon_blob );

        critter.set_speed_base( critter.get_speed_base() - rng( 5, 25 ) );
        critter.set_hp( critter.get_speed() );
    } else {
        if( g->u.sees( goop ) ) {
            add_msg( _( "Living black goo emerges from the canister!" ) );
        }
        if( monster *const goo = g->summon_mon( mon_blob, goop ) ) {
            goo->friendly = -1;
        }
    }
    tries = 0;
    while( !one_in( 4 ) && tries < 10 ) {
        tries = 0;
        do {
            goop.x = p->posx() + rng( -2, 2 );
            goop.y = p->posy() + rng( -2, 2 );
            tries++;
        } while( g->m.impassable( goop ) &&
                 g->m.tr_at( goop ).is_null() && tries < 10 );
        if( tries < 10 ) {
            if( g->u.sees( goop ) ) {
                add_msg( m_warning, _( "A nearby splatter of goo forms into a goo pit." ) );
            }
            g->m.trap_set( goop, tr_goo );
        } else {
            return 0;
        }
    }
    return it->type->charges_to_use();
}

int iuse::throwable_extinguisher_act( player *, item *it, bool, const tripoint &pos )
{
    if( pos.x == -999 || pos.y == -999 ) {
        return 0;
    }
    if( g->m.get_field( pos, fd_fire ) != nullptr ) {
        // Reduce the strength of fire (if any) in the target tile.
        g->m.adjust_field_strength( pos, fd_fire, 0 - 1 );
        // Slightly reduce the strength of fire around and in the target tile.
        for( const tripoint &dest : g->m.points_in_radius( pos, 1 ) ) {
            if( g->m.passable( dest ) && dest != pos ) {
                g->m.adjust_field_strength( dest, fd_fire, 0 - rng( 0, 1 ) );
            }
        }
        return 1;
    }
    it->active = false;
    return 0;
}

int iuse::pipebomb_act( player *, item *it, bool t, const tripoint &pos )
{
    if( pos.x == -999 || pos.y == -999 ) {
        return 0;
    }
    if( t ) { // Simple timer effects
        //~ the sound of a lit fuse
        sounds::sound( pos, 0, _( "ssss..." ) ); // Vol 0 = only heard if you hold it
    } else if( it->charges > 0 ) {
        add_msg( m_info, _( "You've already lit the %s, try throwing it instead." ), it->tname().c_str() );
        return 0;
    } else { // The timer has run down
        if( one_in( 10 ) ) {
            // Fizzled, but we may not have seen it to know that
            if( g->u.sees( pos ) ) {
                add_msg( _( "The pipe bomb fizzles out." ) );
            }
        } else {
            g->explosion( pos, rng( 10, 24 ), 0.6, false, rng( 0, 4 ) );
        }
    }
    return 0;
}

int iuse::granade( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( _( "You pull the pin on the Granade." ) );
    it->convert( "granade_act" );
    it->charges = 5;
    it->active = true;
    return it->type->charges_to_use();
}

int iuse::granade_act( player *, item *it, bool t, const tripoint &pos )
{
    if( pos.x == -999 || pos.y == -999 ) {
        return 0;
    }
    if( t ) { // Simple timer effects
        sounds::sound( pos, 0, _( "Merged!" ) ); // Vol 0 = only heard if you hold it
    } else if( it->charges > 0 ) {
        add_msg( m_info, _( "You've already pulled the %s's pin, try throwing it instead." ),
                 it->tname().c_str() );
        return 0;
    } else { // When that timer runs down...
        int explosion_radius = 3;
        int effect_roll = rng( 1, 5 );
        auto buff_stat = [&]( int &current_stat, int modify_by ) {
            auto modified_stat = current_stat + modify_by;
            current_stat = std::max( current_stat, std::min( 15, modified_stat ) );
        };
        switch( effect_roll ) {
            case 1:
                sounds::sound( pos, 100, _( "BUGFIXES!" ) );
                g->draw_explosion( pos, explosion_radius, c_light_cyan );
                for( const tripoint &dest : g->m.points_in_radius( pos, explosion_radius ) ) {
                    monster *const mon = g->critter_at<monster>( dest, true );
                    if( mon && ( mon->type->in_species( INSECT ) || mon->is_hallucination() ) ) {
                        mon->die_in_explosion( nullptr );
                    }
                }
                break;

            case 2:
                sounds::sound( pos, 100, _( "BUFFS!" ) );
                g->draw_explosion( pos, explosion_radius, c_green );
                for( const tripoint &dest : g->m.points_in_radius( pos, explosion_radius ) ) {
                    if( monster *const mon_ptr = g->critter_at<monster>( dest ) ) {
                        monster &critter = *mon_ptr;
                        critter.set_speed_base(
                            critter.get_speed_base() * rng_float( 1.1, 2.0 ) );
                        critter.set_hp( critter.get_hp() * rng_float( 1.1, 2.0 ) );
                    } else if( npc *const person = g->critter_at<npc>( dest ) ) {
                        /** @EFFECT_STR_MAX increases possible granade str buff for NPCs */
                        buff_stat( person->str_max, rng( 0, person->str_max / 2 ) );
                        /** @EFFECT_DEX_MAX increases possible granade dex buff for NPCs */
                        buff_stat( person->dex_max, rng( 0, person->dex_max / 2 ) );
                        /** @EFFECT_INT_MAX increases possible granade int buff for NPCs */
                        buff_stat( person->int_max, rng( 0, person->int_max / 2 ) );
                        /** @EFFECT_PER_MAX increases possible granade per buff for NPCs */
                        buff_stat( person->per_max, rng( 0, person->per_max / 2 ) );
                    } else if( g->u.pos() == dest ) {
                        /** @EFFECT_STR_MAX increases possible granade str buff */
                        buff_stat( g->u.str_max, rng( 0, g->u.str_max / 2 ) );
                        /** @EFFECT_DEX_MAX increases possible granade dex buff */
                        buff_stat( g->u.dex_max, rng( 0, g->u.dex_max / 2 ) );
                        /** @EFFECT_INT_MAX increases possible granade int buff */
                        buff_stat( g->u.int_max, rng( 0, g->u.int_max / 2 ) );
                        /** @EFFECT_PER_MAX increases possible granade per buff */
                        buff_stat( g->u.per_max, rng( 0, g->u.per_max / 2 ) );
                        g->u.recalc_hp();
                        for( int part = 0; part < num_hp_parts; part++ ) {
                            g->u.hp_cur[part] *= 1 + rng( 0, 20 ) * .1;
                            if( g->u.hp_cur[part] > g->u.hp_max[part] ) {
                                g->u.hp_cur[part] = g->u.hp_max[part];
                            }
                        }
                    }
                }
                break;

            case 3:
                sounds::sound( pos, 100, _( "NERFS!" ) );
                g->draw_explosion( pos, explosion_radius, c_red );
                for( const tripoint &dest : g->m.points_in_radius( pos, explosion_radius ) ) {
                    if( monster *const mon_ptr = g->critter_at<monster>( dest ) ) {
                        monster &critter = *mon_ptr;
                        critter.set_speed_base(
                            rng( 0, critter.get_speed_base() ) );
                        critter.set_hp( rng( 1, critter.get_hp() ) );
                    } else if( npc *const person = g->critter_at<npc>( dest ) ) {
                        /** @EFFECT_STR_MAX increases possible granade str debuff for NPCs (NEGATIVE) */
                        person->str_max -= rng( 0, person->str_max / 2 );
                        /** @EFFECT_DEX_MAX increases possible granade dex debuff for NPCs (NEGATIVE) */
                        person->dex_max -= rng( 0, person->dex_max / 2 );
                        /** @EFFECT_INT_MAX increases possible granade int debuff for NPCs (NEGATIVE) */
                        person->int_max -= rng( 0, person->int_max / 2 );
                        /** @EFFECT_PER_MAX increases possible granade per debuff for NPCs (NEGATIVE) */
                        person->per_max -= rng( 0, person->per_max / 2 );
                    } else if( g->u.pos() == dest ) {
                        /** @EFFECT_STR_MAX increases possible granade str debuff (NEGATIVE) */
                        g->u.str_max -= rng( 0, g->u.str_max / 2 );
                        /** @EFFECT_DEX_MAX increases possible granade dex debuff (NEGATIVE) */
                        g->u.dex_max -= rng( 0, g->u.dex_max / 2 );
                        /** @EFFECT_INT_MAX increases possible granade int debuff (NEGATIVE) */
                        g->u.int_max -= rng( 0, g->u.int_max / 2 );
                        /** @EFFECT_PER_MAX increases possible granade per debuff (NEGATIVE) */
                        g->u.per_max -= rng( 0, g->u.per_max / 2 );
                        g->u.recalc_hp();
                        for( int part = 0; part < num_hp_parts; part++ ) {
                            if( g->u.hp_cur[part] > 0 ) {
                                g->u.hp_cur[part] = rng( 1, g->u.hp_cur[part] );
                            }
                        }
                    }
                }
                break;

            case 4:
                sounds::sound( pos, 100, _( "REVERTS!" ) );
                g->draw_explosion( pos, explosion_radius, c_pink );
                for( const tripoint &dest : g->m.points_in_radius( pos, explosion_radius ) ) {
                    if( monster *const mon_ptr = g->critter_at<monster>( dest ) ) {
                        monster &critter = *mon_ptr;
                        critter.set_speed_base( critter.type->speed );
                        critter.set_hp( critter.get_hp_max() );
                        critter.clear_effects();
                    } else if( npc *const person = g->critter_at<npc>( dest ) ) {
                        person->environmental_revert_effect();
                    } else if( g->u.pos() == dest ) {
                        g->u.environmental_revert_effect();
                        do_purify( g->u );
                    }
                }
                break;
            case 5:
                sounds::sound( pos, 100, _( "BEES!" ) );
                g->draw_explosion( pos, explosion_radius, c_yellow );
                for( const tripoint &dest : g->m.points_in_radius( pos, explosion_radius ) ) {
                    if( one_in( 5 ) && !g->critter_at( dest ) ) {
                        g->m.add_field( dest, fd_bees, rng( 1, 3 ) );
                    }
                }
                break;
        }
    }
    return it->type->charges_to_use();
}

int iuse::c4( player *p, item *it, bool, const tripoint & )
{
    int time;
    bool got_value = query_int( time, _( "Set the timer to (0 to cancel)?" ) );
    if( !got_value || time <= 0 ) {
        p->add_msg_if_player( _( "Never mind." ) );
        return 0;
    }
    p->add_msg_if_player( _( "You set the timer to %d." ), time );
    it->convert( "c4armed" );
    it->charges = time;
    it->active = true;
    return it->type->charges_to_use();
}

int iuse::acidbomb_act( player *p, item *it, bool, const tripoint &pos )
{
    if( !p->has_item( *it ) ) {
        it->charges = -1;
        for( const tripoint &tmp : g->m.points_in_radius( pos.x == -999 ? p->pos() : pos, 1 ) ) {
            g->m.add_field( tmp, fd_acid, 3 );
        }
        return 1;
    }
    return 0;
}

int iuse::grenade_inc_act( player *p, item *it, bool t, const tripoint &pos )
{
    if( pos.x == -999 || pos.y == -999 ) {
        return 0;
    }

    if( t ) { // Simple timer effects
        sounds::sound( pos, 0, _( "Tick!" ) ); // Vol 0 = only heard if you hold it
    } else if( it->charges > 0 ) {
        p->add_msg_if_player( m_info, _( "You've already released the handle, try throwing it instead." ) );
        return 0;
    } else {  // blow up
        int num_flames = rng( 3, 5 );
        for( int current_flame = 0; current_flame < num_flames; current_flame++ ) {
            tripoint dest( pos.x + rng( -5, 5 ), pos.y + rng( -5, 5 ), pos.z );
            std::vector<tripoint> flames = line_to( pos, dest, 0, 0 );
            for( auto &flame : flames ) {
                g->m.add_field( flame, fd_fire, rng( 0, 2 ) );
            }
        }
        g->explosion( pos, 8, 0.8, true );
        for( const tripoint &dest : g->m.points_in_radius( pos, 2 ) ) {
            g->m.add_field( dest, fd_incendiary, 3 );
        }

    }
    return 0;
}

int iuse::arrow_flamable( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    if( !p->use_charges_if_avail( "fire", 1 ) ) {
        p->add_msg_if_player( m_info, _( "You need a source of fire!" ) );
        return 0;
    }
    p->add_msg_if_player( _( "You light the arrow!" ) );
    p->moves -= 150;
    if( it->charges == 1 ) {
        it->convert( "arrow_flamming" );
        return 0;
    }
    item lit_arrow( *it );
    lit_arrow.convert( "arrow_flamming" ).charges = 1;
    p->i_add( lit_arrow );
    return 1;
}

int iuse::molotov_lit( player *p, item *it, bool t, const tripoint &pos )
{
    if( pos.x == -999 || pos.y == -999 ) {
        return 0;
    } else if( it->charges > 0 ) {
        add_msg( m_info, _( "You've already lit the %s, try throwing it instead." ), it->tname().c_str() );
        return 0;
    } else if( p->has_item( *it ) && it->charges == 0 ) {
        it->charges += 1;
        if( one_in( 5 ) ) {
            p->add_msg_if_player( _( "Your lit Molotov goes out." ) );
            it->convert( "molotov" ).active = false;
        }
    } else {
        if( !t ) {
            for( auto &pt : g->m.points_in_radius( pos, 1, 0 ) ) {
                const int density = 1 + one_in( 3 ) + one_in( 5 );
                g->m.add_field( pt, fd_fire, density );
            }
        }
    }
    return 0;
}

int iuse::firecracker_pack( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    if( !p->has_charges( "fire", 1 ) ) {
        p->add_msg_if_player( m_info, _( "You need a source of fire!" ) );
        return 0;
    }
    p->add_msg_if_player( _( "You light the pack of firecrackers." ) );
    it->convert( "firecracker_pack_act" );
    it->charges = 26;
    it->set_age( 0 );
    it->active = true;
    return 0; // don't use any charges at all. it has became a new item
}

int iuse::firecracker_pack_act( player *, item *it, bool, const tripoint &pos )
{
    time_duration timer = it->age();
    if( timer < 2_turns ) {
        sounds::sound( pos, 0, _( "ssss..." ) );
        it->inc_damage();
    } else if( it->charges > 0 ) {
        int ex = rng( 4, 6 );
        int i = 0;
        if( ex > it->charges ) {
            ex = it->charges;
        }
        for( i = 0; i < ex; i++ ) {
            sounds::sound( pos, 20, _( "Bang!" ) );
        }
        it->charges -= ex;
    }
    if( it->charges == 0 ) {
        it->charges = -1;
    }
    return 0;
}

int iuse::firecracker( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    if( !p->use_charges_if_avail( "fire", 1 ) ) {
        p->add_msg_if_player( m_info, _( "You need a source of fire!" ) );
        return 0;
    }
    p->add_msg_if_player( _( "You light the firecracker." ) );
    it->convert( "firecracker_act" );
    it->charges = 2;
    it->active = true;
    return it->type->charges_to_use();
}

int iuse::firecracker_act( player *, item *it, bool t, const tripoint &pos )
{
    if( pos.x == -999 || pos.y == -999 ) {
        return 0;
    }
    if( t ) { // Simple timer effects
        sounds::sound( pos, 0, _( "ssss..." ) );
    } else if( it->charges > 0 ) {
        add_msg( m_info, _( "You've already lit the %s, try throwing it instead." ), it->tname().c_str() );
        return 0;
    } else { // When that timer runs down...
        sounds::sound( pos, 20, _( "Bang!" ) );
    }
    return 0;
}

int iuse::mininuke( player *p, item *it, bool, const tripoint & )
{
    int time;
    bool got_value = query_int( time, _( "Set the timer to (0 to cancel)?" ) );
    if( !got_value || time <= 0 ) {
        p->add_msg_if_player( _( "Never mind." ) );
        return 0;
    }
    p->add_msg_if_player( _( "You set the timer to %d." ), time );
    if( !p->is_npc() ) {
        p->add_memorial_log( pgettext( "memorial_male", "Activated a mininuke." ),
                             pgettext( "memorial_female", "Activated a mininuke." ) );
    }
    it->convert( "mininuke_act" );
    it->charges = time;
    it->active = true;
    return it->type->charges_to_use();
}


int iuse::pheromone( player *p, item *it, bool, const tripoint &pos )
{
    if( !it->ammo_sufficient() ) {
        return 0;
    }
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }

    if( pos.x == -999 || pos.y == -999 ) {
        return 0;
    }

    p->add_msg_player_or_npc( _( "You squeeze the pheromone ball..." ),
                              _( "<npcname> squeezes the pheromone ball..." ) );

    p->moves -= 15;

    int converts = 0;
    for( const tripoint &dest : g->m.points_in_radius( pos, 4 ) ) {
        monster *const mon_ptr = g->critter_at<monster>( dest, true );
        if( !mon_ptr ) {
            continue;
        }
        monster &critter = *mon_ptr;
        if( critter.type->in_species( ZOMBIE ) && critter.friendly == 0 &&
            rng( 0, 500 ) > critter.get_hp() ) {
            converts++;
            critter.make_friendly();
        }
    }

    if( g->u.sees( *p ) ) {
        if( converts == 0 ) {
            add_msg( _( "...but nothing happens." ) );
        } else if( converts == 1 ) {
            add_msg( m_good, _( "...and a nearby zombie turns friendly!" ) );
        } else {
            add_msg( m_good, _( "...and several nearby zombies turn friendly!" ) );
        }
    }
    return it->type->charges_to_use();
}


int iuse::portal( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        return 0;
    }
    tripoint t( p->posx() + rng( -2, 2 ), p->posy() + rng( -2, 2 ), p->posz() );
    g->m.trap_set( t, tr_portal );
    return it->type->charges_to_use();
}

int iuse::tazer( player *p, item *it, bool, const tripoint &pos )
{
    if( !it->ammo_sufficient() ) {
        return 0;
    }

    tripoint dirp = pos;
    if( p->pos() == pos && !choose_adjacent( _( "Shock where?" ), dirp ) ) {
        return 0;
    }

    if( dirp == p->pos() ) {
        p->add_msg_if_player( m_info, _( "Umm.  No." ) );
        return 0;
    }

    Creature *target = g->critter_at( dirp, true );
    if( target == nullptr ) {
        p->add_msg_if_player( _( "There's nothing to zap there!" ) );
        return 0;
    }

    npc *foe = dynamic_cast<npc *>( target );
    if( foe != nullptr &&
        !foe->is_enemy() &&
        !p->query_yn( _( "Really shock %s?" ), target->disp_name().c_str() ) ) {
        return 0;
    }

    /** @EFFECT_DEX slightly increases chance of successfully using tazer */
    /** @EFFECT_MELEE increases chance of successfully using a tazer */
    int numdice = 3 + ( p->dex_cur / 2.5 ) + p->get_skill_level( skill_melee ) * 2;
    p->moves -= 100;

    /** @EFFECT_DODGE increases chance of dodging a tazer attack */
    const bool tazer_was_dodged = dice( numdice, 10 ) < dice( target->get_dodge(), 10 );
    if( tazer_was_dodged ) {
        p->add_msg_player_or_npc( _( "You attempt to shock %s, but miss." ),
                                  _( "<npcname> attempts to shock %s, but misses." ),
                                  target->disp_name().c_str() );
    } else {
        // Maybe-TODO: Execute an attack and maybe zap something other than torso
        // Maybe, because it's torso (heart) that fails when zapped with electricity
        int dam = target->deal_damage( p, bp_torso, damage_instance( DT_ELECTRIC, rng( 5,
                                       25 ) ) ).total_damage();
        if( dam > 0 ) {
            p->add_msg_player_or_npc( m_good,
                                      _( "You shock %s!" ),
                                      _( "<npcname> shocks %s!" ),
                                      target->disp_name().c_str() );
        } else {
            p->add_msg_player_or_npc( m_warning,
                                      _( "You unsuccessfully attempt to shock %s!" ),
                                      _( "<npcname> unsuccessfully attempts to shock %s!" ),
                                      target->disp_name().c_str() );
        }
    }

    if( foe != nullptr ) {
        foe->on_attacked( *p );
    }

    return it->type->charges_to_use();
}

int iuse::tazer2( player *p, item *it, bool b, const tripoint &pos )
{
    if( it->ammo_remaining() >= 100 ) {
        // Instead of having a ctrl+c+v of the function above, spawn a fake tazer and use it
        // Ugly, but less so than copied blocks
        item fake( "tazer", 0 );
        fake.charges = 100;
        return tazer( p, &fake, b, pos );
    } else {
        p->add_msg_if_player( m_info, _( "Insufficient power" ) );
    }

    return 0;
}

int iuse::shocktonfa_off( player *p, item *it, bool t, const tripoint &pos )
{
    int choice = menu( true, _( "tactical tonfa" ), _( "Zap something" ),
                       _( "Turn on light" ), _( "Cancel" ), NULL );

    switch( choice ) {
        case 1: {
            return iuse::tazer2( p, it, t, pos );
        }
        break;

        case 2: {
            if( !it->ammo_sufficient() ) {
                p->add_msg_if_player( m_info, _( "The batteries are dead." ) );
                return 0;
            } else {
                p->add_msg_if_player( _( "You turn the light on." ) );
                it->convert( "shocktonfa_on" ).active = true;
                return it->type->charges_to_use();
            }
        }
    }
    return 0;
}

int iuse::shocktonfa_on( player *p, item *it, bool t, const tripoint &pos )
{
    if( t ) { // Effects while simply on

    } else {
        if( !it->ammo_sufficient() ) {
            p->add_msg_if_player( m_info, _( "Your tactical tonfa is out of power." ) );
            it->convert( "shocktonfa_off" ).active = false;
        } else {
            int choice = menu( true, _( "tactical tonfa" ), _( "Zap something" ),
                               _( "Turn off light" ), _( "cancel" ), NULL );

            switch( choice ) {
                case 1: {
                    return iuse::tazer2( p, it, t, pos );
                }
                break;

                case 2: {
                    p->add_msg_if_player( _( "You turn off the light." ) );
                    it->convert( "shocktonfa_off" ).active = false;
                }
            }
        }
    }
    return 0;
}

int iuse::mp3( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( m_info, _( "The mp3 player's batteries are dead." ) );
    } else if( p->has_active_item( "mp3_on" ) ) {
        p->add_msg_if_player( m_info, _( "You are already listening to an mp3 player!" ) );
    } else {
        p->add_msg_if_player( m_info, _( "You put in the earbuds and start listening to music." ) );
        it->convert( "mp3_on" ).active = true;
    }
    return it->type->charges_to_use();
}

const std::string &get_music_description()
{
    static const std::string no_description;
    static const std::string rare = _( "some bass-heavy post-glam speed polka." );
    static const std::array<std::string, 5> descriptions = {{
            _( "a sweet guitar solo!" ),
            _( "a funky bassline." ),
            _( "some amazing vocals." ),
            _( "some pumping bass." ),
            _( "dramatic classical music." )

        }
    };

    if( one_in( 50 ) ) {
        return rare;
    }

    size_t i = ( size_t )rng( 0, descriptions.size() * 2 );
    if( i < descriptions.size() ) {
        return descriptions[i];
    }

    return no_description;
}

void iuse::play_music( player &p, const tripoint &source, int const volume, int const max_morale )
{
    // TODO: what about other "player", e.g. when a NPC is listening or when the PC is listening,
    // the other characters around should be able to profit as well.
    bool const do_effects = p.can_hear( source, volume );
    std::string sound;
    if( calendar::once_every( 5_minutes ) ) {
        // Every 5 minutes, describe the music
        const std::string &music = get_music_description();
        if( !music.empty() ) {
            sound = music;
            if( p.pos() == source && volume == 0 && p.can_hear( source, volume ) ) {
                // in-ear music, such as mp3 player
                p.add_msg_if_player( _( "You listen to %s" ), music.c_str() );
            }
        }
    }
    // do not process mp3 player
    if( volume != 0 ) {
        sounds::ambient_sound( source, volume, sound );
    }
    if( do_effects ) {
        p.add_effect( effect_music, 1_turns );
        p.add_morale( MORALE_MUSIC, 1, max_morale, 5_turns, 2_turns );
        // mp3 player reduces hearing
        if( volume == 0 ) {
            p.add_effect( effect_earphones, 1_turns );
        }
    }
}

int iuse::mp3_on( player *p, item *it, bool t, const tripoint &pos )
{
    if( t ) { // Normal use
        if( p->has_item( *it ) ) {
            // mp3 player in inventory, we can listen
            play_music( *p, pos, 0, 20 );
        }
    } else { // Turning it off
        p->add_msg_if_player( _( "The mp3 player turns off." ) );
        it->convert( "mp3" ).active = false;
    }
    return it->type->charges_to_use();
}

int iuse::solarpack( player *p, item *it, bool, const tripoint & )
{
    if( !p->has_bionic( bionic_id( "bio_cable" ) ) ) {  // Cable CBM required
        p->add_msg_if_player(
            _( "You have no cable charging system to plug it in, so you leave it alone." ) );
        return 0;
    } else if( !p->has_active_bionic( bionic_id( "bio_cable" ) ) ) {  // when OFF it takes no effect
        p->add_msg_if_player( _( "Activate your cable charging system to take advantage of it." ) );
    }

    if( it->is_armor() && !( p->is_worn( *it ) ) ) {
        p->add_msg_if_player( m_neutral, _( "You need to wear the %1$s before you can unfold it." ),
                              it->tname().c_str() );
        return 0;
    }
    // no doubled sources of power
    if( p->is_wearing( "solarpack_on" ) || p->is_wearing( "q_solarpack_on" ) ) {
        p->add_msg_if_player( m_neutral, _( "You cannot use the %1$s with another of it's kind." ),
                              it->tname().c_str() );
        return 0;
    }
    p->add_msg_if_player( _( "You unfold solar array from the pack and plug it in." ) );

    if( it->typeId() == "solarpack" ) {
        it->convert( "solarpack_on" );
    } else {
        it->convert( "q_solarpack_on" );
    }
    return 0;
}

int iuse::solarpack_off( player *p, item *it, bool, const tripoint & )
{
    if( !p->is_worn( *it ) ) {  // folding when not worn
        p->add_msg_if_player( _( "You fold your portable solar array into the pack." ) );
    } else {
        p->add_msg_if_player( _( "You unplug and fold your portable solar array into the pack." ) );
    }

    if( it->typeId() == "solarpack_on" ) {
        it->convert( "solarpack" );
    } else {
        it->convert( "q_solarpack" );
    }
    return 0;
}

int iuse::gasmask( player *p, item *it, bool t, const tripoint &pos )
{
    if( t ) { // Normal use
        if( p->is_worn( *it ) ) {
            // calculate amount of absorbed gas per filter charge
            const field &gasfield = g->m.field_at( pos );
            for( auto &dfield : gasfield ) {
                const field_entry &entry = dfield.second;
                const field_id fid = entry.getFieldType();
                switch( fid ) {
                    case fd_smoke:
                        it->set_var( "gas_absorbed", it->get_var( "gas_absorbed", 0 ) + 12 );
                        break;
                    case fd_tear_gas:
                    case fd_toxic_gas:
                    case fd_gas_vent:
                    case fd_smoke_vent:
                    case fd_relax_gas:
                    case fd_fungal_haze:
                        it->set_var( "gas_absorbed", it->get_var( "gas_absorbed", 0 ) + 15 );
                        break;
                    default:
                        break;
                }
            }
            if( it->get_var( "gas_absorbed", 0 ) >= 100 ) {
                it->ammo_consume( 1, p->pos() );
                it->set_var( "gas_absorbed", 0 );
            }
            if( it->charges == 0 ) {
                p->add_msg_player_or_npc(
                    m_bad,
                    _( "Your %s requires new filter!" ),
                    _( "<npcname> needs new gas mask filter!" )
                    , it->tname().c_str() );
            }
        }
    } else { // activate
        if( it->charges == 0 ) {
            p->add_msg_if_player( _( "Your %s don't have a filter." ), it->tname().c_str() );
        } else {
            p->add_msg_if_player( _( "You prepared your %s." ), it->tname().c_str() );
            it->active = true;
            it->set_var( "overwrite_env_resist", it->get_env_resist_w_filter() );
        }
    }
    if( it->charges == 0 ) {
        it->set_var( "overwrite_env_resist", 0 );
        it->active = false;
    }
    return it->type->charges_to_use();
}

int iuse::portable_game( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        // Long action
        return 0;
    }

    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    if( p->has_trait( trait_ILLITERATE ) ) {
        add_msg( _( "You're illiterate!" ) );
        return 0;
    } else if( it->ammo_remaining() < 15 ) {
        p->add_msg_if_player( m_info, _( "The %s's batteries are dead." ), it->tname().c_str() );
        return 0;
    } else {
        std::string loaded_software = "robot_finds_kitten";

        uimenu as_m;
        as_m.text = _( "What do you want to play?" );
        as_m.entries.push_back( uimenu_entry( 1, true, '1', _( "Robot finds Kitten" ) ) );
        as_m.entries.push_back( uimenu_entry( 2, true, '2', _( "S N A K E" ) ) );
        as_m.entries.push_back( uimenu_entry( 3, true, '3', _( "Sokoban" ) ) );
        as_m.entries.push_back( uimenu_entry( 4, true, '4', _( "Minesweeper" ) ) );
        as_m.entries.push_back( uimenu_entry( 5, true, '5', _( "Lights on!" ) ) );
        as_m.entries.push_back( uimenu_entry( 6, true, '6', _( "Cancel" ) ) );
        as_m.query();

        switch( as_m.ret ) {
            case 1:
                loaded_software = "robot_finds_kitten";
                break;
            case 2:
                loaded_software = "snake_game";
                break;
            case 3:
                loaded_software = "sokoban_game";
                break;
            case 4:
                loaded_software = "minesweeper_game";
                break;
            case 5:
                loaded_software = "lightson_game";
                break;
            case 6: //Cancel
                return 0;
        }

        //Play in 15-minute chunks
        int time = 15000;

        p->add_msg_if_player( _( "You play on your %s for a while." ), it->tname().c_str() );
        p->assign_activity( activity_id( "ACT_GAME" ), time, -1, p->get_item_position( it ), "gaming" );

        std::map<std::string, std::string> game_data;
        game_data.clear();
        int game_score = 0;

        play_videogame( loaded_software, game_data, game_score );

        if( game_data.find( "end_message" ) != game_data.end() ) {
            p->add_msg_if_player( game_data["end_message"] );
        }

        if( game_score != 0 ) {
            if( game_data.find( "moraletype" ) != game_data.end() ) {
                std::string moraletype = game_data.find( "moraletype" )->second;
                if( moraletype == "MORALE_GAME_FOUND_KITTEN" ) {
                    p->add_morale( MORALE_GAME_FOUND_KITTEN, game_score, 110 );
                } /*else if ( ...*/
            } else {
                p->add_morale( MORALE_GAME, game_score, 110 );
            }
        }

    }
    return it->type->charges_to_use();
}

int iuse::vibe( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        // Long action
        // Also, that would be creepy as fuck, seriously
        return 0;
    }

    if( ( p->is_underwater() ) && ( !( ( p->has_trait( trait_GILLS ) ) ||
                                       ( p->is_wearing( "rebreather_on" ) ) ||
                                       ( p->is_wearing( "rebreather_xl_on" ) ) || ( p->is_wearing( "mask_h20survivor_on" ) ) ) ) ) {
        p->add_msg_if_player( m_info, _( "It's waterproof, but oxygen maybe?" ) );
        return 0;
    }
    if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( m_info, _( "The %s's batteries are dead." ), it->tname().c_str() );
        return 0;
    }
    if( p->get_fatigue() >= DEAD_TIRED ) {
        p->add_msg_if_player( m_info, _( "*Your* batteries are dead." ) );
        return 0;
    } else {
        int time = 20000; // 20 minutes per
        if( it->ammo_remaining() > 0 ) {
            p->add_msg_if_player( _( "You fire up your %s and start getting the tension out." ),
                                  it->tname().c_str() );
        } else {
            p->add_msg_if_player( _( "You whip out your %s and start getting the tension out." ),
                                  it->tname().c_str() );
        }
        p->assign_activity( activity_id( "ACT_VIBE" ), time, -1, p->get_item_position( it ),
                            "de-stressing" );
    }
    return it->type->charges_to_use();
}

int iuse::vortex( player *p, item *it, bool, const tripoint & )
{
    std::vector<tripoint> spawn;
    auto empty_add = [&]( int x, int y ) {
        tripoint pt( x, y, p->posz() );
        if( g->is_empty( pt ) ) {
            spawn.push_back( pt );
        }
    };
    for( int i = -3; i <= 3; i++ ) {
        empty_add( p->posx() - 3, p->posy() + i );
        empty_add( p->posx() + 3, p->posy() + i );
        empty_add( p->posx() + i, p->posy() - 3 );
        empty_add( p->posx() + i, p->posy() + 3 );
    }
    if( spawn.empty() ) {
        p->add_msg_if_player( m_warning, _( "Air swirls around you for a moment." ) );
        return it->convert( "spiral_stone" ).type->charges_to_use();
    }

    p->add_msg_if_player( m_warning, _( "Air swirls all over..." ) );
    p->moves -= 100;
    it->convert( "spiral_stone" );
    monster mvortex( mon_vortex, random_entry( spawn ) );
    mvortex.friendly = -1;
    g->add_zombie( mvortex );
    return it->type->charges_to_use();
}

int iuse::dog_whistle( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    p->add_msg_if_player( _( "You blow your dog whistle." ) );
    for( monster &critter : g->all_monsters() ) {
        if( critter.friendly != 0 && critter.has_flag( MF_DOGFOOD ) ) {
            bool u_see = g->u.sees( critter );
            if( critter.has_effect( effect_docile ) ) {
                if( u_see ) {
                    p->add_msg_if_player( _( "Your %s looks ready to attack." ), critter.name().c_str() );
                }
                critter.remove_effect( effect_docile );
            } else {
                if( u_see ) {
                    p->add_msg_if_player( _( "Your %s goes docile." ), critter.name().c_str() );
                }
                critter.add_effect( effect_docile, 1_turns, num_bp, true );
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::blood_draw( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        return 0;    // No NPCs for now!
    }

    if( !it->contents.empty() ) {
        p->add_msg_if_player( m_info, _( "That %s is full!" ), it->tname().c_str() );
        return 0;
    }

    item blood( "blood", calendar::turn );
    bool drew_blood = false;
    bool acid_blood = false;
    for( auto &map_it : g->m.i_at( p->posx(), p->posy() ) ) {
        if( map_it.is_corpse() &&
            query_yn( _( "Draw blood from %s?" ), map_it.tname().c_str() ) ) {
            p->add_msg_if_player( m_info, _( "You drew blood from the %s..." ), map_it.tname().c_str() );
            drew_blood = true;
            auto bloodtype( map_it.get_mtype()->bloodType() );
            if( bloodtype == fd_acid ) {
                acid_blood = true;
            } else {
                blood.set_mtype( map_it.get_mtype() );
            }
        }
    }

    if( !drew_blood && query_yn( _( "Draw your own blood?" ) ) ) {
        p->add_msg_if_player( m_info, _( "You drew your own blood..." ) );
        drew_blood = true;
        if( p->has_trait( trait_ACIDBLOOD ) ) {
            acid_blood = true;
        }
        p->mod_hunger( 10 );
        p->mod_thirst( 10 );
        p->mod_pain( 3 );
    }

    if( acid_blood ) {
        item acid( "acid", calendar::turn );
        it->put_in( acid );
        if( one_in( 3 ) ) {
            if( it->inc_damage( DT_ACID ) ) {
                p->add_msg_if_player( m_info, _( "...but acidic blood melts the %s, destroying it!" ),
                                      it->tname().c_str() );
                p->i_rem( it );
                return 0;
            }
            p->add_msg_if_player( m_info, _( "...but acidic blood damages the %s!" ), it->tname().c_str() );
        }
        return it->type->charges_to_use();
    }

    if( !drew_blood ) {
        return it->type->charges_to_use();
    }

    it->put_in( blood );
    return it->type->charges_to_use();
}

void iuse::cut_log_into_planks( player &p )
{
    p.moves -= 300;
    p.add_msg_if_player( _( "You cut the log into planks." ) );
    const int max_planks = 10;
    /** @EFFECT_FABRICATION increases number of planks cut from a log */
    int planks = normal_roll( 2 + p.get_skill_level( skill_fabrication ), 1 );
    int wasted_planks = max_planks - planks;
    int scraps = rng( wasted_planks, wasted_planks * 3 );
    planks = std::min( planks, max_planks );
    if( planks > 0 ) {
        item plank( "2x4", calendar::turn );
        p.i_add_or_drop( plank, planks );
        p.add_msg_if_player( m_good, _( "You produce %d planks." ), planks );
    }
    if( scraps > 0 ) {
        item scrap( "splinter", calendar::turn );
        p.i_add_or_drop( scrap, scraps );
        p.add_msg_if_player( m_good, _( "You produce %d splinters." ), scraps );
    }
    if( planks < max_planks / 2 ) {
        add_msg( m_bad, _( "You waste a lot of the wood." ) );
    }
}

int iuse::lumber( player *p, item *it, bool t, const tripoint & )
{
    if( t ) {
        return 0;
    }

    // Check if player is standing on any lumber
    for( auto &i : g->m.i_at( p->pos() ) ) {
        if( i.typeId() == "log" ) {
            g->m.i_rem( p->pos(), &i );
            cut_log_into_planks( *p );
            return it->type->charges_to_use();
        }
    }

    // If the player is not standing on a log, check inventory
    int pos = g->inv_for_id( itype_id( "log" ), _( "Cut up what?" ) );

    item &cut = p->i_at( pos );

    if( cut.is_null() ) {
        add_msg( m_info, _( "You do not have that item!" ) );
        return 0;
    }
    p->i_rem( &cut );
    cut_log_into_planks( *p );
    return it->type->charges_to_use();
}

static int chop_moves( player *p, item *it )
{
    // quality of tool
    const int quality = it->get_quality( AXE );

    // attribute; regular tools - based on STR, powered tools - based on DEX
    const int attr = it->has_flag( "POWERED" ) ? p->dex_cur : p->str_cur;

    const int moves = MINUTES( 60 - attr ) / std::pow( 2, quality - 1 ) * 100;

    return moves;
}

int iuse::chop_tree( player *p, item *it, bool t, const tripoint &pos )
{
    if( !p || t ) {
        return 0;
    }

    tripoint dirp = pos;
    if( !choose_adjacent( _( "Chop down which tree?" ), dirp ) ) {
        return 0;
    }

    if( dirp == p->pos() ) {
        add_msg( m_info, _( "You're not stern enough to shave yourself with THIS." ) );
        return 0;
    }

    int moves;

    if( g->m.has_flag( "TREE", dirp ) ) {
        moves = chop_moves( p, it );
    } else {
        add_msg( m_info, _( "You can't chop down that." ) );
        return 0;
    }

    p->assign_activity( activity_id( "ACT_CHOP_TREE" ), moves, -1, p->get_item_position( it ) );
    p->activity.placement = dirp;

    return it->type->charges_to_use();
}

int iuse::chop_logs( player *p, item *it, bool t, const tripoint &pos )
{
    if( !p || t ) {
        return 0;
    }

    tripoint dirp = pos;
    if( !choose_adjacent( _( "Chop which tree trunk?" ), dirp ) ) {
        return 0;
    }

    int moves;

    const ter_id ter = g->m.ter( dirp );
    if( ter == t_trunk ) {
        moves = chop_moves( p, it );
    } else {
        add_msg( m_info, _( "You can't chop that." ) );
        return 0;
    }

    p->assign_activity( activity_id( "ACT_CHOP_LOGS" ), moves, -1, p->get_item_position( it ) );
    p->activity.placement = dirp;

    return it->type->charges_to_use();
}

int iuse::oxytorch( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        // Long action
        return 0;
    }

    static const quality_id GLARE( "GLARE" );
    if( !p->has_quality( GLARE, 2 ) ) {
        add_msg( m_info, _( "You need welding goggles to do that." ) );
        return 0;
    }

    tripoint dirp = p->pos();
    if( !choose_adjacent( _( "Cut up metal where?" ), dirp ) ) {
        return 0;
    }

    if( dirp == p->pos() ) {
        add_msg( m_info, _( "Yuck.  Acetylene gas smells weird." ) );
        return 0;
    }

    const ter_id ter = g->m.ter( dirp );
    const auto furn = g->m.furn( dirp );
    int moves;

    if( furn == f_rack || ter == t_chainfence_posts ) {
        moves = 200;
    } else if( ter == t_window_enhanced || ter == t_window_enhanced_noglass ) {
        moves = 500;
    } else if( ter == t_chainfence || ter == t_chaingate_c ||
               ter == t_chaingate_l  || ter == t_bars || ter == t_window_bars_alarm ||
               ter == t_window_bars || ter == t_reb_cage ) {
        moves = 1000;
    } else if( ter == t_door_metal_locked || ter == t_door_metal_c || ter == t_door_bar_c ||
               ter == t_door_bar_locked || ter == t_door_metal_pickable ) {
        moves = 1500;
    } else {
        add_msg( m_info, _( "You can't cut that." ) );
        return 0;
    }

    const int charges = moves / 100 * it->ammo_required();

    if( charges > it->ammo_remaining() ) {
        add_msg( m_info, _( "Your torch doesn't have enough acetylene to cut that." ) );
        return 0;
    }

    // placing ter here makes resuming tasks work better
    p->assign_activity( activity_id( "ACT_OXYTORCH" ), moves, ( int )ter, p->get_item_position( it ) );
    p->activity.placement = dirp;
    p->activity.values.push_back( charges );

    // charges will be consumed in oxytorch_do_turn, not here
    return 0;
}

int iuse::hacksaw( player *p, item *it, bool t, const tripoint &pos )
{
    if( !p || t ) {
        return 0;
    }

    tripoint dirp = pos;
    if( !choose_adjacent( _( "Cut up metal where?" ), dirp ) ) {
        return 0;
    }

    if( dirp == p->pos() ) {
        add_msg( m_info, _( "Why would you do that?" ) );
        add_msg( m_info, _( "You're not even chained to a boiler." ) );
        return 0;
    }

    const ter_id ter = g->m.ter( dirp );
    int moves;

    if( ter == t_chainfence_posts || g->m.furn( dirp ) == f_rack ) {
        moves = 10000;
    } else if( ter == t_window_enhanced || ter == t_window_enhanced_noglass ) {
        moves = 30000;
    } else if( ter == t_chainfence || ter == t_chaingate_c ||
               ter == t_chaingate_l || ter == t_window_bars_alarm || ter == t_window_bars || ter == t_reb_cage ) {
        moves = 60000;
    } else if( ter == t_door_bar_c || ter == t_door_bar_locked || ter == t_bars ) {
        moves = 90000;
    } else {
        add_msg( m_info, _( "You can't cut that." ) );
        return 0;
    }

    p->assign_activity( activity_id( "ACT_HACKSAW" ), moves, ( int )ter, p->get_item_position( it ) );
    p->activity.placement = dirp;

    return it->type->charges_to_use();
}

int iuse::portable_structure( player *p, item *it, bool, const tripoint & )
{
    int radius = it->typeId() == "large_tent_kit" ? 2 : 1;
    furn_id floor =
        it->typeId() == "tent_kit"       ? f_groundsheet
        : it->typeId() == "large_tent_kit" ? f_large_groundsheet
        :                                    f_skin_groundsheet;
    furn_id wall =
        it->typeId() == "tent_kit"       ? f_canvas_wall
        : it->typeId() == "large_tent_kit" ? f_large_canvas_wall
        :                                    f_skin_wall;
    furn_id door =
        it->typeId() == "tent_kit"       ? f_canvas_door
        : it->typeId() == "large_tent_kit" ? f_large_canvas_door
        :                                    f_skin_door;
    furn_id center_floor =
        it->typeId() == "large_tent_kit" ? f_center_groundsheet
        : floor;

    int diam = 2 * radius + 1;

    int dirx = 0;
    int diry = 0;
    if( !choose_adjacent(
            string_format( _( "Put up the %s where (%dx%d clear area)?" ),
                           it->tname().c_str(),
                           diam, diam ),
            dirx, diry ) ) {
        return 0;
    }

    // We place the center of the structure (radius + 1)
    // spaces away from the player.
    // First check there's enough room.
    const tripoint center( radius * ( dirx - p->posx() ) + dirx, radius * ( diry - p->posy() ) + diry,
                           p->posz() );
    for( const tripoint &dest : g->m.points_in_radius( center, radius ) ) {
        if( !g->m.has_flag( "FLAT", dest ) ||
            g->m.veh_at( dest ) ||
            !g->is_empty( dest ) ||
            g->critter_at( dest ) != nullptr ||
            g->m.has_furn( dest ) ) {
            add_msg( m_info, _( "There isn't enough space in that direction." ) );
            return 0;
        }
    }
    // Make a square of floor surrounded by wall.
    for( const tripoint &dest : g->m.points_in_radius( center, radius ) ) {
        g->m.furn_set( dest, wall );
    }
    for( const tripoint &dest : g->m.points_in_radius( center, radius - 1 ) ) {
        g->m.furn_set( dest, floor );
    }
    // Place the center floor and the door.
    g->m.furn_set( center, center_floor );
    g->m.furn_set( center.x - radius * ( dirx - p->posx() ), center.y - radius * ( diry - p->posy() ),
                   door );
    add_msg( m_info, _( "You set up the %s on the ground." ), it->tname().c_str() );
    add_msg( m_info, _( "Examine the center square to pack it up again." ) );
    return 1;
}


int iuse::torch_lit( player *p, item *it, bool t, const tripoint &pos )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( _( "The torch is extinguished." ) );
        it->convert( "torch" ).active = false;
        return 0;
    }
    if( t ) {
        if( !it->ammo_sufficient() ) {
            p->add_msg_if_player( _( "The torch burns out." ) );
            it->convert( "torch_done" ).active = false;
        }
    } else if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( _( "The %s winks out." ), it->tname().c_str() );
    } else { // Turning it off
        int choice = menu( true, _( "torch (lit)" ), _( "extinguish" ),
                           _( "light something" ), _( "cancel" ), NULL );
        switch( choice ) {
            case 1: {
                p->add_msg_if_player( _( "The torch is extinguished." ) );
                if( it->charges <= 1 ) {
                    it->charges = 0;
                    it->convert( "torch_done" ).active = false;
                } else {
                    it->charges -= 1;
                    it->convert( "torch" ).active = false;
                }
                return 0;
            }
            break;
            case 2: {
                tripoint temp = pos;
                if( firestarter_actor::prep_firestarter_use( *p, temp ) ) {
                    p->moves -= 5;
                    firestarter_actor::resolve_firestarter_use( *p, temp );
                    return it->type->charges_to_use();
                }
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::battletorch_lit( player *p, item *it, bool t, const tripoint &pos )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( _( "The Louisville Slaughterer is extinguished." ) );
        it->convert( "bat" ).active = false;
        return 0;
    }
    if( t ) {
        if( !it->ammo_sufficient() ) {
            p->add_msg_if_player( _( "The Louisville Slaughterer burns out." ) );
            it->convert( "battletorch_done" ).active = false;
        }
    } else if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( _( "The %s winks out" ), it->tname().c_str() );
    } else { // Turning it off
        int choice = menu( true, _( "Louisville Slaughterer (lit)" ), _( "extinguish" ),
                           _( "light something" ), _( "cancel" ), NULL );
        switch( choice ) {
            case 1: {
                p->add_msg_if_player( _( "The Louisville Slaughterer is extinguished." ) );
                if( it->charges <= 1 ) {
                    it->charges = 0;
                    it->convert( "battletorch_done" ).active = false;
                } else {
                    it->charges -= 1;
                    it->convert( "battletorch" ).active = false;
                }
                return 0;
            }
            break;
            case 2: {
                tripoint temp = pos;
                if( firestarter_actor::prep_firestarter_use( *p, temp ) ) {
                    p->moves -= 5;
                    firestarter_actor::resolve_firestarter_use( *p, temp );
                    return it->type->charges_to_use();
                }
            }
        }
    }
    return it->type->charges_to_use();
}

int iuse::boltcutters( player *p, item *it, bool, const tripoint &pos )
{
    tripoint dirp = pos;
    if( !choose_adjacent( _( "Cut up metal where?" ), dirp ) ) {
        return 0;
    }
    int &dirx = dirp.x;
    int &diry = dirp.y;

    if( dirx == p->posx() && diry == p->posy() ) {
        p->add_msg_if_player(
            _( "You neatly sever all of the veins and arteries in your body.  Oh wait, Never mind." ) );
        return 0;
    }
    if( g->m.ter( dirx, diry ) == t_chaingate_l ) {
        p->moves -= 100;
        g->m.ter_set( dirx, diry, t_chaingate_c );
        sounds::sound( dirp, 5, _( "Gachunk!" ) );
        g->m.spawn_item( p->posx(), p->posy(), "scrap", 3 );
    } else if( g->m.ter( dirx, diry ) == t_chainfence ) {
        p->moves -= 500;
        g->m.ter_set( dirx, diry, t_chainfence_posts );
        sounds::sound( dirp, 5, _( "Snick, snick, gachunk!" ) );
        g->m.spawn_item( dirx, diry, "wire", 20 );
    } else {
        add_msg( m_info, _( "You can't cut that." ) );
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::mop( player *p, item *it, bool, const tripoint & )
{
    int dirx = 0;
    int diry = 0;
    if( !choose_adjacent( _( "Mop where?" ), dirx, diry ) ) {
        return 0;
    }

    tripoint dirp( dirx, diry, p->posz() );
    if( dirx == p->posx() && diry == p->posy() ) {
        p->add_msg_if_player( _( "You mop yourself up." ) );
        p->add_msg_if_player( _( "The universe implodes and reforms around you." ) );
        return 0;
    }
    if( p->is_blind() ) {
        add_msg( _( "You move the mop around, unsure whether it's doing any good." ) );
        p->moves -= 15;
        if( one_in( 3 ) ) {
            g->m.mop_spills( dirp );
        }
    } else if( g->m.mop_spills( dirp ) ) {
        add_msg( _( "You mop up the spill." ) );
        p->moves -= 15;
    } else {
        p->add_msg_if_player( m_info, _( "There's nothing to mop there." ) );
        return 0;
    }
    return it->type->charges_to_use();
}

int iuse::artifact( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ) {
        // TODO: Allow this for trusting NPCs
        return 0;
    }

    if( !it->is_artifact() ) {
        debugmsg( "iuse::artifact called on a non-artifact item! %s",
                  it->tname().c_str() );
        return 0;
    } else if( !it->is_tool() ) {
        debugmsg( "iuse::artifact called on a non-tool artifact! %s",
                  it->tname().c_str() );
        return 0;
    }
    if( !p->is_npc() ) {
        //~ %s is artifact name
        p->add_memorial_log( pgettext( "memorial_male", "Activated the %s." ),
                             pgettext( "memorial_female", "Activated the %s." ),
                             it->tname( 1, false ).c_str() );
    }

    const auto &art = it->type->artifact;
    size_t num_used = rng( 1, art->effects_activated.size() );
    if( num_used < art->effects_activated.size() ) {
        num_used += rng( 1, art->effects_activated.size() - num_used );
    }

    std::vector<art_effect_active> effects = art->effects_activated;
    for( size_t i = 0; i < num_used && !effects.empty(); i++ ) {
        const art_effect_active used = random_entry_removed( effects );

        switch( used ) {
            case AEA_STORM: {
                sounds::sound( p->pos(), 10, _( "Ka-BOOM!" ) );
                int num_bolts = rng( 2, 4 );
                for( int j = 0; j < num_bolts; j++ ) {
                    int xdir = 0;
                    int ydir = 0;
                    while( xdir == 0 && ydir == 0 ) {
                        xdir = rng( -1, 1 );
                        ydir = rng( -1, 1 );
                    }
                    int dist = rng( 4, 12 );
                    int boltx = p->posx(), bolty = p->posy();
                    for( int n = 0; n < dist; n++ ) {
                        boltx += xdir;
                        bolty += ydir;
                        g->m.add_field( {boltx, bolty, p->posz()}, fd_electricity, rng( 2, 3 ) );
                        if( one_in( 4 ) ) {
                            if( xdir == 0 ) {
                                xdir = rng( 0, 1 ) * 2 - 1;
                            } else {
                                xdir = 0;
                            }
                        }
                        if( one_in( 4 ) ) {
                            if( ydir == 0 ) {
                                ydir = rng( 0, 1 ) * 2 - 1;
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
                    g->explosion( fireball, 24, 0.5, true );
                }
            }
            break;

            case AEA_ADRENALINE:
                p->add_msg_if_player( m_good, _( "You're filled with a roaring energy!" ) );
                p->add_effect( effect_adrenaline, rng( 20_minutes, 25_minutes ) );
                break;

            case AEA_MAP: {
                const tripoint center = p->global_omt_location();
                const bool new_map = overmap_buffer.reveal(
                                         point( center.x, center.y ), 20, center.z );
                if( new_map ) {
                    p->add_msg_if_player( m_warning, _( "You have a vision of the surrounding area..." ) );
                    p->moves -= 100;
                }
            }
            break;

            case AEA_BLOOD: {
                bool blood = false;
                for( const tripoint &tmp : g->m.points_in_radius( p->pos(), 4 ) ) {
                    if( !one_in( 4 ) && g->m.add_field( tmp, fd_blood, 3 ) &&
                        ( blood || g->u.sees( tmp ) ) ) {
                        blood = true;
                    }
                }
                if( blood ) {
                    p->add_msg_if_player( m_warning, _( "Blood soaks out of the ground and walls." ) );
                }
            }
            break;

            case AEA_FATIGUE: {
                p->add_msg_if_player( m_warning, _( "The fabric of space seems to decay." ) );
                int x = rng( p->posx() - 3, p->posx() + 3 ), y = rng( p->posy() - 3, p->posy() + 3 );
                g->m.add_field( {x, y, p->posz()}, fd_fatigue, rng( 1, 2 ) );
            }
            break;

            case AEA_ACIDBALL: {
                tripoint acidball = g->look_around();
                if( acidball != tripoint_min ) {
                    for( const tripoint &tmp : g->m.points_in_radius( acidball, 1 ) ) {
                        g->m.add_field( tmp, fd_acid, rng( 2, 3 ) );
                    }
                }
            }
            break;

            case AEA_PULSE:
                sounds::sound( p->pos(), 30, _( "The earth shakes!" ) );
                for( const tripoint &pt : g->m.points_in_radius( p->pos(), 2 ) ) {
                    g->m.bash( pt, 40 );
                    g->m.bash( pt, 40 );  // Multibash effect, so that doors &c will fall
                    g->m.bash( pt, 40 );
                    if( g->m.is_bashable( pt ) && rng( 1, 10 ) >= 3 ) {
                        g->m.bash( pt, 999, false, true );
                    }
                }
                break;

            case AEA_HEAL:
                p->add_msg_if_player( m_good, _( "You feel healed." ) );
                p->healall( 2 );
                break;

            case AEA_CONFUSED:
                for( const tripoint &dest : g->m.points_in_radius( p->pos(), 8 ) ) {
                    if( monster *const mon = g->critter_at<monster>( dest, true ) ) {
                        mon->add_effect( effect_stunned, rng( 5_turns, 15_turns ) );
                    }
                }
                break;

            case AEA_ENTRANCE:
                for( const tripoint &dest : g->m.points_in_radius( p->pos(), 8 ) ) {
                    monster *const mon = g->critter_at<monster>( dest, true );
                    if( mon && mon->friendly == 0 && rng( 0, 600 ) > mon->get_hp() ) {
                        mon->make_friendly();
                    }
                }
                break;

            case AEA_BUGS: {
                int roll = rng( 1, 10 );
                mtype_id bug = mtype_id::NULL_ID();
                int num = 0;
                std::vector<tripoint> empty;
                for( const tripoint &dest : g->m.points_in_radius( p->pos(), 1 ) ) {
                    if( g->is_empty( dest ) ) {
                        empty.push_back( dest );
                    }
                }
                if( empty.empty() || roll <= 4 ) {
                    p->add_msg_if_player( m_warning, _( "Flies buzz around you." ) );
                } else if( roll <= 7 ) {
                    p->add_msg_if_player( m_warning, _( "Giant flies appear!" ) );
                    bug = mon_fly;
                    num = rng( 2, 4 );
                } else if( roll <= 9 ) {
                    p->add_msg_if_player( m_warning, _( "Giant bees appear!" ) );
                    bug = mon_bee;
                    num = rng( 1, 3 );
                } else {
                    p->add_msg_if_player( m_warning, _( "Giant wasps appear!" ) );
                    bug = mon_wasp;
                    num = rng( 1, 2 );
                }
                if( bug ) {
                    for( int j = 0; j < num && !empty.empty(); j++ ) {
                        const tripoint spawnp = random_entry_removed( empty );
                        if( monster *const b = g->summon_mon( bug, spawnp ) ) {
                            b->friendly = -1;
                        }
                    }
                }
            }
            break;

            case AEA_TELEPORT:
                g->teleport( p );
                break;

            case AEA_LIGHT:
                p->add_msg_if_player( _( "The %s glows brightly!" ), it->tname().c_str() );
                g->events.add( EVENT_ARTIFACT_LIGHT, calendar::turn + 3_minutes );
                break;

            case AEA_GROWTH: {
                monster tmptriffid( mtype_id::NULL_ID(), p->pos() );
                mattack::growplants( &tmptriffid );
            }
            break;

            case AEA_HURTALL:
                for( monster &critter : g->all_monsters() ) {
                    critter.apply_damage( nullptr, bp_torso, rng( 0, 5 ) );
                }
                break;

            case AEA_RADIATION:
                add_msg( m_warning, _( "Horrible gases are emitted!" ) );
                for( const tripoint &dest : g->m.points_in_radius( p->pos(), 1 ) ) {
                    g->m.add_field( dest, fd_nuke_gas, rng( 2, 3 ) );
                }
                break;

            case AEA_PAIN:
                p->add_msg_if_player( m_bad, _( "You're wracked with pain!" ) );
                // OK, the Lovecraftian thingamajig can bring Deadened
                // masochists & Cenobites the stimulation they've been
                // craving ;)
                p->mod_pain_noresist( rng( 5, 15 ) );
                break;

            case AEA_MUTATE:
                if( !one_in( 3 ) ) {
                    p->mutate();
                }
                break;

            case AEA_PARALYZE:
                p->add_msg_if_player( m_bad, _( "You're paralyzed!" ) );
                p->moves -= rng( 50, 200 );
                break;

            case AEA_FIRESTORM: {
                p->add_msg_if_player( m_bad, _( "Fire rains down around you!" ) );
                std::vector<tripoint> ps = closest_tripoints_first( 3, p->pos() );
                for( auto p_it : ps ) {
                    if( !one_in( 3 ) ) {
                        g->m.add_field( p_it, fd_fire, 1 + rng( 0, 1 ) * rng( 0, 1 ), 3_minutes );
                    }
                }
                break;
            }

            case AEA_ATTENTION:
                p->add_msg_if_player( m_warning, _( "You feel like your action has attracted attention." ) );
                p->add_effect( effect_attention, rng( 1_hours, 3_hours ) );
                break;

            case AEA_TELEGLOW:
                p->add_msg_if_player( m_warning, _( "You feel unhinged." ) );
                p->add_effect( effect_teleglow, rng( 30_minutes, 120_minutes ) );
                break;

            case AEA_NOISE:
                p->add_msg_if_player( m_bad, _( "Your %s emits a deafening boom!" ), it->tname().c_str() );
                sounds::sound( p->pos(), 100, "" );
                break;

            case AEA_SCREAM:
                sounds::sound( p->pos(), 40, "" );
                if( !p->is_deaf() ) {
                    p->add_msg_if_player( m_warning, _( "Your %s screams disturbingly." ), it->tname().c_str() );
                    p->add_morale( MORALE_SCREAM, -10, 0, 30_minutes, 5_turns );
                }
                break;

            case AEA_DIM:
                p->add_msg_if_player( _( "The sky starts to dim." ) );
                g->events.add( EVENT_DIM, calendar::turn + 5_minutes );
                break;

            case AEA_FLASH:
                p->add_msg_if_player( _( "The %s flashes brightly!" ), it->tname().c_str() );
                g->flashbang( p->pos() );
                break;

            case AEA_VOMIT:
                p->add_msg_if_player( m_bad, _( "A wave of nausea passes through you!" ) );
                p->vomit();
                break;

            case AEA_SHADOWS: {
                int num_shadows = rng( 4, 8 );
                int num_spawned = 0;
                for( int j = 0; j < num_shadows; j++ ) {
                    int tries = 0;
                    tripoint monp = p->pos();
                    do {
                        if( one_in( 2 ) ) {
                            monp.x = rng( p->posx() - 5, p->posx() + 5 );
                            monp.y = ( one_in( 2 ) ? p->posy() - 5 : p->posy() + 5 );
                        } else {
                            monp.x = ( one_in( 2 ) ? p->posx() - 5 : p->posx() + 5 );
                            monp.y = rng( p->posy() - 5, p->posy() + 5 );
                        }
                    } while( tries < 5 && !g->is_empty( monp ) &&
                             !g->m.sees( monp, p->pos(), 10 ) );
                    if( tries < 5 ) { // @todo: tries increment is missing, so this expression is always true
                        if( monster *const  spawned = g->summon_mon( mon_shadow, monp ) ) {
                            num_spawned++;
                            spawned->reset_special_rng( "DISAPPEAR" );
                        }
                    }
                }
                if( num_spawned > 1 ) {
                    p->add_msg_if_player( m_warning, _( "Shadows form around you." ) );
                } else if( num_spawned == 1 ) {
                    p->add_msg_if_player( m_warning, _( "A shadow forms nearby." ) );
                }
            }
            break;

            case AEA_STAMINA_EMPTY:
                p->add_msg_if_player( m_bad, _( "Your body feels like jelly." ) );
                p->stamina = p->stamina * 1 / ( rng( 3, 8 ) );
                break;

            case AEA_FUN:
                p->add_msg_if_player( m_good, _( "You're filled with euphoria!" ) );
                p->add_morale( MORALE_FEELING_GOOD, rng( 20, 50 ), 0, 5_minutes, 5_turns, false );
                break;

            case AEA_SPLIT: // TODO
                break;

            case AEA_NULL: // BUG
            case NUM_AEAS:
            default:
                debugmsg( "iuse::artifact(): wrong artifact type (%d)", used );
                break;
        }
    }
    return it->type->charges_to_use();
}

int iuse::spray_can( player *p, item *it, bool, const tripoint & )
{
    bool ismarker = ( it->typeId() == "permanent_marker" || it->typeId() == "survival_marker" );
    if( ismarker ) {
        int ret = menu( true, _( "Write on what?" ), _( "The ground" ), _( "An item" ), _( "Cancel" ),
                        NULL );

        if( ret == 2 ) {
            // inscribe_item returns false if the action fails or is canceled somehow.
            bool canceled_inscription = !inscribe_item( *p, _( "Write" ), _( "Written" ), false );
            if( canceled_inscription ) {
                return 0;
            }
            return it->type->charges_to_use();
        } else if( ret != 1 ) { // User chose cancel or some other undefined key.
            return 0;
        }
    }

    return handle_ground_graffiti( *p, it, ismarker ? _( "Write what?" ) : _( "Spray what?" ) );
}

int iuse::handle_ground_graffiti( player &p, item *it, const std::string prefix )
{
    std::string message = string_input_popup()
                          .title( prefix + " " + _( "(To delete, input one '.')" ) )
                          .identifier( "graffiti" )
                          .query_string();

    if( message.empty() ) {
        return 0;
    } else {
        const auto where = p.pos();
        int move_cost;
        if( message == "." ) {
            if( g->m.has_graffiti_at( where ) ) {
                move_cost = 3 * g->m.graffiti_at( where ).length();
                g->m.delete_graffiti( where );
                add_msg( _( "You manage to get rid of the message on the ground." ) );
            } else {
                add_msg( _( "There isn't anything to erase here." ) );
                return 0;
            }
        } else {
            g->m.set_graffiti( where, message );
            add_msg( _( "You write a message on the ground." ) );
            move_cost = 2 * message.length();
        }
        p.moves -= move_cost;
    }

    return it->type->charges_to_use();
}

/**
 * Heats up a food item.
 * @return 1 if an item was heated, false if nothing was heated.
 */
static bool heat_item( player &p )
{
    auto loc = g->inv_map_splice( []( const item & itm ) {
        return ( itm.is_food() && itm.has_flag( "EATEN_HOT" ) ) ||
               ( itm.is_food_container() && itm.contents.front().has_flag( "EATEN_HOT" ) );
    }, _( "Heat up what?" ), 1, _( "You don't have appropriate food to heat up." ) );

    item *heat = loc.get_item();
    if( heat == nullptr ) {
        add_msg( m_info, _( "Never mind." ) );
        return false;
    }
    item &target = heat->is_food_container() ? heat->contents.front() : *heat;
    p.mod_moves( -300 );
    add_msg( _( "You heat up the food." ) );
    target.item_tags.insert( "HOT" );
    target.active = true;
    target.item_counter = 600; // sets the hot food flag for 60 minutes
    return true;
}

int iuse::heatpack( player *p, item *it, bool, const tripoint & )
{
    if( heat_item( *p ) ) {
        it->convert( "heatpack_used" );
    }
    return 0;
}

int iuse::heat_food( player *p, item *it, bool, const tripoint & )
{
    if( g->m.has_nearby_fire( p->pos() ) ) {
        heat_item( *p );
    } else {
        p->add_msg_if_player( m_info, _( "You need to be next to fire to heat something up with the %s." ),
                              it->tname().c_str() );
    }
    return 0;
}

int iuse::hotplate( player *p, item *it, bool, const tripoint & )
{
    if( it->typeId() != "atomic_coffeepot" && ( !it->ammo_sufficient() ) ) {
        p->add_msg_if_player( m_info, _( "The %s's batteries are dead." ), it->tname().c_str() );
        return 0;
    }

    int choice = 1;
    if( ( p->has_effect( effect_bite ) || p->has_effect( effect_bleed ) ||
          p->has_trait( trait_MASOCHIST ) ||
          p->has_trait( trait_MASOCHIST_MED ) || p->has_trait( trait_CENOBITE ) ) && !p->is_underwater() ) {
        //Might want to cauterize
        choice = menu( true, _( "Using hotplate:" ), _( "Heat food" ), _( "Cauterize wound" ),
                       _( "Cancel" ), NULL );
    }

    if( choice == 1 ) {
        if( heat_item( *p ) ) {
            return it->type->charges_to_use();
        }
    } else if( choice == 2 ) {
        return cauterize_elec( *p, *it );
    }
    return 0;
}

int iuse::towel( player *p, item *it, bool t, const tripoint & )
{
    if( t ) {
        // Continuous usage, do nothing as not initiated by the player, this is for
        // wet towels only as they are active items.
        return 0;
    }
    bool slime = p->has_effect( effect_slimed );
    bool boom = p->has_effect( effect_boomered );
    bool glow = p->has_effect( effect_glowing );
    int mult = slime + boom + glow; // cleaning off more than one at once makes it take longer
    bool towelUsed = false;

    // can't use an already wet towel!
    if( it->has_flag( "WET" ) ) {
        p->add_msg_if_player( m_info, _( "That %s is too wet to soak up any more liquid!" ),
                              it->tname().c_str() );


        // clean off the messes first, more important
    } else if( slime || boom || glow ) {
        p->remove_effect( effect_slimed ); // able to clean off all at once
        p->remove_effect( effect_boomered );
        p->remove_effect( effect_glowing );
        p->add_msg_if_player( _( "You use the %s to clean yourself off, saturating it with slime!" ),
                              it->tname().c_str() );

        towelUsed = true;
        if( it->typeId() == "towel" ) {
            it->convert( "towel_soiled" );
        }

        // dry off from being wet
    } else if( abs( p->has_morale( MORALE_WET ) ) ) {
        p->rem_morale( MORALE_WET );
        p->body_wetness.fill( 0 );
        p->add_msg_if_player( _( "You use the %s to dry off, saturating it with water!" ),
                              it->tname().c_str() );

        towelUsed = true;
        it->item_counter = 300;

        // default message
    } else {
        p->add_msg_if_player( _( "You are already dry, the %s does nothing." ), it->tname().c_str() );
    }

    // towel was used
    if( towelUsed ) {
        if( mult == 0 ) {
            mult = 1;
        }
        p->moves -= 50 * mult;
        // change "towel" to a "towel_wet" (different flavor text/color)
        if( it->typeId() == "towel" ) {
            it->convert( "towel_wet" );
        }

        // WET, active items have their timer decremented every turn
        it->item_tags.insert( "WET" );
        it->active = true;
    }
    return it->type->charges_to_use();
}

int iuse::unfold_generic( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    vehicle *veh = g->m.add_vehicle( vproto_id( "none" ), p->posx(), p->posy(), 0, 0, 0, false );
    if( veh == NULL ) {
        p->add_msg_if_player( m_info, _( "There's no room to unfold the %s." ), it->tname().c_str() );
        return 0;
    }
    veh->name = it->get_var( "vehicle_name" );
    if( !veh->restore( it->get_var( "folding_bicycle_parts" ) ) ) {
        g->m.destroy_vehicle( veh );
        return 0;
    }
    g->m.add_vehicle_to_cache( veh );

    std::string unfold_msg = it->get_var( "unfold_msg" );
    if( unfold_msg.empty() ) {
        unfold_msg = _( "You painstakingly unfold the %s and make it ready to ride." );
    } else {
        unfold_msg = _( unfold_msg.c_str() );
    }
    p->add_msg_if_player( unfold_msg.c_str(), veh->name.c_str() );

    p->moves -= it->get_var( "moves", 500 );
    return 1;
}

int iuse::adrenaline_injector( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() && p->get_effect_dur( effect_adrenaline ) >= 30_minutes ) {
        return 0;
    }

    p->moves -= 100;
    p->add_msg_player_or_npc( _( "You inject yourself with adrenaline." ),
                              _( "<npcname> injects themselves with adrenaline." ) );

    item syringe( "syringe", it->birthday() );
    p->i_add( syringe );
    if( p->has_effect( effect_adrenaline ) ) {
        p->add_msg_if_player( m_bad, _( "Your heart spasms!" ) );
        // Note: not the mod, the health
        p->mod_healthy( -20 );
    }

    p->add_effect( effect_adrenaline, 20_minutes );

    return it->type->charges_to_use();
}

int iuse::jet_injector( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( m_info, _( "The jet injector is empty." ) );
        return 0;
    } else {
        p->add_msg_if_player( _( "You inject yourself with the jet injector." ) );
        // Intensity is 2 here because intensity = 1 is the comedown
        p->add_effect( effect_jetinjector, 20_minutes, num_bp, false, 2 );
        p->mod_painkiller( 20 );
        p->stim += 10;
        p->healall( 20 );
    }

    if( p->has_effect( effect_jetinjector ) ) {
        if( p->get_effect_dur( effect_jetinjector ) > 20_minutes ) {
            p->add_msg_if_player( m_warning, _( "Your heart is beating alarmingly fast!" ) );
        }
    }
    return it->type->charges_to_use();
}

int iuse::stimpack( player *p, item *it, bool, const tripoint & )
{
    if( p->get_item_position( it ) >= -1 ) {
        p->add_msg_if_player( m_info,
                              _( "You must wear the stimulant delivery system before you can activate it." ) );
        return 0;
    }

    if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( m_info, _( "The stimulant delivery system is empty." ) );
        return 0;
    } else {
        p->add_msg_if_player( _( "You inject yourself with the stimulants." ) );
        // Intensity is 2 here because intensity = 1 is the comedown
        p->add_effect( effect_stimpack, 25_minutes, num_bp, false, 2 );
        p->mod_painkiller( 2 );
        p->stim += 20;
        p->mod_fatigue( -100 );
        p->stamina = p->get_stamina_max();
    }
    return it->type->charges_to_use();
}

int iuse::radglove( player *p, item *it, bool, const tripoint & )
{
    if( p->get_item_position( it ) >= -1 ) {
        p->add_msg_if_player( m_info,
                              _( "You must wear the radiation biomonitor before you can activate it." ) );
        return 0;
    } else if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( m_info, _( "The radiation biomonitor needs batteries to function." ) );
        return 0;
    } else {
        p->add_msg_if_player( _( "You activate your radiation biomonitor." ) );
        if( p->radiation >= 1 ) {
            p->add_msg_if_player( m_warning, _( "You are currently irradiated." ) );
            p->add_msg_player_or_say( m_info,
                                      _( "Your radiation level: %d" ),
                                      _( "It says here that my radiation level is %d" ),
                                      p->radiation );
        } else {
            p->add_msg_player_or_say( m_info,
                                      _( "You are not currently irradiated." ),
                                      _( "It says I'm not irradiated" ) );
        }
        p->add_msg_if_player( _( "Have a nice day!" ) );
    }

    return it->type->charges_to_use();
}


int iuse::contacts( player *p, item *it, bool, const tripoint & )
{
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    const time_duration duration = rng( 6_days, 8_days );
    if( p->has_effect( effect_contacts ) ) {
        if( query_yn( _( "Replace your current lenses?" ) ) ) {
            p->moves -= 200;
            p->add_msg_if_player( _( "You replace your current %s." ), it->tname().c_str() );
            p->remove_effect( effect_contacts );
            p->add_effect( effect_contacts, duration );
            return it->type->charges_to_use();
        } else {
            p->add_msg_if_player( _( "You don't do anything with your %s." ), it->tname().c_str() );
            return 0;
        }
    } else if( p->has_trait( trait_HYPEROPIC ) || p->has_trait( trait_MYOPIC ) ||
               p->has_trait( trait_URSINE_EYE ) ) {
        p->moves -= 200;
        p->add_msg_if_player( _( "You put the %s in your eyes." ), it->tname().c_str() );
        p->add_effect( effect_contacts, duration );
        return it->type->charges_to_use();
    } else {
        p->add_msg_if_player( m_info, _( "Your vision is fine already." ) );
        return 0;
    }
}

int iuse::talking_doll( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( m_info, _( "The %s's batteries are dead." ), it->tname().c_str() );
        return 0;
    }

    const SpeechBubble speech = get_speech( it->typeId() );

    sounds::ambient_sound( p->pos(), speech.volume, speech.text );

    return it->type->charges_to_use();
}

int iuse::gun_repair( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        return 0;
    }
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    /** @EFFECT_MECHANICS >1 allows gun repair */
    if( p->get_skill_level( skill_mechanics ) < 2 ) {
        p->add_msg_if_player( m_info, _( "You need a mechanics skill of 2 to use this repair kit." ) );
        return 0;
    }
    int inventory_index = g->inv_for_all( _( "Select the firearm to repair" ) );
    item &fix = p->i_at( inventory_index );
    if( fix.is_null() ) {
        p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
        return 0;
    }
    if( !fix.is_firearm() ) {
        p->add_msg_if_player( m_info, _( "That isn't a firearm!" ) );
        return 0;
    }
    if( fix.has_flag( "NO_REPAIR" ) ) {
        p->add_msg_if_player( m_info, _( "You cannot repair your %s." ), fix.tname().c_str() );
        return 0;
    }
    if( fix.precise_damage() <= fix.min_damage() ) {
        p->add_msg_if_player( m_info, _( "You cannot improve your %s any more this way." ),
                              fix.tname().c_str() );
        return 0;
    }
    if( fix.precise_damage() <= 0 && p->get_skill_level( skill_mechanics ) < 8 ) {
        p->add_msg_if_player( m_info, _( "Your %s is already in peak condition." ), fix.tname().c_str() );
        p->add_msg_if_player( m_info,
                              _( "With a higher mechanics skill, you might be able to improve it." ) );
        return 0;
    }
    /** @EFFECT_MECHANICS >=8 allows accurizing ranged weapons */
    if( fix.precise_damage() <= 0 ) {
        sounds::sound( p->pos(), 6, "" );
        p->moves -= 2000 * p->fine_detail_vision_mod();
        p->practice( skill_mechanics, 10 );
        p->add_msg_if_player( m_good, _( "You accurize your %s." ), fix.tname().c_str() );
        fix.mod_damage( -1 );

    } else if( fix.precise_damage() > 1 ) {
        sounds::sound( p->pos(), 8, "" );
        p->moves -= 1000 * p->fine_detail_vision_mod();
        p->practice( skill_mechanics, 10 );
        p->add_msg_if_player( m_good, _( "You repair your %s!" ), fix.tname().c_str() );
        fix.mod_damage( -1 );

    } else {
        sounds::sound( p->pos(), 8, "" );
        p->moves -= 500 * p->fine_detail_vision_mod();
        p->practice( skill_mechanics, 10 );
        p->add_msg_if_player( m_good, _( "You repair your %s completely!" ),
                              fix.tname().c_str() );
        fix.set_damage( 0 );
    }
    return it->type->charges_to_use();
}

int iuse::gunmod_attach( player *p, item *it, bool, const tripoint & )
{
    if( !it || !it->is_gunmod() ) {
        debugmsg( "tried to attach non-gunmod" );
        return 0;
    }

    if( !p ) {
        return 0;
    }

    auto loc = game_menus::inv::gun_to_modify( *p, *it );

    if( !loc ) {
        add_msg( m_info, _( "Never mind." ) );
        return 0;
    }

    p->gunmod_add( *loc, *it );

    return 0;
}

int iuse::toolmod_attach( player *p, item *it, bool, const tripoint & )
{
    if( !it || !it->is_toolmod() ) {
        debugmsg( "tried to attach non-toolmod" );
        return 0;
    }

    if( !p ) {
        return 0;
    }

    auto filter = [&it]( const item & e ) {
        // don't allow ups battery mods on a UPS
        if( it->has_flag( "USE_UPS" ) && ( e.typeId() == "UPS_off" || e.typeId() == "adv_UPS_off" ) ) {
            return false;
        }

        // can only attach to unmodified tools that use compatible ammo
        return e.is_tool() && e.toolmods().empty() && !e.magazine_current() &&
               it->type->mod->acceptable_ammo.count( e.ammo_type( false ) );
    };

    auto loc = g->inv_map_splice( filter, _( "Select tool to modify" ), 1,
                                  _( "You don't have compatible tools." ) );

    if( !loc ) {
        add_msg( m_info, _( "Never mind." ) );
        return 0;
    }

    if( loc->ammo_remaining() ) {
        if( !g->unload( *loc ) ) {
            p->add_msg_if_player( m_info, _( "You cancel unloading the tool." ) );
            return 0;
        }
    }

    p->toolmod_add( std::move( loc ), item_location( *p, it ) );
    return 0;
}

int iuse::misc_repair( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        return 0;
    }
    if( p->is_underwater() ) {
        p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return 0;
    }
    if( p->fine_detail_vision_mod() > 4 ) {
        add_msg( m_info, _( "You can't see to repair!" ) );
        return 0;
    }
    /** @EFFECT_FABRICATION >0 allows use of repair kit */
    if( p->get_skill_level( skill_fabrication ) < 1 ) {
        p->add_msg_if_player( m_info, _( "You need a fabrication skill of 1 to use this repair kit." ) );
        return 0;
    }
    int inventory_index = g->inv_for_filter( _( "Select the item to repair" ), []( const item & itm ) {
        return ( !itm.is_firearm() ) && ( itm.made_of( material_id( "wood" ) ) ||
                                          itm.made_of( material_id( "paper" ) ) ||
                                          itm.made_of( material_id( "bone" ) ) || itm.made_of( material_id( "chitin" ) ) ||
                                          itm.made_of( material_id( "acidchitin" ) ) ) &&
               !itm.count_by_charges();
    } );
    item &fix = p->i_at( inventory_index );
    if( fix.is_null() ) {
        p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
        return 0;
    }
    if( fix.precise_damage() <= fix.min_damage() ) {
        p->add_msg_if_player( m_info, _( "You cannot improve your %s any more this way." ),
                              fix.tname().c_str() );
        return 0;
    }
    if( fix.precise_damage() <= 0 && fix.has_flag( "PRIMITIVE_RANGED_WEAPON" ) ) {
        p->add_msg_if_player( m_info, _( "You cannot improve your %s any more this way." ),
                              fix.tname().c_str() );
        return 0;
    }
    if( fix.precise_damage() <= 0 ) {
        p->moves -= 1000 * p->fine_detail_vision_mod();
        p->practice( skill_fabrication, 10 );
        p->add_msg_if_player( m_good, _( "You reinforce your %s." ), fix.tname().c_str() );
        fix.mod_damage( -1 );

    } else if( fix.precise_damage() > 1 ) {
        p->moves -= 500 * p->fine_detail_vision_mod();
        p->practice( skill_fabrication, 10 );
        p->add_msg_if_player( m_good, _( "You repair your %s!" ), fix.tname().c_str() );
        fix.mod_damage( -1 );

    } else {
        p->moves -= 250 * p->fine_detail_vision_mod();
        p->practice( skill_fabrication, 10 );
        p->add_msg_if_player( m_good, _( "You repair your %s completely!" ), fix.tname().c_str() );
        fix.set_damage( 0 );
    }
    return it->type->charges_to_use();
}

int iuse::bell( player *p, item *it, bool, const tripoint & )
{
    if( it->typeId() == "cow_bell" ) {
        sounds::sound( p->pos(), 12, _( "Clank! Clank!" ) );
        if( !p->is_deaf() ) {
            const int cow_factor = 1 + ( p->mutation_category_level.find( "CATTLE" ) ==
                                         p->mutation_category_level.end() ?
                                         0 :
                                         ( p->mutation_category_level.find( "CATTLE" )->second ) / 8
                                       );
            if( x_in_y( cow_factor, 1 + cow_factor ) ) {
                p->add_morale( MORALE_MUSIC, 1, 15 * ( cow_factor > 10 ? 10 : cow_factor ) );
            }
        }
    } else {
        sounds::sound( p->pos(), 4, _( "Ring! Ring!" ) );
    }
    return it->type->charges_to_use();
}

int iuse::seed( player *p, item *it, bool, const tripoint & )
{
    if( p->is_npc() ||
        query_yn( _( "Sure you want to eat the %s? You could plant it in a mound of dirt." ),
                  it->tname().c_str() ) ) {
        return it->type->charges_to_use(); //This eats the seed object.
    }
    return 0;
}

int iuse::robotcontrol( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( _( "The %s's batteries are dead." ), it->tname().c_str() );
        return 0;

    }
    if( p->has_trait( trait_ILLITERATE ) ) {
        p->add_msg_if_player( _( "You cannot read a computer screen." ) );
        return 0;
    }

    int choice = menu( true, _( "Welcome to hackPRO!:" ), _( "Override IFF protocols" ),
                       _( "Set friendly robots to passive mode" ),
                       _( "Set friendly robots to combat mode" ), _( "Cancel" ), NULL );
    switch( choice ) {
        case 1: { // attempt to make a robot friendly
            uimenu pick_robot;
            pick_robot.text = _( "Choose an endpoint to hack." );
            // Build a list of all unfriendly robots in range.
            std::vector< monster * > mons; // @todo: change into vector<Creature*>
            std::vector< tripoint > locations;
            int entry_num = 0;
            for( monster &candidate : g->all_monsters() ) {
                if( candidate.type->in_species( ROBOT ) && candidate.friendly == 0 &&
                    rl_dist( p->pos(), candidate.pos() ) <= 10 ) {
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
                p->add_msg_if_player( m_info, _( "No enemy robots in range." ) );
                return it->type->charges_to_use();
            }
            pointmenu_cb callback( locations );
            pick_robot.callback = &callback;
            pick_robot.addentry( INT_MAX, true, -1, _( "Cancel" ) );
            pick_robot.query();
            const size_t mondex = pick_robot.ret;
            if( mondex >= mons.size() ) {
                p->add_msg_if_player( m_info, _( "Never mind" ) );
                return it->type->charges_to_use();
            }
            monster *z = mons[mondex];
            p->add_msg_if_player( _( "You start reprogramming the %s into an ally." ), z->name().c_str() );
            /** @EFFECT_INT speeds up robot reprogramming */

            /** @EFFECT_COMPUTER speeds up robot reprogramming */
            p->moves -= std::max( 100, 1000 - p->int_cur * 10 - p->get_skill_level( skill_computer ) * 10 );
            /** @EFFECT_INT increases chance of successful robot reprogramming, vs difficulty */

            /** @EFFECT_COMPUTER increases chance of successful robot reprogramming, vs difficulty */
            float success = p->get_skill_level( skill_computer ) - 1.5 * ( z->type->difficulty ) /
                            ( ( rng( 2, p->int_cur ) / 2 ) + ( p->get_skill_level( skill_computer ) / 2 ) );
            if( success >= 0 ) {
                p->add_msg_if_player( _( "You successfully override the %s's IFF protocols!" ),
                                      z->name().c_str() );
                z->friendly = -1;
            } else if( success >= -2 ) { //A near success
                p->add_msg_if_player( _( "The %s short circuits as you attempt to reprogram it!" ),
                                      z->name().c_str() );
                z->apply_damage( p, bp_torso, rng( 1, 10 ) ); //damage it a little
                if( z->is_dead() ) {
                    p->practice( skill_computer, 10 );
                    return it->type->charges_to_use(); // Do not do the other effects if the robot died
                }
                if( one_in( 3 ) ) {
                    p->add_msg_if_player( _( "...and turns friendly!" ) );
                    if( one_in( 3 ) ) { //did the robot became friendly permanently?
                        z->friendly = -1; //it did
                    } else {
                        z->friendly = rng( 5, 40 ); // it didn't
                    }
                }
            } else {
                p->add_msg_if_player( _( "...but the robot refuses to acknowledge you as an ally!" ) );
            }
            p->practice( skill_computer, 10 );
            return it->type->charges_to_use();
        }
        case 2: { //make all friendly robots stop their purposeless extermination of (un)life.
            p->moves -= 100;
            int f = 0; //flag to check if you have robotic allies
            for( monster &critter : g->all_monsters() ) {
                if( critter.friendly != 0 && critter.type->in_species( ROBOT ) ) {
                    p->add_msg_if_player( _( "A following %s goes into passive mode." ),
                                          critter.name().c_str() );
                    critter.add_effect( effect_docile, 1_turns, num_bp, true );
                    f = 1;
                }
            }
            if( f == 0 ) {
                p->add_msg_if_player( _( "You are not commanding any robots." ) );
                return 0;
            }
            return it->type->charges_to_use();
        }
        case 3: { //make all friendly robots terminate (un)life with extreme prejudice
            p->moves -= 100;
            int f = 0; //flag to check if you have robotic allies
            for( monster &critter : g->all_monsters() ) {
                if( critter.friendly != 0 && critter.has_flag( MF_ELECTRONIC ) ) {
                    p->add_msg_if_player( _( "A following %s goes into combat mode." ),
                                          critter.name().c_str() );
                    critter.remove_effect( effect_docile );
                    f = 1;
                }
            }
            if( f == 0 ) {
                p->add_msg_if_player( _( "You are not commanding any robots." ) );
                return 0;
            }
            return it->type->charges_to_use();
        }

    }
    return 0;
}

void init_memory_card_with_random_stuff( item &it )
{
    if( it.has_flag( "MC_MOBILE" ) && ( it.has_flag( "MC_RANDOM_STUFF" ) ||
                                        it.has_flag( "MC_SCIENCE_STUFF" ) ) && !( it.has_flag( "MC_USED" ) ||
                                                it.has_flag( "MC_HAS_DATA" ) ) ) {

        it.item_tags.insert( "MC_HAS_DATA" );

        bool encrypted = false;

        if( it.has_flag( "MC_MAY_BE_ENCRYPTED" ) && one_in( 8 ) ) {
            it.convert( it.typeId() + "_encrypted" );
        }

        //some special cards can contain "MC_ENCRYPTED" flag
        if( it.has_flag( "MC_ENCRYPTED" ) ) {
            encrypted = true;
        }

        int data_chance = 2;

        //encrypted memory cards often contain data
        if( encrypted && !one_in( 3 ) ) {
            data_chance--;
        }

        //just empty memory card
        if( !one_in( data_chance ) ) {
            return;
        }

        //add someone's personal photos
        if( one_in( data_chance ) ) {

            //decrease chance to more data
            data_chance++;

            if( encrypted && one_in( 3 ) ) {
                data_chance--;
            }

            const int duckfaces_count = rng( 5, 30 );
            it.set_var( "MC_PHOTOS", duckfaces_count );
        }
        //decrease chance to music and other useful data
        data_chance++;
        if( encrypted && one_in( 2 ) ) {
            data_chance--;
        }

        if( one_in( data_chance ) ) {
            data_chance++;

            if( encrypted && one_in( 3 ) ) {
                data_chance--;
            }

            const int new_songs_count = rng( 5, 15 );
            it.set_var( "MC_MUSIC", new_songs_count );
        }
        data_chance++;
        if( encrypted && one_in( 2 ) ) {
            data_chance--;
        }

        if( one_in( data_chance ) ) {
            it.set_var( "MC_RECIPE", "SIMPLE" );
        }

        if( it.has_flag( "MC_SCIENCE_STUFF" ) ) {
            it.set_var( "MC_RECIPE", "SCIENCE" );
        }
    }
}

bool einkpc_download_memory_card( player &p, item &eink, item &mc )
{
    bool something_downloaded = false;
    if( mc.get_var( "MC_PHOTOS", 0 ) > 0 ) {
        something_downloaded = true;

        int new_photos = mc.get_var( "MC_PHOTOS", 0 );
        mc.erase_var( "MC_PHOTOS" );

        p.add_msg_if_player( m_good, string_format(
                                 ngettext( "You download %d new photo into internal memory.",
                                           "You download %d new photos into internal memory.", new_photos ),
                                 new_photos ).c_str() );

        const int old_photos = eink.get_var( "EIPC_PHOTOS", 0 );
        eink.set_var( "EIPC_PHOTOS", old_photos + new_photos );
    }

    if( mc.get_var( "MC_MUSIC", 0 ) > 0 ) {
        something_downloaded = true;

        int new_songs = mc.get_var( "MC_MUSIC", 0 );
        mc.erase_var( "MC_MUSIC" );

        p.add_msg_if_player( m_good, string_format(
                                 ngettext( "You download %d new song into internal memory.",
                                           "You download %d new songs into internal memory.", new_songs ),
                                 new_songs ).c_str() );

        const int old_songs = eink.get_var( "EIPC_MUSIC", 0 );
        eink.set_var( "EIPC_MUSIC", old_songs + new_songs );
    }

    if( !mc.get_var( "MC_RECIPE" ).empty() ) {
        const bool science = mc.get_var( "MC_RECIPE" ) == "SCIENCE";

        mc.erase_var( "MC_RECIPE" );

        std::vector<const recipe *> candidates;

        for( const auto &e : recipe_dict ) {
            const auto &r = e.second;

            if( science ) {
                if( r.difficulty >= 3 && one_in( r.difficulty + 1 ) ) {
                    candidates.push_back( &r );
                }
            } else {
                if( r.category == "CC_FOOD" ) {
                    if( r.difficulty <= 3 && one_in( r.difficulty ) ) {
                        candidates.push_back( &r );
                    }
                }

            }


        }

        if( candidates.size() > 0 ) {

            const recipe *r = random_entry( candidates );
            const recipe_id rident = r->ident();

            const auto old_recipes = eink.get_var( "EIPC_RECIPES" );
            if( old_recipes.empty() ) {
                something_downloaded = true;
                eink.set_var( "EIPC_RECIPES", "," + rident.str() + "," );

                p.add_msg_if_player( m_good, _( "You download a recipe for %s into the tablet's memory." ),
                                     r->result_name() );
            } else {
                if( old_recipes.find( "," + rident.str() + "," ) == std::string::npos ) {
                    something_downloaded = true;
                    eink.set_var( "EIPC_RECIPES", old_recipes + rident.str() + "," );

                    p.add_msg_if_player( m_good, _( "You download a recipe for %s into the tablet's memory." ),
                                         r->result_name() );
                } else {
                    p.add_msg_if_player( m_good, _( "Your tablet already has a recipe for %s." ),
                                         r->result_name() );
                }
            }
        }
    }

    const auto monster_photos = mc.get_var( "MC_MONSTER_PHOTOS" );
    if( !monster_photos.empty() ) {
        something_downloaded = true;
        p.add_msg_if_player( m_good, _( "You have updated your monster collection." ) );

        auto photos = eink.get_var( "EINK_MONSTER_PHOTOS" );
        if( photos.empty() ) {
            eink.set_var( "EINK_MONSTER_PHOTOS", monster_photos );
        } else {
            std::istringstream f( monster_photos );
            std::string s;
            while( getline( f, s, ',' ) ) {

                if( s.empty() ) {
                    continue;
                }

                const std::string mtype = s;
                getline( f, s, ',' );
                char *chq = &s[0];
                const int quality = atoi( chq );

                const size_t eink_strpos = photos.find( "," + mtype + "," );

                if( eink_strpos == std::string::npos ) {
                    photos += mtype + "," + string_format( "%d", quality ) + ",";
                } else {

                    const size_t strqpos = eink_strpos + mtype.size() + 2;
                    char *chq = &photos[strqpos];
                    const int old_quality = atoi( chq );

                    if( quality > old_quality ) {
                        chq = &string_format( "%d", quality )[0];
                        photos[strqpos] = *chq;
                    }
                }

            }
            eink.set_var( "EINK_MONSTER_PHOTOS", photos );
        }
    }

    if( mc.has_flag( "MC_TURN_USED" ) ) {
        mc.clear_vars();
        mc.unset_flags();
        mc.convert( "mobile_memory_card_used" );
    }

    if( !something_downloaded ) {
        p.add_msg_if_player( m_info, _( "This memory card does not contain any new data." ) );
        return false;
    }

    return true;

}

static const std::string &photo_quality_name( const int index )
{
    static std::array<std::string, 6> const names { {
            //~ photo quality adjective
            { _( "awful" ) }, { _( "bad" ) }, { _( "not bad" ) }, { _( "good" ) }, { _( "fine" ) }, { _( "exceptional" ) }
        } };
    return names[index];
}


int iuse::einktabletpc( player *p, item *it, bool t, const tripoint &pos )
{
    if( t ) {
        if( !it->get_var( "EIPC_MUSIC_ON" ).empty() && ( it->ammo_remaining() > 0 ) ) {
            if( calendar::once_every( 5_minutes ) ) {
                it->ammo_consume( 1, p->pos() );
            }

            //the more varied music, the better max mood.
            const int songs = it->get_var( "EIPC_MUSIC", 0 );
            play_music( *p, pos, 8, std::min( 25, songs ) );
        } else {
            it->active = false;
            it->erase_var( "EIPC_MUSIC_ON" );
            p->add_msg_if_player( m_info, _( "Tablet's batteries are dead." ) );
        }

        return 0;

    } else if( !p->is_npc() ) {

        enum {
            ei_cancel, ei_photo, ei_music, ei_recipe, ei_monsters, ei_download, ei_decrypt
        };

        if( p->is_underwater() ) {
            p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
            return 0;
        }
        if( p->has_trait( trait_ILLITERATE ) ) {
            add_msg( m_info, _( "You cannot read a computer screen." ) );
            return 0;
        }
        if( p->has_trait( trait_HYPEROPIC ) && !p->worn_with_flag( "FIX_FARSIGHT" ) &&
            !p->has_effect( effect_contacts ) ) {
            add_msg( m_info, _( "You'll need to put on reading glasses before you can see the screen." ) );
            return 0;
        }

        uimenu amenu;

        amenu.selected = 0;
        amenu.text = _( "Choose menu option:" );
        amenu.addentry( ei_cancel, true, 'q', _( "Cancel" ) );

        const int photos = it->get_var( "EIPC_PHOTOS", 0 );
        if( photos > 0 ) {
            amenu.addentry( ei_photo, true, 'p', _( "Photos [%d]" ), photos );
        } else {
            amenu.addentry( ei_photo, false, 'p', _( "No photos on device" ) );
        }

        const int songs = it->get_var( "EIPC_MUSIC", 0 );
        if( songs > 0 ) {
            if( it->active ) {
                amenu.addentry( ei_music, true, 'm', _( "Turn music off" ) );
            } else {
                amenu.addentry( ei_music, true, 'm', _( "Turn music on [%d]" ), songs );
            }
        } else {
            amenu.addentry( ei_music, false, 'm', _( "No music on device" ) );
        }

        if( !it->get_var( "RECIPE" ).empty() ) {
            const item dummy( it->get_var( "RECIPE" ), 0 );
            amenu.addentry( 0, false, -1, _( "Recipe: %s" ), dummy.tname().c_str() );
        }

        if( !it->get_var( "EIPC_RECIPES" ).empty() ) {
            amenu.addentry( ei_recipe, true, 'r', _( "View recipe on E-ink screen" ) );
        }

        if( !it->get_var( "EINK_MONSTER_PHOTOS" ).empty() ) {
            amenu.addentry( ei_monsters, true, 'y', _( "Your collection of monsters" ) );
        } else {
            amenu.addentry( ei_monsters, false, 'y', _( "Collection of monsters is empty" ) );
        }

        amenu.addentry( ei_download, true, 'w', _( "Download data from memory card" ) );

        /** @EFFECT_COMPUTER >2 allows decrypting memory cards more easily */
        if( p->get_skill_level( skill_computer ) > 2 ) {
            amenu.addentry( ei_decrypt, true, 'd', _( "Decrypt memory card" ) );
        } else {
            amenu.addentry( ei_decrypt, false, 'd', _( "Decrypt memory card (low skill)" ) );
        }

        amenu.query();

        const int choice = amenu.ret;

        if( ei_cancel == choice ) {
            return 0;
        }

        if( ei_photo == choice ) {

            const int photos = it->get_var( "EIPC_PHOTOS", 0 );
            const int viewed = std::min( photos, int( rng( 10, 30 ) ) );
            const int count = photos - viewed;
            if( count == 0 ) {
                it->erase_var( "EIPC_PHOTOS" );
            } else {
                it->set_var( "EIPC_PHOTOS", count );
            }

            p->moves -= rng( 3, 7 ) * 100;

            if( p->has_trait( trait_PSYCHOPATH ) ) {
                p->add_msg_if_player( m_info, _( "Wasted time, these pictures do not provoke your senses." ) );
            } else {
                p->add_morale( MORALE_PHOTOS, rng( 15, 30 ), 100 );

                const int random_photo = rng( 1, 20 );
                switch( random_photo ) {
                    case 1:
                        p->add_msg_if_player( m_good, _( "You used to have a dog like this..." ) );
                        break;
                    case 2:
                        p->add_msg_if_player( m_good, _( "Ha-ha!  An amusing cat photo." ) );
                        break;
                    case 3:
                        p->add_msg_if_player( m_good, _( "Excellent pictures of nature." ) );
                        break;
                    case 4:
                        p->add_msg_if_player( m_good, _( "Food photos... your stomach rumbles!" ) );
                        break;
                    case 5:
                        p->add_msg_if_player( m_good, _( "Some very interesting travel photos." ) );
                        break;
                    case 6:
                        p->add_msg_if_player( m_good, _( "Pictures of a concert of popular band." ) );
                        break;
                    case 7:
                        p->add_msg_if_player( m_good, _( "Photos of someone's luxurious house." ) );
                        break;
                    default:
                        p->add_msg_if_player( m_good, _( "You feel nostalgic as you stare at the photo." ) );
                        break;
                }
            }

            return it->type->charges_to_use();
        }

        if( ei_music == choice ) {

            p->moves -= 30;

            if( it->active ) {
                it->active = false;
                it->erase_var( "EIPC_MUSIC_ON" );

                p->add_msg_if_player( m_info, _( "You turned off music on your %s." ), it->tname().c_str() );
            } else {
                it->active = true;
                it->set_var( "EIPC_MUSIC_ON", "1" );

                p->add_msg_if_player( m_info, _( "You turned on music on your %s." ), it->tname().c_str() );

            }

            return it->type->charges_to_use();
        }

        if( ei_recipe == choice ) {
            p->moves -= 50;

            uimenu rmenu;

            rmenu.selected = 0;
            rmenu.text = _( "Choose recipe to view:" );
            rmenu.addentry( 0, true, 'q', _( "Cancel" ) );

            std::vector<recipe_id> candidate_recipes;
            std::istringstream f( it->get_var( "EIPC_RECIPES" ) );
            std::string s;
            int k = 1;
            while( getline( f, s, ',' ) ) {

                if( s.empty() ) {
                    continue;
                }

                candidate_recipes.emplace_back( s );

                const auto &recipe = *candidate_recipes.back();
                if( recipe ) {
                    rmenu.addentry( k++, true, -1, recipe.result_name() );
                }
            }

            rmenu.query();

            const int rchoice = rmenu.ret;
            if( 0 == rchoice ) {
                return it->type->charges_to_use();
            } else {
                it->item_tags.insert( "HAS_RECIPE" );
                const auto rec_id = candidate_recipes[rchoice - 1];
                it->set_var( "RECIPE", rec_id.str() );

                const auto &recipe = *rec_id;
                if( recipe ) {
                    p->add_msg_if_player( m_info,
                                          _( "You change the e-ink screen to show a recipe for %s." ),
                                          recipe.result_name() );
                }
            }

            return it->type->charges_to_use();
        }

        if( ei_monsters == choice ) {

            uimenu pmenu;

            pmenu.selected = 0;
            pmenu.text = _( "Your collection of monsters:" );
            pmenu.addentry( 0, true, 'q', _( "Cancel" ) );

            std::vector<mtype_id> monster_photos;

            std::istringstream f( it->get_var( "EINK_MONSTER_PHOTOS" ) );
            std::string s;
            int k = 1;
            while( getline( f, s, ',' ) ) {
                if( s.empty() ) {
                    continue;
                }
                monster_photos.push_back( mtype_id( s ) );
                std::string menu_str;
                const monster dummy( monster_photos.back() );
                menu_str = dummy.name();
                getline( f, s, ',' );
                char *chq = &s[0];
                const int quality = atoi( chq );
                menu_str += " [" + photo_quality_name( quality ) + "]";
                pmenu.addentry( k++, true, -1, menu_str.c_str() );
            }

            int choice;
            do {
                pmenu.query();
                choice = pmenu.ret;

                if( 0 == choice ) {
                    break;
                }

                const monster dummy( monster_photos[choice - 1] );
                popup( dummy.type->get_description().c_str() );
            } while( true );
            return it->type->charges_to_use();
        }

        if( ei_download == choice ) {

            p->moves -= 200;

            const int inventory_index = g->inv_for_flag( "MC_MOBILE", _( "Insert memory card" ) );
            item &mc = p->i_at( inventory_index );

            if( mc.is_null() ) {
                p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
                return it->type->charges_to_use();
            }
            if( !mc.has_flag( "MC_MOBILE" ) ) {
                p->add_msg_if_player( m_info, _( "This is not a compatible memory card." ) );
                return it->type->charges_to_use();
            }

            init_memory_card_with_random_stuff( mc );

            if( mc.has_flag( "MC_ENCRYPTED" ) ) {
                p->add_msg_if_player( m_info, _( "This memory card is encrypted." ) );
                return it->type->charges_to_use();
            }
            if( !mc.has_flag( "MC_HAS_DATA" ) ) {
                p->add_msg_if_player( m_info, _( "This memory card does not contain any new data." ) );
                return it->type->charges_to_use();
            }

            einkpc_download_memory_card( *p, *it, mc );

            return it->type->charges_to_use();
        }

        if( ei_decrypt == choice ) {
            p->moves -= 200;
            const int inventory_index = g->inv_for_flag( "MC_MOBILE", _( "Insert memory card" ) );
            item &mc = p->i_at( inventory_index );

            if( mc.is_null() ) {
                p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
                return it->type->charges_to_use();
            }
            if( !mc.has_flag( "MC_MOBILE" ) ) {
                p->add_msg_if_player( m_info, _( "This is not a compatible memory card." ) );
                return it->type->charges_to_use();
            }

            init_memory_card_with_random_stuff( mc );

            if( !mc.has_flag( "MC_ENCRYPTED" ) ) {
                p->add_msg_if_player( m_info, _( "This memory card is not encrypted." ) );
                return it->type->charges_to_use();
            }

            p->practice( skill_computer, rng( 2, 5 ) );

            /** @EFFECT_INT increases chance of safely decrypting memory card */

            /** @EFFECT_COMPUTER increases chance of safely decrypting memory card */
            const int success = p->get_skill_level( skill_computer ) * rng( 1,
                                p->get_skill_level( skill_computer ) ) *
                                rng( 1, p->int_cur ) - rng( 30, 80 );
            if( success > 0 ) {
                p->practice( skill_computer, rng( 5, 10 ) );
                p->add_msg_if_player( m_good, _( "You successfully decrypted content on %s!" ),
                                      mc.tname().c_str() );
                einkpc_download_memory_card( *p, *it, mc );
            } else {
                if( success > -10 || one_in( 5 ) ) {
                    p->add_msg_if_player( m_neutral, _( "You failed to decrypt the %s." ), mc.tname().c_str() );
                } else {
                    p->add_msg_if_player( m_bad,
                                          _( "You tripped the firmware protection, and the card deleted its data!" ) );
                    mc.clear_vars();
                    mc.unset_flags();
                    mc.convert( "mobile_memory_card_used" );
                }
            }
            return it->type->charges_to_use();
        }
    }
    return 0;
}

int iuse::camera( player *p, item *it, bool, const tripoint & )
{
    enum {c_cancel, c_shot, c_photos, c_upload};

    uimenu amenu;

    amenu.selected = 0;
    amenu.text = _( "What to do with camera?" );
    amenu.addentry( c_shot, true, 'p', _( "Take a photo" ) );
    if( !it->get_var( "CAMERA_MONSTER_PHOTOS" ).empty() ) {
        amenu.addentry( c_photos, true, 'l', _( "List photos" ) );
        amenu.addentry( c_upload, true, 'u', _( "Upload photos to memory card" ) );
    } else {
        amenu.addentry( c_photos, false, 'l', _( "No photos in memory" ) );
    }

    amenu.addentry( c_cancel, true, 'q', _( "Cancel" ) );

    amenu.query();
    const int choice = amenu.ret;

    if( c_cancel == choice ) {
        return 0;
    }

    if( c_shot == choice ) {

        tripoint aim_point = g->look_around();

        if( aim_point == tripoint_min ) {
            p->add_msg_if_player( _( "Never mind." ) );
            return 0;
        }

        if( aim_point == p->pos() ) {
            p->add_msg_if_player( _( "You decide not to flash yourself." ) );
            return 0;
        }

        const monster *const sel_mon = g->critter_at<monster>( aim_point, true );
        const npc *const sel_npc = g->critter_at<npc>( aim_point );

        if( !g->critter_at( aim_point ) ) {
            p->add_msg_if_player( _( "There's nothing particularly interesting there." ) );
            return 0;
        }

        std::vector<tripoint> trajectory = line_to( p->pos(), aim_point, 0, 0 );
        trajectory.push_back( aim_point );

        p->moves -= 50;
        sounds::sound( p->pos(), 8, _( "Click." ) );

        for( auto &i : trajectory ) {

            monster *const mon = g->critter_at<monster>( i, true );
            npc *const guy = g->critter_at<npc>( i );
            if( mon || guy ) {
                int dist = rl_dist( p->pos(), i );

                int camera_bonus = it->has_flag( "CAMERA_PRO" ) ? 10 : 0;
                int photo_quality = 20 - rng( dist, dist * 2 ) * 2 + rng( camera_bonus / 2, camera_bonus );
                if( photo_quality > 5 ) {
                    photo_quality = 5;
                }
                if( photo_quality < 0 ) {
                    photo_quality = 0;
                }
                if( p->is_blind() ) {
                    photo_quality /= 2;
                }

                const std::string quality_name = photo_quality_name( photo_quality );

                if( mon ) {
                    monster &z = *mon;

                    if( dist < 4 && one_in( dist + 2 ) && z.has_flag( MF_SEES ) ) {
                        p->add_msg_if_player( _( "%s looks blinded." ), z.name().c_str() );
                        z.add_effect( effect_blind, rng( 5_turns, 10_turns ) );
                    }

                    // shoot past small monsters and hallucinations
                    if( mon != sel_mon && ( z.type->size <= MS_SMALL || z.is_hallucination() ||
                                            z.type->in_species( HALLUCINATION ) ) ) {
                        continue;
                    }

                    // get an empty photo if the target is a hallucination
                    if( mon == sel_mon && ( z.is_hallucination() || z.type->in_species( HALLUCINATION ) ) ) {
                        p->add_msg_if_player( _( "Strange... there's nothing in the picture?" ) );
                        return it->type->charges_to_use();
                    }

                    if( z.mission_id != -1 ) {
                        //quest processing...
                    }

                    if( mon == sel_mon ) {
                        // if the loop makes it to the target, take its photo
                        if( p->is_blind() ) {
                            p->add_msg_if_player( _( "You took a photo of %s." ), z.name().c_str() );
                        } else {
                            p->add_msg_if_player( _( "You took a %1$s photo of %2$s." ), quality_name.c_str(),
                                                  z.name().c_str() );
                        }
                    } else {
                        // or take a photo of the monster that's in the way
                        p->add_msg_if_player( m_warning, _( "A %s got in the way of your photo." ), z.name().c_str() );
                        photo_quality = 0;
                    }

                    const std::string mtype = z.type->id.str();

                    auto monster_photos = it->get_var( "CAMERA_MONSTER_PHOTOS" );
                    if( monster_photos.empty() ) {
                        monster_photos = "," + mtype + "," + string_format( "%d",
                                         photo_quality ) + ",";
                    } else {

                        const size_t strpos = monster_photos.find( "," + mtype + "," );

                        if( strpos == std::string::npos ) {
                            monster_photos += mtype + "," + string_format( "%d", photo_quality ) + ",";
                        } else {

                            const size_t strqpos = strpos + mtype.size() + 2;
                            char *chq = &monster_photos[strqpos];
                            const int old_quality = atoi( chq );

                            if( !p->is_blind() ) {
                                if( photo_quality > old_quality ) {
                                    chq = &string_format( "%d", photo_quality )[0];
                                    monster_photos[strqpos] = *chq;

                                    p->add_msg_if_player( _( "This photo is better than the previous one." ) );

                                }
                            }
                        }
                    }
                    it->set_var( "CAMERA_MONSTER_PHOTOS", monster_photos );

                    return it->type->charges_to_use();

                } else if( guy ) {
                    if( dist < 4 && one_in( dist + 2 ) ) {
                        p->add_msg_if_player( _( "%s looks blinded." ), guy->name.c_str() );
                        guy->add_effect( effect_blind, rng( 5_turns, 10_turns ) );
                    }

                    //just photo, no save. Maybe in the future we will need to create CAMERA_NPC_PHOTOS
                    if( sel_npc == guy ) {
                        if( p->is_blind() ) {
                            p->add_msg_if_player( _( "You took a photo of %s." ), guy->name.c_str() );
                        } else {
                            //~ 1s - thing being photographed, 2s - photo quality (adjective).
                            p->add_msg_if_player( _( "You took a photo of %1$s. It is %2$s." ), guy->name.c_str(),
                                                  quality_name.c_str() );
                        }
                    } else {
                        p->add_msg_if_player( m_warning, _( "%s got in the way of your photo." ), guy->name.c_str() );
                        photo_quality = 0;
                    }

                    return it->type->charges_to_use();
                }

                return it->type->charges_to_use();
            }

        }

        return it->type->charges_to_use();
    }

    if( c_photos == choice ) {

        if( p->is_blind() ) {
            p->add_msg_if_player( _( "You can't see the camera screen, you're blind." ) );
            return 0;
        }

        uimenu pmenu;

        pmenu.selected = 0;
        pmenu.text = _( "Critter photos saved on camera:" );
        pmenu.addentry( 0, true, 'q', _( "Cancel" ) );

        std::vector<mtype_id> monster_photos;

        std::istringstream f( it->get_var( "CAMERA_MONSTER_PHOTOS" ) );
        std::string s;
        int k = 1;
        while( getline( f, s, ',' ) ) {

            if( s.empty() ) {
                continue;
            }

            monster_photos.push_back( mtype_id( s ) );

            std::string menu_str;

            const monster dummy( monster_photos.back() );
            menu_str = dummy.name();

            getline( f, s, ',' );
            char *chq = &s[0];
            const int quality = atoi( chq );

            menu_str += " [" + photo_quality_name( quality ) + "]";

            pmenu.addentry( k++, true, -1, menu_str.c_str() );
        }

        int choice;
        do {
            pmenu.query();
            choice = pmenu.ret;

            if( 0 == choice ) {
                break;
            }

            const monster dummy( monster_photos[choice - 1] );
            popup( dummy.type->get_description().c_str() );

        } while( true );

        return it->type->charges_to_use();
    }

    if( c_upload == choice ) {

        if( p->is_blind() ) {
            p->add_msg_if_player( _( "You can't see the camera screen, you're blind." ) );
            return 0;
        }

        p->moves -= 200;

        const int inventory_index = g->inv_for_flag( "MC_MOBILE", _( "Insert memory card" ) );
        item &mc = p->i_at( inventory_index );

        if( mc.is_null() ) {
            p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
            return it->type->charges_to_use();
        }
        if( !mc.has_flag( "MC_MOBILE" ) ) {
            p->add_msg_if_player( m_info, _( "This is not a compatible memory card." ) );
            return it->type->charges_to_use();
        }

        init_memory_card_with_random_stuff( mc );

        if( mc.has_flag( "MC_ENCRYPTED" ) ) {
            if( !query_yn( _( "This memory card is encrypted.  Format and clear data?" ) ) ) {
                return it->type->charges_to_use();
            }
        }
        if( mc.has_flag( "MC_HAS_DATA" ) ) {
            if( !query_yn( _( "Are you sure you want to clear the old data on the card?" ) ) ) {
                return it->type->charges_to_use();
            }
        }

        mc.convert( "mobile_memory_card" );
        mc.clear_vars();
        mc.unset_flags();
        mc.item_tags.insert( "MC_HAS_DATA" );

        mc.set_var( "MC_MONSTER_PHOTOS", it->get_var( "CAMERA_MONSTER_PHOTOS" ) );
        p->add_msg_if_player( m_info, _( "You upload monster photos to memory card." ) );

        return it->type->charges_to_use();
    }

    return it->type->charges_to_use();
}

int iuse::ehandcuffs( player *p, item *it, bool t, const tripoint &pos )
{
    if( t ) {

        if( g->m.has_flag( "SWIMMABLE", pos.x, pos.y ) ) {
            it->item_tags.erase( "NO_UNWIELD" );
            it->ammo_unset();
            it->active = false;
            add_msg( m_good, _( "%s automatically turned off!" ), it->tname().c_str() );
            return it->type->charges_to_use();
        }

        if( it->charges == 0 ) {

            sounds::sound( pos, 2, "Click." );
            it->item_tags.erase( "NO_UNWIELD" );
            it->active = false;

            if( p->has_item( *it ) && p->weapon.typeId() == "e_handcuffs" ) {
                add_msg( m_good, _( "%s on your hands opened!" ), it->tname().c_str() );
            }

            return it->type->charges_to_use();
        }

        if( p->has_item( *it ) ) {
            if( p->has_active_bionic( bionic_id( "bio_shock" ) ) && p->power_level >= 2 && one_in( 5 ) ) {
                p->charge_power( -2 );

                it->item_tags.erase( "NO_UNWIELD" );
                it->ammo_unset();
                it->active = false;
                add_msg( m_good, _( "The %s crackle with electricity from your bionic, then come off your hands!" ),
                         it->tname().c_str() );

                return it->type->charges_to_use();
            }
        }

        if( calendar::once_every( 1_minutes ) ) {
            sounds::sound( pos, 10, _( "a police siren, whoop WHOOP." ) );
        }

        const int x = it->get_var( "HANDCUFFS_X", 0 );
        const int y = it->get_var( "HANDCUFFS_Y", 0 );

        if( ( it->ammo_remaining() > it->type->maximum_charges() - 1000 ) && ( x != pos.x ||
                y != pos.y ) ) {

            if( p->has_item( *it ) && p->weapon.typeId() == "e_handcuffs" ) {

                if( p->is_elec_immune() ) {
                    if( one_in( 10 ) ) {
                        add_msg( m_good, _( "The cuffs try to shock you, but you're protected from electricity." ) );
                    }
                } else {
                    add_msg( m_bad, _( "Ouch, the cuffs shock you!" ) );

                    p->apply_damage( nullptr, bp_arm_l, rng( 0, 2 ) );
                    p->apply_damage( nullptr, bp_arm_r, rng( 0, 2 ) );
                    p->mod_pain( rng( 2, 5 ) );

                }

            } else {
                add_msg( m_bad, _( "The %s spark with electricity!" ), it->tname().c_str() );
            }

            it->charges -= 50;
            if( it->charges < 1 ) {
                it->charges = 1;
            }

            it->set_var( "HANDCUFFS_X", pos.x );
            it->set_var( "HANDCUFFS_Y", pos.y );

            return it->type->charges_to_use();

        }

        return it->type->charges_to_use();

    }

    if( it->active ) {
        add_msg( _( "The %s are clamped tightly on your wrists.  You can't take them off." ),
                 it->tname().c_str() );
    } else {
        add_msg( _( "The %s have discharged and can be taken off." ), it->tname().c_str() );
    }

    return it->type->charges_to_use();
}

int iuse::radiocar( player *p, item *it, bool, const tripoint & )
{
    int choice = -1;
    if( it->contents.empty() ) {
        choice = menu( true, _( "Using RC car:" ), _( "Turn on" ),
                       _( "Put a bomb to car" ), _( "Cancel" ), NULL );
    } else if( it->contents.size() == 1 ) {
        choice = menu( true, _( "Using RC car:" ), _( "Turn on" ),
                       it->contents.front().tname().c_str(), _( "Cancel" ), NULL );
    }
    if( choice == 3 ) {
        return 0;
    }

    if( choice == 1 ) { //Turn car ON
        if( !it->ammo_sufficient() ) {
            p->add_msg_if_player( _( "The RC car's batteries seem to be dead." ) );
            return 0;
        }

        item bomb;

        if( !it->contents.empty() ) {
            bomb = it->contents.front();
        }

        it->convert( "radio_car_on" ).active = true;

        if( !( bomb.is_null() ) ) {
            it->put_in( bomb );
        }

        p->add_msg_if_player(
            _( "You turned on your RC car, now place it on ground, and use radio control to play." ) );

        return 0;
    }

    if( choice == 2 ) {

        if( it->contents.empty() ) { //arming car with bomb
            int inventory_index = g->inv_for_flag( "RADIOCARITEM", _( "Arm what?" ) );
            item &put = p->i_at( inventory_index );
            if( put.is_null() ) {
                p->add_msg_if_player( m_info, _( "You do not have that item!" ) );
                return 0;
            }

            if( put.has_flag( "RADIOCARITEM" ) && ( put.volume() <= 1250_ml ||
                                                    ( put.weight() <= 2_kilogram ) ) ) {
                p->moves -= 300;
                p->add_msg_if_player( _( "You armed your RC car with %s." ),
                                      put.tname().c_str() );
                it->put_in( p->i_rem( inventory_index ) );
            } else if( !put.has_flag( "RADIOCARITEM" ) ) {
                p->add_msg_if_player( _( "RC car with %s ? How?" ),
                                      put.tname().c_str() );
            } else {
                p->add_msg_if_player( _( "Your %s is too heavy or bulky for this RC car." ),
                                      put.tname().c_str() );
            }
        } else { // Disarm the car
            p->moves -= 150;
            item &bomb = it->contents.front();

            p->inv.assign_empty_invlet( bomb, *p, true ); // force getting an invlet.
            p->i_add( bomb );
            it->contents.erase( it->contents.begin() );

            p->add_msg_if_player( _( "You disarmed your RC car" ) );
        }
    }

    return it->type->charges_to_use();
}

int iuse::radiocaron( player *p, item *it, bool t, const tripoint &pos )
{
    if( t ) {
        //~Sound of a radio controlled car moving around
        sounds::sound( pos, 6, _( "buzzz..." ) );

        return it->type->charges_to_use();
    } else if( !it->ammo_sufficient() ) {
        // Deactivate since other mode has an iuse too.
        it->active = false;
        return 0;
    }

    int choice = menu( true, _( "What to do with activated RC car?" ), _( "Turn off" ),
                       _( "Cancel" ), NULL );

    if( choice == 2 ) {
        return it->type->charges_to_use();
    }

    if( choice == 1 ) {
        item bomb;

        if( !it->contents.empty() ) {
            bomb = it->contents.front();
        }

        it->convert( "radio_car" ).active = false;

        if( !( bomb.is_null() ) ) {
            it->put_in( bomb );
        }

        p->add_msg_if_player( _( "You turned off your RC car" ) );
        return it->type->charges_to_use();
    }

    return it->type->charges_to_use();
}

void sendRadioSignal( player &p, const std::string &signal )
{
    for( size_t i = 0; i < p.inv.size(); i++ ) {
        item &it = p.inv.find_item( i );

        if( it.has_flag( "RADIO_ACTIVATION" ) && it.has_flag( signal ) ) {
            sounds::sound( p.pos(), 6, _( "beep." ) );

            if( it.has_flag( "RADIO_INVOKE_PROC" ) ) {
                // Invoke twice: first to transform, then later to proc
                it.type->invoke( p, it, p.pos() );
                it.ammo_unset();
                // The type changed
            }

            it.type->invoke( p, it, p.pos() );
        }
    }

    g->m.trigger_rc_items( signal );
}

int iuse::radiocontrol( player *p, item *it, bool t, const tripoint & )
{
    if( t ) {
        if( it->charges == 0 ) {
            it->active = false;
            p->remove_value( "remote_controlling" );
        } else if( p->get_value( "remote_controlling" ).empty() ) {
            it->active = false;
        }

        return it->type->charges_to_use();
    }

    int choice = -1;
    const char *car_action = NULL;

    if( !it->active ) {
        car_action = _( "Take control of RC car" );
    } else {
        car_action = _( "Stop controlling RC car" );
    }

    choice = menu( true, _( "What to do with radio control?" ), _( "Nothing" ), car_action,
                   _( "Press red button" ), _( "Press blue button" ), _( "Press green button" ), NULL );

    if( choice == 1 ) {
        return 0;
    } else if( choice == 2 ) {
        if( it->active ) {
            it->active = false;
            p->remove_value( "remote_controlling" );
        } else {
            std::list<std::pair<tripoint, item *>> rc_pairs = g->m.get_rc_items();
            tripoint rc_item_location = {999, 999, 999};
            // TODO: grab the closest car or similar?
            for( auto &rc_pairs_rc_pair : rc_pairs ) {
                if( rc_pairs_rc_pair.second->typeId() == "radio_car_on" &&
                    rc_pairs_rc_pair.second->active ) {
                    rc_item_location = rc_pairs_rc_pair.first;
                }
            }
            if( rc_item_location.x == 999 ) {
                p->add_msg_if_player( _( "No active RC cars on ground and in range." ) );
                return it->type->charges_to_use();
            } else {
                std::stringstream car_location_string;
                // Populate with the point and stash it.
                car_location_string << rc_item_location.x << ' ' <<
                                    rc_item_location.y << ' ' << rc_item_location.z;
                p->add_msg_if_player( m_good, _( "You take control of the RC car." ) );

                p->set_value( "remote_controlling", car_location_string.str() );
                it->active = true;
            }
        }
    } else if( choice > 2 ) {
        std::string signal = "RADIOSIGNAL_";
        std::stringstream choice_str;
        choice_str << ( choice - 2 );
        signal += choice_str.str();

        auto item_list = p->get_radio_items();
        for( auto &elem : item_list ) {
            if( ( elem )->has_flag( "BOMB" ) && ( elem )->has_flag( signal ) ) {
                p->add_msg_if_player( m_warning,
                                      _( "The %s in you inventory would explode on this signal.  Place it down before sending the signal." ),
                                      ( elem )->display_name().c_str() );
                return 0;
            }
        }

        p->add_msg_if_player( _( "Click." ) );
        sendRadioSignal( *p, signal );
        p->moves -= 150;
    }

    return it->type->charges_to_use();
}

static bool hackveh( player &p, item &it, vehicle &veh )
{
    if( !veh.is_locked || !veh.has_security_working() ) {
        return true;
    }
    bool advanced = veh.all_parts_with_feature( "REMOTE_CONTROLS", true ).size() > 0;
    if( advanced && veh.is_alarm_on ) {
        p.add_msg_if_player( m_bad, _( "This vehicle's security system has locked you out!" ) );
        return false;
    }

    /** @EFFECT_INT increases chance of bypassing vehicle security system */

    /** @EFFECT_COMPUTER increases chance of bypassing vehicle security system */
    int roll = dice( p.get_skill_level( skill_computer ) + 2, p.int_cur ) - ( advanced ? 50 : 25 );
    int effort = 0;
    bool success = false;
    if( roll < -20 ) { // Really bad rolls will trigger the alarm before you know it exists
        effort = 1;
        p.add_msg_if_player( m_bad, _( "You trigger the alarm!" ) );
        veh.is_alarm_on = true;
    } else if( roll >= 20 ) { // Don't bother the player if it's trivial
        effort = 1;
        p.add_msg_if_player( m_good, _( "You quickly bypass the security system!" ) );
        success = true;
    }

    if( effort == 0 && !query_yn( _( "Try to hack this car's security system?" ) ) ) {
        // Scanning for security systems isn't free
        p.moves -= 100;
        it.charges -= 1;
        return false;
    }

    p.practice( skill_computer, advanced ? 10 : 3 );
    if( roll < -10 ) {
        effort = rng( 4, 8 );
        p.add_msg_if_player( m_bad, _( "You waste some time, but fail to affect the security system." ) );
    } else if( roll < 0 ) {
        effort = 1;
        p.add_msg_if_player( m_bad, _( "You fail to affect the security system." ) );
    } else if( roll < 20 ) {
        effort = rng( 2, 8 );
        p.add_msg_if_player( m_mixed,
                             _( "You take some time, but manage to bypass the security system!" ) );
        success = true;
    }

    p.moves -= effort * 100;
    it.charges -= effort;
    if( success && advanced ) { // Unlock controls, but only if they're drive-by-wire
        veh.is_locked = false;
    }
    return success;
}

vehicle *pickveh( const tripoint &center, bool advanced )
{
    static const std::string ctrl = "CTRL_ELECTRONIC";
    static const std::string advctrl = "REMOTE_CONTROLS";
    uimenu pmenu;
    pmenu.title = _( "Select vehicle to access" );
    std::vector< vehicle * > vehs;

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
    for( int i = 0; i < ( int )vehs.size(); i++ ) {
        auto veh = vehs[i];
        locations.push_back( veh->global_pos3() );
        pmenu.addentry( i, true, MENU_AUTOASSIGN, veh->name.c_str() );
    }

    if( vehs.empty() ) {
        add_msg( m_bad, _( "No vehicle available." ) );
        return nullptr;
    }

    pmenu.addentry( vehs.size(), true, 'q', _( "Cancel" ) );
    pointmenu_cb callback( locations );
    pmenu.callback = &callback;
    pmenu.w_y = 0;
    pmenu.query();

    if( pmenu.ret < 0 || pmenu.ret >= ( int )vehs.size() ) {
        return nullptr;
    } else {
        return vehs[pmenu.ret];
    }
}

int iuse::remoteveh( player *p, item *it, bool t, const tripoint &pos )
{
    vehicle *remote = g->remoteveh();
    if( t ) {
        bool stop = false;
        if( it->charges == 0 ) {
            p->add_msg_if_player( m_bad, _( "The remote control's battery goes dead." ) );
            stop = true;
        } else if( remote == nullptr ) {
            p->add_msg_if_player( _( "Lost contact with the vehicle." ) );
            stop = true;
        } else if( remote->fuel_left( "battery", true ) == 0 ) {
            p->add_msg_if_player( m_bad, _( "The vehicle's battery died." ) );
            stop = true;
        }
        if( stop ) {
            it->active = false;
            g->setremoteveh( nullptr );
        }

        return it->type->charges_to_use();
    }

    bool controlling = it->active && remote != nullptr;
    int choice = menu( true, _( "What to do with remote vehicle control:" ), _( "Nothing" ),
                       controlling ? _( "Stop controlling the vehicle." ) : _( "Take control of a vehicle." ),
                       _( "Execute one vehicle action" ), NULL );

    if( choice < 2 || choice > 3 ) {
        return 0;
    }

    if( choice == 2 && controlling ) {
        it->active = false;
        g->setremoteveh( nullptr );
        return 0;
    }

    int px = g->u.view_offset.x;
    int py = g->u.view_offset.y;

    vehicle *veh = pickveh( pos, choice == 2 );

    if( veh == nullptr ) {
        return 0;
    }

    if( !hackveh( *p, *it, *veh ) ) {
        return 0;
    }

    if( choice == 2 ) {
        it->active = true;
        g->setremoteveh( veh );
        p->add_msg_if_player( m_good, _( "You take control of the vehicle." ) );
        if( !veh->engine_on ) {
            veh->start_engines();
        }
    } else if( choice == 3 ) {
        veh->use_controls( pos );
    }

    g->u.view_offset.x = px;
    g->u.view_offset.y = py;
    return it->type->charges_to_use();
}

bool multicooker_hallu( player &p )
{
    p.moves -= 200;
    const int random_hallu = rng( 1, 7 );
    std::vector<tripoint> points;
    switch( random_hallu ) {

        case 1:
            add_msg( m_info, _( "And when you gaze long into a screen, the screen also gazes into you." ) );
            return true;

        case 2:
            add_msg( m_bad, _( "The multi-cooker boiled your head!" ) );
            return true;

        case 3:
            add_msg( m_info, _( "The characters on the screen display an obscene joke.  Strange humor." ) );
            return true;

        case 4:
            //~ Single-spaced & lowercase are intentional, conveying hurried speech-KA101
            add_msg( m_warning, _( "Are you sure?! the multi-cooker wants to poison your food!" ) );
            return true;

        case 5:
            add_msg( m_info,
                     _( "The multi-cooker argues with you about the taste preferences.  You don't want to deal with it." ) );
            return true;

        case 6:
            for( const tripoint &pt : g->m.points_in_radius( p.pos(), 1 ) ) {
                if( g->is_empty( pt ) ) {
                    points.push_back( pt );
                }
            }

            if( !one_in( 5 ) ) {
                add_msg( m_warning, _( "The multi-cooker runs away!" ) );
                const tripoint random_point = random_entry( points );
                if( monster *const m = g->summon_mon( mon_hallu_multicooker, random_point ) ) {
                    m->hallucination = true;
                    m->add_effect( effect_run, 1_turns, num_bp, true );
                }
            } else {
                add_msg( m_bad, _( "You're surrounded by aggressive multi-cookers!" ) );

                for( auto &point : points ) {
                    if( monster *const m = g->summon_mon( mon_hallu_multicooker, point ) ) {
                        m->hallucination = true;
                    }
                }
            }
            return true;

        default:
            return false;
    }

}

int iuse::multicooker( player *p, item *it, bool t, const tripoint &pos )
{
    static const std::set<std::string> multicooked_subcats = { "CSC_FOOD_MEAT", "CSC_FOOD_VEGGI", "CSC_FOOD_PASTA" };

    if( t ) {
        if( it->charges == 0 ) {
            it->active = false;
            return 0;
        }

        int cooktime = it->get_var( "COOKTIME", 0 );
        cooktime -= 100;

        if( cooktime >= 300 && cooktime < 400 ) {
            //Smart or good cook or careful
            /** @EFFECT_INT increases chance of checking multi-cooker on time */

            /** @EFFECT_SURVIVAL increases chance of checking multi-cooker on time */
            if( p->int_cur + p->get_skill_level( skill_cooking ) + p->get_skill_level( skill_survival ) > 16 ) {
                add_msg( m_info, _( "The multi-cooker should be finishing shortly..." ) );
            }
        }

        if( cooktime <= 0 ) {
            item &meal = it->emplace_back( it->get_var( "DISH" ) );
            if( meal.has_flag( "EATEN_HOT" ) ) {
                meal.active = true;
                meal.item_tags.insert( "HOT" );
                meal.item_counter = 600;
            }

            it->active = false;
            it->erase_var( "DISH" );
            it->erase_var( "COOKTIME" );

            //~ sound of a multi-cooker finishing its cycle!
            sounds::sound( pos, 8, _( "ding!" ) );

            return 0;
        } else {
            it->set_var( "COOKTIME", cooktime );
            return 0;
        }

    } else {
        enum {
            mc_cancel, mc_start, mc_stop, mc_take, mc_upgrade
        };

        if( p->is_underwater() ) {
            p->add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
            return 0;
        }

        if( p->has_trait( trait_ILLITERATE ) ) {
            add_msg( m_info, _( "You cannot read, and don't understand the screen or the buttons!" ) );
            return 0;
        }

        if( p->has_effect( effect_hallu ) || p->has_effect( effect_visuals ) ) {
            if( multicooker_hallu( *p ) ) {
                return 0;
            }
        }

        if( p->has_trait( trait_HYPEROPIC ) && !p->worn_with_flag( "FIX_FARSIGHT" ) &&
            !p->has_effect( effect_contacts ) ) {
            add_msg( m_info, _( "You'll need to put on reading glasses before you can see the screen." ) );
            return 0;
        }

        uimenu menu;
        menu.selected = 0;
        menu.text = _( "Welcome to the RobotChef3000.  Choose option:" );

        menu.addentry( mc_cancel, true, 'q', _( "Cancel" ) );

        if( it->active ) {
            menu.addentry( mc_stop, true, 's', _( "Stop cooking" ) );
        } else {
            if( it->contents.empty() ) {
                if( it->ammo_remaining() < 50 ) {
                    p->add_msg_if_player( _( "Batteries are low." ) );
                    return 0;
                }
                menu.addentry( mc_start, true, 's', _( "Start cooking" ) );

                /** @EFFECT_ELECTRONICS >3 allows multicooker upgrade */

                /** @EFFECT_FABRICATION >3 allows multicooker upgrade */
                if( p->get_skill_level( skill_electronics ) > 3 && p->get_skill_level( skill_fabrication ) > 3 ) {
                    const auto upgr = it->get_var( "MULTI_COOK_UPGRADE" );
                    if( upgr.empty() ) {
                        menu.addentry( mc_upgrade, true, 'u', _( "Upgrade multi-cooker" ) );
                    } else {
                        if( upgr == "UPGRADE" ) {
                            menu.addentry( mc_upgrade, false, 'u', _( "Multi-cooker already upgraded" ) );
                        } else {
                            menu.addentry( mc_upgrade, false, 'u', _( "Multi-cooker unable to upgrade" ) );
                        }
                    }
                }
            } else {
                menu.addentry( mc_take, true, 't', _( "Take out dish" ) );
            }
        }

        menu.query();
        int choice = menu.ret;

        if( mc_cancel == choice ) {
            return 0;
        }

        if( mc_stop == choice ) {
            if( query_yn( _( "Really stop cooking?" ) ) ) {
                it->active = false;
                it->erase_var( "DISH" );
                it->erase_var( "COOKTIME" );
            }
            return 0;
        }

        if( mc_take == choice ) {
            item &dish = it->contents.front();

            if( dish.has_flag( "HOT" ) ) {
                p->add_msg_if_player( m_good,
                                      _( "You got the dish from the multi-cooker.  The %s smells delicious." ),
                                      dish.tname( dish.charges, false ).c_str() );
            } else {
                p->add_msg_if_player( m_good, _( "You got the %s from the multi-cooker." ),
                                      dish.tname( dish.charges, false ).c_str() );
            }

            p->i_add( dish );
            it->contents.clear();

            return 0;
        }

        if( mc_start == choice ) {
            enum {
                d_cancel
            };

            uimenu dmenu;
            dmenu.selected = 0;
            dmenu.text = _( "Choose desired meal:" );

            dmenu.addentry( d_cancel, true, 'q', _( "Cancel" ) );

            std::vector<const recipe *> dishes;

            inventory crafting_inv = g->u.crafting_inventory();
            //add some tools and qualities. we can't add this qualities to json, because multicook must be used only by activating, not as component other crafts.
            crafting_inv.push_back( item( "hotplate", 0 ) ); //hotplate inside
            crafting_inv.push_back( item( "tongs", 0 ) ); //some recipes requires tongs
            crafting_inv.push_back( item( "toolset", 0 ) ); //toolset with CUT and other qualities inside
            crafting_inv.push_back( item( "pot", 0 ) ); //good COOK, BOIL, CONTAIN qualities inside

            int counter = 1;

            for( const auto &r : g->u.get_learned_recipes().in_category( "CC_FOOD" ) ) {
                if( multicooked_subcats.count( r->subcategory ) > 0 ) {
                    dishes.push_back( r );
                    const bool can_make = r->requirements().can_make_with_inventory( crafting_inv );

                    dmenu.addentry( counter++, can_make, -1, r->result_name() );
                }
            }

            dmenu.query();

            int choice = dmenu.ret;

            if( d_cancel == choice ) {
                return 0;
            } else {
                const recipe *meal = dishes[choice - 1];
                int mealtime;
                if( it->get_var( "MULTI_COOK_UPGRADE" ) == "UPGRADE" ) {
                    mealtime = meal->time;
                } else {
                    mealtime = meal->time * 2 ;
                }

                const int all_charges = 50 + mealtime / ( it->type->tool->turns_per_charge * 100 );

                if( it->ammo_remaining() < all_charges ) {

                    p->add_msg_if_player( m_warning,
                                          _( "The multi-cooker needs %d charges to cook this dish." ),
                                          all_charges );

                    return 0;
                }

                auto reqs = meal->requirements();
                for( auto it : reqs.get_components() ) {
                    p->consume_items( it );
                }

                it->set_var( "DISH", meal->result() );
                it->set_var( "COOKTIME", mealtime );

                p->add_msg_if_player( m_good,
                                      _( "The screen flashes blue symbols and scales as the multi-cooker begins to shake." ) );

                it->active = true;
                it->charges -= 50;

                p->practice( skill_cooking, meal->difficulty * 3 ); //little bonus

                return 0;
            }
        }

        if( mc_upgrade == choice ) {

            if( !p->has_morale_to_craft() ) {
                add_msg( m_info, _( "Your morale is too low to craft..." ) );
                return 0;
            }

            bool has_tools = true;

            const inventory &cinv = g->u.crafting_inventory();

            if( !cinv.has_amount( "soldering_iron", 1 ) ) {
                p->add_msg_if_player( m_warning, _( "You need a %s." ), item::nname( "soldering_iron" ).c_str() );
                has_tools = false;
            }

            static const quality_id SCREW_FINE( "SCREW_FINE" );
            if( !cinv.has_quality( SCREW_FINE ) ) {
                p->add_msg_if_player( m_warning, _( "You need an item with %s of 1 or more to disassemble this." ),
                                      SCREW_FINE.obj().name.c_str() );
                has_tools = false;
            }

            if( !has_tools ) {
                return 0;
            }

            p->practice( skill_electronics, rng( 5, 10 ) );
            p->practice( skill_fabrication, rng( 5, 10 ) );

            p->moves -= 700;

            /** @EFFECT_INT increases chance to successfully upgrade multi-cooker */

            /** @EFFECT_ELECTRONICS increases chance to successfully upgrade multi-cooker */

            /** @EFFECT_FABRICATION increases chance to successfully upgrade multi-cooker */
            if( p->get_skill_level( skill_electronics ) + p->get_skill_level( skill_fabrication ) + p->int_cur >
                rng( 20, 35 ) ) {

                p->practice( skill_electronics, rng( 5, 20 ) );
                p->practice( skill_fabrication, rng( 5, 20 ) );

                p->add_msg_if_player( m_good,
                                      _( "You've successfully upgraded the multi-cooker, master tinkerer!  Now it cooks faster!" ) );

                it->set_var( "MULTI_COOK_UPGRADE", "UPGRADE" );

                return 0;

            } else {

                if( !one_in( 5 ) ) {
                    p->add_msg_if_player( m_neutral,
                                          _( "You sagely examine and analyze the multi-cooker, but don't manage to accomplish anything." ) );
                } else {
                    p->add_msg_if_player( m_bad,
                                          _( "Your tinkering nearly breaks the multi-cooker!  Fortunately, it still works, but best to stop messing with it." ) );
                    it->set_var( "MULTI_COOK_UPGRADE", "DAMAGED" );
                }

                return 0;

            }

        }

    }

    return 0;
}

int iuse::cable_attach( player *p, item *it, bool, const tripoint & )
{
    std::string initial_state = it->get_var( "state", "attach_first" );

    if( initial_state == "attach_first" ) {
        tripoint posp;
        if( !choose_adjacent( _( "Attach cable to vehicle where?" ), posp ) ) {
            return 0;
        }
        const optional_vpart_position vp = g->m.veh_at( posp );
        auto ter = g->m.ter( posp );
        if( !vp && ter != t_chainfence ) {
            p->add_msg_if_player( _( "There's no vehicle there." ) );
            return 0;
        } else {
            const auto abspos = g->m.getabs( posp );
            it->active = true;
            it->set_var( "state", "pay_out_cable" );
            it->set_var( "source_x", abspos.x );
            it->set_var( "source_y", abspos.y );
            it->set_var( "source_z", g->get_levz() );
            it->process( p, p->pos(), false );
        }
        p->moves -= 15;
    } else if( initial_state == "pay_out_cable" ) {
        int choice = -1;
        uimenu kmenu;
        kmenu.selected = 0;
        kmenu.text = _( "Using cable:" );
        kmenu.addentry( 0, true, -1, _( "Attach loose end of the cable" ) );
        kmenu.addentry( 1, true, -1, _( "Detach and re-spool the cable" ) );
        kmenu.addentry( -1, true, 'q', _( "Cancel" ) );
        kmenu.query();
        choice = kmenu.ret;

        if( choice == -1 ) {
            return 0; // we did nothing.
        } else if( choice == 1 ) {
            it->reset_cable( p );
            return 0;
        }

        tripoint vpos;
        if( !choose_adjacent( _( "Attach cable to vehicle where?" ), vpos ) ) {
            return 0;
        }
        const optional_vpart_position target_vp = g->m.veh_at( vpos );
        if( !target_vp ) {
            p->add_msg_if_player( _( "There's no vehicle there." ) );
            return 0;
        } else {
            vehicle *const target_veh = &target_vp->vehicle();
            tripoint source_global( it->get_var( "source_x", 0 ),
                                    it->get_var( "source_y", 0 ),
                                    it->get_var( "source_z", 0 ) );
            tripoint source_local = g->m.getlocal( source_global );
            const optional_vpart_position source_vp = g->m.veh_at( source_local );
            vehicle *const source_veh = veh_pointer_or_null( source_vp );

            if( source_veh == target_veh ) {
                if( p != nullptr && p->has_item( *it ) ) {
                    p->add_msg_if_player( m_warning, _( "The %s already has access to its own electric system!" ),
                                          source_veh->name.c_str() );
                }
                return 0;
            }

            tripoint target_global = g->m.getabs( vpos );

            if( source_veh == nullptr ) {
                if( p != nullptr && p->has_item( *it ) ) {
                    p->add_msg_if_player( m_bad, _( "You notice the cable has come loose!" ) );
                }
                it->reset_cable( p );
                return 0;
            }

            const auto veh_part_coordinates = []( const vehicle & veh, const int part_num ) {
                return veh.parts[part_num].mount;
            };

            // TODO: make sure there is always a matching vpart id here. Maybe transform this into
            // a iuse_actor class, or add a check in item_factory.
            const vpart_id vpid( it->typeId() );

            point vcoords = veh_part_coordinates( *source_veh, source_vp->part_index() );
            vehicle_part source_part( vpid, vcoords.x, vcoords.y, item( *it ) );
            source_part.target.first = target_global;
            source_part.target.second = target_veh->real_global_pos3();
            source_veh->install_part( vcoords.x, vcoords.y, source_part );

            vcoords = veh_part_coordinates( *target_veh, target_vp->part_index() );
            vehicle_part target_part( vpid, vcoords.x, vcoords.y, item( *it ) );
            target_part.target.first = source_global;
            target_part.target.second = source_veh->real_global_pos3();
            target_veh->install_part( vcoords.x, vcoords.y, target_part );

            if( p != nullptr && p->has_item( *it ) ) {
                p->add_msg_if_player( m_good, _( "You link up the electric systems of the %1$s and the %2$s." ),
                                      source_veh->name.c_str(), target_veh->name.c_str() );
            }

            return 1; // Let the cable be destroyed.
        }
    }

    return 0;
}

int iuse::shavekit( player *p, item *it, bool, const tripoint & )
{
    if( !it->ammo_sufficient() ) {
        p->add_msg_if_player( _( "You need soap to use this." ) );
    } else {
        p->assign_activity( activity_id( "ACT_SHAVE" ), 3000 );
    }
    return it->type->charges_to_use();
}

int iuse::hairkit( player *p, item *it, bool, const tripoint & )
{
    p->assign_activity( activity_id( "ACT_HAIRCUT" ), 3000 );
    return it->type->charges_to_use();
}

int iuse::weather_tool( player *p, item *it, bool, const tripoint & )
{
    w_point const weatherPoint = *g->weather_precise;

    if( it->typeId() == "weather_reader" ) {
        p->add_msg_if_player( m_neutral, _( "The %s's monitor slowly outputs the data..." ),
                              it->tname().c_str() );
    }
    if( it->has_flag( "THERMOMETER" ) ) {
        if( it->typeId() == "thermometer" ) {
            p->add_msg_if_player( m_neutral, _( "The %1$s reads %2$s." ), it->tname().c_str(),
                                  print_temperature( g->get_temperature( g->u.pos() ) ).c_str() );
        } else {
            p->add_msg_if_player( m_neutral, _( "Temperature: %s." ),
                                  print_temperature( g->get_temperature( g->u.pos() ) ).c_str() );
        }
    }
    if( it->has_flag( "HYGROMETER" ) ) {
        if( it->typeId() == "hygrometer" ) {
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
        if( it->typeId() == "barometer" ) {
            p->add_msg_if_player(
                m_neutral, _( "The %1$s reads %2$s." ), it->tname().c_str(),
                print_pressure( ( int )weatherPoint.pressure ).c_str() );
        } else {
            p->add_msg_if_player( m_neutral, _( "Pressure: %s." ),
                                  print_pressure( ( int )weatherPoint.pressure ).c_str() );
        }
    }

    if( it->typeId() == "weather_reader" ) {
        int vehwindspeed = 0;
        if( optional_vpart_position vp = g->m.veh_at( p->pos() ) ) {
            vehwindspeed = abs( vp->vehicle().velocity / 100 ); // For mph
        }
        const oter_id &cur_om_ter = overmap_buffer.ter( p->global_omt_location() );
        /* windpower defined in internal velocity units (=.01 mph) */
        int windpower = int( 100.0f * get_local_windpower( weatherPoint.windpower + vehwindspeed,
                             cur_om_ter, g->is_sheltered( g->u.pos() ) ) );

        p->add_msg_if_player( m_neutral, _( "Wind Speed: %.1f %s." ),
                              convert_velocity( windpower, VU_WIND ),
                              velocity_units( VU_WIND ) );
        p->add_msg_if_player(
            m_neutral, _( "Feels Like: %s." ),
            print_temperature(
                get_local_windchill( weatherPoint.temperature, weatherPoint.humidity, windpower ) +
                g->get_temperature( g->u.pos() ) ).c_str() );
    }

    return 0;
}

int iuse::directional_hologram( player *p, item *it, bool, const tripoint &pos )
{
    if( it->is_armor() &&  !( p->is_worn( *it ) ) ) {
        p->add_msg_if_player( m_neutral, _( "You need to wear the %1$s before activating it." ),
                              it->tname().c_str() );
        return 0;
    }
    tripoint posp = pos;
    if( !choose_adjacent( _( "Choose hologram direction." ), posp ) ) {
        return 0;
    }
    if( !g->is_empty( posp ) ) {
        p->add_msg_if_player( m_info, _( "Can't create a hologram there." ) );
        return 0;
    }
    monster *const hologram = g->summon_mon( mon_hologram, posp );
    tripoint target = pos;
    target.x = p->posx() + 2 * SEEX * ( posp.x - p->posx() );
    target.y = p->posy() + 2 * SEEY * ( posp.y - p->posy() );
    hologram->set_dest( target );
    p->mod_moves( -100 );
    return it->type->charges_to_use();
}

int iuse::capture_monster_veh( player *p, item *it, bool, const tripoint &pos )
{
    if( !it->has_flag( "VEHICLE" ) ) {
        p->add_msg_if_player( m_info, _( "The %s must be installed in a vehicle before being loaded." ),
                              it->tname().c_str() );
        return 0;
    }
    capture_monster_act( p, it, 0, pos );
    return 0;
}

int iuse::capture_monster_act( player *p, item *it, bool, const tripoint &pos )
{
    if( it->has_var( "contained_name" ) ) {
        tripoint target;
        if( g->is_empty( pos ) ) {
            // It's been activated somewhere where there isn't a player or monster, good.
            target = pos;
        } else {
            if( it->has_flag( "PLACE_RANDOMLY" ) ) {
                std::vector<tripoint> valid;
                for( const tripoint &dest : g->m.points_in_radius( p->pos(), 1 ) ) {
                    if( g->is_empty( dest ) ) {
                        valid.push_back( dest );
                    }
                }
                if( valid.empty() ) {
                    p->add_msg_if_player( _( "There is no place to put the %s." ),
                                          it->get_var( "contained_name", "" ).c_str() );
                    return 0;
                }
                target = random_entry( valid );
            } else {
                const std::string query = string_format( _( "Place the %s where?" ),
                                          it->get_var( "contained_name", "" ).c_str() );
                if( !choose_adjacent( query, target ) ) {
                    return 0;
                }
                if( !g->is_empty( target ) ) {
                    p->add_msg_if_player( m_info, _( "You cannot place the %s there!" ),
                                          it->get_var( "contained_name", "" ).c_str() );
                    return 0;
                }
            }
        }
        monster new_monster;
        try {
            deserialize( new_monster, it->get_var( "contained_json", "" ) );
        } catch( const std::exception &e ) {
            debugmsg( _( "Error restoring monster: %s" ), e.what() );
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
        const std::string query = string_format( _( "Capture what with the %s?" ), it->tname().c_str() );
        if( !choose_adjacent( query, target ) ) {
            p->add_msg_if_player( m_info, _( "You cannot use a %s there." ), it->tname().c_str() );
            return 0;
        }
        // Capture the thing, if it's on the same square.
        if( const monster *const mon_ptr = g->critter_at<monster>( target ) ) {
            const monster &f = *mon_ptr;

            if( !it->has_property( "monster_size_capacity" ) ) {
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
                p->add_msg_if_player( m_info, _( "The %1$s is too big to put in your %2$s." ),
                                      f.type->nname().c_str(), it->tname().c_str() );
                return 0;
            }
            // TODO: replace this with some kind of melee check.
            int chance = f.hp_percentage() / 10;
            // A weaker monster is easier to capture.
            // If the monster is friendly, then put it in the item
            // without checking if it rolled a success.
            if( f.friendly != 0 || one_in( chance ) ) {
                it->set_var( "contained_json", serialize( f ) );
                it->set_var( "contained_name", f.type->nname() );
                it->set_var( "name", string_format( _( "%s holding %s" ), it->type->nname( 1 ).c_str(),
                                                    f.type->nname().c_str() ) );
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
                g->remove_zombie( f );
                return 0;
            } else {
                p->add_msg_if_player( m_bad, _( "The %1$s avoids your attempts to put it in the %2$s." ),
                                      f.type->nname().c_str(), it->type->nname( 1 ).c_str() );
            }
            p->moves -= 100;
        } else {
            add_msg( _( "The %s can't capture nothing" ), it->tname().c_str() );
            return 0;
        }
    }
    return 0;
}

int iuse::ladder( player *p, item *, bool, const tripoint & )
{
    if( !g->m.has_zlevels() ) {
        debugmsg( "Ladder can't be used in non-z-level mode" );
        return 0;
    }

    tripoint dirp;
    if( !choose_adjacent( _( "Put the ladder where?" ), dirp ) ) {
        return 0;
    }

    if( !g->is_empty( dirp ) || g->m.has_furn( dirp ) ) {
        p->add_msg_if_player( m_bad, _( "Can't place it there." ) );
        return 0;
    }

    p->add_msg_if_player( _( "You set down the ladder." ) );
    p->moves -= 500;
    g->m.furn_set( dirp, furn_str_id( "f_ladder" ) );
    return 1;
}

int iuse::washclothes( player *p, item *it, bool, const tripoint & )
{
    // Check that player isn't over volume limit as this might cause it to break... this is a hack.
    // TODO: find a better solution.
    if( p->volume_capacity() < p->volume_carried() ) {
        p->add_msg_if_player( _( "You're carrying too much to clean anything." ) );
        return 0;
    }
    if( it->charges < it->type->charges_to_use() ) {
        p->add_msg_if_player( _( "You need a cleansing agent to use this." ) );
        return 0;
    }

    player player = *p;

    player.inv.restack( player );

    const inventory_filter_preset preset( []( const item_location & location ) {
        return location->item_tags.find( "FILTHY" ) != location->item_tags.end();
    } );
    // TODO: this should also search surrounding area, not just player inventory.
    inventory_iuse_selector inv_s( player, _( "ITEMS TO CLEAN" ), preset );
    inv_s.add_character_items( player );
    inv_s.set_title( _( "Multiclean" ) );
    inv_s.set_hint( _( "To clean x items, type a number before selecting." ) );
    std::list<std::pair<int, int>> to_clean;
    if( inv_s.empty() ) {
        popup( std::string( _( "You have nothing to clean." ) ), PF_GET_KEY );
        to_clean = std::list<std::pair<int, int> >();
        return 0;
    }

    to_clean = inv_s.execute();
    if( to_clean.size() == 0 ) {
        return 0;
    }
    int required_water = 0;
    int time = 0;
    int required_cleanser = 0;

    // Determine if we have enough water and cleanser for all the items.
    for( std::pair<int, int> pair : to_clean ) {
        item mod = p->i_at( pair.first );
        if( pair.first == INT_MIN ) {
            p->add_msg_if_player( m_info, _( "Never mind." ) );
            return 0;
        }
        required_water += ( mod.volume() / 125_ml ) * pair.second;
        time += ( 1000 * mod.volume() / 250_ml ) * pair.second;
        required_cleanser += ( mod.volume() / 1000_ml ) * pair.second;
    }
    if( required_water < 1 ) {
        required_water = 1;
    }
    if( required_cleanser < 1 ) {
        required_cleanser = 1;
    }

    const inventory &crafting_inv = p->crafting_inventory();
    if( !crafting_inv.has_charges( "water", required_water ) &&
        !crafting_inv.has_charges( "water_clean", required_water ) ) {
        p->add_msg_if_player( _( "You need %1$i charges of water or clean water to wash these items." ),
                              required_water );
        return 0;
    } else if( !crafting_inv.has_charges( "soap", required_cleanser ) &&
               !crafting_inv.has_charges( "detergent", required_cleanser ) ) {
        p->add_msg_if_player( _( "You need %1$i charges of cleansing agent to wash these items." ),
                              required_cleanser );
        return 0;
    }
    // Assign the activity values.
    p->assign_activity( activity_id( "ACT_WASH" ), time );

    for( std::pair<int, int> pair : to_clean ) {
        p->activity.values.push_back( pair.first );
        p->activity.values.push_back( pair.second );
    }

    return 0;
}
int iuse::break_stick( player *p, item *it, bool, const tripoint & )
{
    p->moves -= 200;
    p->mod_stat( "stamina", -50.0f * p->stamina / p->get_stamina_max() );

    if( p->get_str() < 5 ) {
        p->add_msg_if_player( _( "You are too weak to even try." ) );
        return 0;
    } else if( p->get_str() <= rng( 5, 11 ) ) {
        p->add_msg_if_player(
            _( "You use all your strength, but the stick won't break.  Perhaps try again?" ) );
        return 0;
    }
    std::vector<item_comp> comps;
    comps.push_back( item_comp( it->typeId(), 1 ) );
    p->consume_items( comps );
    int chance = rng( 0, 100 );
    if( chance <= 20 ) {
        p->add_msg_if_player( _( "You try to break the stick in two, but it shatters into splinters." ) );
        g->m.spawn_item( p->pos(), "splinter", 2 );
        return 1;
    } else if( chance <= 40 ) {
        p->add_msg_if_player( _( "The stick breaks clean into two parts." ) );
        g->m.spawn_item( p->pos(), "stick", 2 );
        return 1;
    } else if( chance <= 100 ) {
        p->add_msg_if_player( _( "You break the stick, but one half shatters into splinters." ) );
        g->m.spawn_item( p->pos(), "stick", 1 );
        g->m.spawn_item( p->pos(), "splinter", 1 );
        return 1;
    }
    return 0;
}

int iuse::weak_antibiotic( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( _( "You take some %s." ), it->tname().c_str() );
    if( p->has_effect( effect_infected ) && !p->has_effect( effect_weak_antibiotic ) ) {
        p->add_msg_if_player( m_good, _( "The throbbing of the infection diminishes. Slightly." ) );
    }
    p->add_effect( effect_weak_antibiotic, 12_hours );
    return it->type->charges_to_use();
}

int iuse::strong_antibiotic( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( _( "You take some %s." ), it->tname().c_str() );
    if( p->has_effect( effect_infected ) && !p->has_effect( effect_strong_antibiotic ) ) {
        p->add_msg_if_player( m_good, _( "You feel much better - almost entirely." ) );
    }
    p->add_effect( effect_strong_antibiotic, 12_hours );
    return it->type->charges_to_use();
}

int iuse::panacea( player *p, item *it, bool, const tripoint & )
{
    p->add_msg_if_player( _( "You take some %s." ), it->tname().c_str() );
    if( !p->has_effect( effect_panacea ) ) {
        p->add_msg_if_player( m_good, _( "You feel AMAZING!" ) );
    }
    p->add_effect( effect_panacea, 1_minutes );
    return it->type->charges_to_use();
}

int iuse::disassemble( player *p, item *it, bool, const tripoint & )
{
    const int pos = p->inv.position_by_item( it );
    p->disassemble( *it, pos, false, false );
    return 0;
}
