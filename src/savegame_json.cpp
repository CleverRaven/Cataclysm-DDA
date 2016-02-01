#include "player.h"
#include "npc.h"
#include "profession.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "rng.h"
#include "addiction.h"
#include "inventory.h"
#include "artifact.h"
#include "options.h"
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include "weather.h"
#include "item.h"
#include "material.h"
#include "translations.h"
#include "name.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "crafting.h"
#include "get_version.h"
#include "scenario.h"
#include "monster.h"
#include "monfaction.h"
#include "morale.h"
#include "veh_type.h"
#include "vehicle.h"
#include "mutation.h"
#include "io.h"
#include "mtype.h"

#include "tile_id_data.h" // for monster::json_save
#include <ctime>
#include <bitset>

#include "json.h"

#include "debug.h"
#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

const std::string obj_type_name[11]={ "OBJECT_NONE", "OBJECT_ITEM", "OBJECT_ACTOR", "OBJECT_PLAYER",
    "OBJECT_NPC", "OBJECT_MONSTER", "OBJECT_VEHICLE", "OBJECT_TRAP", "OBJECT_FIELD",
    "OBJECT_TERRAIN", "OBJECT_FURNITURE"
};

std::vector<item> item::magazine_convert() {
    std::vector<item> res;

    // only guns, auxiliary gunmods and tools require conversion
    if( !is_gun() && !is_tool() ) {
        return res;
    }

    // only consider items without integral magazines that have not yet been converted
    if( magazine_integral() || has_var( "magazine_converted" ) ) {
        return res;
    }

    item mag( type->magazine_default, calendar::turn );
    item ammo( get_curammo() ? get_curammo()->id : default_ammo( ammo_type() ), calendar::turn );

    // give base item an appropriate magazine and add to that any ammo originally stored in base item
    if( !magazine_current() ) {
        contents.push_back( mag );
        if( charges > 0 ) {
            ammo.charges = std::min( charges, mag.ammo_capacity() );
            charges -= ammo.charges;
            contents.back().contents.push_back( ammo );
        }
    }

    // remove any spare magazine and place an equivalent loaded magazine in inventory
    item *spare_mag = has_gunmod( "spare_mag" ) >= 0 ? &contents[ has_gunmod( "spare_mag" ) ] : nullptr;
    if( spare_mag ) {
        res.push_back( mag );
        if( spare_mag->charges > 0 ) {
            ammo.charges = std::min( spare_mag->charges, mag.ammo_capacity() );
            charges += spare_mag->charges - ammo.charges;
            res.back().contents.push_back( ammo );
        }
    }

    // return any excess ammo (from either item or spare mag) to character inventory
    if( charges > 0 ) {
        ammo.charges = charges;
        res.push_back( ammo );
    }

    // remove incompatible magazine mods
    contents.erase( std::remove_if( contents.begin(), contents.end(), []( const item& e ) {
        return e.typeId() == "spare_mag" || e.typeId() == "clip" || e.typeId() == "clip2";
    } ), contents.end() );

    // normalize the base item and mark it as converted
    charges = 0;
    unset_curammo();
    set_var( "magazine_converted", true );

    return res;
}

////////////////////////////////////////////////////////////////////////////////////////
///// on runtime populate lookup tables.
std::map<std::string, int> obj_type_id;

void game::init_savedata_translation_tables() {
  obj_type_id.clear();
  for(int i = 0; i < NUM_OBJECTS; i++) {
    obj_type_id[ obj_type_name[i] ] = i;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h

void player_activity::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "type", int(type) );
    json.member( "moves_left", moves_left );
    json.member( "index", index );
    json.member( "position", position );
    json.member( "coords", coords );
    json.member( "name", name );
    json.member( "placement", placement );
    json.member( "values", values );
    json.member( "str_values", str_values );
    json.member( "auto_resume", auto_resume );
    json.end_object();
}

void player_activity::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int tmptype;
    int tmppos;
    if ( !data.read( "type", tmptype ) || type >= NUM_ACTIVITIES ) {
        debugmsg( "Bad activity data:\n%s", data.str().c_str() );
    }
    if ( !data.read( "position", tmppos)) {
        tmppos = INT_MIN;  // If loading a save before position existed, hope.
    }
    type = activity_type(tmptype);
    data.read( "moves_left", moves_left );
    data.read( "index", index );
    position = tmppos;
    data.read( "coords", coords );
    data.read( "name", name );
    data.read( "placement", placement );
    values = data.get_int_array("values");
    str_values = data.get_string_array("str_values");
    data.read( "auto_resume", auto_resume );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// skill.h
void SkillLevel::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("level", level() );
    json.member("exercise", exercise(true) );
    json.member("istraining", isTraining() );
    json.member("lastpracticed", int ( lastPracticed() ) );
    json.end_object();
}

void SkillLevel::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int lastpractice = 0;
    data.read( "level", _level );
    data.read( "exercise", _exercise );
    data.read( "istraining", _isTraining );
    data.read( "lastpracticed", lastpractice );
    if(lastpractice == 0) {
        _lastPracticed = HOURS(OPTIONS["INITIAL_TIME"]);
    } else {
        _lastPracticed = lastpractice;
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
void Character::load(JsonObject &data)
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
    data.read("hunger", hunger);
    data.read( "stomach_food", stomach_food);
    data.read( "stomach_water", stomach_water);

    // health
    data.read( "healthy", healthy );
    data.read( "healthy_mod", healthy_mod );

    JsonArray parray;

    data.read("underwater", underwater);

    data.read("traits", my_traits);
    for( auto it = my_traits.begin(); it != my_traits.end(); ) {
        const auto &tid = *it;
        if( mutation_branch::has( tid ) ) {
            ++it;
        } else {
            debugmsg( "character %s has invalid trait %s, it will be ignored", name.c_str(), tid.c_str() );
            my_traits.erase( it++ );
        }
    }

    if( savegame_loading_version <= 23 ) {
        std::unordered_set<std::string> old_my_mutations;
        data.read( "mutations", old_my_mutations );
        for( const auto & mut : old_my_mutations ) {
            my_mutations[mut]; // Creates a new entry with default values
        }
        std::map<std::string, char> trait_keys;
        data.read( "mutation_keys", trait_keys );
        for( const auto & k : trait_keys ) {
            my_mutations[k.first].key = k.second;
        }
        std::set<std::string> active_muts;
        data.read( "active_mutations_hacky", active_muts );
        for( const auto & mut : active_muts ) {
            my_mutations[mut].powered = true;
        }
    } else {
        data.read( "mutations", my_mutations );
    }
    for( auto it = my_mutations.begin(); it != my_mutations.end(); ) {
        const auto &mid = it->first;
        if( mutation_branch::has( mid ) ) {
            ++it;
        } else {
            debugmsg( "character %s has invalid mutation %s, it will be ignored", name.c_str(), mid.c_str() );
            my_mutations.erase( it++ );
        }
    }

    data.read( "my_bionics", my_bionics );

    worn.clear();
    data.read( "worn", worn );

    if( !data.read( "hp_cur", hp_cur ) ) {
        debugmsg("Error, incompatible hp_cur in save file '%s'", parray.str().c_str());
    }

    if( !data.read( "hp_max", hp_max ) ) {
        debugmsg("Error, incompatible hp_max in save file '%s'", parray.str().c_str());
    }

    inv.clear();
    if ( data.has_member( "inv" ) ) {
        JsonIn *invin = data.get_raw( "inv" );
        inv.json_load_items( *invin );
    }

    weapon = item( "null", 0 );
    data.read( "weapon", weapon );

    if (data.has_object("skills")) {
        JsonObject pmap = data.get_object("skills");
        for( auto &skill : Skill::skills ) {
            if( pmap.has_object( skill.ident().str() ) ) {
                pmap.read( skill.ident().str(), skillLevel( &skill ) );
            } else {
                debugmsg( "Load (%s) Missing skill %s", "", skill.ident().c_str() );
            }
        }
    } else {
        debugmsg("Skills[] no bueno");
    }

    visit_items([&]( item *it, item * /* parent */ ) {
        for( auto& e: it->magazine_convert() ) {
            i_add( e );
        }
        return VisitResponse::NEXT;
    } );
}

void Character::store(JsonOut &json) const
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
    json.member( "hunger", hunger );
    json.member( "stomach_food", stomach_food );
    json.member( "stomach_water", stomach_water );

    // breathing
    json.member( "underwater", underwater );

    // traits: permanent 'mutations' more or less
    json.member( "traits", my_traits );
    json.member( "mutations", my_mutations );

    // "Fracking Toasters" - Saul Tigh, toaster
    json.member( "my_bionics", my_bionics );

    // skills
    json.member( "skills" );
    json.start_object();
    for( auto const &skill : Skill::skills ) {
        json.member( skill.ident().str(), get_skill_level(skill) );
    }
    json.end_object();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h, player (+ npc for now, should eventually only be the player)
/*
 * Gather variables for saving.
 */

void player::load(JsonObject &data)
{
    Character::load( data );

    JsonArray parray;
    int tmpid = 0;

    if(!data.read("posx", position.x) ) { // uh-oh.
        debugmsg("BAD PLAYER/NPC JSON: no 'posx'?");
    }
    data.read("posy", position.y);
    if( !data.read("posz", position.z) && g != nullptr ) {
      position.z = g->get_levz();
    }
    data.read("thirst", thirst);
    data.read("fatigue", fatigue);
    data.read("stim", stim);
    data.read("pkill", pkill);
    data.read("radiation", radiation);
    data.read("tank_plut", tank_plut);
    data.read("reactor_plut", reactor_plut);
    data.read("slow_rad", slow_rad);
    data.read("scent", scent);
    data.read("oxygen", oxygen);
    data.read("male", male);
    data.read("cash", cash);
    data.read("recoil", recoil);
    data.read("in_vehicle", in_vehicle);
    if( data.read( "id", tmpid ) ) {
        setID( tmpid );
    }

    data.read("power_level", power_level);
    data.read("max_power_level", max_power_level);
    // Bionic power scale has been changed, savegame version 21 has the new scale
    if( savegame_loading_version <= 20 ) {
        power_level *= 25;
        max_power_level *= 25;
    }

    // Bionic power should not be negative!
    if( power_level < 0) {
        power_level = 0;
    }

    data.read("ma_styles", ma_styles);
    data.read( "addictions", addictions );

    JsonArray traps = data.get_array("known_traps");
    known_traps.clear();
    while(traps.has_more()) {
        JsonObject pmap = traps.next_object();
        const tripoint p(pmap.get_int("x"), pmap.get_int("y"), pmap.get_int("z"));
        const std::string t = pmap.get_string("trap");
        known_traps.insert(trap_map::value_type(p, t));
    }

    // Add the earplugs.
    if( has_bionic( "bio_ears" ) && !has_bionic( "bio_earplugs" ) ) {
        add_bionic("bio_earplugs");
    }

    // Add the blindfold.
    if( has_bionic( "bio_sunglasses" ) && !has_bionic( "bio_blindfold" ) ) {
        add_bionic( "bio_blindfold" );
    }

    // Fixes bugged characters for telescopic eyes CBM.
    if( has_bionic( "bio_eye_optic" ) && has_trait( "HYPEROPIC" ) ) {
        remove_mutation( "HYPEROPIC" );
    }

    if( has_bionic( "bio_eye_optic" ) && has_trait( "MYOPIC" ) ) {
        remove_mutation( "MYOPIC" );
    }


}

/*
 * Variables common to player (and npc's, should eventually just be players)
 */
void player::store(JsonOut &json) const
{
    Character::store( json );

    // assumes already in player object
    // positional data
    json.member( "posx", position.x );
    json.member( "posy", position.y );
    json.member( "posz", position.z );

    // om-noms or lack thereof
    json.member( "thirst", thirst );
    // energy
    json.member( "fatigue", fatigue );
    json.member( "stim", stim );
    // pain
    json.member( "pkill", pkill );
    // misc levels
    json.member( "radiation", radiation );
    json.member( "tank_plut", tank_plut );
    json.member( "reactor_plut", reactor_plut );
    json.member( "slow_rad", slow_rad );
    json.member( "scent", int(scent) );
    json.member( "body_wetness", body_wetness );

    // breathing
    json.member( "oxygen", oxygen );

    // gender
    json.member( "male", male );

    json.member( "cash", cash );
    json.member( "recoil", int(recoil) );
    json.member( "in_vehicle", in_vehicle );
    json.member( "id", getID() );

    // potential incompatibility with future expansion
    // todo: consider ["parts"]["head"]["hp_cur"] instead of ["hp_cur"][head_enum_value]
    json.member( "hp_cur", hp_cur );
    json.member( "hp_max", hp_max );

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
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h, player
/*
 * Prepare a json object for player, including player specific data, and data common to
   players and npcs ( which json_save_actor_data() handles ).
 */
void player::serialize(JsonOut &json) const
{
    json.start_object();
    // This must be after the json object has been started, so any super class
    // puts their data into the same json object.
    store( json );

    // TODO: once npcs are seperated from the player class,
    // this code should go into player::store, serialize will then only
    // contain start_object(), store(), end_object().

    // player-specific specifics
    if ( prof != NULL ) {
        json.member( "profession", prof->ident() );
    }
    if ( g->scen != NULL ) {
        json.member( "scenario", g->scen->ident() );
    }
    // someday, npcs may drive
    json.member( "driving_recoil", int(driving_recoil) );
    json.member( "controlling_vehicle", controlling_vehicle );

    // shopping carts, furniture etc
    json.member( "grab_point", grab_point );
    json.member( "grab_type", obj_type_name[ (int)grab_type ] );

    // misc player specific stuff
    json.member( "focus_pool", focus_pool );
    json.member( "style_selected", style_selected );
    json.member( "keep_hands_free", keep_hands_free );

    json.member( "stamina", stamina);
    json.member( "move_mode", move_mode );

    // crafting etc
    json.member( "activity", activity );
    json.member( "backlog", backlog );

    // "The cold wakes you up."
    json.member( "temp_cur", temp_cur );
    json.member( "temp_conv", temp_conv );
    json.member( "frostbite_timer", frostbite_timer );

    // npc: unimplemented, potentially useful
    json.member( "learned_recipes" );
    json.start_array();
    for( auto iter = learned_recipes.cbegin(); iter != learned_recipes.cend(); ++iter ) {
        json.write( iter->first );
    }
    json.end_array();

    // Player only, books they have read at least once.
    json.member( "items_identified", items_identified );

    // :(
    json.member( "morale", morale );

    // mission stuff
    json.member("active_mission", active_mission == nullptr ? -1 : active_mission->get_id() );

    json.member( "active_missions", mission::to_uid_vector( active_missions ) );
    json.member( "completed_missions", mission::to_uid_vector( completed_missions ) );
    json.member( "failed_missions", mission::to_uid_vector( failed_missions ) );

    json.member( "player_stats", get_stats() );

    json.member("assigned_invlet");
    json.start_array();
    for (auto iter : assigned_invlet) {
        json.start_array();
        json.write(iter.first);
        json.write(iter.second);
        json.end_array();
    }
    json.end_array();

    json.member("invcache");
    inv.json_save_invcache(json);

    //FIXME: seperate function, better still another file
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
void player::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    JsonArray parray;

    load( data );

    // TODO: once npcs are seperated from the player class,
    // this code should go into player::load, deserialize will then only
    // contain get_object(), load()

    std::string prof_ident = "(null)";
    if ( data.read("profession", prof_ident) && string_id<profession>( prof_ident ).is_valid() ) {
        prof = &string_id<profession>( prof_ident ).obj();
    } else {
        debugmsg("Tried to use non-existent profession '%s'", prof_ident.c_str());
    }

    data.read("activity", activity);
    // Changed from a single element to a list, handle either.
    // Can deprecate once we stop handling pre-0.B saves.
    if( data.has_array("backlog") ) {
        data.read("backlog", backlog);
    } else {
        player_activity temp;
        data.read("backlog", temp);
        backlog.push_front( temp );
    }

    data.read("driving_recoil", driving_recoil);
    data.read("controlling_vehicle", controlling_vehicle);

    data.read("grab_point", grab_point);
    std::string grab_typestr = "OBJECT_NONE";
    if( grab_point.x != 0 || grab_point.y != 0 ) {
        grab_typestr = "OBJECT_VEHICLE";
        data.read( "grab_type", grab_typestr);
    }

    if ( obj_type_id.find(grab_typestr) != obj_type_id.end() ) {
        grab_type = (object_type)obj_type_id[grab_typestr];
    }

    data.read( "focus_pool", focus_pool);
    data.read( "style_selected", style_selected );
    data.read( "keep_hands_free", keep_hands_free );

    data.read( "stamina", stamina);
    data.read( "move_mode", move_mode );

    set_highest_cat_level();
    drench_mut_calc();
    std::string scen_ident="(null)";
    if ( data.read("scenario",scen_ident) && string_id<scenario>( scen_ident ).is_valid() ) {
        g->scen = &string_id<scenario>( scen_ident ).obj();
        start_location = g->scen->start_location();
    } else {
        const scenario *generic_scenario = scenario::generic();
        // Only display error message if from a game file after scenarios existed.
        if (savegame_loading_version > 20) {
            debugmsg("Tried to use non-existent scenario '%s'. Setting to generic '%s'.",
                        scen_ident.c_str(), generic_scenario->ident().c_str());
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

    parray = data.get_array("learned_recipes");
    if ( !parray.empty() ) {
        std::string pstr = "";
        learned_recipes.clear();
        while ( parray.has_more() ) {
            if ( parray.read_next(pstr) ) {
                learned_recipes[ pstr ] = (recipe *)recipe_by_name( pstr );
            }
        }
    }

    items_identified.clear();
    data.read( "items_identified", items_identified );

    data.read("morale", morale);

    int tmpactive_mission;
    if( data.read( "active_mission", tmpactive_mission ) && tmpactive_mission != -1 ) {
        active_mission = mission::find( tmpactive_mission );
    }

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

    // Normally there is only one player character loaded, so if a mission that is assigned to
    // another character (not the current one) fails, the other character(s) are not informed.
    // We must inform them when they get loaded the next time.
    // Only active missions need checking, failed/complete will not change anymore.
    auto const last = std::remove_if( active_missions.begin(), active_missions.end(), []( mission const *m ) {
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

    stats &pstats = *lifetime_stats();
    data.read("player_stats", pstats);

    parray = data.get_array("assigned_invlet");
    while (parray.has_more()) {
        JsonArray pair = parray.next_array();
        assigned_invlet[(char)pair.get_int(0)] = pair.get_string(1);
    }

    if ( data.has_member("invcache") ) {
        JsonIn *jip = data.get_raw("invcache");
        inv.json_load_invcache( *jip );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// npc.h

void npc_follower_rules::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("engagement", (int)engagement );
    json.member("use_guns", use_guns );
    json.member("use_grenades", use_grenades );
    json.member("use_silent", use_silent );

    json.member( "allow_pick_up", allow_pick_up );
    json.member( "allow_bash", allow_bash );
    json.member( "allow_sleep", allow_sleep );
    json.member( "allow_complain", allow_complain );
    json.end_object();
}

void npc_follower_rules::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int tmpeng;
    data.read("engagement", tmpeng);
    engagement = (combat_engagement)tmpeng;
    data.read( "use_guns", use_guns);
    data.read( "use_grenades", use_grenades);
    data.read( "use_silent", use_silent);

    data.read( "allow_pick_up", allow_pick_up );
    data.read( "allow_bash", allow_bash );
    data.read( "allow_sleep", allow_sleep );
    data.read( "allow_complain", allow_complain );
}

extern std::string convert_talk_topic( talk_topic_enum );

void npc_chatbin::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "first_topic", first_topic );
    if( mission_selected != nullptr ) {
        json.member( "mission_selected", mission_selected->get_id() );
    }
    if ( skill ) {
        json.member("skill", skill->ident() );
    }
    json.member( "missions", mission::to_uid_vector( missions ) );
    json.member( "missions_assigned", mission::to_uid_vector( missions_assigned ) );
    json.end_object();
}

void npc_chatbin::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    std::string skill_ident;

    if( data.has_int( "first_topic" ) ) {
        int tmptopic;
        data.read("first_topic", tmptopic);
        first_topic = convert_talk_topic( talk_topic_enum(tmptopic) );
    } else {
        data.read("first_topic", first_topic);
    }

    if ( data.read("skill", skill_ident) ) {
        skill = &skill_id( skill_ident ).obj();
    }

    std::vector<int> tmpmissions;
    data.read( "missions", tmpmissions );
    missions = mission::to_ptr_vector( tmpmissions );
    std::vector<int> tmpmissions_assigned;
    data.read( "missions_assigned", tmpmissions_assigned );
    missions_assigned = mission::to_ptr_vector( tmpmissions_assigned );
    int tmpmission_selected;
    mission_selected = nullptr;
    if( savegame_loading_version <= 23 ) {
        mission_selected = nullptr; // player can re-select which mision to talk about in the dialog
    } else if( data.read( "mission_selected", tmpmission_selected ) && tmpmission_selected != -1 ) {
        mission_selected = mission::find( tmpmission_selected );
    }
}

void npc_personality::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int tmpagg, tmpbrav, tmpcol, tmpalt;
    if ( data.read("aggression", tmpagg) &&
         data.read("bravery", tmpbrav) &&
         data.read("collector", tmpcol) &&
         data.read("altruism", tmpalt) ) {
        aggression = (signed char)tmpagg;
        bravery = (signed char)tmpbrav;
        collector = (signed char)tmpcol;
        altruism = (signed char)tmpalt;
    } else {
        debugmsg("npc_personality: bad data");
    }
}

void npc_personality::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "aggression", (int)aggression );
    json.member( "bravery", (int)bravery );
    json.member( "collector", (int)collector );
    json.member( "altruism", (int)altruism );
    json.end_object();
}

void npc_opinion::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    data.read("trust", trust);
    data.read("fear", fear);
    data.read("value", value);
    data.read("anger", anger);
    data.read("owed", owed);
    data.read("favors", favors);
}

void npc_opinion::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "trust", trust );
    json.member( "fear", fear );
    json.member( "value", value );
    json.member( "anger", anger );
    json.member( "owed", owed );
    json.member( "favors", favors );
    json.end_object();
}

void npc_favor::deserialize(JsonIn &jsin)
{
    JsonObject jo = jsin.get_object();
    type = npc_favor_type(jo.get_int("type"));
    jo.read("value", value);
    jo.read("itype_id", item_id);
    skill = NULL;
    if (jo.has_int("skill_id")) {
        skill = Skill::from_legacy_int( jo.get_int("skill_id") );
    } else if (jo.has_string("skill_id")) {
        skill = &skill_id( jo.get_string("skill_id") ).obj();
    }
}

void npc_favor::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("type", (int)type);
    json.member("value", value);
    json.member("itype_id", (std::string)item_id);
    if (skill != NULL) {
        json.member("skill_id", skill->ident());
    }
    json.end_object();
}

/*
 * load npc
 */
void npc::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    load( data );
}

void npc::load(JsonObject &data)
{
    // TODO: once npcs are seperated from the player class,
    // this should call load on the parent class of npc (probably Character).
    player::load( data );

    int misstmp, classtmp, flagstmp, atttmp, comp_miss_t, stock;
    std::string facID, comp_miss;

    data.read("name", name);
    data.read("marked_for_death", marked_for_death);
    data.read("dead", dead);
    if ( data.read( "myclass", classtmp) ) {
        myclass = npc_class( classtmp );
    }

    data.read("personality", personality);

    data.read( "wandf", wander_time );
    data.read( "wandx", wander_pos.x );
    data.read( "wandy", wander_pos.y );
    if( !data.read( "wandz", wander_pos.z ) ) {
        wander_pos.z = posz();
    }

    data.read("mapx", mapx);
    data.read("mapy", mapy);
    if(!data.read("mapz", position.z)) {
        data.read("omz", position.z); // omz/mapz got moved to position.z
    }
    int o;
    if(data.read("omx", o)) {
        mapx += o * OMAPX * 2;
    }
    if(data.read("omy", o)) {
        mapy += o * OMAPY * 2;
    }

    data.read( "plx", last_player_seen_pos.x );
    data.read( "ply", last_player_seen_pos.y );
    if( !data.read( "plz", last_player_seen_pos.z ) ) {
        last_player_seen_pos.z = posz();
    }

    data.read( "goalx", goal.x );
    data.read( "goaly", goal.y );
    data.read( "goalz", goal.z );

    data.read( "guardx", guard_pos.x );
    data.read( "guardy", guard_pos.y );
    data.read( "guardz", guard_pos.z );

    if ( data.read("mission", misstmp) ) {
        mission = npc_mission( misstmp );
    }

    if ( data.read( "flags", flagstmp) ) {
        flags = flagstmp;
    }

    if ( data.read( "my_fac", facID) ) {
        fac_id = facID;
    }

    if ( data.read( "attitude", atttmp) ) {
        attitude = npc_attitude(atttmp);
    }

    if ( data.read( "companion_mission", comp_miss) ) {
        companion_mission = comp_miss;
    }

    if ( data.read( "companion_mission_time", comp_miss_t) ) {
        companion_mission_time = comp_miss_t;
    }

    if ( data.read( "restock", stock) ) {
        restock = stock;
    }

    data.read("op_of_u", op_of_u);
    data.read("chatbin", chatbin);
    if( !data.read( "rules", rules ) ) {
        data.read("misc_rules", rules);
        data.read("combat_rules", rules);
    }

    last_updated = data.get_int( "last_updated", calendar::turn );
    if( data.has_object( "complaints" ) ) {
        data.read( "complaints", complaints );
    }
}

/*
 * save npc
 */
void npc::serialize(JsonOut &json) const
{
    json.start_object();
    // This must be after the json object has been started, so any super class
    // puts their data into the same json object.
    store( json );
    json.end_object();
}

void npc::store(JsonOut &json) const
{
    // TODO: once npcs are seperated from the player class,
    // this should call store on the parent class of npc (probably Character).
    player::store( json );

    json.member( "name", name );
    json.member( "marked_for_death", marked_for_death );
    json.member( "dead", dead );
    json.member( "patience", patience );
    json.member( "myclass", (int)myclass );

    json.member( "personality", personality );
    json.member( "wandf", wander_time );
    json.member( "wandx", wander_pos.x );
    json.member( "wandy", wander_pos.y );
    json.member( "wandz", wander_pos.z );

    json.member( "mapx", mapx );
    json.member( "mapy", mapy );

    json.member( "plx", last_player_seen_pos.x );
    json.member( "ply", last_player_seen_pos.y );
    json.member( "plz", last_player_seen_pos.z );

    json.member( "goalx", goal.x );
    json.member( "goaly", goal.y );
    json.member( "goalz", goal.z );

    json.member( "guardx", guard_pos.x );
    json.member( "guardy", guard_pos.y );
    json.member( "guardz", guard_pos.z );

    json.member( "mission", mission ); // todo: stringid
    json.member( "flags", flags );
    if ( fac_id != "" ) { // set in constructor
        json.member( "my_fac", my_fac->id.c_str() );
    }
    json.member( "attitude", (int)attitude );
    json.member("op_of_u", op_of_u);
    json.member("chatbin", chatbin);
    json.member("rules", rules);

    json.member("companion_mission", companion_mission);
    json.member("companion_mission_time", companion_mission_time);
    json.member("restock", restock);

    json.member("last_updated", last_updated);
    json.member("complaints", complaints);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// inventory.h
/*
 * Save invlet cache
 */
void inventory::json_save_invcache(JsonOut &json) const
{
    json.start_array();
    for( const auto &elem : invlet_cache ) {
        json.start_object();
        json.member( elem.first );
        json.start_array();
        for( const auto &_sym : elem.second ) {
            json.write( int( _sym ) );
        }
        json.end_array();
        json.end_object();
    }
    json.end_array();
}

/*
 * Invlet cache: player specific, thus not wrapped in inventory::json_load/save
 */
void inventory::json_load_invcache(JsonIn &jsin)
{
    try {
        JsonArray ja = jsin.get_array();
        while ( ja.has_more() ) {
            JsonObject jo = ja.next_object();
            std::set<std::string> members = jo.get_member_names();
            for( const auto &member : members ) {
                std::vector<char> vect;
                JsonArray pvect = jo.get_array( member );
                while ( pvect.has_more() ) {
                    vect.push_back( pvect.next_int() );
                }
                invlet_cache[member] = vect;
            }
        }
    } catch( const JsonError &jsonerr ) {
        debugmsg("bad invcache json:\n%s", jsonerr.c_str() );
    }
}

/*
 * save all items. Just this->items, invlet cache saved seperately
 */
void inventory::json_save_items(JsonOut &json) const
{
    json.start_array();
    for( const auto &elem : items ) {
        for( const auto &elem_stack_iter : elem ) {
            elem_stack_iter.serialize( json, true );
        }
    }
    json.end_array();
}


void inventory::json_load_items(JsonIn &jsin)
{
    try {
        JsonArray ja = jsin.get_array();
        while ( ja.has_more() ) {
            JsonObject jo = ja.next_object();
            add_item(item( jo ), false, false);
        }
    } catch( const JsonError &jsonerr ) {
        debugmsg("bad inventory json:\n%s", jsonerr.c_str() );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// monster.h

void monster::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    load( data );
}

void monster::load(JsonObject &data)
{
    Creature::load( data );

    std::string sidtmp;
    // load->str->int
    data.read("typeid", sidtmp);
    type = &mtype_id( sidtmp ).obj();

    data.read( "unique_name", unique_name );
    data.read("posx", position.x);
    data.read("posy", position.y);
    if( !data.read("posz", position.z) ) {
        position.z = g->get_levz();
    }

    data.read("wandf", wandf);
    data.read("wandx", wander_pos.x);
    data.read("wandy", wander_pos.y);
    if( data.read("wandz", wander_pos.z) ) {
        wander_pos.z = position.z;
    }

    data.read("hp", hp);

    // sp_timeout indicates an old save, prior to the special_attacks refactor
    if (data.has_array("sp_timeout")) {
        JsonArray parray = data.get_array("sp_timeout");
        if ( !parray.empty() ) {
            int index = 0;
            int ptimeout = 0;
            while ( parray.has_more() && index < (int)(type->special_attacks_names.size()) ) {
                if ( parray.read_next(ptimeout) ) {
                    // assume timeouts saved in same order as current monsters.json listing
                    const std::string &aname = type->special_attacks_names[index++];
                    auto &entry = special_attacks[aname];
                    if ( ptimeout >= 0 ) {
                        entry.cooldown = ptimeout;
                    } else { // -1 means disabled, unclear what <-1 values mean in old saves
                        entry.cooldown = type->special_attacks.at(aname).get_cooldown();
                        entry.enabled = false;
                    }
                }
            }
        }
    }

    // special_attacks indicates a save after the special_attacks refactor
    if (data.has_object("special_attacks")) {
        JsonObject pobject = data.get_object("special_attacks");
        for( const std::string &aname : pobject.get_member_names()) {
            JsonObject saobject = pobject.get_object(aname);
            auto &entry = special_attacks[aname];
            entry.cooldown = saobject.get_int("cooldown");
            entry.enabled = saobject.get_bool("enabled");
        }
    }

    // make sure the loaded monster has every special attack its type says it should have
    for ( auto &sa : type->special_attacks ) {
        const std::string &aname = sa.first;
        if (special_attacks.find(aname) == special_attacks.end()) {
            auto &entry = special_attacks[aname];
            entry.cooldown = rng(0, sa.second.get_cooldown());
        }
    }

    data.read("friendly", friendly);
    data.read("mission_id", mission_id);
    data.read("no_extra_death_drops", no_extra_death_drops);
    data.read("dead", dead);
    data.read("anger", anger);
    data.read("morale", morale);
    data.read("hallucination", hallucination);
    data.read("stairscount", staircount); // really?

    // Load legacy plans.
    std::vector<tripoint> plans;
    data.read("plans", plans);
    if( !plans.empty() ) {
        goal = plans.back();
    }

    // This is relative to the monster so it isn't invalidated by map shifting.
    tripoint destination;
    data.read("destination", destination);
    goal = pos() + destination;

    upgrades = data.get_bool("upgrades", type->upgrades);
    upgrade_time = data.get_int("upgrade_time", -1);

    data.read("inv", inv);
    if( data.has_int("ammo") && !type->starting_ammo.empty() ) {
        // Legacy loading for ammo.
        normalize_ammo( data.get_int("ammo") );
    } else {
        data.read("ammo", ammo);
    }

    faction = mfaction_str_id( data.get_string( "faction", "" ) );
    last_updated = data.get_int( "last_updated", calendar::turn );
}

/*
 * Save, json ed; serialization that won't break as easily. In theory.
 */
void monster::serialize(JsonOut &json) const
{
    json.start_object();
    // This must be after the json object has been started, so any super class
    // puts their data into the same json object.
    store( json );
    json.end_object();
}

void monster::store(JsonOut &json) const
{
    Creature::store( json );
    json.member( "typeid", type->id );
    json.member( "unique_name", unique_name );
    json.member("posx", position.x);
    json.member("posy", position.y);
    json.member("posz", position.z);
    json.member("wandx", wander_pos.x);
    json.member("wandy", wander_pos.y);
    json.member("wandz", wander_pos.z);
    json.member("wandf", wandf);
    json.member("hp", hp);
    json.member("special_attacks", special_attacks);
    json.member("friendly", friendly);
    json.member("faction", faction.id().str());
    json.member("mission_id", mission_id);
    json.member("no_extra_death_drops", no_extra_death_drops );
    json.member("dead", dead);
    json.member("anger", anger);
    json.member("morale", morale);
    json.member("hallucination", hallucination);
    json.member("stairscount", staircount);
    // Store the relative position of the goal so it loads correctly after a map shift.
    json.member("destination", goal - pos());
    json.member("ammo", ammo);
    json.member( "underwater", underwater );
    json.member("upgrades", upgrades);
    json.member("upgrade_time", upgrade_time);
    json.member("last_updated", last_updated);

    json.member( "inv", inv );
}

void mon_special_attack::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("cooldown",cooldown);
    json.member("enabled",enabled);
    json.end_object();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// item.h

template<typename Archive>
void item::io( Archive& archive )
{
    const auto load_type = [this]( const std::string& id ) {
        init();
        // only for backward compatibility (there are no "on" versions of those anymore)
        if( id == "UPS_on" ) {
            make( "UPS_off" );
        } else if( id == "adv_UPS_on" ) {
            make( "adv_UPS_off" );
        } else {
            make( id );
        }
    };
    const auto load_curammo = [this]( const std::string& id ) {
        set_curammo( id );
    };
    const auto load_corpse = [this]( const std::string& id ) {
        if( id == "null" ) {
            // backwards compatibility, nullptr should not be stored at all
            corpse = nullptr;
        } else {
            corpse = &mtype_id( id ).obj();
        }
    };

    archive.template io<const itype>( "typeid", type, load_type, []( const itype& i ) { return i.id; }, io::required_tag() );
    archive.io( "charges", charges, -1l );
    archive.io( "burnt", burnt, 0 );
    archive.io( "poison", poison, 0 );
    archive.io( "bday", bday, 0 );
    archive.io( "mission_id", mission_id, -1 );
    archive.io( "player_id", player_id, -1 );
    archive.io( "item_vars", item_vars, io::empty_default_tag() );
    archive.io( "name", name, type_name( 1 ) ); // TODO: change default to empty string
    archive.io( "invlet", invlet, '\0' );
    archive.io( "damage", damage, static_cast<decltype(damage)>( 0 ) );
    archive.io( "active", active, false );
    archive.io( "item_counter", item_counter, static_cast<decltype(item_counter)>( 0 ) );
    archive.io( "fridge", fridge, 0 );
    archive.io( "rot", rot, 0 );
    archive.io( "last_rot_check", last_rot_check, 0 );
    archive.io( "techniques", techniques, io::empty_default_tag() );
    archive.io( "item_tags", item_tags, io::empty_default_tag() );
    archive.io( "contents", contents, io::empty_default_tag() );
    archive.io( "components", components, io::empty_default_tag() );
    archive.template io<const itype>( "curammo", curammo, load_curammo, []( const itype& i ) { return i.id; } );
    archive.template io<const mtype>( "corpse", corpse, load_corpse, []( const mtype& i ) { return i.id.str(); } );
    archive.io( "light", light.luminance, nolight.luminance );
    archive.io( "light_width", light.width, nolight.width );
    archive.io( "light_dir", light.direction, nolight.direction );

    if( !Archive::is_input::value ) {
        return;
    }
    /* Loading has finished, following code is to ensure consistency and fixes bugs in saves. */

    // Compatiblity for item type changes: for example soap changed from being a generic item
    // (item::charges == -1) to comestible (and thereby counted by charges), old saves still have
    // charges == -1, this fixes the charges value to the default charges.
    if( count_by_charges() && charges < 0 ) {
        charges = item( type->id, 0 ).charges;
    }
    if( !active && !rotten() && goes_bad() ) {
        // Rotting found *must* be active to trigger the rotting process,
        // if it's already rotten, no need to do this.
        active = true;
    }
    if( active && dynamic_cast<const it_comest*>(type) && (rotten() || !goes_bad()) ) {
        // There was a bug that set all comestibles active, this reverses that.
        active = false;
    }
    if( !active &&
        (item_tags.count( "HOT" ) > 0 || item_tags.count( "COLD" ) > 0 ||
         item_tags.count( "WET" ) > 0) ) {
        // Some hot/cold items from legacy saves may be inactive
        active = true;
    }
    std::string mode;
    if( archive.read( "mode", mode ) ) {
        // only for backward compatibility (nowadays mode is stored in item_vars)
        set_gun_mode(mode);
    }
}

void item::deserialize(JsonObject &data)
{
    io::JsonObjectInputArchive archive( data );
    io( archive );
}

void item::serialize(JsonOut &json, bool save_contents) const
{
    (void) save_contents;
    io::JsonObjectOutputArchive archive( json );
    const_cast<item*>(this)->io( archive );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// vehicle.h

/*
 * vehicle_part
 */
void vehicle_part::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    vpart_str_id pid;
    data.read("id", pid);
    // if we don't know what type of part it is, it'll cause problems later.
    if( !pid.is_valid() ) {
        if( pid.str() == "wheel_underbody" ) {
            pid = vpart_str_id( "wheel_wide" );
        } else {
            data.throw_error( "bad vehicle part", "id" );
        }
    }
    set_id(pid);
    data.read("mount_dx", mount.x);
    data.read("mount_dy", mount.y);
    data.read("hp", hp );
    data.read("amount", amount );
    data.read("blood", blood );
    data.read("bigness", bigness );
    data.read("enabled", enabled );
    data.read("flags", flags );
    data.read("passenger_id", passenger_id );
    data.read("items", items);
    data.read("target_first_x", target.first.x);
    data.read("target_first_y", target.first.y);
    data.read("target_first_z", target.first.z);
    data.read("target_second_x", target.second.x);
    data.read("target_second_y", target.second.y);
    data.read("target_second_z", target.second.z);
}

void vehicle_part::serialize(JsonOut &json) const
{
    json.start_object();
    // TODO: the json classes should automatically convert the int-id to the string-id and the inverse
    json.member("id", id.id().str());
    json.member("mount_dx", mount.x);
    json.member("mount_dy", mount.y);
    json.member("hp", hp);
    json.member("amount", amount);
    json.member("blood", blood);
    json.member("bigness", bigness);
    json.member("enabled", enabled);
    json.member("flags", flags);
    json.member("passenger_id", passenger_id);
    json.member("items", items);
    json.member("target_first_x", target.first.x);
    json.member("target_first_y", target.first.y);
    json.member("target_first_z", target.first.z);
    json.member("target_second_x", target.second.x);
    json.member("target_second_y", target.second.y);
    json.member("target_second_z", target.second.z);
    json.end_object();
}

/*
 * label
 */
void label::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    data.read("x", x);
    data.read("y", y);
    data.read("text", text);
}

void label::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("x", x);
    json.member("y", y);
    json.member("text", text);
    json.end_object();
}

/*
 * Load vehicle from a json blob that might just exceed player in size.
 */
void vehicle::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();

    int fdir, mdir;

    data.read("type", type);
    data.read("posx", posx);
    data.read("posy", posy);
    data.read("om_id", om_id);
    data.read("faceDir", fdir);
    data.read("moveDir", mdir);
    data.read("turn_dir", turn_dir);
    data.read("velocity", velocity);
    data.read("falling", falling);
    data.read("cruise_velocity", cruise_velocity);
    data.read("vertical_velocity", vertical_velocity);
    data.read("cruise_on", cruise_on);
    data.read("engine_on", engine_on);
    data.read("tracking_on", tracking_on);
    data.read("lights_on", lights_on);
    data.read("stereo_on", stereo_on);
    data.read("chimes_on", chimes_on);
    data.read("overhead_lights_on", overhead_lights_on);
    data.read("fridge_on", fridge_on);
    data.read("recharger_on", recharger_on);
    data.read("skidding", skidding);
    data.read("turret_mode", turret_mode);
    data.read("of_turn_carry", of_turn_carry);
    data.read("is_locked", is_locked);
    data.read("is_alarm_on", is_alarm_on);
    data.read("camera_on", camera_on);
    data.read("dome_lights_on", dome_lights_on);
    data.read("aisle_lights_on", aisle_lights_on);
    data.read("has_atomic_lights", has_atomic_lights);
    data.read("scoop_on",scoop_on);
    data.read("plow_on",plow_on);
    data.read("reaper_on",reaper_on);
    data.read("planter_on",planter_on);
    int last_updated = calendar::turn;
    data.read( "last_update_turn", last_updated );
    last_update_turn = last_updated;

    face.init (fdir);
    move.init (mdir);
    data.read("name", name);

    data.read("parts", parts);

    // we persist the pivot anchor so that if the rules for finding
    // the pivot change, existing vehicles do not shift around.
    // Loading vehicles that predate the pivot logic is a special
    // case of this, they will load with an anchor of (0,0) which
    // is what they're expecting.
    data.read("pivot", pivot_anchor[0]);
    pivot_anchor[1] = pivot_anchor[0];
    pivot_rotation[1] = pivot_rotation[0] = fdir;

    // Need to manually backfill the active item cache since the part loader can't call its vehicle.
    for( auto cargo_index : all_parts_with_feature(VPFLAG_CARGO, true) ) {
        auto it = parts[cargo_index].items.begin();
        auto end = parts[cargo_index].items.end();
        for( ; it != end; ++it ) {
            if( it->needs_processing() ) {
                active_items.add( it, parts[cargo_index].mount );
            }
        }
    }
    /* After loading, check if the vehicle is from the old rules and is missing
     * frames. */
    if ( savegame_loading_version < 11 ) {
        add_missing_frames();
    }

    // Handle steering changes
    if (savegame_loading_version < 25) {
        add_steerable_wheels();
    }

    refresh();

    data.read("tags", tags);
    data.read("labels", labels);

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
}

void vehicle::serialize(JsonOut &json) const
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
    json.member( "falling", falling );
    json.member( "cruise_velocity", cruise_velocity );
    json.member( "vertical_velocity", vertical_velocity );
    json.member( "cruise_on", cruise_on );
    json.member( "engine_on", engine_on );
    json.member( "tracking_on", tracking_on );
    json.member( "lights_on", lights_on );
    json.member( "stereo_on", stereo_on);
    json.member( "chimes_on", chimes_on);
    json.member( "overhead_lights_on", overhead_lights_on );
    json.member( "fridge_on", fridge_on );
    json.member( "recharger_on", recharger_on );
    json.member( "skidding", skidding );
    json.member( "turret_mode", turret_mode );
    json.member( "of_turn_carry", of_turn_carry );
    json.member( "name", name );
    json.member( "parts", parts );
    json.member( "tags", tags );
    json.member( "labels", labels );
    json.member( "is_locked", is_locked );
    json.member( "is_alarm_on", is_alarm_on );
    json.member( "camera_on", camera_on );
    json.member( "dome_lights_on", dome_lights_on );
    json.member( "aisle_lights_on", aisle_lights_on );
    json.member( "has_atomic_lights", has_atomic_lights );
    json.member( "last_update_turn", last_update_turn.get_turn() );
    json.member("scoop_on",scoop_on);
    json.member("plow_on",plow_on);
    json.member("reaper_on",reaper_on);
    json.member("planter_on",planter_on);
    json.member("pivot",pivot_anchor[0]);
    json.end_object();
}

////////////////// mission.h
////
void mission::deserialize(JsonIn &jsin)
{
    JsonObject jo = jsin.get_object();

    if (jo.has_member("type_id")) {
        type = mission_type::get( static_cast<mission_type_id>( jo.get_int( "type_id" ) ) );
    }
    jo.read("description", description);
    jo.read("failed", failed);
    jo.read("value", value);
    jo.read("reward", reward);
    jo.read("uid", uid );
    JsonArray ja = jo.get_array("target");
    if( ja.size() == 3 ) {
        target.x = ja.get_int(0);
        target.y = ja.get_int(1);
        target.z = ja.get_int(2);
    } else if( ja.size() == 2 ) {
        target.x = ja.get_int(0);
        target.y = ja.get_int(1);
    }
    follow_up = static_cast<mission_type_id>(jo.get_int("follow_up", follow_up));
    item_id = itype_id(jo.get_string("item_id", item_id));

    const std::string omid = jo.get_string( "target_id", "" );
    if( !omid.empty() ) {
        target_id = oter_id( omid );
    }
    recruit_class = static_cast<npc_class>( jo.get_int( "recruit_class", recruit_class ) );
    jo.read( "target_npc_id", target_npc_id );
    jo.read( "monster_type", monster_type );
    jo.read( "monster_kill_goal", monster_kill_goal );
    jo.read("deadline", deadline );
    jo.read("step", step );
    jo.read("item_count", item_count );
    jo.read("npc_id", npc_id );
    jo.read("good_fac_id", good_fac_id );
    jo.read("bad_fac_id", bad_fac_id );
    jo.read("player_id", player_id );
    jo.read("was_started", was_started );
}

void mission::serialize(JsonOut &json) const
{
    json.start_object();

    json.member("type_id", type == NULL ? -1 : static_cast<int>( type->id ) );
    json.member("description", description);
    json.member("failed", failed);
    json.member("value", value);
    json.member("reward", reward);
    json.member("uid", uid);

    json.member("target");
    json.start_array();
    json.write(target.x);
    json.write(target.y);
    json.write(target.z);
    json.end_array();

    json.member("item_id", item_id);
    json.member("item_count", item_count);
    json.member("target_id", target_id.t().id);
    json.member("recruit_class", recruit_class);
    json.member("target_npc_id", target_npc_id);
    json.member("monster_type", monster_type);
    json.member("monster_kill_goal", monster_kill_goal);
    json.member("deadline", deadline);
    json.member("npc_id", npc_id);
    json.member("good_fac_id", good_fac_id);
    json.member("bad_fac_id", bad_fac_id);
    json.member("step", step);
    json.member("follow_up", (int)follow_up);
    json.member("player_id", player_id);
    json.member("was_started", was_started);

    json.end_object();
}

////////////////// faction.h
////
void faction::deserialize(JsonIn &jsin)
{
    JsonObject jo = jsin.get_object();

    jo.read("id", id);
    jo.read("name", name);
    if ( !jo.read( "desc", desc )) {
        desc = "";
    }
    goal = faction_goal(jo.get_int("goal", goal));
    values = jo.get_int("values", values);
    job1 = faction_job(jo.get_int("job1", job1));
    job2 = faction_job(jo.get_int("job2", job2));
    jo.read("likes_u", likes_u);
    jo.read("respects_u", respects_u);
    jo.read("known_by_u", known_by_u);
    jo.read("strength", strength);
    jo.read("sneak", sneak);
    jo.read("crime", crime);
    jo.read("cult", cult);
    jo.read("good", good);
    jo.read("mapx", mapx);
    jo.read("mapy", mapy);
    // omx,omy are obsolete, use them (if present) to make mapx,mapy global coordinates
    int o;
    if(jo.read("omx", o)) {
        mapx += o * OMAPX * 2;
    }
    if(jo.read("omy", o)) {
        mapy += o * OMAPY * 2;
    }
    jo.read("size", size);
    jo.read("power", power);
    if ( !jo.read( "combat_ability", combat_ability )){
        combat_ability = 100;
    }
    if ( !jo.read( "food_supply", food_supply )){
        food_supply = 100;
    }
    if ( !jo.read( "wealth", wealth )){
        wealth = 100;
    }
    if (jo.has_array("opinion_of")) {
        opinion_of = jo.get_int_array("opinion_of");
    }
}

void faction::serialize(JsonOut &json) const
{
    json.start_object();

    json.member("id", id);
    json.member("name", name);
    json.member("desc", desc);
    json.member("values", values);
    json.member("goal", goal);
    json.member("job1", job1);
    json.member("job2", job2);
    json.member("likes_u", likes_u);
    json.member("respects_u", respects_u);
    json.member("known_by_u", known_by_u);
    json.member("strength", strength);
    json.member("sneak", sneak);
    json.member("crime", crime);
    json.member("cult", cult);
    json.member("good", good);
    json.member("mapx", mapx);
    json.member("mapy", mapy);
    json.member("size", size);
    json.member("power", power);
    json.member("combat_ability", combat_ability);
    json.member("food_supply", food_supply);
    json.member("wealth", wealth);
    json.member("opinion_of", opinion_of);

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
    for (auto maps : effects) {
        for (auto i : maps.second) {
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
    if(savegame_loading_version >= 23) {
        if( jsin.has_object( "effects" ) ) {
            // Because JSON requires string keys we need to convert back to our bp keys
            std::unordered_map<std::string, std::unordered_map<std::string, effect>> tmp_map;
            jsin.read( "effects", tmp_map );
            int key_num;
            for (auto maps : tmp_map) {
                const efftype_id id( maps.first );
                if( !id.is_valid() ) {
                    debugmsg( "Invalid effect: %s", id.c_str() );
                    continue;
                }
                for (auto i : maps.second) {
                    if ( !(std::istringstream(i.first) >> key_num) ) {
                        key_num = 0;
                    }
                    effects[id][(body_part)key_num] = i.second;
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
}

void morale_point::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    type = static_cast<morale_type>( jo.get_int( "type_enum" ) );
    std::string tmpitype;
    if( jo.read( "item_type", tmpitype ) && item::type_is_defined( tmpitype ) ) {
        item_type = item::find_type( tmpitype );
    }
    jo.read( "bonus", bonus );
    jo.read( "duration", duration );
    jo.read( "decay_start", decay_start );
    jo.read( "age", age );
}

void morale_point::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "type_enum", static_cast<int>( type ) );
    if( item_type != NULL ) {
        json.member( "item_type", item_type->id );
    }
    json.member( "bonus", bonus );
    json.member( "duration", duration );
    json.member( "decay_start", decay_start );
    json.member( "age", age );
    json.end_object();
}
