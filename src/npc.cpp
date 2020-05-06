#include "npc.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>

#include "auto_pickup.h"
#include "avatar.h"
#include "basecamp.h"
#include "bodypart.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "clzones.h"
#include "compatibility.h"
#include "coordinate_conversions.h"
#include "damage.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "flat_set.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "int_id.h"
#include "item.h"
#include "item_contents.h"
#include "item_group.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "json.h"
#include "magic.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "math_defines.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "npc_class.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "player_activity.h"
#include "pldata.h"
#include "ret_val.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "tileray.h"
#include "trait_group.h"
#include "translations.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "visitable.h"
#include "vpart_position.h"

static const activity_id ACT_READ( "ACT_READ" );

static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_contacts( "contacts" );
static const efftype_id effect_controlled( "controlled" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_high( "high" );
static const efftype_id effect_infection( "infection" );
static const efftype_id effect_mending( "mending" );
static const efftype_id effect_npc_flee_player( "npc_flee_player" );
static const efftype_id effect_npc_suspend( "npc_suspend" );
static const efftype_id effect_pkill_l( "pkill_l" );
static const efftype_id effect_pkill1( "pkill1" );
static const efftype_id effect_pkill2( "pkill2" );
static const efftype_id effect_pkill3( "pkill3" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_riding( "riding" );

static const skill_id skill_archery( "archery" );
static const skill_id skill_barter( "barter" );
static const skill_id skill_bashing( "bashing" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_pistol( "pistol" );
static const skill_id skill_rifle( "rifle" );
static const skill_id skill_shotgun( "shotgun" );
static const skill_id skill_smg( "smg" );
static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_throw( "throw" );

static const bionic_id bio_eye_optic( "bio_eye_optic" );
static const bionic_id bio_memory( "bio_memory" );

static const trait_id trait_BEE( "BEE" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_DEBUG_MIND_CONTROL( "DEBUG_MIND_CONTROL" );
static const trait_id trait_HALLUCINATION( "HALLUCINATION" );
static const trait_id trait_HYPEROPIC( "HYPEROPIC" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_MUTE( "MUTE" );
static const trait_id trait_PROF_DICEMASTER( "PROF_DICEMASTER" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SCHIZOPHRENIC( "SCHIZOPHRENIC" );
static const trait_id trait_TERRIFYING( "TERRIFYING" );

static const std::string flag_NPC_SAFE( "NPC_SAFE" );

class monfaction;

void starting_clothes( npc &who, const npc_class_id &type, bool male );
void starting_inv( npc &who, const npc_class_id &type );

npc::npc()
    : restock( calendar::turn_zero )
    , companion_mission_time( calendar::before_time_starts )
    , companion_mission_time_ret( calendar::before_time_starts )
    , last_updated( calendar::turn )
{
    submap_coords = point_zero;
    position.x = -1;
    position.y = -1;
    position.z = 500;
    last_player_seen_pos = cata::nullopt;
    last_seen_player_turn = 999;
    wanted_item_pos = no_goal_point;
    guard_pos = no_goal_point;
    goal = no_goal_point;
    fetching_item = false;
    has_new_items = true;
    worst_item_value = 0;
    str_max = 0;
    dex_max = 0;
    int_max = 0;
    per_max = 0;
    marked_for_death = false;
    death_drops = true;
    dead = false;
    hit_by_player = false;
    hallucination = false;
    moves = 100;
    mission = NPC_MISSION_NULL;
    myclass = npc_class_id::NULL_ID();
    fac_id = faction_id::NULL_ID();
    patience = 0;
    attitude = NPCATT_NULL;

    *path_settings = pathfinding_settings( 0, 1000, 1000, 10, true, true, true, false, true );
    for( direction threat_dir : npc_threat_dir ) {
        ai_cache.threat_map[ threat_dir ] = 0.0f;
    }
}

standard_npc::standard_npc( const std::string &name, const tripoint &pos,
                            const std::vector<itype_id> &clothing,
                            int sk_lvl, int s_str, int s_dex, int s_int, int s_per )
{
    this->name = name;
    position = pos;

    str_cur = std::max( s_str, 0 );
    str_max = std::max( s_str, 0 );
    dex_cur = std::max( s_dex, 0 );
    dex_max = std::max( s_dex, 0 );
    per_cur = std::max( s_per, 0 );
    per_max = std::max( s_per, 0 );
    int_cur = std::max( s_int, 0 );
    int_max = std::max( s_int, 0 );

    recalc_hp();
    for( int i = 0; i < num_hp_parts; i++ ) {
        hp_cur[i] = hp_max[i];
    }
    for( auto &e : Skill::skills ) {
        set_skill_level( e.ident(), std::max( sk_lvl, 0 ) );
    }

    for( const auto &e : clothing ) {
        wear_item( item( e ) );
    }

    for( item &e : worn ) {
        if( e.has_flag( "VARSIZE" ) ) {
            e.item_tags.insert( "FIT" );
        }
    }
}

npc::npc( npc && ) = default;
npc &npc::operator=( npc && ) = default;

static std::map<string_id<npc_template>, npc_template> npc_templates;

void npc_template::load( const JsonObject &jsobj )
{
    npc_template tem;
    npc &guy = tem.guy;
    guy.idz = jsobj.get_string( "id" );
    guy.name.clear();
    jsobj.read( "name_unique", tem.name_unique );
    jsobj.read( "name_suffix", tem.name_suffix );
    if( jsobj.has_string( "gender" ) ) {
        if( jsobj.get_string( "gender" ) == "male" ) {
            tem.gender_override = gender::male;
        } else {
            tem.gender_override = gender::female;
        }
    } else {
        tem.gender_override = gender::random;
    }
    if( jsobj.has_string( "faction" ) ) {
        guy.set_fac_id( jsobj.get_string( "faction" ) );
    }

    if( jsobj.has_int( "class" ) ) {
        guy.myclass = npc_class::from_legacy_int( jsobj.get_int( "class" ) );
    } else if( jsobj.has_string( "class" ) ) {
        guy.myclass = npc_class_id( jsobj.get_string( "class" ) );
    }

    guy.set_attitude( static_cast<npc_attitude>( jsobj.get_int( "attitude" ) ) );
    guy.mission = static_cast<npc_mission>( jsobj.get_int( "mission" ) );
    guy.chatbin.first_topic = jsobj.get_string( "chat" );
    if( jsobj.has_string( "mission_offered" ) ) {
        guy.miss_ids.emplace_back( mission_type_id( jsobj.get_string( "mission_offered" ) ) );
    } else if( jsobj.has_array( "mission_offered" ) ) {
        for( const std::string line : jsobj.get_array( "mission_offered" ) ) {
            guy.miss_ids.emplace_back( mission_type_id( line ) );
        }
    }
    npc_templates.emplace( string_id<npc_template>( guy.idz ), std::move( tem ) );
}

void npc_template::reset()
{
    npc_templates.clear();
}

void npc_template::check_consistency()
{
    for( const auto &e : npc_templates ) {
        const auto &guy = e.second.guy;
        if( !guy.myclass.is_valid() ) {
            debugmsg( "Invalid NPC class %s", guy.myclass.c_str() );
        }
    }
}

template<>
bool string_id<npc_template>::is_valid() const
{
    return npc_templates.count( *this ) > 0;
}

template<>
const npc_template &string_id<npc_template>::obj() const
{
    const auto found = npc_templates.find( *this );
    if( found == npc_templates.end() ) {
        debugmsg( "Tried to get invalid npc: %s", c_str() );
        static const npc_template dummy{};
        return dummy;
    }
    return found->second;
}

void npc::load_npc_template( const string_id<npc_template> &ident )
{
    auto found = npc_templates.find( ident );
    if( found == npc_templates.end() ) {
        debugmsg( "Tried to get invalid npc: %s", ident.c_str() );
        return;
    }
    const npc_template &tem = found->second;
    const npc &tguy = tem.guy;

    idz = tguy.idz;
    myclass = npc_class_id( tguy.myclass );
    randomize( myclass );
    if( !tem.name_unique.empty() ) {
        name = tem.name_unique.translated();
    }
    if( !tem.name_suffix.empty() ) {
        //~ %1$s: npc name, %2$s: name suffix
        name = string_format( pgettext( "npc name", "%1$s, %2$s" ), name, tem.name_suffix );
    }
    if( tem.gender_override != npc_template::gender::random ) {
        male = tem.gender_override == npc_template::gender::male;
    }
    fac_id = tguy.fac_id;
    set_fac( fac_id );
    attitude = tguy.attitude;
    mission = tguy.mission;
    chatbin.first_topic = tguy.chatbin.first_topic;
    for( const mission_type_id &miss_id : tguy.miss_ids ) {
        add_new_mission( mission::reserve_new( miss_id, getID() ) );
    }
}

npc::~npc() = default;

void npc::randomize( const npc_class_id &type )
{
    if( !getID().is_valid() ) {
        setID( g->assign_npc_id() );
    }

    weapon   = item( "null", 0 );
    inv.clear();
    personality.aggression = rng( -10, 10 );
    personality.bravery    = rng( -3, 10 );
    personality.collector  = rng( -1, 10 );
    personality.altruism   = rng( -10, 10 );
    moves = 100;
    mission = NPC_MISSION_NULL;
    male = one_in( 2 );
    pick_name();

    if( !type.is_valid() ) {
        debugmsg( "Invalid NPC class %s", type.c_str() );
        myclass = npc_class_id::NULL_ID();
    } else if( type.is_null() ) {
        myclass = npc_class::random_common();
    } else {
        myclass = type;
    }

    const auto &the_class = myclass.obj();
    str_max = the_class.roll_strength();
    dex_max = the_class.roll_dexterity();
    int_max = the_class.roll_intelligence();
    per_max = the_class.roll_perception();

    for( auto &skill : Skill::skills ) {
        int level = myclass->roll_skill( skill.ident() );

        set_skill_level( skill.ident(), level );
    }

    if( type.is_null() ) { // Untyped; no particular specialization
    } else if( type == NC_EVAC_SHOPKEEP ) {
        personality.collector += rng( 1, 5 );

    } else if( type == NC_BARTENDER ) {
        personality.collector += rng( 1, 5 );

    } else if( type == NC_JUNK_SHOPKEEP ) {
        personality.collector += rng( 1, 5 );

    } else if( type == NC_ARSONIST ) {
        personality.aggression += rng( 0, 1 );
        personality.collector += rng( 0, 2 );

    } else if( type == NC_SOLDIER ) {
        personality.aggression += rng( 1, 3 );
        personality.bravery += rng( 0, 5 );

    } else if( type == NC_HACKER ) {
        personality.bravery -= rng( 1, 3 );
        personality.aggression -= rng( 0, 2 );

    } else if( type == NC_DOCTOR ) {
        personality.aggression -= rng( 0, 4 );
        cash += 10000 * rng( 0, 3 ) * rng( 0, 3 );

    } else if( type == NC_TRADER ) {
        personality.collector += rng( 1, 5 );
        cash += 25000 * rng( 1, 10 );

    } else if( type == NC_NINJA ) {
        personality.bravery += rng( 0, 3 );
        personality.collector -= rng( 1, 6 );
        // TODO: give ninja his styles back

    } else if( type == NC_COWBOY ) {
        personality.aggression += rng( 0, 2 );
        personality.bravery += rng( 1, 5 );

    } else if( type == NC_SCIENTIST ) {
        personality.aggression -= rng( 1, 5 );
        personality.bravery -= rng( 2, 8 );
        personality.collector += rng( 0, 2 );

    } else if( type == NC_BOUNTY_HUNTER ) {
        personality.aggression += rng( 1, 6 );
        personality.bravery += rng( 0, 5 );

    } else if( type == NC_THUG ) {
        personality.aggression += rng( 1, 6 );
        personality.bravery += rng( 0, 5 );

    } else if( type == NC_SCAVENGER ) {
        personality.aggression += rng( 1, 3 );
        personality.bravery += rng( 1, 4 );

    }
    //A universal barter boost to keep NPCs competitive with players
    //The int boost from trade wasn't active... now that it is, most
    //players will vastly outclass npcs in trade without a little help.
    mod_skill_level( skill_barter, rng( 2, 4 ) );

    recalc_hp();
    for( int i = 0; i < num_hp_parts; i++ ) {
        hp_cur[i] = hp_max[i];
    }
    starting_weapon( myclass );
    starting_clothes( *this, myclass, male );
    starting_inv( *this, myclass );
    has_new_items = true;
    clear_mutations();

    // Add fixed traits
    for( const auto &tid : trait_group::traits_from( myclass->traits ) ) {
        set_mutation( tid );
    }

    // Run mutation rounds
    for( const auto &mr : type->mutation_rounds ) {
        int rounds = mr.second.roll();
        for( int i = 0; i < rounds; ++i ) {
            mutate_category( mr.first );
        }
    }
    // Add bionics
    for( const auto &bl : type->bionic_list ) {
        int chance = bl.second;
        if( rng( 0, 100 ) <= chance ) {
            add_bionic( bl.first );
        }
    }
    // Add spells for magiclysm mod
    for( std::pair<spell_id, int> spell_pair : type->_starting_spells ) {
        this->magic.learn_spell( spell_pair.first, *this, true );
        spell &sp = this->magic.get_spell( spell_pair.first );
        while( sp.get_level() < spell_pair.second && !sp.is_max_level() ) {
            sp.gain_level();
        }
    }
}

void npc::randomize_from_faction( faction *fac )
{
    // Personality = aggression, bravery, altruism, collector
    set_fac( fac->id );
    randomize( npc_class_id::NULL_ID() );
}

void npc::set_fac( const faction_id &id )
{
    if( my_fac ) {
        my_fac->remove_member( getID() );
    }
    my_fac = g->faction_manager_ptr->get( id );
    if( my_fac ) {
        if( !is_fake() && !is_hallucination() ) {
            my_fac->add_to_membership( getID(), disp_name(), known_to_u );
        }
        fac_id = my_fac->id;
    } else {
        return;
    }
    apply_ownership_to_inv();
}

void npc::apply_ownership_to_inv()
{
    for( auto &e : inv_dump() ) {
        e->set_owner( *this );
    }
}

faction_id npc::get_fac_id() const
{
    return fac_id;
}

faction *npc::get_faction() const
{
    if( !my_fac ) {
        return g->faction_manager_ptr->get( faction_id( "no_faction" ) );
    }
    return my_fac;
}

// item id from group "<class-name>_<what>" or from fallback group
// may still be a null item!
static item random_item_from( const npc_class_id &type, const std::string &what,
                              const std::string &fallback )
{
    auto result = item_group::item_from( type.str() + "_" + what, calendar::turn );
    if( result.is_null() ) {
        result = item_group::item_from( fallback, calendar::turn );
    }
    return result;
}

// item id from "<class-name>_<what>" or from "npc_<what>"
static item random_item_from( const npc_class_id &type, const std::string &what )
{
    return random_item_from( type, what, "npc_" + what );
}

// item id from "<class-name>_<what>_<gender>" or from "npc_<what>_<gender>"
static item get_clothing_item( const npc_class_id &type, const std::string &what, bool male )
{
    item result;
    //Check if class has gendered clothing
    //Then check if it has an ungendered version
    //Only if all that fails, grab from the default class.
    if( male ) {
        result = random_item_from( type, what + "_male", "null" );
    } else {
        result = random_item_from( type, what + "_female", "null" );
    }
    if( result.is_null() ) {
        if( male ) {
            result = random_item_from( type, what, "npc_" + what + "_male" );
        } else {
            result = random_item_from( type, what, "npc_" + what + "_female" );
        }
    }

    return result;
}

void starting_clothes( npc &who, const npc_class_id &type, bool male )
{
    std::vector<item> ret;
    if( item_group::group_is_defined( type->worn_override ) ) {
        ret = item_group::items_from( type->worn_override );
    } else {
        ret.push_back( get_clothing_item( type, "pants", male ) );
        ret.push_back( get_clothing_item( type, "shirt", male ) );
        ret.push_back( get_clothing_item( type, "underwear_top", male ) );
        ret.push_back( get_clothing_item( type, "underwear_bottom", male ) );
        ret.push_back( get_clothing_item( type, "underwear_feet", male ) );
        ret.push_back( get_clothing_item( type, "shoes", male ) );
        ret.push_back( random_item_from( type, "gloves" ) );
        ret.push_back( random_item_from( type, "coat" ) );
        ret.push_back( random_item_from( type, "vest" ) );
        ret.push_back( random_item_from( type, "masks" ) );
        // Why is the alternative group not named "npc_glasses" but "npc_eyes"?
        ret.push_back( random_item_from( type, "glasses", "npc_eyes" ) );
        ret.push_back( random_item_from( type, "hat" ) );
        ret.push_back( random_item_from( type, "scarf" ) );
        ret.push_back( random_item_from( type, "storage" ) );
        ret.push_back( random_item_from( type, "holster" ) );
        ret.push_back( random_item_from( type, "belt" ) );
        ret.push_back( random_item_from( type, "wrist" ) );
        ret.push_back( random_item_from( type, "extra" ) );
    }

    for( item &it : who.worn ) {
        it.on_takeoff( who );
    }
    who.worn.clear();
    for( item &it : ret ) {
        if( it.has_flag( "VARSIZE" ) ) {
            it.item_tags.insert( "FIT" );
        }
        if( who.can_wear( it ).success() ) {
            it.on_wear( who );
            who.worn.push_back( it );
            it.set_owner( who );
        }
    }
}

void starting_inv( npc &who, const npc_class_id &type )
{
    std::list<item> res;
    who.inv.clear();
    if( item_group::group_is_defined( type->carry_override ) ) {
        who.inv += item_group::items_from( type->carry_override );
        return;
    }

    res.emplace_back( "lighter" );
    // If wielding a gun, get some additional ammo for it
    if( who.weapon.is_gun() ) {
        item ammo( who.weapon.ammo_default() );
        ammo = ammo.in_its_container();
        if( ammo.made_of( LIQUID ) ) {
            item container( "bottle_plastic" );
            container.put_in( ammo, item_pocket::pocket_type::CONTAINER );
            ammo = container;
        }

        // TODO: Move to npc_class
        // NC_COWBOY and NC_BOUNTY_HUNTER get 5-15 whilst all others get 3-6
        int qty = 1 + ( type == NC_COWBOY ||
                        type == NC_BOUNTY_HUNTER );
        qty = rng( qty, qty * 2 );

        while( qty-- != 0 && who.can_pickVolume( ammo ) ) {
            // TODO: give NPC a default magazine instead
            res.push_back( ammo );
        }
    }

    if( type == NC_ARSONIST ) {
        res.emplace_back( "molotov" );
    }

    int qty = ( type == NC_EVAC_SHOPKEEP ||
                type == NC_TRADER ) ? 5 : 2;
    qty = rng( qty, qty * 3 );

    while( qty-- != 0 ) {
        item tmp = random_item_from( type, "misc" ).in_its_container();
        if( !tmp.is_null() ) {
            if( !one_in( 3 ) && tmp.has_flag( "VARSIZE" ) ) {
                tmp.item_tags.insert( "FIT" );
            }
            if( who.can_pickVolume( tmp ) ) {
                res.push_back( tmp );
            }
        }
    }

    res.erase( std::remove_if( res.begin(), res.end(), [&]( const item & e ) {
        return e.has_flag( "TRADER_AVOID" );
    } ), res.end() );
    for( auto &it : res ) {
        it.set_owner( who );
    }
    who.inv += res;
}

void npc::revert_after_activity()
{
    mission = previous_mission;
    attitude = previous_attitude;
    activity = player_activity();
    current_activity_id = activity_id::NULL_ID();
    clear_destination();
    backlog.clear();
}

npc_mission npc::get_previous_mission()
{
    return previous_mission;
}

npc_attitude npc::get_previous_attitude()
{
    return previous_attitude;
}

bool npc::get_known_to_u()
{
    return known_to_u;
}

void npc::set_known_to_u( bool known )
{
    known_to_u = known;
    if( my_fac ) {
        my_fac->add_to_membership( getID(), disp_name(), known_to_u );
    }
}

void npc::setpos( const tripoint &pos )
{
    position = pos;
    const point pos_om_old = sm_to_om_copy( submap_coords );
    submap_coords.x = g->get_levx() + pos.x / SEEX;
    submap_coords.y = g->get_levy() + pos.y / SEEY;
    const point pos_om_new = sm_to_om_copy( submap_coords );
    if( !is_fake() && pos_om_old != pos_om_new ) {
        overmap &om_old = overmap_buffer.get( pos_om_old );
        overmap &om_new = overmap_buffer.get( pos_om_new );
        if( const auto ptr = om_old.erase_npc( getID() ) ) {
            om_new.insert_npc( ptr );
        } else {
            // Don't move the npc pointer around to avoid having two overmaps
            // with the same npc pointer
            debugmsg( "could not find npc %s on its old overmap", name );
        }
    }
}

void npc::travel_overmap( const tripoint &pos )
{
    const point pos_om_old = sm_to_om_copy( submap_coords );
    spawn_at_sm( pos );
    const point pos_om_new = sm_to_om_copy( submap_coords );
    if( global_omt_location() == goal ) {
        reach_omt_destination();
    }
    if( !is_fake() && pos_om_old != pos_om_new ) {
        overmap &om_old = overmap_buffer.get( pos_om_old );
        overmap &om_new = overmap_buffer.get( pos_om_new );
        if( const auto ptr = om_old.erase_npc( getID() ) ) {
            om_new.insert_npc( ptr );
        } else {
            // Don't move the npc pointer around to avoid having two overmaps
            // with the same npc pointer
            debugmsg( "could not find npc %s on its old overmap", name );
        }
    }
}

void npc::spawn_at_sm( const tripoint &p )
{
    spawn_at_precise( p.xy(), tripoint( rng( 0, SEEX - 1 ), rng( 0, SEEY - 1 ), p.z ) );
}

void npc::spawn_at_precise( const point &submap_offset, const tripoint &square )
{
    submap_coords = submap_offset;
    submap_coords.x += square.x / SEEX;
    submap_coords.y += square.y / SEEY;
    position.x = square.x % SEEX;
    position.y = square.y % SEEY;
    position.z = square.z;
}

tripoint npc::global_square_location() const
{
    return sm_to_ms_copy( submap_coords ) + tripoint( posx() % SEEX, posy() % SEEY, position.z );
}

void npc::place_on_map()
{
    // The global absolute position (in map squares) of the npc is *always*
    // "submap_coords.x * SEEX + posx() % SEEX" (analog for y).
    // The main map assumes that pos is in its own (local to the main map)
    // coordinate system. We have to change pos to match that assumption
    const int dmx = submap_coords.x - g->get_levx();
    const int dmy = submap_coords.y - g->get_levy();
    const int offset_x = position.x % SEEX;
    const int offset_y = position.y % SEEY;
    // value of "submap_coords.x * SEEX + posx()" is unchanged
    setpos( tripoint( offset_x + dmx * SEEX, offset_y + dmy * SEEY, posz() ) );

    if( g->is_empty( pos() ) || is_mounted() ) {
        return;
    }

    for( const tripoint &p : closest_tripoints_first( pos(), SEEX + 1 ) ) {
        if( g->is_empty( p ) ) {
            setpos( p );
            return;
        }
    }

    debugmsg( "Failed to place NPC in a valid location near (%d,%d,%d)", posx(), posy(), posz() );
}

skill_id npc::best_skill() const
{
    int highest_level = std::numeric_limits<int>::min();
    skill_id highest_skill( skill_id::NULL_ID() );

    for( const auto &p : *_skills ) {
        if( p.first.obj().is_combat_skill() ) {
            const int level = p.second.level();
            if( level > highest_level ) {
                highest_level = level;
                highest_skill = p.first;
            }
        }
    }

    return highest_skill;
}

int npc::best_skill_level() const
{
    int highest_level = std::numeric_limits<int>::min();

    for( const auto &p : *_skills ) {
        if( p.first.obj().is_combat_skill() ) {
            const int level = p.second.level();
            if( level > highest_level ) {
                highest_level = level;
            }
        }
    }

    return highest_level;
}

void npc::starting_weapon( const npc_class_id &type )
{
    if( item_group::group_is_defined( type->weapon_override ) ) {
        weapon = item_group::item_from( type->weapon_override, calendar::turn );
        return;
    }

    const skill_id best = best_skill();

    // if NPC has no suitable skills default to stabbing weapon
    if( !best || best == skill_stabbing ) {
        weapon = random_item_from( type, "stabbing", "survivor_stabbing" );
    } else if( best == skill_bashing ) {
        weapon = random_item_from( type, "bashing", "survivor_bashing" );
    } else if( best == skill_cutting ) {
        weapon = random_item_from( type, "cutting", "survivor_cutting" );
    } else if( best == skill_throw ) {
        weapon = random_item_from( type, "throw" );
    } else if( best == skill_archery ) {
        weapon = random_item_from( type, "archery" );
    } else if( best == skill_pistol ) {
        weapon = random_item_from( type, "pistol", "guns_pistol_common" );
    } else if( best == skill_shotgun ) {
        weapon = random_item_from( type, "shotgun", "guns_shotgun_common" );
    } else if( best == skill_smg ) {
        weapon = random_item_from( type, "smg", "guns_smg_common" );
    } else if( best == skill_rifle ) {
        weapon = random_item_from( type, "rifle", "guns_rifle_common" );
    }

    if( weapon.is_gun() ) {
        weapon.ammo_set( weapon.ammo_default() );
    }
    weapon.set_owner( get_faction()->id );
}

bool npc::can_read( const item &book, std::vector<std::string> &fail_reasons )
{
    if( !book.is_book() ) {
        fail_reasons.push_back( string_format( _( "This %s is not good reading material." ),
                                               book.tname() ) );
        return false;
    }
    player *pl = dynamic_cast<player *>( this );
    if( !pl ) {
        return false;
    }
    const auto &type = book.type->book;
    const skill_id &skill = type->skill;
    const int skill_level = pl->get_skill_level( skill );
    if( skill && skill_level < type->req ) {
        fail_reasons.push_back( string_format( _( "I'm not smart enough to read this book." ) ) );
        return false;
    }
    if( !skill || skill_level >= type->level ) {
        fail_reasons.push_back( string_format( _( "I won't learn anything from this book." ) ) );
        return false;
    }

    // Check for conditions that disqualify us
    if( type->intel > 0 && has_trait( trait_ILLITERATE ) ) {
        fail_reasons.emplace_back( _( "I can't read!" ) );
    } else if( has_trait( trait_HYPEROPIC ) && !worn_with_flag( "FIX_FARSIGHT" ) &&
               !has_effect( effect_contacts ) && !has_bionic( bio_eye_optic ) ) {
        fail_reasons.emplace_back( _( "I can't read without my glasses." ) );
    } else if( fine_detail_vision_mod() > 4 ) {
        // Too dark to read only applies if the player can read to himself
        fail_reasons.emplace_back( _( "It's too dark to read!" ) );
        return false;
    }
    return true;
}

int npc::time_to_read( const item &book, const player &reader ) const
{
    const auto &type = book.type->book;
    const skill_id &skill = type->skill;
    // The reader's reading speed has an effect only if they're trying to understand the book as they read it
    // Reading speed is assumed to be how well you learn from books (as opposed to hands-on experience)
    const bool try_understand = reader.fun_to_read( book ) ||
                                reader.get_skill_level( skill ) < type->level;
    int reading_speed = try_understand ? std::max( reader.read_speed(), read_speed() ) : read_speed();

    int retval = type->time * reading_speed;
    retval *= std::min( fine_detail_vision_mod(), reader.fine_detail_vision_mod() );

    if( type->intel > reader.get_int() && !reader.has_trait( trait_PROF_DICEMASTER ) ) {
        retval += type->time * ( type->intel - reader.get_int() ) * 100;
    }
    return retval;
}

void npc::finish_read( item &book )
{
    const auto &reading = book.type->book;
    if( !reading ) {
        revert_after_activity();
        return;
    }
    const skill_id &skill = reading->skill;
    // NPCs don't need to identify the book or learn recipes yet.
    // NPCs don't read to other NPCs yet.
    const bool display_messages = my_fac->id == faction_id( "your_followers" ) && g->u.sees( pos() );
    bool continuous = false; //whether to continue reading or not

    if( book_fun_for( book, *this ) != 0 ) {
        //Fun bonus is no longer calculated here.
        add_morale( MORALE_BOOK, book_fun_for( book, *this ) * 5, book_fun_for( book,
                    *this ) * 15, 1_hours, 30_minutes, true,
                    book.type );
    }

    book.mark_chapter_as_read( *this );

    if( skill && get_skill_level( skill ) < reading->level &&
        get_skill_level_object( skill ).can_train() ) {
        SkillLevel &skill_level = get_skill_level_object( skill );
        const int originalSkillLevel = skill_level.level();

        // Calculate experience gained
        /** @EFFECT_INT increases reading comprehension */
        // Enhanced Memory Banks modestly boosts experience
        int min_ex = std::max( 1, reading->time / 10 + get_int() / 4 );
        int max_ex = reading->time / 5 + get_int() / 2 - originalSkillLevel;
        if( has_active_bionic( bio_memory ) ) {
            min_ex += 2;
        }
        if( max_ex < 2 ) {
            max_ex = 2;
        }
        if( max_ex > 10 ) {
            max_ex = 10;
        }
        if( max_ex < min_ex ) {
            max_ex = min_ex;
        }
        const std::string &s = activity.get_str_value( 0, "1" );
        double penalty = strtod( s.c_str(), nullptr );
        min_ex *= ( originalSkillLevel + 1 ) * penalty;
        min_ex = std::max( min_ex, 1 );
        max_ex *= ( originalSkillLevel + 1 ) * penalty;
        max_ex = std::max( min_ex, max_ex );

        skill_level.readBook( min_ex, max_ex, reading->level );

        std::string skill_name = skill.obj().name();

        if( skill_level != originalSkillLevel ) {
            g->events().send<event_type::gains_skill_level>( getID(), skill, skill_level.level() );
            if( display_messages ) {
                add_msg( m_good, _( "%s increases their %s level." ), disp_name(), skill_name );
                // NPC reads until they gain a level, then stop.
                revert_after_activity();
                return;
            }
        } else {
            continuous = true;
            if( display_messages ) {
                add_msg( m_info, _( "%s learns a little about %s!" ), disp_name(), skill.obj().name() );
            }
        }

        if( ( skill_level == reading->level || !skill_level.can_train() ) ||
            ( ( has_trait( trait_SCHIZOPHRENIC ) ||
                has_artifact_with( AEP_SCHIZO ) ) && one_in( 25 ) ) ) {
            if( display_messages ) {
                add_msg( m_info, _( "%s can no longer learn from %s." ), disp_name(), book.type_name() );
            }
        }
    } else if( skill ) {
        if( display_messages ) {
            add_msg( m_info, _( "%s can no longer learn from %s." ), disp_name(), book.type_name() );
        }
    }

    // NPCs can't learn martial arts from manuals (yet)

    if( continuous ) {
        activity.set_to_null();
        player *pl = dynamic_cast<player *>( this );
        if( pl ) {
            start_read( book, pl );
        }
        if( activity ) {
            return;
        }
    }
    activity.set_to_null();
    revert_after_activity();
}

void npc::start_read( item &chosen, player *pl )
{
    const int time_taken = time_to_read( chosen, *pl );
    const double penalty = static_cast<double>( time_taken ) / time_to_read( chosen, *pl );
    player_activity act( ACT_READ, time_taken, 0, pl->getID().get_value() );
    act.targets.emplace_back( item_location( *this, &chosen ) );
    act.str_values.push_back( to_string( penalty ) );
    // push an identifier of martial art book to the action handling
    if( chosen.type->use_methods.count( "MA_MANUAL" ) ) {
        act.str_values.clear();
        act.str_values.emplace_back( "martial_art" );
    }
    assign_activity( act );
}

void npc::do_npc_read()
{
    // Can read items from inventory or within one tile (including in vehicles)
    player *pl = dynamic_cast<player *>( this );
    if( !pl ) {
        return;
    }
    auto loc = game_menus::inv::read( *pl );

    if( loc ) {
        std::vector<std::string> fail_reasons;
        Character *ch = dynamic_cast<Character *>( pl );
        if( !ch ) {
            return;
        }
        item &chosen = *loc.obtain( *ch );
        if( can_read( chosen, fail_reasons ) ) {
            if( g->u.sees( pos() ) ) {
                add_msg( m_info, _( "%s starts reading." ), disp_name() );
            }
            start_read( chosen, pl );
        } else {
            for( const auto &elem : fail_reasons ) {
                say( elem );
            }
        }
    } else {
        add_msg( _( "Never mind." ) );
    }
}

bool npc::wear_if_wanted( const item &it, std::string &reason )
{
    // Note: this function isn't good enough to use with NPC AI alone
    // Restrict it to player's orders for now
    if( !it.is_armor() ) {
        reason = _( "This can't be worn." );
        return false;
    }

    // Splints ignore limits, but only when being equipped on a broken part
    // TODO: Drop splints when healed
    if( it.has_flag( "SPLINT" ) ) {
        for( int i = 0; i < num_hp_parts; i++ ) {
            hp_part hpp = static_cast<hp_part>( i );
            body_part bp = player::hp_to_bp( hpp );
            if( is_limb_broken( hpp ) && !has_effect( effect_mending, bp ) && it.covers( bp ) ) {
                reason = _( "Thanks, I'll wear that now." );
                return !!wear_item( it, false );
            }
        }
    }

    while( !worn.empty() ) {
        auto size_before = worn.size();
        // Strip until we can put the new item on
        // This is one of the reasons this command is not used by the AI
        if( can_wear( it ).success() ) {
            // TODO: Hazmat/power armor makes this not work due to 1 boots/headgear limit

            if( !!wear_item( it, false ) ) {
                reason = _( "Thanks, I'll wear that now." );
                return true;
            } else {
                reason = _( "I tried but couldn't wear it." );
                return false;
            }
        }
        // Otherwise, maybe we should take off one or more items and replace them
        bool took_off = false;
        for( const body_part bp : all_body_parts ) {
            if( !it.covers( bp ) ) {
                continue;
            }
            // Find an item that covers the same body part as the new item
            auto iter = std::find_if( worn.begin(), worn.end(), [bp]( const item & armor ) {
                return armor.covers( bp );
            } );
            if( iter != worn.end() && !( is_limb_broken( bp_to_hp( bp ) ) && iter->has_flag( "SPLINT" ) ) ) {
                took_off = takeoff( *iter );
                break;
            }
        }

        if( !took_off || worn.size() >= size_before ) {
            // Shouldn't happen, but does
            reason = _( "I tried but couldn't wear it." );
            return false;
        }
    }
    reason = _( "Thanks, I'll wear that now." );
    return worn.empty() && wear_item( it, false );
}

void npc::stow_item( item &it )
{
    if( wear_item( weapon, false ) ) {
        // Wearing the item was successful, remove weapon and post message.
        if( g->u.sees( pos() ) ) {
            add_msg_if_npc( m_info, _( "<npcname> wears the %s." ), weapon.tname() );
        }
        remove_weapon();
        moves -= 15;
        // Weapon cannot be worn or wearing was not successful. Store it in inventory if possible,
        // otherwise drop it.
        return;
    }
    for( auto &e : worn ) {
        if( e.can_holster( it ) ) {
            if( g->u.sees( pos() ) ) {
                //~ %1$s: weapon name, %2$s: holster name
                add_msg_if_npc( m_info, _( "<npcname> puts away the %1$s in the %2$s." ),
                                weapon.tname(), e.tname() );
            }
            auto ptr = dynamic_cast<const holster_actor *>( e.type->get_use( "holster" )->get_actor_ptr() );
            ptr->store( *this, e, it );
            return;
        }
    }
    if( volume_carried() + weapon.volume() <= volume_capacity() ) {
        if( g->u.sees( pos() ) ) {
            add_msg_if_npc( m_info, _( "<npcname> puts away the %s." ), weapon.tname() );
        }
        i_add( remove_weapon() );
        moves -= 15;
    } else { // No room for weapon, so we drop it
        if( g->u.sees( pos() ) ) {
            add_msg_if_npc( m_info, _( "<npcname> drops the %s." ), weapon.tname() );
        }
        g->m.add_item_or_charges( pos(), remove_weapon() );
    }
}

bool npc::wield( item &it )
{
    cached_info.erase( "weapon_value" );
    if( is_armed() ) {
        stow_item( weapon );
    }

    if( it.is_null() ) {
        weapon = item();
        return true;
    }

    // check if the item is in a holster
    int position = inv.position_by_item( &it );
    if( position != INT_MIN ) {
        item &maybe_holster = inv.find_item( position );
        assert( !maybe_holster.is_null() );
        if( &maybe_holster != &it && maybe_holster.is_holster() ) {
            assert( !maybe_holster.contents.empty() );
            const size_t old_size = maybe_holster.contents.num_item_stacks();
            invoke_item( &maybe_holster );
            // TODO: change invoke_item to somehow report this change
            // HACK: test whether wielding the item from the holster has been done.
            // (Wielding may be prevented by various reasons: see player::wield_contained)
            if( old_size != maybe_holster.contents.num_item_stacks() ) {
                return true;
            }
        }
    }

    moves -= 15;
    if( has_item( it ) ) {
        weapon = remove_item( it );
    } else {
        weapon = it;
    }

    if( g->u.sees( pos() ) ) {
        add_msg_if_npc( m_info, _( "<npcname> wields a %s." ),  weapon.tname() );
    }
    invalidate_range_cache();
    return true;
}

void npc::drop( const drop_locations &what, const tripoint &target,
                bool stash )
{
    Character::drop( what, target, stash );
    // TODO: Remove the hack. Its here because npcs didn't process activities, but they do now
    // so is this necessary?
    activity.do_turn( *this );
}

void npc::invalidate_range_cache()
{
    if( weapon.is_gun() ) {
        confident_range_cache = confident_shoot_range( weapon, get_most_accurate_sight( weapon ) );
    } else {
        confident_range_cache = weapon.reach_range( *this );
    }
}

void npc::form_opinion( const player &u )
{
    // FEAR
    if( u.weapon.is_gun() ) {
        // TODO: Make bows not guns
        if( weapon.is_gun() ) {
            op_of_u.fear += 2;
        } else {
            op_of_u.fear += 6;
        }
    } else if( u.weapon_value( u.weapon ) > 20 ) {
        op_of_u.fear += 2;
    } else if( !u.is_armed() ) {
        // Unarmed, but actually unarmed ("unarmed weapons" are not unarmed)
        op_of_u.fear -= 3;
    }

    ///\EFFECT_STR increases NPC fear of the player
    if( u.str_max >= 16 ) {
        op_of_u.fear += 2;
    } else if( u.str_max >= 12 ) {
        op_of_u.fear += 1;
    } else if( u.str_max <= 3 ) {
        op_of_u.fear -= 3;
    } else if( u.str_max <= 5 ) {
        op_of_u.fear -= 1;
    }

    for( int i = 0; i < num_hp_parts; i++ ) {
        if( u.hp_cur[i] <= u.hp_max[i] / 2 ) {
            op_of_u.fear--;
        }
        if( hp_cur[i] <= hp_max[i] / 2 ) {
            op_of_u.fear++;
        }
    }

    if( u.has_trait( trait_SAPIOVORE ) ) {
        op_of_u.fear += 10; // Sapiovores = Scary
    }
    if( u.has_trait( trait_TERRIFYING ) ) {
        op_of_u.fear += 6;
    }

    int u_ugly = 0;
    for( trait_id &mut : u.get_mutations() ) {
        u_ugly += mut.obj().ugliness;
    }
    op_of_u.fear += u_ugly / 2;
    op_of_u.trust -= u_ugly / 3;

    if( u.get_stim() > 20 ) {
        op_of_u.fear++;
    }

    if( u.has_effect( effect_drunk ) ) {
        op_of_u.fear -= 2;
    }

    // TRUST
    if( op_of_u.fear > 0 ) {
        op_of_u.trust -= 3;
    } else {
        op_of_u.trust += 1;
    }

    if( u.weapon.is_gun() ) {
        op_of_u.trust -= 2;
    } else if( !u.is_armed() ) {
        op_of_u.trust += 2;
    }

    // TODO: More effects
    if( u.has_effect( effect_high ) ) {
        op_of_u.trust -= 1;
    }
    if( u.has_effect( effect_drunk ) ) {
        op_of_u.trust -= 2;
    }
    if( u.get_stim() > 20 || u.get_stim() < -20 ) {
        op_of_u.trust -= 1;
    }
    if( u.get_painkiller() > 30 ) {
        op_of_u.trust -= 1;
    }

    if( op_of_u.trust > 0 ) {
        // Trust is worth a lot right now
        op_of_u.trust /= 2;
    }

    // VALUE
    op_of_u.value = 0;
    for( int i = 0; i < num_hp_parts; i++ ) {
        if( hp_cur[i] < hp_max[i] * 0.8f ) {
            op_of_u.value++;
        }
    }
    decide_needs();
    for( auto &i : needs ) {
        if( i == need_food || i == need_drink ) {
            op_of_u.value += 2;
        }
    }

    if( op_of_u.fear < personality.bravery + 10 &&
        op_of_u.fear - personality.aggression > -10 && op_of_u.trust > -8 ) {
        set_attitude( NPCATT_TALK );
    } else if( op_of_u.fear - 2 * personality.aggression - personality.bravery < -30 ) {
        set_attitude( NPCATT_KILL );
    } else if( my_fac && my_fac->likes_u < -10 ) {
        if( is_player_ally() ) {
            mutiny();
        }
        set_attitude( NPCATT_KILL );
    } else {
        set_attitude( NPCATT_FLEE_TEMP );
    }

    add_msg( m_debug, "%s formed an opinion of u: %s", name, npc_attitude_id( attitude ) );
}

void npc::mutiny()
{
    if( !my_fac || !is_player_ally() ) {
        return;
    }
    const bool seen = g->u.sees( pos() );
    if( seen ) {
        add_msg( m_bad, _( "%s is tired of your incompetent leadership and abuse!" ), disp_name() );
    }
    // NPCs leaving your faction due to mistreatment further reduce their opinion of you
    if( my_fac->likes_u < -10 ) {
        op_of_u.trust += my_fac->respects_u / 10;
        op_of_u.anger += my_fac->likes_u / 10;
    }
    // NPCs leaving your faction for abuse reduce the hatred your (remaining) followers
    // feel for you, but also reduces their respect for you.
    my_fac->likes_u = std::max( 0, my_fac->likes_u / 2 + 10 );
    my_fac->respects_u -= 5;
    g->remove_npc_follower( getID() );
    set_fac( faction_id( "amf" ) );
    job.clear_all_priorities();
    if( assigned_camp ) {
        assigned_camp = cata::nullopt;
    }
    chatbin.first_topic = "TALK_STRANGER_NEUTRAL";
    set_attitude( NPCATT_NULL );
    say( _( "<follower_mutiny>  Adios, motherfucker!" ), sounds::sound_t::order );
    if( seen ) {
        my_fac->known_by_u = true;
    }
}

float npc::vehicle_danger( int radius ) const
{
    const tripoint from( posx() - radius, posy() - radius, posz() );
    const tripoint to( posx() + radius, posy() + radius, posz() );
    VehicleList vehicles = g->m.get_vehicles( from, to );

    int danger = 0;

    // TODO: check for most dangerous vehicle?
    for( size_t i = 0; i < vehicles.size(); ++i ) {
        const wrapped_vehicle &wrapped_veh = vehicles[i];
        if( wrapped_veh.v->is_moving() ) {
            // FIXME: this can't be the right way to do this
            float facing = wrapped_veh.v->face.dir();

            int ax = wrapped_veh.v->global_pos3().x;
            int ay = wrapped_veh.v->global_pos3().y;
            int bx = int( ax + std::cos( facing * M_PI / 180.0 ) * radius );
            int by = int( ay + std::sin( facing * M_PI / 180.0 ) * radius );

            // fake size
            /* This will almost certainly give the wrong size/location on customized
             * vehicles. This should just count frames instead. Or actually find the
             * size. */
            vehicle_part last_part = wrapped_veh.v->parts.back();
            int size = std::max( last_part.mount.x, last_part.mount.y );

            double normal = std::sqrt( static_cast<float>( ( bx - ax ) * ( bx - ax ) + ( by - ay ) *
                                       ( by - ay ) ) );
            int closest = static_cast<int>( std::abs( ( posx() - ax ) * ( by - ay ) - ( posy() - ay ) *
                                            ( bx - ax ) ) / normal );

            if( size > closest ) {
                danger = i;
            }
        }
    }
    return danger;
}

bool npc::turned_hostile() const
{
    return ( op_of_u.anger >= hostile_anger_level() );
}

int npc::hostile_anger_level() const
{
    return ( 20 + op_of_u.fear - personality.aggression );
}

void npc::make_angry()
{
    if( is_enemy() ) {
        return; // We're already angry!
    }

    // player allies that become angry should stop being player allies
    if( is_player_ally() ) {
        mutiny();
    }

    // Make associated faction, if any, angry at the player too.
    if( my_fac && my_fac->id != faction_id( "no_faction" ) && my_fac->id != faction_id( "amf" ) ) {
        my_fac->likes_u = std::min( -15, my_fac->likes_u - 5 );
        my_fac->respects_u = std::min( -15, my_fac->respects_u - 5 );
    }
    if( op_of_u.fear > 10 + personality.aggression + personality.bravery ) {
        set_attitude( NPCATT_FLEE_TEMP ); // We don't want to take u on!
    } else {
        set_attitude( NPCATT_KILL ); // Yeah, we think we could take you!
    }
}

void npc::on_attacked( const Creature &attacker )
{
    if( is_hallucination() ) {
        die( nullptr );
    }
    if( attacker.is_player() && !is_enemy() ) {
        make_angry();
        hit_by_player = true;
    }
}

int npc::assigned_missions_value()
{
    int ret = 0;
    for( auto &m : chatbin.missions_assigned ) {
        ret += m->get_value();
    }
    return ret;
}

std::vector<skill_id> npc::skills_offered_to( const player &p ) const
{
    std::vector<skill_id> ret;
    for( const auto &pair : *_skills ) {
        const skill_id &id = pair.first;
        if( p.get_skill_level( id ) < pair.second.level() ) {
            ret.push_back( id );
        }
    }
    return ret;
}

std::vector<matype_id> npc::styles_offered_to( const player &p ) const
{
    return p.martial_arts_data.get_unknown_styles( martial_arts_data );
}

void npc::decide_needs()
{
    double needrank[num_needs];
    for( auto &elem : needrank ) {
        elem = 20;
    }
    if( weapon.is_gun() ) {
        int ups_drain = weapon.get_gun_ups_drain();
        if( ups_drain > 0 ) {
            int ups_charges = charges_of( "UPS_off", ups_drain ) +
                              charges_of( "UPS_off", ups_drain );
            needrank[need_ammo] = static_cast<double>( ups_charges ) / ups_drain;
        } else {
            needrank[need_ammo] = get_ammo( ammotype( *weapon.type->gun->ammo.begin() ) ).size();
        }
        needrank[need_ammo] *= 5;
    }
    if( !base_location ) {
        needrank[need_safety] = 1;
    }

    needrank[need_weapon] = weapon_value( weapon );
    needrank[need_food] = 15 - get_hunger();
    needrank[need_drink] = 15 - get_thirst();
    invslice slice = inv.slice();
    for( auto &i : slice ) {
        item inventory_item = i->front();
        if( const item *food = inventory_item.get_food() ) {
            needrank[ need_food ] += nutrition_for( *food ) / 4.0;
            needrank[ need_drink ] += food->get_comestible()->quench / 4.0;
        }
    }
    needs.clear();
    size_t j;
    bool serious = false;
    for( int i = 1; i < num_needs; i++ ) {
        if( needrank[i] < 10 ) {
            serious = true;
        }
    }
    if( !serious ) {
        needs.push_back( need_none );
        needrank[0] = 10;
    }
    for( int i = 1; i < num_needs; i++ ) {
        if( needrank[i] < 20 ) {
            for( j = 0; j < needs.size(); j++ ) {
                if( needrank[i] < needrank[needs[j]] ) {
                    needs.insert( needs.begin() + j, static_cast<npc_need>( i ) );
                    j = needs.size() + 1;
                }
            }
            if( j == needs.size() ) {
                needs.push_back( static_cast<npc_need>( i ) );
            }
        }
    }
}

void npc::say( const std::string &line, const sounds::sound_t spriority ) const
{
    std::string formatted_line = line;
    parse_tags( formatted_line, g->u, *this );
    if( has_trait( trait_MUTE ) ) {
        return;
    }

    std::string sound = string_format( _( "%1$s saying \"%2$s\"" ), name, formatted_line );
    if( g->u.sees( *this ) && g->u.is_deaf() ) {
        add_msg( m_warning, _( "%1$s says something but you can't hear it!" ), name );
    }
    // Hallucinations don't make noise when they speak
    if( is_hallucination() ) {
        add_msg( _( "%1$s saying \"%2$s\"" ), name, formatted_line );
        return;
    }
    // Sound happens even if we can't hear it
    if( spriority == sounds::sound_t::order || spriority == sounds::sound_t::alert ) {
        sounds::sound( pos(), get_shout_volume(), spriority, sound, false, "speech",
                       male ? "NPC_m" : "NPC_f" );
    } else {
        sounds::sound( pos(), 16, sounds::sound_t::speech, sound, false, "speech",
                       male ? "NPC_m_loud" : "NPC_f_loud" );
    }
}

bool npc::wants_to_sell( const item &it ) const
{
    if( !it.is_owned_by( *this ) ) {
        return false;
    }
    const int market_price = it.price( true );
    return wants_to_sell( it, value( it, market_price ), market_price );
}

bool npc::wants_to_sell( const item &/*it*/, int at_price, int market_price ) const
{
    if( mission == NPC_MISSION_SHOPKEEP ) {
        return true;
    }

    if( is_player_ally() ) {
        return true;
    }

    // TODO: Base on inventory
    return at_price - market_price <= 50;
}

bool npc::wants_to_buy( const item &it ) const
{
    const int market_price = it.price( true );
    return wants_to_buy( it, value( it, market_price ), market_price );
}

bool npc::wants_to_buy( const item &/*it*/, int at_price, int /*market_price*/ ) const
{
    if( is_player_ally() ) {
        return true;
    }

    // TODO: Base on inventory
    return at_price >= 80;
}

// Will the NPC freely exchange items with the player?
bool npc::will_exchange_items_freely() const
{
    return is_player_ally();
}

// What's the maximum credit the NPC is willing to extend to the player?
// This is currently very scrooge-like; NPCs are only likely to extend a few dollars
// of credit at most.
int npc::max_credit_extended() const
{
    if( is_player_ally() ) {
        return INT_MAX;
    }

    const int credit_trust    = 50;
    const int credit_value    = 50;
    const int credit_fear     = 50;
    const int credit_altruism = 100;
    const int credit_anger    = -200;

    return std::max( 0,
                     op_of_u.trust * credit_trust +
                     op_of_u.value * credit_value +
                     op_of_u.fear  * credit_fear  +
                     personality.altruism * credit_altruism +
                     op_of_u.anger * credit_anger
                   );
}

// How much is the NPC willing to owe the player?
// This is much more generous, as it's the essentially the player holding the risk here.
int npc::max_willing_to_owe() const
{
    if( is_player_ally() ) {
        return INT_MAX;
    }

    const int credit_trust    = 10000;
    const int credit_value    = 10000;
    const int credit_fear     = 10000;
    const int credit_altruism = 0;
    const int credit_anger    = -10000;

    return std::max( 0,
                     op_of_u.trust * credit_trust +
                     op_of_u.value * credit_value +
                     op_of_u.fear  * credit_fear  +
                     personality.altruism * credit_altruism +
                     op_of_u.anger * credit_anger
                   );

}

void npc::shop_restock()
{
    if( ( restock != calendar::turn_zero ) && ( ( calendar::turn - restock ) < 3_days ) ) {
        return;
    }

    restock = calendar::turn + 3_days;
    if( is_player_ally() ) {
        return;
    }
    const Group_tag &from = myclass->get_shopkeeper_items();
    if( from == "EMPTY_GROUP" ) {
        return;
    }

    units::volume total_space = volume_capacity();
    if( mission == NPC_MISSION_SHOPKEEP ) {
        total_space = units::from_liter( 5000 );
    }

    std::list<item> ret;
    int shop_value = 75000;
    if( my_fac ) {
        shop_value = my_fac->wealth * 0.0075;
        if( mission == NPC_MISSION_SHOPKEEP && !my_fac->currency.empty() ) {
            item my_currency( my_fac->currency );
            if( !my_currency.is_null() ) {
                my_currency.set_owner( *this );
                int my_amount = rng( 5, 15 ) * shop_value / 100 / my_currency.price( true );
                for( int lcv = 0; lcv < my_amount; lcv++ ) {
                    ret.push_back( my_currency );
                }
            }
        }
    }

    int count = 0;
    bool last_item = false;
    while( shop_value > 0 && total_space > 0_ml && !last_item ) {
        item tmpit = item_group::item_from( from, calendar::turn );
        if( !tmpit.is_null() && total_space >= tmpit.volume() ) {
            tmpit.set_owner( *this );
            ret.push_back( tmpit );
            shop_value -= tmpit.price( true );
            total_space -= tmpit.volume();
            count += 1;
            last_item = count > 10 && one_in( 100 );
        }
    }

    has_new_items = true;
    inv.clear();
    inv.push_back( ret );
}

int npc::minimum_item_value() const
{
    // TODO: Base on inventory
    int ret = 20;
    ret -= personality.collector;
    return ret;
}

void npc::update_worst_item_value()
{
    worst_item_value = 99999;
    // TODO: Cache this
    int inv_val = inv.worst_item_value( this );
    if( inv_val < worst_item_value ) {
        worst_item_value = inv_val;
    }
}

int npc::value( const item &it ) const
{
    int market_price = it.price( true );
    return value( it, market_price );
}

int npc::value( const item &it, int market_price ) const
{
    if( it.is_dangerous() || ( it.has_flag( "BOMB" ) && it.active ) || it.made_of( LIQUID ) ) {
        // NPCs won't be interested in buying active explosives or spilled liquids
        return -1000;
    }

    // faction currency trades at market price
    if( my_fac && my_fac->currency == it.typeId() ) {
        return market_price;
    }

    int ret = 0;
    // TODO: Cache own weapon value (it can be a bit expensive to compute 50 times/turn)
    double weapon_val = weapon_value( it ) - weapon_value( weapon );
    if( weapon_val > 0 ) {
        ret += weapon_val;
    }

    if( it.is_food() ) {
        int comestval = 0;
        if( nutrition_for( it ) > 0 || it.get_comestible()->quench > 0 ) {
            comestval++;
        }
        if( get_hunger() > 40 ) {
            comestval += ( nutrition_for( it ) + get_hunger() - 40 ) / 6;
        }
        if( get_thirst() > 40 ) {
            comestval += ( it.get_comestible()->quench + get_thirst() - 40 ) / 4;
        }
        if( comestval > 0 && will_eat( it ).success() ) {
            ret += comestval;
        }
    }

    if( it.is_ammo() ) {
        if( weapon.is_gun() && weapon.ammo_types().count( it.ammo_type() ) ) {
            // TODO: magazines - don't count ammo as usable if the weapon isn't.
            ret += 14;
        }

        if( has_gun_for_ammo( it.ammo_type() ) ) {
            // TODO: consider making this cumulative (once was)
            ret += 14;
        }
    }

    if( it.is_book() ) {
        auto &book = *it.type->book;
        ret += book.fun;
        if( book.skill && get_skill_level( book.skill ) < book.level &&
            get_skill_level( book.skill ) >= book.req ) {
            ret += book.level * 3;
        }
    }

    // Practical item value is more important than price
    ret *= 50;

    // TODO: Sometimes we want more than one tool?  Also we don't want EVERY tool.
    if( it.is_tool() && !has_amount( it.typeId(), 1 ) ) {
        ret += market_price * 0.2; // 20% premium for fresh tools
    }
    ret += market_price;
    return ret;
}

void healing_options::clear_all()
{
    bandage = false;
    disinfect = false;
    bleed = false;
    bite = false;
    infect = false;
}

bool healing_options::all_false()
{
    return !any_true();
}

bool healing_options::any_true()
{
    return bandage || bleed || bite || infect || disinfect;
}

void healing_options::set_all()
{
    bandage = true;
    bleed = true;
    bite = true;
    infect = true;
    disinfect = true;
}

bool npc::has_healing_item( healing_options try_to_fix )
{
    return !get_healing_item( try_to_fix, true ).is_null();
}

healing_options npc::has_healing_options()
{
    healing_options try_to_fix;
    try_to_fix.set_all();
    return has_healing_options( try_to_fix );
}

healing_options npc::has_healing_options( healing_options try_to_fix )
{
    healing_options can_fix;
    can_fix.clear_all();
    healing_options *fix_p = &can_fix;

    visit_items( [&fix_p, try_to_fix]( item * node ) {
        const auto use = node->type->get_use( "heal" );
        if( use == nullptr ) {
            return VisitResponse::NEXT;
        }

        auto &actor = dynamic_cast<const heal_actor &>( *( use->get_actor_ptr() ) );
        if( try_to_fix.bandage && !fix_p->bandage && actor.bandages_power > 0.0f ) {
            fix_p->bandage = true;
        }
        if( try_to_fix.disinfect && !fix_p->disinfect && actor.disinfectant_power > 0.0f ) {
            fix_p->disinfect = true;
        }
        if( try_to_fix.bleed && !fix_p->bleed && actor.bleed > 0 ) {
            fix_p->bleed = true;
        }
        if( try_to_fix.bite && !fix_p->bite && actor.bite > 0 ) {
            fix_p->bite = true;
        }
        if( try_to_fix.infect && !fix_p->infect && actor.infect > 0 ) {
            fix_p->infect = true;
        }
        // if we've found items for everything we're looking for, we're done
        if( ( !try_to_fix.bandage || fix_p->bandage ) &&
            ( !try_to_fix.disinfect || fix_p->disinfect ) &&
            ( !try_to_fix.bleed || fix_p->bleed ) &&
            ( !try_to_fix.bite || fix_p->bite ) &&
            ( !try_to_fix.infect || fix_p->infect ) ) {
            return VisitResponse::ABORT;
        }

        return VisitResponse::NEXT;
    } );
    return can_fix;
}

item &npc::get_healing_item( healing_options try_to_fix, bool first_best )
{
    item *best = &null_item_reference();
    visit_items( [&best, try_to_fix, first_best]( item * node ) {
        const auto use = node->type->get_use( "heal" );
        if( use == nullptr ) {
            return VisitResponse::NEXT;
        }

        auto &actor = dynamic_cast<const heal_actor &>( *( use->get_actor_ptr() ) );
        if( ( try_to_fix.bandage && actor.bandages_power > 0.0f ) ||
            ( try_to_fix.disinfect && actor.disinfectant_power > 0.0f ) ||
            ( try_to_fix.bleed && actor.bleed > 0 ) ||
            ( try_to_fix.bite && actor.bite > 0 ) ||
            ( try_to_fix.infect && actor.infect > 0 ) ) {
            best = node;
            if( first_best ) {
                return VisitResponse::ABORT;
            }
        }

        return VisitResponse::NEXT;
    } );

    return *best;
}

bool npc::has_painkiller()
{
    return inv.has_enough_painkiller( get_pain() );
}

bool npc::took_painkiller() const
{
    return ( has_effect( effect_pkill1 ) || has_effect( effect_pkill2 ) ||
             has_effect( effect_pkill3 ) || has_effect( effect_pkill_l ) );
}

int npc::get_faction_ver() const
{
    return faction_api_version;
}

void npc::set_faction_ver( int new_version )
{
    faction_api_version = new_version;
}

bool npc::has_faction_relationship( const player &p, const npc_factions::relationship flag ) const
{
    faction *p_fac = p.get_faction();
    if( !my_fac || !p_fac ) {
        return false;
    }

    return my_fac->has_relationship( p_fac->id, flag );
}

bool npc::is_ally( const player &p ) const
{
    if( p.getID() == getID() ) {
        return true;
    }
    if( p.is_player() ) {
        if( my_fac && my_fac->id == faction_id( "your_followers" ) ) {
            return true;
        }
        if( faction_api_version < 2 ) {
            // legacy attitude support so let's be specific here
            if( attitude == NPCATT_FOLLOW || attitude == NPCATT_LEAD ||
                attitude == NPCATT_WAIT || mission == NPC_MISSION_ACTIVITY ||
                mission == NPC_MISSION_TRAVELLING || mission == NPC_MISSION_GUARD_ALLY ||
                has_companion_mission() ) {
                return true;
            }
        }
    } else {
        const npc &guy = dynamic_cast<const npc &>( p );
        if( my_fac && guy.get_faction() && my_fac->id == guy.get_faction()->id ) {
            return true;
        }
        if( faction_api_version < 2 ) {
            if( is_ally( g->u ) && guy.is_ally( g->u ) ) {
                return true;
            } else if( get_attitude_group( get_attitude() ) ==
                       guy.get_attitude_group( guy.get_attitude() ) ) {
                return true;
            }
        }
    }
    return false;
}

bool npc::is_player_ally() const
{
    return is_ally( g->u );
}

bool npc::is_friendly( const player &p ) const
{
    return is_ally( p ) || ( p.is_player() && ( is_walking_with() || is_player_ally() ) );
}

bool npc::is_minion() const
{
    return is_player_ally() && op_of_u.trust >= 5;
}

bool npc::guaranteed_hostile() const
{
    return is_enemy() || ( my_fac && my_fac->likes_u < -10 );
}

bool npc::is_walking_with() const
{
    return attitude == NPCATT_FOLLOW || attitude == NPCATT_LEAD || attitude == NPCATT_WAIT;
}

bool npc::is_obeying( const player &p ) const
{
    return ( p.is_player() && is_walking_with() && is_player_ally() ) ||
           ( is_ally( p ) && is_stationary( true ) );
}

bool npc::is_following() const
{
    return attitude == NPCATT_FOLLOW || attitude == NPCATT_WAIT;
}

bool npc::is_leader() const
{
    return attitude == NPCATT_LEAD;
}

bool npc::within_boundaries_of_camp() const
{
    const int x = global_omt_location().x;
    const int y = global_omt_location().y;
    for( int x2 = x - 3; x2 < x + 3; x2++ ) {
        for( int y2 = y - 3; y2 < y + 3; y2++ ) {
            cata::optional<basecamp *> bcp = overmap_buffer.find_camp( point( x2, y2 ) );
            if( bcp ) {
                return true;
            }
        }
    }
    return false;
}

bool npc::is_enemy() const
{
    return attitude == NPCATT_KILL || attitude == NPCATT_FLEE || attitude == NPCATT_FLEE_TEMP;
}

bool npc::is_stationary( bool include_guards ) const
{
    if( include_guards && is_guarding() ) {
        return true;
    }
    return mission == NPC_MISSION_SHELTER || mission == NPC_MISSION_SHOPKEEP ||
           has_effect( effect_infection );
}

bool npc::is_guarding( ) const
{
    return mission == NPC_MISSION_GUARD || mission == NPC_MISSION_GUARD_ALLY || is_patrolling();
}

bool npc::is_patrolling() const
{
    return mission == NPC_MISSION_GUARD_PATROL;
}

bool npc::has_player_activity() const
{
    return activity && mission == NPC_MISSION_ACTIVITY && attitude == NPCATT_ACTIVITY;
}

bool npc::is_travelling() const
{
    return mission == NPC_MISSION_TRAVELLING;
}

Creature::Attitude npc::attitude_to( const Creature &other ) const
{
    if( other.is_npc() || other.is_player() ) {
        const player &guy = dynamic_cast<const player &>( other );
        // check faction relationships first
        if( has_faction_relationship( guy, npc_factions::kill_on_sight ) ) {
            return A_HOSTILE;
        } else if( has_faction_relationship( guy, npc_factions::watch_your_back ) ) {
            return A_FRIENDLY;
        }
    }

    if( is_player_ally() ) {
        // Friendly NPCs share player's alliances
        return g->u.attitude_to( other );
    }

    if( other.is_npc() ) {
        // Hostile NPCs are also hostile towards player's allies
        if( is_enemy() && other.attitude_to( g->u ) == A_FRIENDLY ) {
            return A_HOSTILE;
        }

        return A_NEUTRAL;
    } else if( other.is_player() ) {
        // For now, make it symmetric.
        return other.attitude_to( *this );
    }

    // TODO: Get rid of the ugly cast without duplicating checks
    const monster &m = dynamic_cast<const monster &>( other );
    switch( m.attitude( this ) ) {
        case MATT_FOLLOW:
        case MATT_FPASSIVE:
        case MATT_IGNORE:
        case MATT_FLEE:
            return A_NEUTRAL;
        case MATT_FRIEND:
        case MATT_ZLAVE:
            return A_FRIENDLY;
        case MATT_ATTACK:
            return A_HOSTILE;
        case MATT_NULL:
        case NUM_MONSTER_ATTITUDES:
            break;
    }

    return A_NEUTRAL;
}

void npc::npc_dismount()
{
    if( !mounted_creature || !has_effect( effect_riding ) ) {
        add_msg( m_debug, "NPC %s tried to dismount, but they have no mount, or they are not riding",
                 disp_name() );
        return;
    }
    cata::optional<tripoint> pnt;
    for( const auto &elem : g->m.points_in_radius( pos(), 1 ) ) {
        if( g->is_empty( elem ) ) {
            pnt = elem;
            break;
        }
    }
    if( !pnt ) {
        add_msg( m_debug, "NPC %s could not find a place to dismount.", disp_name() );
        return;
    }
    remove_effect( effect_riding );
    if( mounted_creature->has_flag( MF_RIDEABLE_MECH ) &&
        !mounted_creature->type->mech_weapon.empty() ) {
        remove_item( weapon );
    }
    mounted_creature->remove_effect( effect_ridden );
    mounted_creature->add_effect( effect_controlled, 5_turns );
    mounted_creature = nullptr;
    setpos( *pnt );
    mod_moves( -100 );
}

int npc::smash_ability() const
{
    if( !is_hallucination() && ( !is_player_ally() || rules.has_flag( ally_rule::allow_bash ) ) ) {
        ///\EFFECT_STR_NPC increases smash ability
        return str_cur + weapon.damage_melee( DT_BASH );
    }

    // Not allowed to bash
    return 0;
}

float npc::danger_assessment()
{
    return ai_cache.danger_assessment;
}

float npc::average_damage_dealt()
{
    return static_cast<float>( melee_value( weapon ) );
}

bool npc::bravery_check( int diff )
{
    return ( dice( 10 + personality.bravery, 6 ) >= dice( diff, 4 ) );
}

bool npc::emergency() const
{
    return emergency( ai_cache.danger_assessment );
}

bool npc::emergency( float danger ) const
{
    return ( danger > ( personality.bravery * 3 * hp_percentage() ) / 100.0 );
}

//Check if this npc is currently in the list of active npcs.
//Active npcs are the npcs near the player that are actively simulated.
bool npc::is_active() const
{
    return g->critter_at<npc>( pos() ) == this;
}

int npc::follow_distance() const
{
    // HACK: If the player is standing on stairs, follow closely
    // This makes the stair hack less painful to use
    if( is_walking_with() &&
        ( g->m.has_flag( TFLAG_GOES_DOWN, g->u.pos() ) ||
          g->m.has_flag( TFLAG_GOES_UP, g->u.pos() ) ) ) {
        return 1;
    }
    // Uses ally_rule follow_distance_2 to determine if should follow by 2 or 4 tiles
    if( rules.has_flag( ally_rule::follow_distance_2 ) ) {
        return 2;
    }
    // If NPC doesn't see player, change follow distance to 2
    if( !sees( g->u ) ) {
        return 2;
    }
    return 4;
}

nc_color npc::basic_symbol_color() const
{
    if( attitude == NPCATT_KILL ) {
        return c_red;
    } else if( attitude == NPCATT_FLEE || attitude == NPCATT_FLEE_TEMP ) {
        return c_light_red;
    } else if( is_player_ally() ) {
        return c_green;
    } else if( is_walking_with() ) {
        return c_light_green;
    } else if( guaranteed_hostile() ) {
        return c_red;
    }
    return c_pink;
}

int npc::print_info( const catacurses::window &w, int line, int vLines, int column ) const
{
    const int last_line = line + vLines;
    const int iWidth = getmaxx( w ) - 2;
    // First line of w is the border; the next 4 are terrain info, and after that
    // is a blank line. w is 13 characters tall, and we can't use the last one
    // because it's a border as well; so we have lines 6 through 11.
    // w is also 48 characters wide - 2 characters for border = 46 characters for us
    mvwprintz( w, point( column, line++ ), c_white, _( "NPC: " ) );
    wprintz( w, basic_symbol_color(), name );

    if( sees( g->u ) ) {
        mvwprintz( w, point( column, line++ ), c_yellow, _( "Aware of your presence!" ) );
    }

    if( is_armed() ) {
        trim_and_print( w, point( column, line++ ), iWidth, c_red, _( "Wielding a %s" ), weapon.tname() );
    }

    const auto enumerate_print = [ w, last_line, column, iWidth, &line ]( const std::string & str_in,
    nc_color color ) {
        const std::vector<std::string> folded = foldstring( str_in, iWidth );
        for( auto it = folded.begin(); it < folded.end() && line < last_line; ++it, ++line ) {
            trim_and_print( w, point( column, line ), iWidth, color, *it );
        }
    };

    const std::string worn_str = enumerate_as_string( worn.begin(), worn.end(), []( const item & it ) {
        return it.tname();
    } );
    if( !worn_str.empty() ) {
        const std::string wearing = _( "Wearing: " ) + worn_str;
        enumerate_print( wearing, c_light_blue );
    }

    // as of now, visibility of mutations is between 0 and 10
    // 10 perception and 10 distance would see all mutations - cap 0
    // 10 perception and 30 distance - cap 5, some mutations visible
    // 3 perception and 3 distance would see all mutations - cap 0
    // 3 perception and 15 distance - cap 5, some mutations visible
    // 3 perception and 20 distance would be barely able to discern huge antlers on a person - cap 10
    const int per = g->u.get_per();
    const int dist = rl_dist( g->u.pos(), pos() );
    int visibility_cap;
    if( per <= 1 ) {
        visibility_cap = INT_MAX;
    } else {
        visibility_cap = std::round( dist * dist / 20.0 / ( per - 1 ) );
    }

    const auto trait_str = visible_mutations( visibility_cap );
    if( !trait_str.empty() ) {
        const std::string mutations = _( "Traits: " ) + trait_str;
        enumerate_print( mutations, c_green );
    }

    return line;
}

std::string npc::opinion_text() const
{
    std::string ret;
    if( op_of_u.trust <= -10 ) {
        ret += _( "Completely untrusting" );
    } else if( op_of_u.trust <= -6 ) {
        ret += _( "Very untrusting" );
    } else if( op_of_u.trust <= -3 ) {
        ret += _( "Untrusting" );
    } else if( op_of_u.trust <= 2 ) {
        ret += _( "Uneasy" );
    } else if( op_of_u.trust <= 4 ) {
        ret += _( "Trusting" );
    } else if( op_of_u.trust < 10 ) {
        ret += _( "Very trusting" );
    } else {
        ret += _( "Completely trusting" );
    }

    ret += string_format( _( " (Trust: %d); " ), op_of_u.trust );

    if( op_of_u.fear <= -10 ) {
        ret += _( "Thinks you're laughably harmless" );
    } else if( op_of_u.fear <= -6 ) {
        ret += _( "Thinks you're harmless" );
    } else if( op_of_u.fear <= -3 ) {
        ret += _( "Unafraid" );
    } else if( op_of_u.fear <= 2 ) {
        ret += _( "Wary" );
    } else if( op_of_u.fear <= 5 ) {
        ret += _( "Afraid" );
    } else if( op_of_u.fear < 10 ) {
        ret += _( "Very afraid" );
    } else {
        ret += _( "Terrified" );
    }

    ret += string_format( _( " (Fear: %d); " ), op_of_u.fear );

    if( op_of_u.value <= -10 ) {
        ret += _( "Considers you a major liability" );
    } else if( op_of_u.value <= -6 ) {
        ret += _( "Considers you a burden" );
    } else if( op_of_u.value <= -3 ) {
        ret += _( "Considers you an annoyance" );
    } else if( op_of_u.value <= 2 ) {
        ret += _( "Doesn't care about you" );
    } else if( op_of_u.value <= 5 ) {
        ret += _( "Values your presence" );
    } else if( op_of_u.value < 10 ) {
        ret += _( "Treasures you" );
    } else {
        ret += _( "Best Friends Forever!" );
    }

    ret += string_format( _( " (Value: %d); " ), op_of_u.value );

    if( op_of_u.anger <= -10 ) {
        ret += _( "You can do no wrong!" );
    } else if( op_of_u.anger <= -6 ) {
        ret += _( "You're good people" );
    } else if( op_of_u.anger <= -3 ) {
        ret += _( "Thinks well of you" );
    } else if( op_of_u.anger <= 2 ) {
        ret += _( "Ambivalent" );
    } else if( op_of_u.anger <= 5 ) {
        ret += _( "Pissed off" );
    } else if( op_of_u.anger < 10 ) {
        ret += _( "Angry" );
    } else {
        ret += _( "About to kill you" );
    }

    ret += string_format( _( " (Anger: %d)" ), op_of_u.anger );

    return ret;
}

static void maybe_shift( cata::optional<tripoint> &pos, const point &d )
{
    if( pos ) {
        *pos += d;
    }
}

static void maybe_shift( tripoint &pos, const point &d )
{
    if( pos != tripoint_min ) {
        pos += d;
    }
}

void npc::shift( const point &s )
{
    const point shift = sm_to_ms_copy( s );

    setpos( pos() - shift );

    maybe_shift( wanted_item_pos, point( -shift.x, -shift.y ) );
    maybe_shift( last_player_seen_pos, point( -shift.x, -shift.y ) );
    maybe_shift( pulp_location, point( -shift.x, -shift.y ) );
    path.clear();
}

bool npc::is_dead() const
{
    return dead || is_dead_state();
}

void npc::reboot()
{
    //The NPC got into an infinite loop, in game.cpp  -monmove() - a debugmsg just popped up
    // informing player of this.
    // put them to sleep and reboot their brain.
    // they can be woken up by the player, and if their brain is fixed, great,
    // if not, they will faint again, and the NPC can be kept asleep until the bug is fixed.
    cancel_activity();
    path.clear();
    last_player_seen_pos = cata::nullopt;
    last_seen_player_turn = 999;
    wanted_item_pos = no_goal_point;
    guard_pos = no_goal_point;
    goal = no_goal_point;
    fetching_item = false;
    has_new_items = true;
    worst_item_value = 0;
    mission = NPC_MISSION_NULL;
    patience = 0;
    ai_cache.danger = 0;
    ai_cache.total_danger = 0;
    ai_cache.danger_assessment = 0;
    ai_cache.target.reset();
    ai_cache.ally.reset();
    ai_cache.can_heal.clear_all();
    ai_cache.sound_alerts.clear();
    ai_cache.s_abs_pos = tripoint_zero;
    ai_cache.stuck = 0;
    ai_cache.guard_pos = cata::nullopt;
    ai_cache.my_weapon_value = 0;
    ai_cache.friends.clear();
    ai_cache.dangerous_explosives.clear();
    ai_cache.threat_map.clear();
    ai_cache.searched_tiles.clear();
    activity = player_activity();
    clear_destination();
    add_effect( effect_npc_suspend, 24_hours, num_bp, true, 1 );
}

void npc::die( Creature *nkiller )
{
    if( dead ) {
        // We are already dead, don't die again, note that npc::dead is
        // *only* set to true in this function!
        return;
    }
    if( assigned_camp ) {
        cata::optional<basecamp *> bcp = overmap_buffer.find_camp( ( *assigned_camp ).xy() );
        if( bcp ) {
            ( *bcp )->remove_assignee( getID() );
        }
    }
    assigned_camp = cata::nullopt;
    // Need to unboard from vehicle before dying, otherwise
    // the vehicle code cannot find us
    if( in_vehicle ) {
        g->m.unboard_vehicle( pos(), true );
    }
    if( is_mounted() ) {
        monster *critter = mounted_creature.get();
        critter->remove_effect( effect_ridden );
        critter->mounted_player = nullptr;
        critter->mounted_player_id = character_id();
    }
    // if this NPC was the only member of a micro-faction, clean it up.
    if( my_fac ) {
        if( !is_fake() && !is_hallucination() ) {
            if( my_fac->members.size() == 1 ) {
                for( auto elem : inv_dump() ) {
                    elem->remove_owner();
                    elem->remove_old_owner();
                }
            }
            my_fac->remove_member( getID() );
        }
    }
    dead = true;
    Character::die( nkiller );

    if( is_hallucination() ) {
        if( g->u.sees( *this ) ) {
            add_msg( _( "%s disappears." ), name.c_str() );
        }
        return;
    }

    if( g->u.sees( *this ) ) {
        add_msg( _( "%s dies!" ), name );
    }

    if( Character *ch = dynamic_cast<Character *>( killer ) ) {
        g->events().send<event_type::character_kills_character>( ch->getID(), getID(), get_name() );
    }

    if( killer == &g->u && ( !guaranteed_hostile() || hit_by_player ) ) {
        bool cannibal = g->u.has_trait( trait_CANNIBAL );
        bool psycho = g->u.has_trait( trait_PSYCHOPATH );
        if( g->u.has_trait( trait_SAPIOVORE ) || psycho ) {
            // No morale effect
        } else if( cannibal ) {
            g->u.add_morale( MORALE_KILLED_INNOCENT, -5, 0, 2_days, 3_hours );
        } else {
            g->u.add_morale( MORALE_KILLED_INNOCENT, -100, 0, 2_days, 3_hours );
        }
    }

    place_corpse();
}

std::string npc_attitude_id( npc_attitude att )
{
    static const std::map<npc_attitude, std::string> npc_attitude_ids = {
        { NPCATT_NULL, "NPCATT_NULL" },
        { NPCATT_TALK, "NPCATT_TALK" },
        { NPCATT_FOLLOW, "NPCATT_FOLLOW" },
        { NPCATT_LEAD, "NPCATT_LEAD" },
        { NPCATT_WAIT, "NPCATT_WAIT" },
        { NPCATT_MUG, "NPCATT_MUG" },
        { NPCATT_WAIT_FOR_LEAVE, "NPCATT_WAIT_FOR_LEAVE" },
        { NPCATT_KILL, "NPCATT_KILL" },
        { NPCATT_FLEE, "NPCATT_FLEE" },
        { NPCATT_FLEE_TEMP, "NPCATT_FLEE_TEMP" },
        { NPCATT_HEAL, "NPCATT_HEAL" },
        { NPCATT_ACTIVITY, "NPCATT_ACTIVITY" },
        { NPCATT_RECOVER_GOODS, "NPCATT_RECOVER_GOODS" },
        { NPCATT_LEGACY_1, "NPCATT_LEGACY_1" },
        { NPCATT_LEGACY_2, "NPCATT_LEGACY_2" },
        { NPCATT_LEGACY_3, "NPCATT_LEGACY_3" },
        { NPCATT_LEGACY_4, "NPCATT_LEGACY_4" },
        { NPCATT_LEGACY_5, "NPCATT_LEGACY_5" },
        { NPCATT_LEGACY_6, "NPCATT_LEGACY_6" },
    };
    const auto &iter = npc_attitude_ids.find( att );
    if( iter == npc_attitude_ids.end() ) {
        debugmsg( "Invalid attitude: %d", att );
        return "NPCATT_INVALID";
    }

    return iter->second;
}

std::string npc_attitude_name( npc_attitude att )
{
    switch( att ) {
        // Don't care/ignoring player
        case NPCATT_NULL:
            return _( "Ignoring" );
        // Move to and talk to player
        case NPCATT_TALK:
            return _( "Wants to talk" );
        // Follow the player
        case NPCATT_FOLLOW:
            return _( "Following" );
        // Lead the player, wait for them if they're behind
        case NPCATT_LEAD:
            return _( "Leading" );
        // Waiting for the player
        case NPCATT_WAIT:
            return _( "Waiting for you" );
        // Mug the player
        case NPCATT_MUG:
            return _( "Mugging you" );
        // Attack the player if our patience runs out
        case NPCATT_WAIT_FOR_LEAVE:
            return _( "Waiting for you to leave" );
        // Kill the player
        case NPCATT_KILL:
            return _( "Attacking to kill" );
        // Get away from the player
        case NPCATT_FLEE:
        case NPCATT_FLEE_TEMP:
            return _( "Fleeing" );
        // Get to the player and heal them
        case NPCATT_HEAL:
            return _( "Healing you" );
        case NPCATT_ACTIVITY:
            return _( "Performing a task" );
        case NPCATT_RECOVER_GOODS:
            return _( "Trying to recover stolen goods" );
        case NPCATT_LEGACY_1:
        case NPCATT_LEGACY_2:
        case NPCATT_LEGACY_3:
        case NPCATT_LEGACY_4:
        case NPCATT_LEGACY_5:
        case NPCATT_LEGACY_6:
            return _( "NPC Legacy Attitude" );
        default:
            break;
    }

    debugmsg( "Invalid attitude: %d", att );
    return _( "Unknown attitude" );
}

//message related stuff

//message related stuff
void npc::add_msg_if_npc( const std::string &msg ) const
{
    add_msg( replace_with_npc_name( msg ) );
}

void npc::add_msg_player_or_npc( const std::string &/*player_msg*/,
                                 const std::string &npc_msg ) const
{
    if( g->u.sees( *this ) ) {
        add_msg( replace_with_npc_name( npc_msg ) );
    }
}

void npc::add_msg_if_npc( const game_message_params &params, const std::string &msg ) const
{
    add_msg( params, replace_with_npc_name( msg ) );
}

void npc::add_msg_player_or_npc( const game_message_params &params,
                                 const std::string &/*player_msg*/,
                                 const std::string &npc_msg ) const
{
    if( g->u.sees( *this ) ) {
        add_msg( params, replace_with_npc_name( npc_msg ) );
    }
}

void npc::add_msg_player_or_say( const std::string &/*player_msg*/,
                                 const std::string &npc_speech ) const
{
    say( npc_speech );
}

void npc::add_msg_player_or_say( const game_message_params &/*params*/,
                                 const std::string &/*player_msg*/, const std::string &npc_speech ) const
{
    say( npc_speech );
}

void npc::add_new_mission( class mission *miss )
{
    chatbin.add_new_mission( miss );
}

void npc::on_unload()
{
}

// A throtled version of player::update_body since npc's don't need to-the-turn updates.
void npc::npc_update_body()
{
    if( calendar::once_every( 10_seconds ) ) {
        update_body( last_updated, calendar::turn );
        last_updated = calendar::turn;
    }
}

void npc::on_load()
{
    const auto advance_effects = [&]( const time_duration & elapsed_dur ) {
        for( auto &elem : *effects ) {
            for( auto &_effect_it : elem.second ) {
                effect &e = _effect_it.second;
                const time_duration &time_left = e.get_duration();
                if( time_left > 1_turns ) {
                    if( time_left < elapsed_dur ) {
                        e.set_duration( 1_turns );
                    } else {
                        e.set_duration( time_left - elapsed_dur );
                    }
                }
            }
        }
    };
    // Cap at some reasonable number, say 2 days
    const time_duration dt = std::min( calendar::turn - last_updated, 2_days );
    // TODO: Sleeping, healing etc.
    last_updated = calendar::turn;
    time_point cur = calendar::turn - dt;
    add_msg( m_debug, "on_load() by %s, %d turns", name, to_turns<int>( dt ) );
    // First update with 30 minute granularity, then 5 minutes, then turns
    for( ; cur < calendar::turn - 30_minutes; cur += 30_minutes + 1_turns ) {
        update_body( cur, cur + 30_minutes );
        advance_effects( 30_minutes );
    }
    for( ; cur < calendar::turn - 5_minutes; cur += 5_minutes + 1_turns ) {
        update_body( cur, cur + 5_minutes );
        advance_effects( 5_minutes );
    }
    for( ; cur < calendar::turn; cur += 1_turns ) {
        update_body( cur, cur + 1_turns );
        process_effects();
    }

    if( dt > 0_turns ) {
        // This ensures food is properly rotten at load
        // Otherwise NPCs try to eat rotten food and fail
        process_items();
        // give NPCs that are doing activities a pile of moves
        if( has_destination() || activity ) {
            mod_moves( to_moves<int>( dt ) );
        }
    }

    // Not necessarily true, but it's not a bad idea to set this
    has_new_items = true;

    // for spawned npcs
    if( g->m.has_flag( "UNSTABLE", pos() ) ) {
        add_effect( effect_bouldering, 1_turns, num_bp, true );
    } else if( has_effect( effect_bouldering ) ) {
        remove_effect( effect_bouldering );
    }
    if( g->m.veh_at( pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) && !in_vehicle ) {
        g->m.board_vehicle( pos(), this );
    }
    if( has_effect( effect_riding ) && !mounted_creature ) {
        if( const monster *const mon = g->critter_at<monster>( pos() ) ) {
            mounted_creature = g->shared_from( *mon );
        } else {
            add_msg( m_debug, "NPC is meant to be riding, though the mount is not found when %s is loaded",
                     disp_name() );
        }
    }
    if( has_trait( trait_HALLUCINATION ) ) {
        hallucination = true;
    }
}

void npc_chatbin::add_new_mission( mission *miss )
{
    if( miss == nullptr ) {
        return;
    }
    missions.push_back( miss );
}

constexpr tripoint npc::no_goal_point;

bool npc::query_yn( const std::string &/*msg*/ ) const
{
    // NPCs don't like queries - most of them are in the form of "Do you want to get hurt?".
    return false;
}

float npc::speed_rating() const
{
    float ret = get_speed() / 100.0f;
    ret *= 100.0f / run_cost( 100, false );

    return ret;
}

bool npc::dispose_item( item_location &&obj, const std::string & )
{
    using dispose_option = struct {
        int moves;
        std::function<void()> action;
    };

    std::vector<dispose_option> opts;

    for( auto &e : worn ) {
        if( e.can_holster( *obj ) ) {
            auto ptr = dynamic_cast<const holster_actor *>( e.type->get_use( "holster" )->get_actor_ptr() );
            opts.emplace_back( dispose_option {
                item_store_cost( *obj, e, false, e.contents.obtain_cost( *obj ) ),
                [this, ptr, &e, &obj]{ ptr->store( *this, e, *obj ); }
            } );
        }
    }

    if( volume_carried() + obj->volume() <= volume_capacity() ) {
        opts.emplace_back( dispose_option {
            item_handling_cost( *obj ),
            [this, &obj] {
                moves -= item_handling_cost( *obj );
                inv.add_item_keep_invlet( *obj );
                obj.remove_item();
                inv.unsort();
            }
        } );
    }

    if( opts.empty() ) {
        // Drop it
        g->m.add_item_or_charges( pos(), *obj );
        obj.remove_item();
        return true;
    }

    const auto mn = std::min_element( opts.begin(), opts.end(),
    []( const dispose_option & lop, const dispose_option & rop ) {
        return lop.moves < rop.moves;
    } );

    mn->action();
    return true;
}

void npc::process_turn()
{
    player::process_turn();

    // NPCs shouldn't be using stamina, but if they have, set it back to max
    if( calendar::once_every( 1_minutes ) && get_stamina() < get_stamina_max() ) {
        set_stamina( get_stamina_max() );
    }

    if( is_player_ally() && calendar::once_every( 1_hours ) &&
        get_hunger() < 200 && get_thirst() < 100 && op_of_u.trust < 5 ) {
        // Friends who are well fed will like you more
        // 24 checks per day, best case chance at trust 0 is 1 in 48 for +1 trust per 2 days
        float trust_chance = 5 - op_of_u.trust;
        // Penalize for bad impression
        // TODO: Penalize for traits and actions (especially murder, unless NPC is psycho)
        int op_penalty = std::max( 0, op_of_u.anger ) +
                         std::max( 0, -op_of_u.value ) +
                         std::max( 0, op_of_u.fear );
        // Being barely hungry and thirsty, not in pain and not wounded means good care
        int state_penalty = get_hunger() + get_thirst() + ( 100 - hp_percentage() ) + get_pain();
        if( x_in_y( trust_chance, 240 + 10 * op_penalty + state_penalty ) ) {
            op_of_u.trust++;
        }

        // TODO: Similar checks for fear and anger
    }

    // TODO: Add decreasing trust/value/etc. here when player doesn't provide food
    // TODO: Make NPCs leave the player if there's a path out of map and player is sleeping/unseen/etc.
}

bool npc::invoke_item( item *used, const tripoint &pt )
{
    const auto &use_methods = used->type->use_methods;

    if( use_methods.empty() ) {
        return false;
    } else if( use_methods.size() == 1 ) {
        return Character::invoke_item( used, use_methods.begin()->first, pt );
    }
    return false;
}

bool npc::invoke_item( item *used, const std::string &method )
{
    return Character::invoke_item( used, method );
}

bool npc::invoke_item( item *used )
{
    return Character::invoke_item( used );
}

std::array<std::pair<std::string, overmap_location_str_id>, npc_need::num_needs> npc::need_data = {
    {
        { "need_none", overmap_location_str_id( "source_of_anything" ) },
        { "need_ammo", overmap_location_str_id( "source_of_ammo" ) },
        { "need_weapon", overmap_location_str_id( "source_of_weapons" )},
        { "need_gun", overmap_location_str_id( "source_of_guns" ) },
        { "need_food", overmap_location_str_id( "source_of_food" )},
        { "need_drink", overmap_location_str_id( "source_of_drink" ) },
        { "need_safety", overmap_location_str_id( "source_of_safety" ) }
    }
};

std::string npc::get_need_str_id( const npc_need &need )
{
    return need_data[static_cast<size_t>( need )].first;
}

overmap_location_str_id npc::get_location_for( const npc_need &need )
{
    return need_data[static_cast<size_t>( need )].second;
}

std::ostream &operator<< ( std::ostream &os, const npc_need &need )
{
    return os << npc::get_need_str_id( need );
}

bool npc::will_accept_from_player( const item &it ) const
{
    if( is_hallucination() ) {
        return false;
    }

    if( is_minion() || g->u.has_trait( trait_DEBUG_MIND_CONTROL ) ||
        it.has_flag( flag_NPC_SAFE ) ) {
        return true;
    }

    if( !it.type->use_methods.empty() ) {
        return false;
    }

    if( it.is_comestible() ) {
        if( it.get_comestible_fun() < 0 || it.poison > 0 ) {
            return false;
        }
    }

    return true;
}

const pathfinding_settings &npc::get_pathfinding_settings() const
{
    return get_pathfinding_settings( false );
}

const pathfinding_settings &npc::get_pathfinding_settings( bool no_bashing ) const
{
    path_settings->bash_strength = no_bashing ? 0 : smash_ability();
    // TODO: Extract climb skill
    const int climb = std::min( 20, get_dex() );
    if( climb > 1 ) {
        // Success is !one_in(dex), so 0%, 50%, 66%, 75%...
        // Penalty for failure chance is 1/success = 1/(1-failure) = 1/(1-(1/dex)) = dex/(dex-1)
        path_settings->climb_cost = ( 10 - climb / 5 ) * climb / ( climb - 1 );
    } else {
        // Climbing at this dexterity will always fail
        path_settings->climb_cost = 0;
    }

    return *path_settings;
}

std::set<tripoint> npc::get_path_avoid() const
{
    std::set<tripoint> ret;
    for( Creature &critter : g->all_creatures() ) {
        // TODO: Cache this somewhere
        ret.insert( critter.pos() );
    }
    if( rules.has_flag( ally_rule::avoid_doors ) ) {
        for( const tripoint &p : g->m.points_in_radius( pos(), 30 ) ) {
            if( g->m.open_door( p, true, true ) ) {
                ret.insert( p );
            }
        }
    }
    if( rules.has_flag( ally_rule::hold_the_line ) ) {
        for( const tripoint &p : g->m.points_in_radius( g->u.pos(), 1 ) ) {
            if( g->m.close_door( p, true, true ) || g->m.move_cost( p ) > 2 ) {
                ret.insert( p );
            }
        }
    }
    return ret;
}

mfaction_id npc::get_monster_faction() const
{
    if( my_fac ) {
        string_id<monfaction> my_mon_fac = string_id<monfaction>( my_fac->mon_faction );
        if( my_mon_fac.is_valid() ) {
            return my_mon_fac;
        }
    }

    // legacy checks
    // Those can't be static int_ids, because mods add factions
    static const string_id<monfaction> human_fac( "human" );
    static const string_id<monfaction> player_fac( "player" );
    static const string_id<monfaction> bee_fac( "bee" );

    if( is_player_ally() ) {
        return player_fac.id();
    }

    if( has_trait( trait_BEE ) ) {
        return bee_fac.id();
    }

    return human_fac.id();
}

std::string npc::extended_description() const
{
    std::string ss;
    // For some reason setting it using str or constructor doesn't work
    ss += Character::extended_description();

    ss += "\n--\n";
    if( attitude == NPCATT_KILL ) {
        ss += _( "Is trying to kill you." );
    } else if( attitude == NPCATT_FLEE || attitude == NPCATT_FLEE_TEMP ) {
        ss += _( "Is trying to flee from you." );
    } else if( is_player_ally() ) {
        ss += _( "Is your friend." );
    } else if( is_following() ) {
        ss += _( "Is following you." );
    } else if( is_leader() ) {
        ss += _( "Is guiding you." );
    } else if( guaranteed_hostile() ) {
        ss += _( "Will try to kill you or flee from you if you reveal yourself." );
    } else {
        ss += _( "Is neutral." );
    }

    if( hit_by_player ) {
        ss += "--\n";
        ss += _( "Is still innocent and killing them will be considered murder." );
        // TODO: "But you don't care because you're an edgy psycho"
    }

    return replace_colors( ss );
}

std::string npc::get_epilogue() const
{
    return SNIPPET.random_from_category(
               male ? "epilogue_npc_male" : "epilogue_npc_female"
           ).value_or( translation() ).translated();
}

void npc::set_companion_mission( npc &p, const std::string &mission_id )
{
    const tripoint omt_pos = p.global_omt_location();
    set_companion_mission( omt_pos, p.companion_mission_role_id, mission_id );
}

std::pair<std::string, nc_color> npc::hp_description() const
{
    int cur_hp = hp_percentage();
    std::string damage_info;
    std::string pronoun;
    if( male ) {
        pronoun = _( "He " );
    } else {
        pronoun = _( "She " );
    }
    nc_color col;
    if( cur_hp == 100 ) {
        damage_info = pronoun + _( "is uninjured." );
        col = c_green;
    } else if( cur_hp >= 80 ) {
        damage_info = pronoun + _( "is lightly injured." );
        col = c_light_green;
    } else if( cur_hp >= 60 ) {
        damage_info = pronoun + _( "is moderately injured." );
        col = c_yellow;
    } else if( cur_hp >= 30 ) {
        damage_info = pronoun + _( "is heavily injured." );
        col = c_yellow;
    } else if( cur_hp >= 10 ) {
        damage_info = pronoun + _( "is severely injured." );
        col = c_light_red;
    } else {
        damage_info = pronoun + _( "is nearly dead!" );
        col = c_red;
    }
    return std::make_pair( damage_info, col );
}
void npc::set_companion_mission( const tripoint &omt_pos, const std::string &role_id,
                                 const std::string &mission_id )
{
    comp_mission.position = omt_pos;
    comp_mission.mission_id = mission_id;
    comp_mission.role_id = role_id;
}

void npc::set_companion_mission( const tripoint &omt_pos, const std::string &role_id,
                                 const std::string &mission_id, const tripoint &destination )
{
    comp_mission.position = omt_pos;
    comp_mission.mission_id = mission_id;
    comp_mission.role_id = role_id;
    comp_mission.destination = destination;
}

void npc::reset_companion_mission()
{
    comp_mission.position = tripoint( -999, -999, -999 );
    comp_mission.mission_id.clear();
    comp_mission.role_id.clear();
    if( comp_mission.destination ) {
        comp_mission.destination = cata::nullopt;
    }
}

cata::optional<tripoint> npc::get_mission_destination() const
{
    if( comp_mission.destination ) {
        return comp_mission.destination;
    } else {
        return cata::nullopt;
    }
}

bool npc::has_companion_mission() const
{
    return !comp_mission.mission_id.empty();
}

npc_companion_mission npc::get_companion_mission() const
{
    return comp_mission;
}

attitude_group npc::get_attitude_group( npc_attitude att ) const
{
    switch( att ) {
        case NPCATT_MUG:
        case NPCATT_WAIT_FOR_LEAVE:
        case NPCATT_KILL:
            return attitude_group::hostile;
        case NPCATT_FLEE:
        case NPCATT_FLEE_TEMP:
            return attitude_group::fearful;
        case NPCATT_FOLLOW:
        case NPCATT_ACTIVITY:
        case NPCATT_LEAD:
            return attitude_group::friendly;
        default:
            break;
    }
    return attitude_group::neutral;
}

void npc::set_mission( npc_mission new_mission )
{
    if( new_mission != mission ) {
        previous_mission = mission;
        mission = new_mission;
    }
    if( mission == NPC_MISSION_ACTIVITY ) {
        current_activity_id = activity.id();
    }
}

bool npc::has_activity() const
{
    return mission == NPC_MISSION_ACTIVITY && attitude == NPCATT_ACTIVITY;
}

npc_attitude npc::get_attitude() const
{
    return attitude;
}

void npc::set_attitude( npc_attitude new_attitude )
{
    if( new_attitude == attitude ) {
        return;
    }
    previous_attitude = attitude;
    if( new_attitude == NPCATT_FLEE ) {
        new_attitude = NPCATT_FLEE_TEMP;
    }
    if( new_attitude == NPCATT_FLEE_TEMP && !has_effect( effect_npc_flee_player ) ) {
        add_effect( effect_npc_flee_player, 24_hours, num_bp );
    }

    add_msg( m_debug, "%s changes attitude from %s to %s",
             name, npc_attitude_id( attitude ), npc_attitude_id( new_attitude ) );
    attitude_group new_group = get_attitude_group( new_attitude );
    attitude_group old_group = get_attitude_group( attitude );
    if( new_group != old_group && !is_fake() && g->u.sees( *this ) ) {
        switch( new_group ) {
            case attitude_group::hostile:
                add_msg_if_npc( m_bad, _( "<npcname> gets angry!" ) );
                break;
            case attitude_group::fearful:
                add_msg_if_npc( m_warning, _( "<npcname> gets scared!" ) );
                break;
            default:
                if( old_group == attitude_group::hostile ) {
                    add_msg_if_npc( m_good, _( "<npcname> calms down." ) );
                } else if( old_group == attitude_group::fearful ) {
                    add_msg_if_npc( _( "<npcname> is no longer afraid." ) );
                }
                break;
        }
    }
    attitude = new_attitude;
}

npc_follower_rules::npc_follower_rules()
{
    engagement = combat_engagement::CLOSE;
    aim = aim_rule::WHEN_CONVENIENT;
    overrides = ally_rule::DEFAULT;
    override_enable = ally_rule::DEFAULT;

    set_flag( ally_rule::use_guns );
    set_flag( ally_rule::use_grenades );
    clear_flag( ally_rule::use_silent );
    set_flag( ally_rule::avoid_friendly_fire );

    clear_flag( ally_rule::allow_pick_up );
    clear_flag( ally_rule::allow_bash );
    clear_flag( ally_rule::allow_sleep );
    set_flag( ally_rule::allow_complain );
    set_flag( ally_rule::allow_pulp );
    clear_flag( ally_rule::close_doors );
    clear_flag( ally_rule::follow_close );
    clear_flag( ally_rule::avoid_doors );
    clear_flag( ally_rule::hold_the_line );
    clear_flag( ally_rule::ignore_noise );
    clear_flag( ally_rule::forbid_engage );
    set_flag( ally_rule::follow_distance_2 );
}

bool npc_follower_rules::has_flag( ally_rule test, bool check_override ) const
{
    if( check_override && ( static_cast<int>( test ) & static_cast<int>( override_enable ) ) ) {
        // if the override is set and false, return false
        if( static_cast<int>( test ) & ~static_cast<int>( overrides ) ) {
            return false;
            // if the override is set and true, return true
        } else if( static_cast<int>( test ) & static_cast<int>( overrides ) ) {
            return true;
        }
    }
    return static_cast<int>( test ) & static_cast<int>( flags );
}

void npc_follower_rules::set_flag( ally_rule setit )
{
    flags = static_cast<ally_rule>( static_cast<int>( flags ) | static_cast<int>( setit ) );
}

void npc_follower_rules::clear_flag( ally_rule clearit )
{
    flags = static_cast<ally_rule>( static_cast<int>( flags ) & ~static_cast<int>( clearit ) );
}

void npc_follower_rules::toggle_flag( ally_rule toggle )
{
    if( has_flag( toggle ) ) {
        clear_flag( toggle );
    } else {
        set_flag( toggle );
    }
}

void npc_follower_rules::set_specific_override_state( ally_rule rule, bool state )
{
    if( state ) {
        set_override( rule );
    } else {
        clear_override( rule );
    }
    enable_override( rule );
}

void npc_follower_rules::toggle_specific_override_state( ally_rule rule, bool state )
{
    if( has_override_enable( rule ) && has_override( rule ) == state ) {
        clear_override( rule );
        disable_override( rule );
    } else {
        set_specific_override_state( rule, state );
    }
}

bool npc::is_hallucination() const
{
    return hallucination;
}

bool npc_follower_rules::has_override_enable( ally_rule test ) const
{
    return static_cast<int>( test ) & static_cast<int>( override_enable );
}

void npc_follower_rules::enable_override( ally_rule setit )
{
    override_enable = static_cast<ally_rule>( static_cast<int>( override_enable ) |
                      static_cast<int>( setit ) );
}

void npc_follower_rules::disable_override( ally_rule clearit )
{
    override_enable = static_cast<ally_rule>( static_cast<int>( override_enable ) &
                      ~static_cast<int>( clearit ) );
}

bool npc_follower_rules::has_override( ally_rule test ) const
{
    return static_cast<int>( test ) & static_cast<int>( overrides );
}

void npc_follower_rules::set_override( ally_rule setit )
{
    overrides = static_cast<ally_rule>( static_cast<int>( overrides ) | static_cast<int>( setit ) );
}

void npc_follower_rules::clear_override( ally_rule clearit )
{
    overrides = static_cast<ally_rule>( static_cast<int>( overrides ) &
                                        ~static_cast<int>( clearit ) );
}

void npc_follower_rules::set_danger_overrides()
{
    overrides = ally_rule::DEFAULT;
    override_enable = ally_rule::DEFAULT;
    set_override( ally_rule::follow_close );
    set_override( ally_rule::avoid_doors );
    set_override( ally_rule::hold_the_line );
    enable_override( ally_rule::follow_close );
    enable_override( ally_rule::allow_sleep );
    enable_override( ally_rule::close_doors );
    enable_override( ally_rule::avoid_doors );
    enable_override( ally_rule::hold_the_line );
}

void npc_follower_rules::clear_overrides()
{
    overrides = ally_rule::DEFAULT;
    override_enable = ally_rule::DEFAULT;
}

int npc::get_thirst() const
{
    return Character::get_thirst() - units::to_milliliter<int>( stomach.get_water() ) / 5;
}
