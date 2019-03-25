// Associated headers here are the ones for which their only non-inline
// functions are serialization functions.  This allows IWYU to check the
// includes in such headers.
#include "enums.h" // IWYU pragma: associated
#include "npc_favor.h" // IWYU pragma: associated
#include "pldata.h" // IWYU pragma: associated

#include <algorithm>
#include <limits>
#include <numeric>
#include <sstream>

#include "ammo.h"
#include "auto_pickup.h"
#include "basecamp.h"
#include "bionics.h"
#include "calendar.h"
#include "crafting.h"
#include "cursesdef.h"
#include "debug.h"
#include "effect.h"
#include "game.h"
#include "inventory.h"
#include "io.h"
#include "item.h"
#include "item_factory.h"
#include "json.h"
#include "mission.h"
#include "monfaction.h"
#include "monster.h"
#include "morale.h"
#include "mtype.h"
#include "npc.h"
#include "npc_class.h"
#include "options.h"
#include "player.h"
#include "player_activity.h"
#include "profession.h"
#include "recipe_dictionary.h"
#include "rng.h"
#include "scenario.h"
#include "skill.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vitamin.h"
#include "vpart_range.h"
#include "vpart_reference.h"
#include "creature_tracker.h"
#include "overmapbuffer.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

static const trait_id trait_HYPEROPIC( "HYPEROPIC" );
static const trait_id trait_MYOPIC( "MYOPIC" );

static const matype_id style_kicks( "style_kicks" );

static const std::array<std::string, NUM_OBJECTS> obj_type_name = { { "OBJECT_NONE", "OBJECT_ITEM", "OBJECT_ACTOR", "OBJECT_PLAYER",
        "OBJECT_NPC", "OBJECT_MONSTER", "OBJECT_VEHICLE", "OBJECT_TRAP", "OBJECT_FIELD",
        "OBJECT_TERRAIN", "OBJECT_FURNITURE"
    }
};

// TODO: investigate serializing other members of the Creature class hierarchy
void serialize( const std::weak_ptr<monster> &obj, JsonOut &jsout )
{
    if( const auto monster_ptr = obj.lock() ) {
        jsout.start_object();

        jsout.member( "monster_at", monster_ptr->pos() );
        // TODO: if monsters/Creatures ever get unique ids,
        // create a differently named member, e.g.
        //     jsout.member("unique_id", monster_ptr->getID());
        jsout.end_object();
    } else {
        // Monster went away. It's up the activity handler to detect this.
        jsout.write_null();
    }
}

void deserialize( std::weak_ptr<monster> &obj, JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    tripoint temp_pos;

    obj.reset();
    if( data.read( "monster_at", temp_pos ) ) {
        const auto monp = g->critter_tracker->find( temp_pos );

        if( monp == nullptr ) {
            debugmsg( "no monster found at %d,%d,%d", temp_pos.x, temp_pos.y, temp_pos.z );
            return;
        }

        obj = monp;
    }

    // TODO: if monsters/Creatures ever get unique ids,
    // look for a differently named member, e.g.
    //     data.read( "unique_id", unique_id );
    //     obj = g->id_registry->from_id( unique_id)
    //    }
}

template<typename T>
void serialize( const cata::optional<T> &obj, JsonOut &jsout )
{
    if( obj ) {
        jsout.write( *obj );
    } else {
        jsout.write_null();
    }
}

template<typename T>
void deserialize( cata::optional<T> &obj, JsonIn &jsin )
{
    if( jsin.test_null() ) {
        obj.reset();
    } else {
        obj.emplace();
        jsin.read( *obj );
    }
}

std::vector<item> item::magazine_convert()
{
    std::vector<item> res;

    // only guns, auxiliary gunmods and tools require conversion
    if( !is_gun() && !is_tool() ) {
        return res;
    }

    // ignore items that have already been converted
    if( has_var( "magazine_converted" ) ) {
        return res;
    }

    item *spare_mag = gunmod_find( "spare_mag" );

    // if item has integral magazine remove any magazine mods but do not mark item as converted
    if( magazine_integral() ) {
        if( !is_gun() ) {
            return res; // only guns can have attached gunmods
        }

        int qty = spare_mag ? spare_mag->charges : 0;
        qty += charges - type->gun->clip; // excess ammo from magazine extensions

        // limit ammo to base capacity and return any excess as a new item
        charges = std::min( charges, static_cast<long>( type->gun->clip ) );
        if( qty > 0 ) {
            res.emplace_back( ammo_current() != "null" ? ammo_current() : ammo_type()->default_ammotype(),
                              calendar::turn, qty );
        }

        contents.erase( std::remove_if( contents.begin(), contents.end(), []( const item & e ) {
            return e.typeId() == "spare_mag" || e.typeId() == "clip" || e.typeId() == "clip2";
        } ), contents.end() );

        return res;
    }

    // now handle items using the new detachable magazines that haven't yet been converted
    item mag( magazine_default(), calendar::turn );
    item ammo( ammo_current() != "null" ? ammo_current() : ammo_type()->default_ammotype(),
               calendar::turn );

    // give base item an appropriate magazine and add to that any ammo originally stored in base item
    if( !magazine_current() ) {
        contents.push_back( mag );
        if( charges > 0 ) {
            ammo.charges = std::min( charges, mag.ammo_capacity() );
            charges -= ammo.charges;
            contents.back().contents.push_back( ammo );
        }
    }

    // remove any spare magazine and replace it with an equivalent loaded magazine
    if( spare_mag ) {
        res.push_back( mag );
        if( spare_mag->charges > 0 ) {
            ammo.charges = std::min( spare_mag->charges, mag.ammo_capacity() );
            charges += spare_mag->charges - ammo.charges;
            res.back().contents.push_back( ammo );
        }
    }

    // return any excess ammo (from either item or spare mag) as a new item
    if( charges > 0 ) {
        ammo.charges = charges;
        res.push_back( ammo );
    }

    // remove incompatible magazine mods
    contents.erase( std::remove_if( contents.begin(), contents.end(), []( const item & e ) {
        return e.typeId() == "spare_mag" || e.typeId() == "clip" || e.typeId() == "clip2";
    } ), contents.end() );

    // normalize the base item and mark it as converted
    charges = 0;
    curammo = nullptr;
    set_var( "magazine_converted", 1 );

    return res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player_activity.h

void player_activity::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "type", type );

    if( !type.is_null() ) {
        json.member( "moves_left", moves_left );
        json.member( "index", index );
        json.member( "position", position );
        json.member( "coords", coords );
        json.member( "name", name );
        json.member( "targets", targets );
        json.member( "placement", placement );
        json.member( "values", values );
        json.member( "str_values", str_values );
        json.member( "auto_resume", auto_resume );
        json.member( "monsters", monsters );
    }
    json.end_object();
}

void player_activity::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    std::string tmptype;
    int tmppos = 0;
    if( !data.read( "type", tmptype ) ) {
        // Then it's a legacy save.
        int tmp_type_legacy;
        data.read( "type", tmp_type_legacy );
        deserialize_legacy_type( tmp_type_legacy, type );
    } else {
        type = activity_id( tmptype );
    }

    if( type.is_null() ) {
        return;
    }

    if( !data.read( "position", tmppos ) ) {
        tmppos = INT_MIN;  // If loading a save before position existed, hope.
    }

    data.read( "moves_left", moves_left );
    data.read( "index", index );
    position = tmppos;
    data.read( "coords", coords );
    data.read( "name", name );
    data.read( "targets", targets );
    data.read( "placement", placement );
    values = data.get_int_array( "values" );
    str_values = data.get_string_array( "str_values" );
    data.read( "auto_resume", auto_resume );
    data.read( "monsters", monsters );

}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// skill.h
void SkillLevel::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "level", level() );
    json.member( "exercise", exercise( true ) );
    json.member( "istraining", isTraining() );
    json.member( "lastpracticed", _lastPracticed );
    json.member( "highestlevel", highestLevel() );
    json.end_object();
}

void SkillLevel::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    data.read( "level", _level );
    data.read( "exercise", _exercise );
    data.read( "istraining", _isTraining );
    if( !data.read( "lastpracticed", _lastPracticed ) ) {
        // TODO: shouldn't that be calendar::start?
        _lastPracticed = calendar::time_of_cataclysm + time_duration::from_hours(
                             get_option<int>( "INITIAL_TIME" ) );
    }
    data.read( "highestlevel", _highestLevel );
    if( _highestLevel < _level ) {
        _highestLevel = _level;
    }
}

void Character::trait_data::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "key", key );
    json.member( "charge", charge );
    json.member( "powered", powered );
    json.end_object();
}

void Character::trait_data::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    data.read( "key", key );
    data.read( "charge", charge );
    data.read( "powered", powered );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// Character.h, player + npc
/*
 * Gather variables for saving. These variables are common to both the player and NPCs.
 */
void Character::load( JsonObject &data )
{
    Creature::load( data );

    // stats
    data.read( "str_cur", str_cur );
    data.read( "str_max", str_max );
    data.read( "dex_cur", dex_cur );
    data.read( "dex_max", dex_max );
    data.read( "int_cur", int_cur );
    data.read( "int_max", int_max );
    data.read( "per_cur", per_cur );
    data.read( "per_max", per_max );

    data.read( "str_bonus", str_bonus );
    data.read( "dex_bonus", dex_bonus );
    data.read( "per_bonus", per_bonus );
    data.read( "int_bonus", int_bonus );

    // needs
    data.read( "thirst", thirst );
    data.read( "hunger", hunger );
    data.read( "starvation", starvation );
    data.read( "fatigue", fatigue );
    data.read( "sleep_deprivation", sleep_deprivation );
    data.read( "stomach_food", stomach_food );
    data.read( "stomach_water", stomach_water );

    // health
    data.read( "healthy", healthy );
    data.read( "healthy_mod", healthy_mod );

    data.read( "damage_bandaged", damage_bandaged );
    data.read( "damage_disinfected", damage_disinfected );

    JsonArray parray;

    data.read( "underwater", underwater );

    data.read( "traits", my_traits );
    for( auto it = my_traits.begin(); it != my_traits.end(); ) {
        const auto &tid = *it;
        if( tid.is_valid() ) {
            ++it;
        } else {
            debugmsg( "character %s has invalid trait %s, it will be ignored", name.c_str(), tid.c_str() );
            my_traits.erase( it++ );
        }
    }

    if( savegame_loading_version <= 23 ) {
        std::unordered_set<trait_id> old_my_mutations;
        data.read( "mutations", old_my_mutations );
        for( const auto &mut : old_my_mutations ) {
            my_mutations[mut]; // Creates a new entry with default values
        }
        std::map<trait_id, char> trait_keys;
        data.read( "mutation_keys", trait_keys );
        for( const auto &k : trait_keys ) {
            my_mutations[k.first].key = k.second;
        }
        std::set<trait_id> active_muts;
        data.read( "active_mutations_hacky", active_muts );
        for( const auto &mut : active_muts ) {
            my_mutations[mut].powered = true;
        }
    } else {
        data.read( "mutations", my_mutations );
    }
    for( auto it = my_mutations.begin(); it != my_mutations.end(); ) {
        const auto &mid = it->first;
        if( mid.is_valid() ) {
            on_mutation_gain( mid );
            cached_mutations.push_back( &mid.obj() );
            ++it;
        } else {
            debugmsg( "character %s has invalid mutation %s, it will be ignored", name.c_str(), mid.c_str() );
            my_mutations.erase( it++ );
        }
    }

    data.read( "my_bionics", *my_bionics );

    for( auto &w : worn ) {
        w.on_takeoff( *this );
    }
    worn.clear();
    data.read( "worn", worn );
    for( auto &w : worn ) {
        on_item_wear( w );
    }

    if( !data.read( "hp_cur", hp_cur ) ) {
        debugmsg( "Error, incompatible hp_cur in save file '%s'", parray.str().c_str() );
    }

    if( !data.read( "hp_max", hp_max ) ) {
        debugmsg( "Error, incompatible hp_max in save file '%s'", parray.str().c_str() );
    }

    inv.clear();
    if( data.has_member( "inv" ) ) {
        JsonIn *invin = data.get_raw( "inv" );
        inv.json_load_items( *invin );
    }

    weapon = item( "null", 0 );
    data.read( "weapon", weapon );

    _skills->clear();
    JsonObject pmap = data.get_object( "skills" );
    for( const std::string &member : pmap.get_member_names() ) {
        pmap.read( member, ( *_skills )[skill_id( member )] );
    }

    visit_items( [&]( item * it ) {
        for( auto &e : it->magazine_convert() ) {
            i_add( e );
        }
        return VisitResponse::NEXT;
    } );

    on_stat_change( "thirst", thirst );
    on_stat_change( "hunger", hunger );
    on_stat_change( "starvation", starvation );
    on_stat_change( "fatigue", fatigue );
    on_stat_change( "sleep_deprivation", sleep_deprivation );
}

void Character::store( JsonOut &json ) const
{
    Creature::store( json );

    // stat
    json.member( "str_cur", str_cur );
    json.member( "str_max", str_max );
    json.member( "dex_cur", dex_cur );
    json.member( "dex_max", dex_max );
    json.member( "int_cur", int_cur );
    json.member( "int_max", int_max );
    json.member( "per_cur", per_cur );
    json.member( "per_max", per_max );

    json.member( "str_bonus", str_bonus );
    json.member( "dex_bonus", dex_bonus );
    json.member( "per_bonus", per_bonus );
    json.member( "int_bonus", int_bonus );

    // health
    json.member( "healthy", healthy );
    json.member( "healthy_mod", healthy_mod );

    // needs
    json.member( "thirst", thirst );
    json.member( "hunger", hunger );
    json.member( "starvation", starvation );
    json.member( "fatigue", fatigue );
    json.member( "sleep_deprivation", sleep_deprivation );
    json.member( "stomach_food", stomach_food );
    json.member( "stomach_water", stomach_water );

    // breathing
    json.member( "underwater", underwater );

    // traits: permanent 'mutations' more or less
    json.member( "traits", my_traits );
    json.member( "mutations", my_mutations );

    // "Fracking Toasters" - Saul Tigh, toaster
    json.member( "my_bionics", *my_bionics );

    // skills
    json.member( "skills" );
    json.start_object();
    for( const auto &pair : *_skills ) {
        json.member( pair.first.str(), pair.second );
    }
    json.end_object();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h, player (+ npc for now, should eventually only be the player)
/*
 * Gather variables for saving.
 */

void player::load( JsonObject &data )
{
    Character::load( data );

    JsonArray parray;
    int tmpid = 0;

    if( !data.read( "posx", position.x ) ) { // uh-oh.
        debugmsg( "BAD PLAYER/NPC JSON: no 'posx'?" );
    }
    data.read( "posy", position.y );
    if( !data.read( "posz", position.z ) && g != nullptr ) {
        position.z = g->get_levz();
    }
    data.read( "stim", stim );
    data.read( "pkill", pkill );
    data.read( "radiation", radiation );
    data.read( "tank_plut", tank_plut );
    data.read( "reactor_plut", reactor_plut );
    data.read( "slow_rad", slow_rad );
    data.read( "scent", scent );
    data.read( "oxygen", oxygen );
    data.read( "male", male );
    data.read( "cash", cash );
    data.read( "recoil", recoil );
    data.read( "in_vehicle", in_vehicle );
    data.read( "last_sleep_check", last_sleep_check );
    if( data.read( "id", tmpid ) ) {
        setID( tmpid );
    }

    data.read( "power_level", power_level );
    data.read( "max_power_level", max_power_level );
    // Bionic power scale has been changed, savegame version 21 has the new scale
    if( savegame_loading_version <= 20 ) {
        power_level *= 25;
        max_power_level *= 25;
    }

    // Bionic power should not be negative!
    if( power_level < 0 ) {
        power_level = 0;
    }

    data.read( "ma_styles", ma_styles );
    // Fix up old ma_styles that doesn't include fake styles
    if( std::find( ma_styles.begin(), ma_styles.end(), style_kicks ) == ma_styles.end() &&
        style_kicks.is_valid() ) {
        ma_styles.insert( ma_styles.begin(), style_kicks );
    }
    if( std::find( ma_styles.begin(), ma_styles.end(), matype_id::NULL_ID() ) == ma_styles.end() ) {
        ma_styles.insert( ma_styles.begin(), matype_id::NULL_ID() );
    }
    data.read( "addictions", addictions );

    JsonArray traps = data.get_array( "known_traps" );
    known_traps.clear();
    while( traps.has_more() ) {
        JsonObject pmap = traps.next_object();
        const tripoint p( pmap.get_int( "x" ), pmap.get_int( "y" ), pmap.get_int( "z" ) );
        const std::string t = pmap.get_string( "trap" );
        known_traps.insert( trap_map::value_type( p, t ) );
    }

    // Add the earplugs.
    if( has_bionic( bionic_id( "bio_ears" ) ) && !has_bionic( bionic_id( "bio_earplugs" ) ) ) {
        add_bionic( bionic_id( "bio_earplugs" ) );
    }

    // Add the blindfold.
    if( has_bionic( bionic_id( "bio_sunglasses" ) ) && !has_bionic( bionic_id( "bio_blindfold" ) ) ) {
        add_bionic( bionic_id( "bio_blindfold" ) );
    }

    // Fixes bugged characters for telescopic eyes CBM.
    if( has_bionic( bionic_id( "bio_eye_optic" ) ) && has_trait( trait_HYPEROPIC ) ) {
        remove_mutation( trait_HYPEROPIC );
    }

    if( has_bionic( bionic_id( "bio_eye_optic" ) ) && has_trait( trait_MYOPIC ) ) {
        remove_mutation( trait_MYOPIC );
    }

    if( has_bionic( bionic_id( "bio_solar" ) ) ) {
        remove_bionic( bionic_id( "bio_solar" ) );
    }

    on_stat_change( "pkill", pkill );
    on_stat_change( "perceived_pain", get_perceived_pain() );

    int tmptar = 0;
    int tmptartyp = 0;
    data.read( "last_target", tmptar );
    data.read( "last_target_type", tmptartyp );
    data.read( "last_target_pos", last_target_pos );
    data.read( "ammo_location", ammo_location );

    // Fixes savefile with invalid last_target_pos.
    if( last_target_pos && *last_target_pos == tripoint_min ) {
        last_target_pos = cata::nullopt;
    }

    if( tmptartyp == +1 ) {
        // Use overmap_buffer because game::active_npc is not filled yet.
        last_target = overmap_buffer.find_npc( tmptar );
    } else if( tmptartyp == -1 ) {
        // Need to do this *after* the monsters have been loaded!
        last_target = g->critter_tracker->from_temporary_id( tmptar );
    }

    JsonArray basecamps = data.get_array( "camps" );
    camps.clear();
    while( basecamps.has_more() ) {
        JsonObject bcdata = basecamps.next_object();
        tripoint bcpt;
        bcdata.read( "pos", bcpt );
        camps.insert( bcpt );
    }
}

/*
 * Variables common to player (and npc's, should eventually just be players)
 */
void player::store( JsonOut &json ) const
{
    Character::store( json );

    // assumes already in player object
    // positional data
    json.member( "posx", position.x );
    json.member( "posy", position.y );
    json.member( "posz", position.z );

    // energy
    json.member( "stim", stim );
    json.member( "last_sleep_check", last_sleep_check );
    // pain
    json.member( "pkill", pkill );
    // misc levels
    json.member( "radiation", radiation );
    json.member( "tank_plut", tank_plut );
    json.member( "reactor_plut", reactor_plut );
    json.member( "slow_rad", slow_rad );
    json.member( "scent", static_cast<int>( scent ) );
    json.member( "body_wetness", body_wetness );

    // breathing
    json.member( "oxygen", oxygen );

    // gender
    json.member( "male", male );

    json.member( "cash", cash );
    json.member( "recoil", recoil );
    json.member( "in_vehicle", in_vehicle );
    json.member( "id", getID() );

    // potential incompatibility with future expansion
    // TODO: consider ["parts"]["head"]["hp_cur"] instead of ["hp_cur"][head_enum_value]
    json.member( "hp_cur", hp_cur );
    json.member( "hp_max", hp_max );
    json.member( "damage_bandaged", damage_bandaged );
    json.member( "damage_disinfected", damage_disinfected );

    // npc; unimplemented
    json.member( "power_level", power_level );
    json.member( "max_power_level", max_power_level );

    // martial arts
    /*for (int i = 0; i < ma_styles.size(); i++) {
        ptmpvect.push_back( pv( ma_styles[i] ) );
    }*/
    json.member( "ma_styles", ma_styles );
    // "Looks like I picked the wrong week to quit smoking." - Steve McCroskey
    json.member( "addictions", addictions );

    json.member( "known_traps" );
    json.start_array();
    for( const auto &elem : known_traps ) {
        json.start_object();
        json.member( "x", elem.first.x );
        json.member( "y", elem.first.y );
        json.member( "z", elem.first.z );
        json.member( "trap", elem.second );
        json.end_object();
    }
    json.end_array();

    json.member( "worn", worn ); // also saves contents

    json.member( "inv" );
    inv.json_save_items( json );

    if( !weapon.is_null() ) {
        json.member( "weapon", weapon ); // also saves contents
    }

    if( const auto lt_ptr = last_target.lock() ) {
        if( const npc *const guy = dynamic_cast<const npc *>( lt_ptr.get() ) ) {
            json.member( "last_target", guy->getID() );
            json.member( "last_target_type", +1 );
        } else if( const monster *const mon = dynamic_cast<const monster *>( lt_ptr.get() ) ) {
            // monsters don't have IDs, so get its index in the Creature_tracker instead
            json.member( "last_target", g->critter_tracker->temporary_id( *mon ) );
            json.member( "last_target_type", -1 );
        }
    } else {
        json.member( "last_target_pos", last_target_pos );
    }

    json.member( "ammo_location", ammo_location );

    json.member( "camps" );
    json.start_array();
    for( const tripoint &bcpt : camps ) {
        json.start_object();
        json.member( "pos", bcpt );
        json.end_object();
    }
    json.end_array();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h, player
/*
 * Prepare a json object for player, including player specific data, and data common to
   players and npcs ( which json_save_actor_data() handles ).
 */
void player::serialize( JsonOut &json ) const
{
    json.start_object();
    // This must be after the json object has been started, so any super class
    // puts their data into the same json object.
    store( json );

    // TODO: once npcs are separated from the player class,
    // this code should go into player::store, serialize will then only
    // contain start_object(), store(), end_object().

    // player-specific specifics
    if( prof != nullptr ) {
        json.member( "profession", prof->ident() );
    }
    if( g->scen != nullptr ) {
        json.member( "scenario", g->scen->ident() );
    }
    // someday, npcs may drive
    json.member( "controlling_vehicle", controlling_vehicle );

    // shopping carts, furniture etc
    json.member( "grab_point", grab_point );
    json.member( "grab_type", obj_type_name[static_cast<int>( grab_type ) ] );

    // misc player specific stuff
    json.member( "focus_pool", focus_pool );
    json.member( "style_selected", style_selected );
    json.member( "keep_hands_free", keep_hands_free );

    json.member( "stamina", stamina );
    json.member( "move_mode", move_mode );

    // crafting etc
    json.member( "activity", activity );
    json.member( "backlog", backlog );

    // "The cold wakes you up."
    json.member( "temp_cur", temp_cur );
    json.member( "temp_conv", temp_conv );
    json.member( "frostbite_timer", frostbite_timer );

    // npc: unimplemented, potentially useful
    json.member( "learned_recipes", *learned_recipes );

    // Player only, books they have read at least once.
    json.member( "items_identified", items_identified );

    json.member( "vitamin_levels", vitamin_levels );

    morale->store( json );

    // mission stuff
    json.member( "active_mission", active_mission == nullptr ? -1 : active_mission->get_id() );

    json.member( "active_missions", mission::to_uid_vector( active_missions ) );
    json.member( "completed_missions", mission::to_uid_vector( completed_missions ) );
    json.member( "failed_missions", mission::to_uid_vector( failed_missions ) );

    json.member( "player_stats", lifetime_stats );

    json.member( "show_map_memory", show_map_memory );

    json.member( "assigned_invlet" );
    json.start_array();
    for( auto iter : inv.assigned_invlet ) {
        json.start_array();
        json.write( iter.first );
        json.write( iter.second );
        json.end_array();
    }
    json.end_array();

    json.member( "invcache" );
    inv.json_save_invcache( json );

    // FIXME: separate function, better still another file
    /*      for( size_t i = 0; i < memorial_log.size(); ++i ) {
              ptmpvect.push_back(pv(memorial_log[i]));
          }
          json.member("memorial",ptmpvect);
    */

    json.end_object();
}

/*
 * load player from ginormous json blob
 */
void player::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();

    load( data );

    // TODO: once npcs are separated from the player class,
    // this code should go into player::load, deserialize will then only
    // contain get_object(), load()

    std::string prof_ident = "(null)";
    if( data.read( "profession", prof_ident ) && string_id<profession>( prof_ident ).is_valid() ) {
        prof = &string_id<profession>( prof_ident ).obj();
    } else {
        debugmsg( "Tried to use non-existent profession '%s'", prof_ident.c_str() );
    }

    data.read( "activity", activity );
    // Changed from a single element to a list, handle either.
    // Can deprecate once we stop handling pre-0.B saves.
    if( data.has_array( "backlog" ) ) {
        data.read( "backlog", backlog );
    } else {
        player_activity temp;
        data.read( "backlog", temp );
        backlog.push_front( temp );
    }

    data.read( "controlling_vehicle", controlling_vehicle );

    data.read( "grab_point", grab_point );
    std::string grab_typestr = "OBJECT_NONE";
    if( grab_point.x != 0 || grab_point.y != 0 ) {
        grab_typestr = "OBJECT_VEHICLE";
        data.read( "grab_type", grab_typestr );
    }
    const auto iter = std::find( obj_type_name.begin(), obj_type_name.end(), grab_typestr );
    grab( iter == obj_type_name.end() ?
          OBJECT_NONE : static_cast<object_type>( std::distance( obj_type_name.begin(), iter ) ),
          grab_point );

    data.read( "focus_pool", focus_pool );
    data.read( "style_selected", style_selected );
    data.read( "keep_hands_free", keep_hands_free );

    data.read( "stamina", stamina );
    data.read( "move_mode", move_mode );

    set_highest_cat_level();
    drench_mut_calc();
    std::string scen_ident = "(null)";
    if( data.read( "scenario", scen_ident ) && string_id<scenario>( scen_ident ).is_valid() ) {
        g->scen = &string_id<scenario>( scen_ident ).obj();
        start_location = g->scen->start_location();
    } else {
        const scenario *generic_scenario = scenario::generic();
        // Only display error message if from a game file after scenarios existed.
        if( savegame_loading_version > 20 ) {
            debugmsg( "Tried to use non-existent scenario '%s'. Setting to generic '%s'.",
                      scen_ident.c_str(), generic_scenario->ident().c_str() );
        }
        g->scen = generic_scenario;
    }
    temp_cur.fill( 5000 );
    data.read( "temp_cur", temp_cur );

    temp_conv.fill( 5000 );
    data.read( "temp_conv", temp_conv );

    frostbite_timer.fill( 0 );
    data.read( "frostbite_timer", frostbite_timer );

    body_wetness.fill( 0 );
    data.read( "body_wetness", body_wetness );

    data.read( "learned_recipes", *learned_recipes );
    valid_autolearn_skills->clear(); // Invalidates the cache

    items_identified.clear();
    data.read( "items_identified", items_identified );

    auto vits = data.get_object( "vitamin_levels" );
    for( const auto &v : vitamin::all() ) {
        int lvl = vits.get_int( v.first.str(), 0 );
        lvl = std::max( std::min( lvl, v.first.obj().max() ), v.first.obj().min() );
        vitamin_levels[ v.first ] = lvl;
    }

    morale->load( data );

    std::vector<int> tmpmissions;
    if( data.read( "active_missions", tmpmissions ) ) {
        active_missions = mission::to_ptr_vector( tmpmissions );
    }
    if( data.read( "failed_missions", tmpmissions ) ) {
        failed_missions = mission::to_ptr_vector( tmpmissions );
    }
    if( data.read( "completed_missions", tmpmissions ) ) {
        completed_missions = mission::to_ptr_vector( tmpmissions );
    }

    int tmpactive_mission = 0;
    if( data.read( "active_mission", tmpactive_mission ) ) {
        if( savegame_loading_version <= 23 ) {
            // In 0.C, active_mission was an index of the active_missions array (-1 indicated no active mission).
            // And it would as often as not be out of bounds (e.g. when a questgiver died).
            // Later, it became a mission * and stored as the mission's uid, and this change broke backward compatibility.
            // Unfortunately, nothing can be done about savegames between the bump to version 24 and 83808a941.
            if( tmpactive_mission >= 0 && tmpactive_mission < static_cast<int>( active_missions.size() ) ) {
                active_mission = active_missions[tmpactive_mission];
            } else if( !active_missions.empty() ) {
                active_mission = active_missions.back();
            }
        } else if( tmpactive_mission != -1 ) {
            active_mission = mission::find( tmpactive_mission );
        }
    }

    // Normally there is only one player character loaded, so if a mission that is assigned to
    // another character (not the current one) fails, the other character(s) are not informed.
    // We must inform them when they get loaded the next time.
    // Only active missions need checking, failed/complete will not change anymore.
    const auto last = std::remove_if( active_missions.begin(),
    active_missions.end(), []( mission const * m ) {
        return m->has_failed();
    } );
    std::copy( last, active_missions.end(), std::back_inserter( failed_missions ) );
    active_missions.erase( last, active_missions.end() );
    if( active_mission && active_mission->has_failed() ) {
        if( active_missions.empty() ) {
            active_mission = nullptr;
        } else {
            active_mission = active_missions.front();
        }
    }

    if( savegame_loading_version <= 23 && is_player() ) {
        // In 0.C there was no player_id member of mission, so it'll be the default -1.
        // When the member was introduced, no steps were taken to ensure compatibility with 0.C, so
        // missions will be buggy for saves between experimental commits bd2088c033 and dd83800.
        // see npc_chatbin::check_missions and npc::talk_to_u
        for( mission *miss : active_missions ) {
            miss->set_player_id_legacy_0c( getID() );
        }
    }

    data.read( "player_stats", lifetime_stats );

    //Load from legacy map_memory save location (now in its own file <playername>.mm)
    if( data.has_member( "map_memory_tiles" ) || data.has_member( "map_memory_curses" ) ) {
        player_map_memory.load( data );
    }
    data.read( "show_map_memory", show_map_memory );

    JsonArray parray = data.get_array( "assigned_invlet" );
    while( parray.has_more() ) {
        JsonArray pair = parray.next_array();
        inv.assigned_invlet[static_cast<char>( pair.get_int( 0 ) )] = pair.get_string( 1 );
    }

    if( data.has_member( "invcache" ) ) {
        JsonIn *jip = data.get_raw( "invcache" );
        inv.json_load_invcache( *jip );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// npc.h

void npc_follower_rules::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "engagement", static_cast<int>( engagement ) );
    json.member( "aim", static_cast<int>( aim ) );

    // serialize the flags so they can be changed between save games
    for( const auto &rule : ally_rule_strs ) {
        json.member( rule.first, has_flag( rule.second ) );
    }
    json.member( "pickup_whitelist", *pickup_whitelist );

    json.end_object();
}

void npc_follower_rules::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    int tmpeng = 0;
    data.read( "engagement", tmpeng );
    engagement = static_cast<combat_engagement>( tmpeng );
    int tmpaim = 0;
    data.read( "aim", tmpaim );
    aim = static_cast<aim_rule>( tmpaim );

    // deserialize the flags so they can be changed between save games
    for( const auto &rule : ally_rule_strs ) {
        bool tmpflag = false;
        data.read( rule.first, tmpflag );
        if( tmpflag ) {
            set_flag( rule.second );
        } else {
            clear_flag( rule.second );
        }
    }

    data.read( "pickup_whitelist", *pickup_whitelist );
}

extern std::string convert_talk_topic( talk_topic_enum );

void npc_chatbin::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "first_topic", first_topic );
    if( mission_selected != nullptr ) {
        json.member( "mission_selected", mission_selected->get_id() );
    }
    json.member( "skill", skill );
    json.member( "style", style );
    json.member( "missions", mission::to_uid_vector( missions ) );
    json.member( "missions_assigned", mission::to_uid_vector( missions_assigned ) );
    json.end_object();
}

void npc_chatbin::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();

    if( data.has_int( "first_topic" ) ) {
        int tmptopic = 0;
        data.read( "first_topic", tmptopic );
        first_topic = convert_talk_topic( talk_topic_enum( tmptopic ) );
    } else {
        data.read( "first_topic", first_topic );
    }

    data.read( "skill", skill );
    data.read( "style", style );

    std::vector<int> tmpmissions;
    data.read( "missions", tmpmissions );
    missions = mission::to_ptr_vector( tmpmissions );
    std::vector<int> tmpmissions_assigned;
    data.read( "missions_assigned", tmpmissions_assigned );
    missions_assigned = mission::to_ptr_vector( tmpmissions_assigned );

    int tmpmission_selected = 0;
    mission_selected = nullptr;
    if( data.read( "mission_selected", tmpmission_selected ) && tmpmission_selected != -1 ) {
        if( savegame_loading_version <= 23 ) {
            // In 0.C, it was an index into the missions_assigned vector
            if( tmpmission_selected >= 0 &&
                tmpmission_selected < static_cast<int>( missions_assigned.size() ) ) {
                mission_selected = missions_assigned[tmpmission_selected];
            }
        } else {
            mission_selected = mission::find( tmpmission_selected );
        }
    }
}

void npc_personality::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    int tmpagg = 0;
    int tmpbrav = 0;
    int tmpcol = 0;
    int tmpalt = 0;
    if( data.read( "aggression", tmpagg ) &&
        data.read( "bravery", tmpbrav ) &&
        data.read( "collector", tmpcol ) &&
        data.read( "altruism", tmpalt ) ) {
        aggression = static_cast<signed char>( tmpagg );
        bravery = static_cast<signed char>( tmpbrav );
        collector = static_cast<signed char>( tmpcol );
        altruism = static_cast<signed char>( tmpalt );
    } else {
        debugmsg( "npc_personality: bad data" );
    }
}

void npc_personality::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "aggression", static_cast<int>( aggression ) );
    json.member( "bravery", static_cast<int>( bravery ) );
    json.member( "collector", static_cast<int>( collector ) );
    json.member( "altruism", static_cast<int>( altruism ) );
    json.end_object();
}

void npc_opinion::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    data.read( "trust", trust );
    data.read( "fear", fear );
    data.read( "value", value );
    data.read( "anger", anger );
    data.read( "owed", owed );
}

void npc_opinion::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "trust", trust );
    json.member( "fear", fear );
    json.member( "value", value );
    json.member( "anger", anger );
    json.member( "owed", owed );
    json.end_object();
}

void npc_favor::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    type = npc_favor_type( jo.get_int( "type" ) );
    jo.read( "value", value );
    jo.read( "itype_id", item_id );
    if( jo.has_int( "skill_id" ) ) {
        skill = Skill::from_legacy_int( jo.get_int( "skill_id" ) );
    } else if( jo.has_string( "skill_id" ) ) {
        skill = skill_id( jo.get_string( "skill_id" ) );
    } else {
        skill = skill_id::NULL_ID();
    }
}

void npc_favor::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "type", static_cast<int>( type ) );
    json.member( "value", value );
    json.member( "itype_id", static_cast<std::string>( item_id ) );
    json.member( "skill_id", skill );
    json.end_object();
}

/*
 * load npc
 */
void npc::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    load( data );
}

void npc::load( JsonObject &data )
{
    // TODO: once npcs are separated from the player class,
    // this should call load on the parent class of npc (probably Character).
    player::load( data );

    int misstmp = 0;
    int classtmp = 0;
    int atttmp = 0;
    std::string facID;
    std::string comp_miss_id;
    std::string comp_miss_role;
    tripoint comp_miss_pt;
    std::string classid;
    std::string companion_mission_role;
    time_point companion_mission_t = 0;
    time_point companion_mission_t_r = 0;

    data.read( "name", name );
    data.read( "marked_for_death", marked_for_death );
    data.read( "dead", dead );
    if( data.read( "myclass", classtmp ) ) {
        myclass = npc_class::from_legacy_int( classtmp );
    } else if( data.read( "myclass", classid ) ) {
        myclass = npc_class_id( classid );
    }

    data.read( "personality", personality );

    data.read( "wandf", wander_time );
    data.read( "wandx", wander_pos.x );
    data.read( "wandy", wander_pos.y );
    if( !data.read( "wandz", wander_pos.z ) ) {
        wander_pos.z = posz();
    }

    if( !data.read( "submap_coords", submap_coords ) ) {
        // Old submap coordinates are for the point (0, 0, 0) on local map
        // New ones are for submap that contains pos
        point old_coords;
        data.read( "mapx", old_coords.x );
        data.read( "mapy", old_coords.y );
        int o = 0;
        if( data.read( "omx", o ) ) {
            old_coords.x += o * OMAPX * 2;
        }
        if( data.read( "omy", o ) ) {
            old_coords.y += o * OMAPY * 2;
        }
        submap_coords = point( old_coords.x + posx() / SEEX, old_coords.y + posy() / SEEY );
    }

    if( !data.read( "mapz", position.z ) ) {
        data.read( "omz", position.z ); // omz/mapz got moved to position.z
    }

    if( data.has_member( "plx" ) ) {
        last_player_seen_pos.emplace();
        data.read( "plx", last_player_seen_pos->x );
        data.read( "ply", last_player_seen_pos->y );
        if( !data.read( "plz", last_player_seen_pos->z ) ) {
            last_player_seen_pos->z = posz();
        }
        // old code used tripoint_min to indicate "not a valid point"
        if( *last_player_seen_pos == tripoint_min ) {
            last_player_seen_pos.reset();
        }
    } else {
        data.read( "last_player_seen_pos", last_player_seen_pos );
    }

    data.read( "goalx", goal.x );
    data.read( "goaly", goal.y );
    data.read( "goalz", goal.z );

    data.read( "guardx", guard_pos.x );
    data.read( "guardy", guard_pos.y );
    data.read( "guardz", guard_pos.z );

    if( data.has_member( "pulp_locationx" ) ) {
        pulp_location.emplace();
        data.read( "pulp_locationx", pulp_location->x );
        data.read( "pulp_locationy", pulp_location->y );
        data.read( "pulp_locationz", pulp_location->z );
        // old code used tripoint_min to indicate "not a valid point"
        if( *pulp_location == tripoint_min ) {
            pulp_location.reset();
        }
    } else {
        data.read( "pulp_location", pulp_location );
    }

    if( data.read( "mission", misstmp ) ) {
        mission = npc_mission( misstmp );
        static const std::set<npc_mission> legacy_missions = {{
                NPC_MISSION_LEGACY_1, NPC_MISSION_LEGACY_2,
                NPC_MISSION_LEGACY_3
            }
        };
        if( legacy_missions.count( mission ) > 0 ) {
            mission = NPC_MISSION_NULL;
        }
    }

    if( data.read( "my_fac", facID ) ) {
        fac_id = faction_id( facID );
    }

    if( data.read( "attitude", atttmp ) ) {
        attitude = npc_attitude( atttmp );
        static const std::set<npc_attitude> legacy_attitudes = {{
                NPCATT_LEGACY_1, NPCATT_LEGACY_2, NPCATT_LEGACY_3,
                NPCATT_LEGACY_4, NPCATT_LEGACY_5, NPCATT_LEGACY_6
            }
        };
        if( legacy_attitudes.count( attitude ) > 0 ) {
            attitude = NPCATT_NULL;
        }
    }

    if( data.read( "comp_mission_id", comp_miss_id ) ) {
        comp_mission.mission_id = comp_miss_id;
    }

    if( data.read( "comp_mission_pt", comp_miss_pt ) ) {
        comp_mission.position = comp_miss_pt;
    }

    if( data.read( "comp_mission_role", comp_miss_role ) ) {
        comp_mission.role_id = comp_miss_role;
    }

    if( data.read( "companion_mission_role_id", companion_mission_role ) ) {
        companion_mission_role_id = companion_mission_role;
    }

    std::vector<tripoint> companion_mission_pts;
    data.read( "companion_mission_points", companion_mission_pts );
    if( !companion_mission_pts.empty() ) {
        for( auto pt : companion_mission_pts ) {
            companion_mission_points.push_back( pt );
        }
    }

    if( !data.read( "companion_mission_time", companion_mission_t ) ) {
        companion_mission_time = calendar::before_time_starts;
    } else {
        companion_mission_time = companion_mission_t;
    }

    if( !data.read( "companion_mission_time_ret", companion_mission_t_r ) ) {
        companion_mission_time_ret = calendar::before_time_starts;
    } else {
        companion_mission_time_ret = companion_mission_t_r;
    }

    companion_mission_inv.clear();
    if( data.has_member( "companion_mission_inv" ) ) {
        JsonIn *invin_mission = data.get_raw( "companion_mission_inv" );
        companion_mission_inv.json_load_items( *invin_mission );
    }

    if( !data.read( "restock", restock ) ) {
        restock = calendar::before_time_starts;
    }

    data.read( "op_of_u", op_of_u );
    data.read( "chatbin", chatbin );
    if( !data.read( "rules", rules ) ) {
        data.read( "misc_rules", rules );
        data.read( "combat_rules", rules );
    }

    if( !data.read( "last_updated", last_updated ) ) {
        last_updated = calendar::turn;
    }
    // TODO: time_point does not have a default constructor, need to read in the map manually
    {
        complaints.clear();
        JsonObject jo = data.get_object( "complaints" );
        for( const std::string &key : jo.get_member_names() ) {
            time_point p = 0;
            jo.read( key, p );
            complaints.emplace( key, p );
        }
    }
}

/*
 * save npc
 */
void npc::serialize( JsonOut &json ) const
{
    json.start_object();
    // This must be after the json object has been started, so any super class
    // puts their data into the same json object.
    store( json );
    json.end_object();
}

void npc::store( JsonOut &json ) const
{
    // TODO: once npcs are separated from the player class,
    // this should call store on the parent class of npc (probably Character).
    player::store( json );

    json.member( "name", name );
    json.member( "marked_for_death", marked_for_death );
    json.member( "dead", dead );
    json.member( "patience", patience );
    json.member( "myclass", myclass.str() );

    json.member( "personality", personality );
    json.member( "wandf", wander_time );
    json.member( "wandx", wander_pos.x );
    json.member( "wandy", wander_pos.y );
    json.member( "wandz", wander_pos.z );

    json.member( "submap_coords", submap_coords );

    json.member( "last_player_seen_pos", last_player_seen_pos );

    json.member( "goalx", goal.x );
    json.member( "goaly", goal.y );
    json.member( "goalz", goal.z );

    json.member( "guardx", guard_pos.x );
    json.member( "guardy", guard_pos.y );
    json.member( "guardz", guard_pos.z );

    json.member( "pulp_location", pulp_location );

    json.member( "mission", mission ); // TODO: stringid
    if( !fac_id.str().empty() ) { // set in constructor
        json.member( "my_fac", my_fac->id.c_str() );
    }
    json.member( "attitude", static_cast<int>( attitude ) );
    json.member( "op_of_u", op_of_u );
    json.member( "chatbin", chatbin );
    json.member( "rules", rules );

    json.member( "comp_mission_id", comp_mission.mission_id );
    json.member( "comp_mission_pt", comp_mission.position );
    json.member( "comp_mission_role", comp_mission.role_id );
    json.member( "companion_mission_role_id", companion_mission_role_id );
    json.member( "companion_mission_points", companion_mission_points );
    json.member( "companion_mission_time", companion_mission_time );
    json.member( "companion_mission_time_ret", companion_mission_time_ret );
    json.member( "companion_mission_inv" );
    companion_mission_inv.json_save_items( json );
    json.member( "restock", restock );

    json.member( "last_updated", last_updated );
    json.member( "complaints", complaints );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// inventory.h
/*
 * Save invlet cache
 */
void inventory::json_save_invcache( JsonOut &json ) const
{
    json.start_array();
    for( const auto &elem : invlet_cache.get_invlets_by_id() ) {
        json.start_object();
        json.member( elem.first );
        json.start_array();
        for( const auto &_sym : elem.second ) {
            json.write( static_cast<int>( _sym ) );
        }
        json.end_array();
        json.end_object();
    }
    json.end_array();
}

/*
 * Invlet cache: player specific, thus not wrapped in inventory::json_load/save
 */
void inventory::json_load_invcache( JsonIn &jsin )
{
    try {
        std::unordered_map<itype_id, std::string> map;
        JsonArray ja = jsin.get_array();
        while( ja.has_more() ) {
            JsonObject jo = ja.next_object();
            std::set<std::string> members = jo.get_member_names();
            for( const auto &member : members ) {
                std::string invlets;
                JsonArray pvect = jo.get_array( member );
                while( pvect.has_more() ) {
                    invlets.push_back( pvect.next_int() );
                }
                map[member] = invlets;
            }
        }
        invlet_cache = { map };
    } catch( const JsonError &jsonerr ) {
        debugmsg( "bad invcache json:\n%s", jsonerr.c_str() );
    }
}

/*
 * save all items. Just this->items, invlet cache saved separately
 */
void inventory::json_save_items( JsonOut &json ) const
{
    json.start_array();
    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            elem_stack_iter.serialize( json );
        }
    }
    json.end_array();
}

void inventory::json_load_items( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        item tmp;
        tmp.deserialize( jsin );
        add_item( tmp, true, false );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// monster.h

void monster::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    load( data );
}

void monster::load( JsonObject &data )
{
    Creature::load( data );

    std::string sidtmp;
    // load->str->int
    data.read( "typeid", sidtmp );
    type = &mtype_id( sidtmp ).obj();

    data.read( "unique_name", unique_name );
    data.read( "posx", position.x );
    data.read( "posy", position.y );
    if( !data.read( "posz", position.z ) ) {
        position.z = g->get_levz();
    }

    data.read( "wandf", wandf );
    data.read( "wandx", wander_pos.x );
    data.read( "wandy", wander_pos.y );
    if( data.read( "wandz", wander_pos.z ) ) {
        wander_pos.z = position.z;
    }

    data.read( "hp", hp );

    // sp_timeout indicates an old save, prior to the special_attacks refactor
    if( data.has_array( "sp_timeout" ) ) {
        JsonArray parray = data.get_array( "sp_timeout" );
        if( !parray.empty() ) {
            int index = 0;
            int ptimeout = 0;
            while( parray.has_more() && index < static_cast<int>( type->special_attacks_names.size() ) ) {
                if( parray.read_next( ptimeout ) ) {
                    // assume timeouts saved in same order as current monsters.json listing
                    const std::string &aname = type->special_attacks_names[index++];
                    auto &entry = special_attacks[aname];
                    if( ptimeout >= 0 ) {
                        entry.cooldown = ptimeout;
                    } else { // -1 means disabled, unclear what <-1 values mean in old saves
                        entry.cooldown = type->special_attacks.at( aname )->cooldown;
                        entry.enabled = false;
                    }
                }
            }
        }
    }

    // special_attacks indicates a save after the special_attacks refactor
    if( data.has_object( "special_attacks" ) ) {
        JsonObject pobject = data.get_object( "special_attacks" );
        for( const std::string &aname : pobject.get_member_names() ) {
            JsonObject saobject = pobject.get_object( aname );
            auto &entry = special_attacks[aname];
            entry.cooldown = saobject.get_int( "cooldown" );
            entry.enabled = saobject.get_bool( "enabled" );
        }
    }

    // make sure the loaded monster has every special attack its type says it should have
    for( auto &sa : type->special_attacks ) {
        const std::string &aname = sa.first;
        if( special_attacks.find( aname ) == special_attacks.end() ) {
            auto &entry = special_attacks[aname];
            entry.cooldown = rng( 0, sa.second->cooldown );
        }
    }

    data.read( "friendly", friendly );
    data.read( "mission_id", mission_id );
    data.read( "no_extra_death_drops", no_extra_death_drops );
    data.read( "dead", dead );
    data.read( "anger", anger );
    data.read( "morale", morale );
    data.read( "hallucination", hallucination );
    data.read( "stairscount", staircount ); // really?

    // Load legacy plans.
    std::vector<tripoint> plans;
    data.read( "plans", plans );
    if( !plans.empty() ) {
        goal = plans.back();
    }

    // This is relative to the monster so it isn't invalidated by map shifting.
    tripoint destination;
    data.read( "destination", destination );
    goal = pos() + destination;

    upgrades = data.get_bool( "upgrades", type->upgrades );
    upgrade_time = data.get_int( "upgrade_time", -1 );

    reproduces = data.get_bool( "reproduces", type->reproduces );
    baby_timer = data.get_int( "baby_timer", -1 );

    biosignatures = data.get_bool( "biosignatures", type->biosignatures );
    biosig_timer = data.get_int( "biosig_timer", -1 );

    horde_attraction = static_cast<monster_horde_attraction>( data.get_int( "horde_attraction", 0 ) );

    data.read( "inv", inv );
    if( data.has_int( "ammo" ) && !type->starting_ammo.empty() ) {
        // Legacy loading for ammo.
        normalize_ammo( data.get_int( "ammo" ) );
    } else {
        data.read( "ammo", ammo );
    }

    faction = mfaction_str_id( data.get_string( "faction", "" ) );
    if( !data.read( "last_updated", last_updated ) ) {
        last_updated = calendar::turn;
    }
    last_baby = data.get_int( "last_baby", calendar::turn );
    last_biosig = data.get_int( "last_biosig", calendar::turn );

    data.read( "path", path );
}

/*
 * Save, json ed; serialization that won't break as easily. In theory.
 */
void monster::serialize( JsonOut &json ) const
{
    json.start_object();
    // This must be after the json object has been started, so any super class
    // puts their data into the same json object.
    store( json );
    json.end_object();
}

void monster::store( JsonOut &json ) const
{
    Creature::store( json );
    json.member( "typeid", type->id );
    json.member( "unique_name", unique_name );
    json.member( "posx", position.x );
    json.member( "posy", position.y );
    json.member( "posz", position.z );
    json.member( "wandx", wander_pos.x );
    json.member( "wandy", wander_pos.y );
    json.member( "wandz", wander_pos.z );
    json.member( "wandf", wandf );
    json.member( "hp", hp );
    json.member( "special_attacks", special_attacks );
    json.member( "friendly", friendly );
    json.member( "faction", faction.id().str() );
    json.member( "mission_id", mission_id );
    json.member( "no_extra_death_drops", no_extra_death_drops );
    json.member( "dead", dead );
    json.member( "anger", anger );
    json.member( "morale", morale );
    json.member( "hallucination", hallucination );
    json.member( "stairscount", staircount );
    // Store the relative position of the goal so it loads correctly after a map shift.
    json.member( "destination", goal - pos() );
    json.member( "ammo", ammo );
    json.member( "underwater", underwater );
    json.member( "upgrades", upgrades );
    json.member( "upgrade_time", upgrade_time );
    json.member( "last_updated", last_updated );
    json.member( "reproduces", reproduces );
    json.member( "baby_timer", baby_timer );
    json.member( "last_baby", last_baby );
    json.member( "biosignatures", biosignatures );
    json.member( "biosig_timer", biosig_timer );
    json.member( "last_biosig", last_biosig );
    if( horde_attraction > MHA_NULL && horde_attraction < NUM_MONSTER_HORDE_ATTRACTION ) {
        json.member( "horde_attraction", horde_attraction );
    }

    json.member( "inv", inv );

    json.member( "path", path );
}

void mon_special_attack::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "cooldown", cooldown );
    json.member( "enabled", enabled );
    json.end_object();
}

void time_point::serialize( JsonOut &jsout ) const
{
    jsout.write( turn_ );
}

void time_point::deserialize( JsonIn &jsin )
{
    turn_ = jsin.get_int();
}

void time_duration::serialize( JsonOut &jsout ) const
{
    jsout.write( turns_ );
}

time_duration time_duration::read_from_json_string( JsonIn &jsin )
{
    static const std::vector<std::pair<std::string, time_duration>> units = { {
            { "turns", 1_turns },
            { "turn", 1_turns },
            { "t", 1_turns },
            // TODO: add seconds
            { "minutes", 1_minutes },
            { "minute", 1_minutes },
            { "m", 1_minutes },
            { "hours", 1_hours },
            { "hour", 1_hours },
            { "h", 1_hours },
            { "days", 1_days },
            { "day", 1_days },
            { "d", 1_days },
            // TODO: maybe add seasons?
            // TODO: maybe add years? Those two things depend on season length!
        }
    };
    const size_t pos = jsin.tell();
    size_t i = 0;
    const auto error = [&]( const char *const msg ) {
        jsin.seek( pos + i );
        jsin.error( msg );
    };

    const std::string s = jsin.get_string();
    // returns whether we are at the end of the string
    const auto skip_spaces = [&]() {
        while( i < s.size() && s[i] == ' ' ) {
            ++i;
        }
        return i >= s.size();
    };
    const auto get_unit = [&]() {
        if( skip_spaces() ) {
            error( "invalid time duration string: missing unit" );
        }
        for( const auto &pair : units ) {
            const std::string &unit = pair.first;
            if( s.size() >= unit.size() + i && s.compare( i, unit.size(), unit ) == 0 ) {
                i += unit.size();
                return pair.second;
            }
        }
        error( "invalid time duration string: unknown unit" );
        throw; // above always throws
    };

    if( skip_spaces() ) {
        error( "invalid time duration string: empty string" );
    }
    time_duration result = 0_turns;
    do {
        int sign_value = +1;
        if( s[i] == '-' ) {
            sign_value = -1;
            ++i;
        } else if( s[i] == '+' ) {
            ++i;
        }
        if( i >= s.size() || !isdigit( s[i] ) ) {
            error( "invalid time duration string: number expected" );
        }
        int value = 0;
        for( ; i < s.size() && isdigit( s[i] ); ++i ) {
            value = value * 10 + ( s[i] - '0' );
        }
        result += sign_value * value * get_unit();
    } while( !skip_spaces() );
    return result;
}

void time_duration::deserialize( JsonIn &jsin )
{
    if( jsin.test_string() ) {
        *this = read_from_json_string( jsin );
    } else {
        turns_ = jsin.get_int();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// item.h

static void migrate_toolmod( item &it );

template<typename Archive>
void item::io( Archive &archive )
{

    itype_id orig; // original ID as loaded from JSON
    const auto load_type = [&]( const itype_id & id ) {
        orig = id;
        convert( item_controller->migrate_id( id ) );
    };

    const auto load_curammo = [this]( const std::string & id ) {
        curammo = item::find_type( id );
    };
    const auto load_corpse = [this]( const std::string & id ) {
        if( id == "null" ) {
            // backwards compatibility, nullptr should not be stored at all
            corpse = nullptr;
        } else {
            corpse = &mtype_id( id ).obj();
        }
    };

    archive.template io<const itype>( "typeid", type, load_type, []( const itype & i ) {
        return i.get_id();
    }, io::required_tag() );

    // normalize legacy saves to always have charges >= 0
    archive.io( "charges", charges, 0L );
    charges = std::max( charges, 0L );

    int cur_phase = static_cast<int>( current_phase );
    archive.io( "burnt", burnt, 0 );
    archive.io( "poison", poison, 0 );
    archive.io( "frequency", frequency, 0 );
    archive.io( "note", note, 0 );
    archive.io( "irridation", irridation, 0 );
    archive.io( "bday", bday, calendar::time_of_cataclysm );
    archive.io( "mission_id", mission_id, -1 );
    archive.io( "player_id", player_id, -1 );
    archive.io( "item_vars", item_vars, io::empty_default_tag() );
    archive.io( "name", corpse_name, std::string() ); // TODO: change default to empty string
    archive.io( "invlet", invlet, '\0' );
    archive.io( "damaged", damage_, 0 );
    archive.io( "active", active, false );
    archive.io( "is_favorite", is_favorite, false );
    archive.io( "item_counter", item_counter, static_cast<decltype( item_counter )>( 0 ) );
    archive.io( "rot", rot, 0_turns );
    archive.io( "last_rot_check", last_rot_check, calendar::time_of_cataclysm );
    archive.io( "last_temp_check", last_temp_check, calendar::time_of_cataclysm );
    archive.io( "current_phase", cur_phase, static_cast<int>( type->phase ) );
    archive.io( "techniques", techniques, io::empty_default_tag() );
    archive.io( "faults", faults, io::empty_default_tag() );
    archive.io( "item_tags", item_tags, io::empty_default_tag() );
    archive.io( "contents", contents, io::empty_default_tag() );
    archive.io( "components", components, io::empty_default_tag() );
    archive.io( "specific_energy", specific_energy, -10 );
    archive.io( "temperature", temperature, 0 );
    archive.io( "recipe_charges", recipe_charges, 1 );
    archive.template io<const itype>( "curammo", curammo, load_curammo,
    []( const itype & i ) {
        return i.get_id();
    } );
    archive.template io<const mtype>( "corpse", corpse, load_corpse,
    []( const mtype & i ) {
        return i.id.str();
    } );
    archive.io( "light", light.luminance, nolight.luminance );
    archive.io( "light_width", light.width, nolight.width );
    archive.io( "light_dir", light.direction, nolight.direction );

    item_controller->migrate_item( orig, *this );

    if( !Archive::is_input::value ) {
        return;
    }
    /* Loading has finished, following code is to ensure consistency and fixes bugs in saves. */

    double float_damage = 0;
    if( archive.read( "damage", float_damage ) ) {
        damage_ = std::min( std::max( min_damage(),
                                      static_cast<int>( float_damage * itype::damage_scale ) ),
                            max_damage() );
    }

    // Old saves used to only contain one of those values (stored under "poison"), it would be
    // loaded into a union of those members. Now they are separate members and must be set separately.
    if( poison != 0 && note == 0 && !type->snippet_category.empty() ) {
        std::swap( note, poison );
    }
    if( poison != 0 && frequency == 0 && ( typeId() == "radio_on" || typeId() == "radio" ) ) {
        std::swap( frequency, poison );
    }
    if( poison != 0 && irridation == 0 && typeId() == "rad_badge" ) {
        std::swap( irridation, poison );
    }

    // Compatibility for item type changes: for example soap changed from being a generic item
    // (item::charges -1 or 0 or anything else) to comestible (and thereby counted by charges),
    // old saves still have invalid charges, this fixes the charges value to the default charges.
    if( count_by_charges() && charges <= 0 ) {
        charges = item( type, 0 ).charges;
    }
    if( is_food() ) {
        active = true;
    }
    if( !active &&
        ( item_tags.count( "HOT" ) > 0 || item_tags.count( "COLD" ) > 0 ||
          item_tags.count( "WET" ) > 0 ) ) {
        // Some hot/cold items from legacy saves may be inactive
        active = true;
    }
    std::string mode;
    if( archive.read( "mode", mode ) ) {
        // only for backward compatibility (nowadays mode is stored in item_vars)
        gun_set_mode( gun_mode_id( mode ) );
    }

    // Fixes #16751 (items could have null contents due to faulty spawn code)
    contents.erase( std::remove_if( contents.begin(), contents.end(), []( const item & cont ) {
        return cont.is_null();
    } ), contents.end() );

    // Sealed item migration: items with "unseals_into" set should always have contents
    if( contents.empty() && is_non_resealable_container() ) {
        convert( type->container->unseals_into );
    }

    // Migrate legacy toolmod flags
    if( is_tool() || is_toolmod() ) {
        migrate_toolmod( *this );
    }

    // Books without any chapters don't need to store a remaining-chapters
    // counter, it will always be 0 and it prevents proper stacking.
    if( get_chapters() == 0 ) {
        for( auto it = item_vars.begin(); it != item_vars.end(); ) {
            if( it->first.compare( 0, 19, "remaining-chapters-" ) == 0 ) {
                item_vars.erase( it++ );
            } else {
                ++it;
            }
        }
    }

    current_phase = static_cast<phase_id>( cur_phase );
    // override phase if frozen, needed for legacy save
    if( item_tags.count( "FROZEN" ) && current_phase == LIQUID ) {
        current_phase = SOLID;
    }
}

static void migrate_toolmod( item &it )
{
    // Convert legacy flags on tools to contained toolmods
    if( it.is_tool() ) {
        if( it.item_tags.count( "ATOMIC_AMMO" ) ) {
            it.item_tags.erase( "ATOMIC_AMMO" );
            it.item_tags.erase( "NO_UNLOAD" );
            it.item_tags.erase( "RADIOACTIVE" );
            it.item_tags.erase( "LEAK_DAM" );
            it.emplace_back( "battery_atomic" );

        } else if( it.item_tags.count( "DOUBLE_REACTOR" ) ) {
            it.item_tags.erase( "DOUBLE_REACTOR" );
            it.item_tags.erase( "DOUBLE_AMMO" );
            it.emplace_back( "double_plutonium_core" );

        } else if( it.item_tags.count( "DOUBLE_AMMO" ) ) {
            it.item_tags.erase( "DOUBLE_AMMO" );
            it.emplace_back( "battery_compartment" );

        } else if( it.item_tags.count( "USE_UPS" ) ) {
            it.item_tags.erase( "USE_UPS" );
            it.item_tags.erase( "NO_RELOAD" );
            it.item_tags.erase( "NO_UNLOAD" );
            it.emplace_back( "battery_ups" );

        }
    }

    // Fix fallout from #18797, which exponentially duplicates migrated toolmods
    if( it.is_toolmod() ) {
        // duplication would add an extra toolmod inside each toolmod on load;
        // delete the nested copies
        if( it.typeId() == "battery_atomic" || it.typeId() == "battery_compartment" ||
            it.typeId() == "battery_ups" || it.typeId() == "double_plutonium_core" ) {
            // Be conservative and only delete nested mods of the same type
            it.contents.remove_if( [&]( const item & cont ) {
                return cont.typeId() == it.typeId();
            } );
        }
    }

    if( it.is_tool() ) {
        // duplication would add an extra toolmod inside each tool on load;
        // delete the duplicates so there is only one copy of each toolmod
        int n_atomic = 0;
        int n_compartment = 0;
        int n_ups = 0;
        int n_plutonium = 0;

        // not safe to use remove_if with a stateful predicate
        for( auto i = it.contents.begin(); i != it.contents.end(); ) {
            if( ( i->typeId() == "battery_atomic" && ++n_atomic > 1 ) ||
                ( i->typeId() == "battery_compartment" && ++n_compartment > 1 ) ||
                ( i->typeId() == "battery_ups" && ++n_ups > 1 ) ||
                ( i->typeId() == "double_plutonium_core" && ++n_plutonium > 1 ) ) {
                i = it.contents.erase( i );
            } else {
                ++i;
            }
        }
    }
}

void item::deserialize( JsonIn &jsin )
{
    const JsonObject data = jsin.get_object();
    io::JsonObjectInputArchive archive( data );
    io( archive );
}

void item::serialize( JsonOut &json ) const
{
    io::JsonObjectOutputArchive archive( json );
    const_cast<item *>( this )->io( archive );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// vehicle.h

/*
 * vehicle_part
 */
void vehicle_part::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    vpart_id pid;
    data.read( "id", pid );

    std::map<std::string, std::pair<std::string, itype_id>> deprecated = {
        { "laser_gun", { "laser_rifle", "none" } },
        { "seat_nocargo", { "seat", "none" } },
        { "engine_plasma", { "minireactor", "none" } },
        { "battery_truck", { "battery_car", "battery" } },

        { "diesel_tank_little", { "tank_little", "diesel" } },
        { "diesel_tank_small", { "tank_small", "diesel" } },
        { "diesel_tank_medium", { "tank_medium", "diesel" } },
        { "diesel_tank", { "tank", "diesel" } },
        { "external_diesel_tank_small", { "external_tank_small", "diesel" } },
        { "external_diesel_tank", { "external_tank", "diesel" } },

        { "gas_tank_little", { "tank_little", "gasoline" } },
        { "gas_tank_small", { "tank_small", "gasoline" } },
        { "gas_tank_medium", { "tank_medium", "gasoline" } },
        { "gas_tank", { "tank", "gasoline" } },
        { "external_gas_tank_small", { "external_tank_small", "gasoline" } },
        { "external_gas_tank", { "external_tank", "gasoline" } },

        { "water_dirty_tank_little", { "tank_little", "water" } },
        { "water_dirty_tank_small", { "tank_small", "water" } },
        { "water_dirty_tank_medium", { "tank_medium", "water" } },
        { "water_dirty_tank", { "tank", "water" } },
        { "external_water_dirty_tank_small", { "external_tank_small", "water" } },
        { "external_water_dirty_tank", { "external_tank", "water" } },
        { "dirty_water_tank_barrel", { "tank_barrel", "water" } },

        { "water_tank_little", { "tank_little", "water_clean" } },
        { "water_tank_small", { "tank_small", "water_clean" } },
        { "water_tank_medium", { "tank_medium", "water_clean" } },
        { "water_tank", { "tank", "water_clean" } },
        { "external_water_tank_small", { "external_tank_small", "water_clean" } },
        { "external_water_tank", { "external_tank", "water_clean" } },
        { "water_tank_barrel", { "tank_barrel", "water_clean" } },

        { "napalm_tank", { "tank", "napalm" } },

        { "hydrogen_tank", { "tank", "none" } }
    };

    // required for compatibility with 0.C saves
    itype_id legacy_fuel;

    auto dep = deprecated.find( pid.str() );
    if( dep != deprecated.end() ) {
        pid = vpart_id( dep->second.first );
        legacy_fuel = dep->second.second;
    }

    // if we don't know what type of part it is, it'll cause problems later.
    if( !pid.is_valid() ) {
        if( pid.str() == "wheel_underbody" ) {
            pid = vpart_id( "wheel_wide" );
        } else {
            data.throw_error( "bad vehicle part", "id" );
        }
    }
    id = pid;

    if( data.has_object( "base" ) ) {
        data.read( "base", base );
    } else {
        // handle legacy format which didn't include the base item
        base = item( id.obj().item );
    }

    data.read( "mount_dx", mount.x );
    data.read( "mount_dy", mount.y );
    data.read( "open", open );
    data.read( "direction", direction );
    data.read( "blood", blood );
    data.read( "enabled", enabled );
    data.read( "flags", flags );
    data.read( "passenger_id", passenger_id );
    JsonArray ja = data.get_array( "carry" );
    // count down from size - 1, then stop after unsigned long 0 - 1 becomes MAX_INT
    for( size_t index = ja.size() - 1; index < ja.size(); index-- ) {
        carry_names.push( ja.get_string( index ) );
    }
    data.read( "crew_id", crew_id );
    data.read( "items", items );
    data.read( "target_first_x", target.first.x );
    data.read( "target_first_y", target.first.y );
    data.read( "target_first_z", target.first.z );
    data.read( "target_second_x", target.second.x );
    data.read( "target_second_y", target.second.y );
    data.read( "target_second_z", target.second.z );
    data.read( "ammo_pref", ammo_pref );

    if( legacy_fuel.empty() ) {
        legacy_fuel = id.obj().fuel_type;
    }

    // with VEHICLE tag migrate fuel tanks only if amount field exists
    if( base.has_flag( "VEHICLE" ) ) {
        if( data.has_int( "amount" ) && ammo_capacity() > 0 && legacy_fuel != "battery" ) {
            ammo_set( legacy_fuel, data.get_int( "amount" ) );
        }

        // without VEHICLE flag always migrate both batteries and fuel tanks
    } else {
        if( ammo_capacity() > 0 ) {
            ammo_set( legacy_fuel, data.get_int( "amount" ) );
        }
        base.item_tags.insert( "VEHICLE" );
    }

    if( data.has_int( "hp" ) && id.obj().durability > 0 ) {
        // migrate legacy savegames exploiting that all base items at that time had max_damage() of 4
        base.set_damage( 4 * itype::damage_scale - 4 * itype::damage_scale * data.get_int( "hp" ) /
                         id.obj().durability );
    }

    // legacy turrets loaded ammo via a pseudo CARGO space
    if( is_turret() && !items.empty() ) {
        const int qty = std::accumulate( items.begin(), items.end(), 0, []( int lhs, const item & rhs ) {
            return lhs + rhs.charges;
        } );
        ammo_set( items.front().ammo_current(), qty );
        items.clear();
    }
}

void vehicle_part::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "id", id.str() );
    json.member( "base", base );
    json.member( "mount_dx", mount.x );
    json.member( "mount_dy", mount.y );
    json.member( "open", open );
    json.member( "direction", direction );
    json.member( "blood", blood );
    json.member( "enabled", enabled );
    json.member( "flags", flags );
    if( !carry_names.empty() ) {
        std::stack<std::string, std::vector<std::string> > carry_copy = carry_names;
        json.member( "carry" );
        json.start_array();
        while( !carry_copy.empty() ) {
            json.write( carry_copy.top() );
            carry_copy.pop();
        }
        json.end_array();
    }
    json.member( "passenger_id", passenger_id );
    json.member( "crew_id", crew_id );
    json.member( "items", items );
    if( target.first != tripoint_min ) {
        json.member( "target_first_x", target.first.x );
        json.member( "target_first_y", target.first.y );
        json.member( "target_first_z", target.first.z );
    }
    if( target.second != tripoint_min ) {
        json.member( "target_second_x", target.second.x );
        json.member( "target_second_y", target.second.y );
        json.member( "target_second_z", target.second.z );
    }
    json.member( "ammo_pref", ammo_pref );
    json.end_object();
}

/*
 * label
 */
void deserialize( label &val, JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    data.read( "x", val.x );
    data.read( "y", val.y );
    data.read( "text", val.text );
}

void serialize( const label &val, JsonOut &json )
{
    json.start_object();
    json.member( "x", val.x );
    json.member( "y", val.y );
    json.member( "text", val.text );
    json.end_object();
}

/*
 * Load vehicle from a json blob that might just exceed player in size.
 */
void vehicle::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();

    int fdir = 0;
    int mdir = 0;

    data.read( "type", type );
    data.read( "posx", posx );
    data.read( "posy", posy );
    data.read( "om_id", om_id );
    data.read( "faceDir", fdir );
    data.read( "moveDir", mdir );
    data.read( "turn_dir", turn_dir );
    data.read( "velocity", velocity );
    data.read( "falling", is_falling );
    data.read( "floating", is_floating );
    data.read( "cruise_velocity", cruise_velocity );
    data.read( "vertical_velocity", vertical_velocity );
    data.read( "cruise_on", cruise_on );
    data.read( "engine_on", engine_on );
    data.read( "tracking_on", tracking_on );
    data.read( "skidding", skidding );
    data.read( "of_turn_carry", of_turn_carry );
    data.read( "is_locked", is_locked );
    data.read( "is_alarm_on", is_alarm_on );
    data.read( "camera_on", camera_on );
    if( !data.read( "last_update_turn", last_update ) ) {
        last_update = calendar::turn;
    }

    face.init( fdir );
    move.init( mdir );
    data.read( "name", name );

    data.read( "parts", parts );

    // we persist the pivot anchor so that if the rules for finding
    // the pivot change, existing vehicles do not shift around.
    // Loading vehicles that predate the pivot logic is a special
    // case of this, they will load with an anchor of (0,0) which
    // is what they're expecting.
    data.read( "pivot", pivot_anchor[0] );
    pivot_anchor[1] = pivot_anchor[0];
    pivot_rotation[1] = pivot_rotation[0] = fdir;

    // Need to manually backfill the active item cache since the part loader can't call its vehicle.
    for( const vpart_reference &vp : get_any_parts( VPFLAG_CARGO ) ) {
        auto it = vp.part().items.begin();
        auto end = vp.part().items.end();
        for( ; it != end; ++it ) {
            if( it->needs_processing() ) {
                active_items.add( it, vp.mount() );
            }
        }
    }

    for( const vpart_reference &vp : get_any_parts( "TURRET" ) ) {
        install_part( vp.mount(), vpart_id( "turret_mount" ), false );
    }

    /* After loading, check if the vehicle is from the old rules and is missing
     * frames. */
    if( savegame_loading_version < 11 ) {
        add_missing_frames();
    }

    // Handle steering changes
    if( savegame_loading_version < 25 ) {
        add_steerable_wheels();
    }

    refresh();

    data.read( "tags", tags );
    data.read( "labels", labels );

    point p;
    zone_data zd;
    JsonArray ja = data.get_array( "zones" );
    while( ja.has_more() ) {
        JsonObject sdata = ja.next_object();
        sdata.read( "point", p );
        sdata.read( "zone", zd );
        loot_zones.emplace( p, zd );
    }

    // Note that it's possible for a vehicle to be loaded midway
    // through a turn if the player is driving REALLY fast and their
    // own vehicle motion takes them in range. An undefined value for
    // on_turn caused occasional weirdness if the undefined value
    // happened to be positive.
    //
    // Setting it to zero means it won't get to move until the start
    // of the next turn, which is what happens anyway if it gets
    // loaded anywhere but midway through a driving cycle.
    //
    // Something similar to vehicle::gain_moves() would be ideal, but
    // that can't be used as it currently stands because it would also
    // make it instantly fire all its turrets upon load.
    of_turn = 0;

    /** Legacy saved games did not store part enabled status within parts */
    const auto set_legacy_state = [&]( const std::string & var, const std::string & flag ) {
        if( data.get_bool( var, false ) ) {
            for( const vpart_reference &vp : get_any_parts( flag ) ) {
                vp.part().enabled = true;
            }
        }
    };
    set_legacy_state( "stereo_on", "STEREO" );
    set_legacy_state( "chimes_on", "CHIMES" );
    set_legacy_state( "fridge_on", "FRIDGE" );
    set_legacy_state( "reaper_on", "REAPER" );
    set_legacy_state( "planter_on", "PLANTER" );
    set_legacy_state( "recharger_on", "RECHARGE" );
    set_legacy_state( "scoop_on", "SCOOP" );
    set_legacy_state( "plow_on", "PLOW" );
    set_legacy_state( "reactor_on", "REACTOR" );
}

void vehicle::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "type", type );
    json.member( "posx", posx );
    json.member( "posy", posy );
    json.member( "om_id", om_id );
    json.member( "faceDir", face.dir() );
    json.member( "moveDir", move.dir() );
    json.member( "turn_dir", turn_dir );
    json.member( "velocity", velocity );
    json.member( "falling", is_falling );
    json.member( "floating", is_floating );
    json.member( "cruise_velocity", cruise_velocity );
    json.member( "vertical_velocity", vertical_velocity );
    json.member( "cruise_on", cruise_on );
    json.member( "engine_on", engine_on );
    json.member( "tracking_on", tracking_on );
    json.member( "skidding", skidding );
    json.member( "of_turn_carry", of_turn_carry );
    json.member( "name", name );
    json.member( "parts", parts );
    json.member( "tags", tags );
    json.member( "labels", labels );
    json.member( "zones" );
    json.start_array();
    for( auto const &z : loot_zones ) {
        json.start_object();
        json.member( "point", z.first );
        json.member( "zone", z.second );
        json.end_object();
    }
    json.end_array();
    json.member( "is_locked", is_locked );
    json.member( "is_alarm_on", is_alarm_on );
    json.member( "camera_on", camera_on );
    json.member( "last_update_turn", last_update );
    json.member( "pivot", pivot_anchor[0] );
    json.end_object();
}

////////////////// mission.h
////
void mission::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    if( jo.has_int( "type_id" ) ) {
        type = &mission_type::from_legacy( jo.get_int( "type_id" ) ).obj();
    } else if( jo.has_string( "type_id" ) ) {
        type = &mission_type_id( jo.get_string( "type_id" ) ).obj();
    } else {
        debugmsg( "Saved mission has no type" );
        type = &mission_type::get_all().front();
    }

    jo.read( "description", description );

    bool failed;
    bool was_started;
    std::string status_string;
    if( jo.read( "status", status_string ) ) {
        status = status_from_string( status_string );
    } else if( jo.read( "failed", failed ) && failed ) {
        status = mission_status::failure;
    } else if( jo.read( "was_started", was_started ) && !was_started ) {
        status = mission_status::yet_to_start;
    } else {
        // Note: old code had no idea of successful missions!
        // We can't check properly here, since most of the game isn't loaded
        status = mission_status::in_progress;
    }

    jo.read( "value", value );
    jo.read( "reward", reward );
    jo.read( "uid", uid );
    JsonArray ja = jo.get_array( "target" );
    if( ja.size() == 3 ) {
        target.x = ja.get_int( 0 );
        target.y = ja.get_int( 1 );
        target.z = ja.get_int( 2 );
    } else if( ja.size() == 2 ) {
        target.x = ja.get_int( 0 );
        target.y = ja.get_int( 1 );
    }

    if( jo.has_int( "follow_up" ) ) {
        follow_up = mission_type::from_legacy( jo.get_int( "follow_up" ) );
    } else if( jo.has_string( "follow_up" ) ) {
        follow_up = mission_type_id( jo.get_string( "follow_up" ) );
    }

    item_id = itype_id( jo.get_string( "item_id", item_id ) );

    const std::string omid = jo.get_string( "target_id", "" );
    if( !omid.empty() ) {
        target_id = string_id<oter_type_t>( omid );
    }

    if( jo.has_int( "recruit_class" ) ) {
        recruit_class = npc_class::from_legacy_int( jo.get_int( "recruit_class" ) );
    } else {
        recruit_class = npc_class_id( jo.get_string( "recruit_class", "NC_NONE" ) );
    }

    jo.read( "target_npc_id", target_npc_id );
    jo.read( "monster_type", monster_type );
    jo.read( "monster_species", monster_species );
    jo.read( "monster_kill_goal", monster_kill_goal );
    jo.read( "deadline", deadline );
    jo.read( "step", step );
    jo.read( "item_count", item_count );
    jo.read( "npc_id", npc_id );
    jo.read( "good_fac_id", good_fac_id );
    jo.read( "bad_fac_id", bad_fac_id );

    // Suppose someone had two living players in an 0.C stable world. When loading player 1 in 0.D
    // (or maybe even creating a new player), the former condition makes legacy_no_player_id true.
    // When loading player 2, there will be a player_id member in master.gsav, but the bool member legacy_no_player_id
    // will have been saved as true (unless the mission belongs to a player that's been loaded into 0.D)
    // See player::deserialize and mission::set_player_id_legacy_0c
    legacy_no_player_id = !jo.read( "player_id", player_id ) ||
                          jo.get_bool( "legacy_no_player_id", false );
}

void mission::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "type_id", type->id );
    json.member( "description", description );
    json.member( "status", status_to_string( status ) );
    json.member( "value", value );
    json.member( "reward", reward );
    json.member( "uid", uid );

    json.member( "target" );
    json.start_array();
    json.write( target.x );
    json.write( target.y );
    json.write( target.z );
    json.end_array();

    json.member( "item_id", item_id );
    json.member( "item_count", item_count );
    json.member( "target_id", target_id.str() );
    json.member( "recruit_class", recruit_class );
    json.member( "target_npc_id", target_npc_id );
    json.member( "monster_type", monster_type );
    json.member( "monster_species", monster_species );
    json.member( "monster_kill_goal", monster_kill_goal );
    json.member( "deadline", deadline );
    json.member( "npc_id", npc_id );
    json.member( "good_fac_id", good_fac_id );
    json.member( "bad_fac_id", bad_fac_id );
    json.member( "step", step );
    json.member( "follow_up", follow_up );
    json.member( "player_id", player_id );
    json.member( "legacy_no_player_id", legacy_no_player_id );

    json.end_object();
}

////////////////// faction.h
////
void faction::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "id", id );
    jo.read( "name", name );
    if( !jo.read( "desc", desc ) ) {
        desc.clear();
    }
    goal = faction_goal( jo.get_int( "goal", goal ) );
    values = jo.get_int( "values", values );
    job1 = faction_job( jo.get_int( "job1", job1 ) );
    job2 = faction_job( jo.get_int( "job2", job2 ) );
    jo.read( "likes_u", likes_u );
    jo.read( "respects_u", respects_u );
    jo.read( "known_by_u", known_by_u );
    jo.read( "strength", strength );
    jo.read( "sneak", sneak );
    jo.read( "crime", crime );
    jo.read( "cult", cult );
    jo.read( "good", good );
    jo.read( "mapx", mapx );
    jo.read( "mapy", mapy );
    // omx,omy are obsolete, use them (if present) to make mapx,mapy global coordinates
    int o = 0;
    if( jo.read( "omx", o ) ) {
        mapx += o * OMAPX * 2;
    }
    if( jo.read( "omy", o ) ) {
        mapy += o * OMAPY * 2;
    }
    jo.read( "size", size );
    jo.read( "power", power );
    if( !jo.read( "combat_ability", combat_ability ) ) {
        combat_ability = 100;
    }
    if( !jo.read( "food_supply", food_supply ) ) {
        food_supply = 100;
    }
    if( !jo.read( "wealth", wealth ) ) {
        wealth = 100;
    }
    if( jo.has_array( "opinion_of" ) ) {
        opinion_of = jo.get_int_array( "opinion_of" );
    }
}

void faction::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "id", id );
    json.member( "name", name );
    json.member( "desc", desc );
    json.member( "values", values );
    json.member( "goal", goal );
    json.member( "job1", job1 );
    json.member( "job2", job2 );
    json.member( "likes_u", likes_u );
    json.member( "respects_u", respects_u );
    json.member( "known_by_u", known_by_u );
    json.member( "strength", strength );
    json.member( "sneak", sneak );
    json.member( "crime", crime );
    json.member( "cult", cult );
    json.member( "good", good );
    json.member( "mapx", mapx );
    json.member( "mapy", mapy );
    json.member( "size", size );
    json.member( "power", power );
    json.member( "combat_ability", combat_ability );
    json.member( "food_supply", food_supply );
    json.member( "wealth", wealth );
    json.member( "opinion_of", opinion_of );

    json.end_object();
}

void Creature::store( JsonOut &jsout ) const
{
    jsout.member( "moves", moves );
    jsout.member( "pain", pain );

    // killer is not stored, it's temporary anyway, any creature that has a non-null
    // killer is dead (as per definition) and should not be stored.

    // Because JSON requires string keys we need to convert our int keys
    std::unordered_map<std::string, std::unordered_map<std::string, effect>> tmp_map;
    for( auto maps : *effects ) {
        for( const auto i : maps.second ) {
            std::ostringstream convert;
            convert << i.first;
            tmp_map[maps.first.str()][convert.str()] = i.second;
        }
    }
    jsout.member( "effects", tmp_map );

    jsout.member( "values", values );

    jsout.member( "blocks_left", num_blocks );
    jsout.member( "dodges_left", num_dodges );
    jsout.member( "num_blocks_bonus", num_blocks_bonus );
    jsout.member( "num_dodges_bonus", num_dodges_bonus );

    jsout.member( "armor_bash_bonus", armor_bash_bonus );
    jsout.member( "armor_cut_bonus", armor_cut_bonus );

    jsout.member( "speed", speed_base );

    jsout.member( "speed_bonus", speed_bonus );
    jsout.member( "dodge_bonus", dodge_bonus );
    jsout.member( "block_bonus", block_bonus );
    jsout.member( "hit_bonus", hit_bonus );
    jsout.member( "bash_bonus", bash_bonus );
    jsout.member( "cut_bonus", cut_bonus );

    jsout.member( "bash_mult", bash_mult );
    jsout.member( "cut_mult", cut_mult );
    jsout.member( "melee_quiet", melee_quiet );

    jsout.member( "grab_resist", grab_resist );
    jsout.member( "throw_resist", throw_resist );

    // fake is not stored, it's temporary anyway, only used to fire with a gun.
}

void Creature::load( JsonObject &jsin )
{
    jsin.read( "moves", moves );
    jsin.read( "pain", pain );

    killer = nullptr; // see Creature::load

    // Just too many changes here to maintain compatibility, so older characters get a free
    // effects wipe. Since most long lasting effects are bad, this shouldn't be too bad for them.
    if( savegame_loading_version >= 23 ) {
        if( jsin.has_object( "effects" ) ) {
            // Because JSON requires string keys we need to convert back to our bp keys
            std::unordered_map<std::string, std::unordered_map<std::string, effect>> tmp_map;
            jsin.read( "effects", tmp_map );
            int key_num = 0;
            for( auto maps : tmp_map ) {
                const efftype_id id( maps.first );
                if( !id.is_valid() ) {
                    debugmsg( "Invalid effect: %s", id.c_str() );
                    continue;
                }
                for( auto i : maps.second ) {
                    if( !( std::istringstream( i.first ) >> key_num ) ) {
                        key_num = 0;
                    }
                    const body_part bp = static_cast<body_part>( key_num );
                    effect &e = i.second;

                    ( *effects )[id][bp] = e;
                    on_effect_int_change( id, e.get_intensity(), bp );
                }
            }
        }
    }
    jsin.read( "values", values );

    jsin.read( "blocks_left", num_blocks );
    jsin.read( "dodges_left", num_dodges );
    jsin.read( "num_blocks_bonus", num_blocks_bonus );
    jsin.read( "num_dodges_bonus", num_dodges_bonus );

    jsin.read( "armor_bash_bonus", armor_bash_bonus );
    jsin.read( "armor_cut_bonus", armor_cut_bonus );

    jsin.read( "speed", speed_base );

    jsin.read( "speed_bonus", speed_bonus );
    jsin.read( "dodge_bonus", dodge_bonus );
    jsin.read( "block_bonus", block_bonus );
    jsin.read( "hit_bonus", hit_bonus );
    jsin.read( "bash_bonus", bash_bonus );
    jsin.read( "cut_bonus", cut_bonus );

    jsin.read( "bash_mult", bash_mult );
    jsin.read( "cut_mult", cut_mult );
    jsin.read( "melee_quiet", melee_quiet );

    jsin.read( "grab_resist", grab_resist );
    jsin.read( "throw_resist", throw_resist );

    jsin.read( "underwater", underwater );

    fake = false; // see Creature::load

    on_stat_change( "pain", pain );
}

void player_morale::morale_point::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    if( !jo.read( "type", type ) ) {
        type = morale_type_data::convert_legacy( jo.get_int( "type_enum" ) );
    }
    std::string tmpitype;
    if( jo.read( "item_type", tmpitype ) && item::type_is_defined( tmpitype ) ) {
        item_type = item::find_type( tmpitype );
    }
    jo.read( "bonus", bonus );
    jo.read( "duration", duration );
    jo.read( "decay_start", decay_start );
    jo.read( "age", age );
}

void player_morale::morale_point::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "type", type );
    if( item_type != nullptr ) {
        // TODO: refactor player_morale to not require this hack
        json.member( "item_type", item_type->get_id() );
    }
    json.member( "bonus", bonus );
    json.member( "duration", duration );
    json.member( "decay_start", decay_start );
    json.member( "age", age );
    json.end_object();
}

void player_morale::store( JsonOut &jsout ) const
{
    jsout.member( "morale", points );
}

void player_morale::load( JsonObject &jsin )
{
    jsin.read( "morale", points );
}

void map_memory::store( JsonOut &jsout ) const
{
    jsout.start_array();
    jsout.start_array();
    for( const auto &elem : tile_cache.list() ) {
        jsout.start_array();
        jsout.write( elem.first.x );
        jsout.write( elem.first.y );
        jsout.write( elem.first.z );
        jsout.write( elem.second.tile );
        jsout.write( elem.second.subtile );
        jsout.write( elem.second.rotation );
        jsout.end_array();
    }
    jsout.end_array();

    jsout.start_array();
    for( const auto &elem : symbol_cache.list() ) {
        jsout.start_array();
        jsout.write( elem.first.x );
        jsout.write( elem.first.y );
        jsout.write( elem.first.z );
        jsout.write( elem.second );
        jsout.end_array();
    }
    jsout.end_array();
    jsout.end_array();
}

void map_memory::load( JsonIn &jsin )
{
    // Legacy loading of object version.
    if( jsin.test_object() ) {
        JsonObject jsobj = jsin.get_object();
        load( jsobj );
    } else {
        // This file is large enough that it's more than called for to minimize the
        // amount of data written and read and make it a bit less "friendly",
        // and use the streaming interface.
        jsin.start_array();
        tile_cache.clear();
        jsin.start_array();
        while( !jsin.end_array() ) {
            jsin.start_array();
            tripoint p;
            p.x = jsin.get_int();
            p.y = jsin.get_int();
            p.z = jsin.get_int();
            const std::string tile = jsin.get_string();
            const int subtile = jsin.get_int();
            const int rotation = jsin.get_int();
            memorize_tile( std::numeric_limits<int>::max(), p,
                           tile, subtile, rotation );
            jsin.end_array();
        }
        symbol_cache.clear();
        jsin.start_array();
        while( !jsin.end_array() ) {
            jsin.start_array();
            tripoint p;
            p.x = jsin.get_int();
            p.y = jsin.get_int();
            p.z = jsin.get_int();
            const long symbol = jsin.get_long();
            memorize_symbol( std::numeric_limits<int>::max(), p, symbol );
            jsin.end_array();
        }
        jsin.end_array();
    }
}

// Deserializer for legacy object-based memory map.
void map_memory::load( JsonObject &jsin )
{
    JsonArray map_memory_tiles = jsin.get_array( "map_memory_tiles" );
    tile_cache.clear();
    while( map_memory_tiles.has_more() ) {
        JsonObject pmap = map_memory_tiles.next_object();
        const tripoint p( pmap.get_int( "x" ), pmap.get_int( "y" ), pmap.get_int( "z" ) );
        memorize_tile( std::numeric_limits<int>::max(), p, pmap.get_string( "tile" ),
                       pmap.get_int( "subtile" ), pmap.get_int( "rotation" ) );
    }

    JsonArray map_memory_curses = jsin.get_array( "map_memory_curses" );
    symbol_cache.clear();
    while( map_memory_curses.has_more() ) {
        JsonObject pmap = map_memory_curses.next_object();
        const tripoint p( pmap.get_int( "x" ), pmap.get_int( "y" ), pmap.get_int( "z" ) );
        memorize_symbol( std::numeric_limits<int>::max(), p, pmap.get_long( "symbol" ) );
    }
}

void deserialize( point &p, JsonIn &jsin )
{
    jsin.start_array();
    p.x = jsin.get_int();
    p.y = jsin.get_int();
    jsin.end_array();
}

void serialize( const point &p, JsonOut &jsout )
{
    jsout.start_array();
    jsout.write( p.x );
    jsout.write( p.y );
    jsout.end_array();
}

void tripoint::deserialize( JsonIn &jsin )
{
    jsin.start_array();
    x = jsin.get_int();
    y = jsin.get_int();
    z = jsin.get_int();
    jsin.end_array();
}

void tripoint::serialize( JsonOut &jsout ) const
{
    jsout.start_array();
    jsout.write( x );
    jsout.write( y );
    jsout.write( z );
    jsout.end_array();
}

void addiction::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "type_enum", type );
    json.member( "intensity", intensity );
    json.member( "sated", sated );
    json.end_object();
}

void addiction::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    type = static_cast<add_type>( jo.get_int( "type_enum" ) );
    intensity = jo.get_int( "intensity" );
    jo.read( "sated", sated );
}

void stats::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "squares_walked", squares_walked );
    json.member( "damage_taken", damage_taken );
    json.member( "damage_healed", damage_healed );
    json.member( "headshots", headshots );
    json.end_object();
}

void stats::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    jo.read( "squares_walked", squares_walked );
    jo.read( "damage_taken", damage_taken );
    jo.read( "damage_healed", damage_healed );
    jo.read( "headshots", headshots );
}

void serialize( const recipe_subset &value, JsonOut &jsout )
{
    jsout.start_array();
    for( const auto &entry : value ) {
        jsout.write( entry->ident() );
    }
    jsout.end_array();
}

void deserialize( recipe_subset &value, JsonIn &jsin )
{
    value.clear();
    jsin.start_array();
    while( !jsin.end_array() ) {
        value.include( &recipe_id( jsin.get_string() ).obj() );
    }
}

// basecamp
void basecamp::serialize( JsonOut &json ) const
{
    if( omt_pos != tripoint_zero ) {
        json.start_object();
        json.member( "name", name );
        json.member( "pos", omt_pos );
        json.member( "bb_pos", bb_pos );
        json.member( "sort_points" );
        json.start_array();
        for( const tripoint &it : sort_points ) {
            json.start_object();
            json.member( "pos", it );
            json.end_object();
        }
        json.end_array();
        json.member( "expansions" );
        json.start_array();
        for( const auto &expansion : expansions ) {
            json.start_object();
            json.member( "dir", expansion.first );
            json.member( "type", expansion.second.type );
            json.member( "cur_level", expansion.second.cur_level );
            json.member( "pos", expansion.second.pos );
            json.end_object();
        }
        json.end_array();
        json.end_object();
    } else {
        return;
    }
}

void basecamp::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    data.read( "name", name );
    data.read( "pos", omt_pos );
    data.read( "bb_pos", bb_pos );
    JsonArray ja = data.get_array( "expansions" );
    while( ja.has_more() ) {
        JsonObject edata = ja.next_object();
        expansion_data e;
        const std::string dir = edata.get_string( "dir" );
        edata.read( "type", e.type );
        edata.read( "cur_level", e.cur_level );
        edata.read( "pos", e.pos );
        expansions[ dir ] = e;
        if( dir != "[B]" ) {
            directions.push_back( dir );
        }
    }
}
