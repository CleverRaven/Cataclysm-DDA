#include "player.h"
#include "npc.h"
#include "profession.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "disease.h"
#include "addiction.h"
#include "moraledata.h"
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
#include "disease.h"
#include "crafting.h"
#include "get_version.h"
#include "monstergenerator.h"

#include "savegame.h"
#include "tile_id_data.h" // for monster::json_save
#include <ctime>
#include <bitset>

#include "json.h"

#include "debug.h"
#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h

void player_activity::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "type", int(type) );
    json.member( "moves_left", moves_left );
    json.member( "index", index );
    json.member( "position", position );
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

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h, player + npc
/*
 * Gather variables for saving. These variables are common to both the player and NPCs (which is a kind of player).
 * Do not overload; NPC or player specific stuff should go to player::json_save or npc::json_save.
 */

void player::load(JsonObject &data)
{
    Creature::load( data );

    JsonArray parray;
    int tmpid = 0;

    // todo/maybe:
    // std::map<std::string, int*> strmap_common_variables;
    // void player::init_strmap_common_variables() {
    //     strmap_common_variables["posx"]=&posx; // + all this below and in save_common_variables
    // }
    // load:
    // for(std::map<std::string, int*>::iterator it...
    //     data.read(it->first,it->second);
    // save:
    // for(...
    //     json.member( it->first, it->second );
    if(!data.read("posx", posx) ) { // uh-oh.
        debugmsg("BAD PLAYER/NPC JSON: no 'posx'?");
    }
    data.read("posy", posy);
    data.read("hunger", hunger);
    data.read("thirst", thirst);
    data.read("fatigue", fatigue);
    data.read("stim", stim);
    data.read("pkill", pkill);
    data.read("radiation", radiation);
    data.read("scent", scent);
    data.read("underwater", underwater);
    data.read("oxygen", oxygen);
    data.read("male", male);
    data.read("cash", cash);
    data.read("recoil", recoil);
    data.read("in_vehicle", in_vehicle);
    if( data.read( "id", tmpid ) ) {
        setID( tmpid );
    }

    if( !data.read( "hp_cur", hp_cur ) ) {
        debugmsg("Error, incompatible hp_cur in save file '%s'", parray.str().c_str());
    }

    if( !data.read( "hp_max", hp_max ) ) {
        debugmsg("Error, incompatible hp_max in save file '%s'", parray.str().c_str());
    }

    data.read("power_level", power_level);
    data.read("max_power_level", max_power_level);
    // Bionic power scale has been changed, savegame version 21 has the new scale
    if( savegame_loading_version <= 20 ) {
        power_level *= 25;
        max_power_level *= 25;
    }
    data.read("traits", my_traits);

    if (data.has_object("skills")) {
        JsonObject pmap = data.get_object("skills");
        for( auto &skill : Skill::skills ) {
            if( pmap.has_object( ( skill )->ident() ) ) {
                pmap.read( ( skill )->ident(), skillLevel( skill ) );
            } else {
                debugmsg( "Load (%s) Missing skill %s", "", ( skill )->ident().c_str() );
            }
        }
    } else {
        debugmsg("Skills[] no bueno");
    }

    data.read("ma_styles", ma_styles);
    // Just too many changes here to maintain compatibility, so older characters get a free
    // diseases wipe. Since most long lasting diseases are bad, this shouldn't be too bad for them.
    if(savegame_loading_version >= 23) {
        data.read("illness", illness);
    }

    data.read( "addictions", addictions );
    data.read( "my_bionics", my_bionics );

    JsonArray traps = data.get_array("known_traps");
    known_traps.clear();
    while(traps.has_more()) {
        JsonObject pmap = traps.next_object();
        const tripoint p(pmap.get_int("x"), pmap.get_int("y"), pmap.get_int("z"));
        const std::string t = pmap.get_string("trap");
        known_traps.insert(trap_map::value_type(p, t));
    }

    inv.clear();
    if ( data.has_member( "inv" ) ) {
        JsonIn *invin = data.get_raw( "inv" );
        inv.json_load_items( *invin );
    }

    worn.clear();
    data.read( "worn", worn );

    weapon = item( "null", 0 );
    data.read( "weapon", weapon );
}

/*
 * Variables common to player and npc
 */
void player::store(JsonOut &json) const
{
    Creature::store( json );

    // assumes already in player object
    // positional data
    json.member( "posx", posx );
    json.member( "posy", posy );

    // om-noms or lack thereof
    json.member( "hunger", hunger );
    json.member( "thirst", thirst );
    // energy
    json.member( "fatigue", fatigue );
    json.member( "stim", stim );
    // pain
    json.member( "pkill", pkill );
    // misc levels
    json.member( "radiation", radiation );
    json.member( "scent", int(scent) );
    json.member( "body_wetness", body_wetness );

    // breathing
    json.member( "underwater", underwater );
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

    // traits: permanent 'mutations' more or less
    json.member( "traits", my_traits );

    // skills
    json.member( "skills" );
    json.start_object();
    for( auto &skill : Skill::skills ) {
        SkillLevel sk = get_skill_level( skill );
        json.member( ( skill )->ident(), sk );
    }
    json.end_object();

    // martial arts
    /*for (int i = 0; i < ma_styles.size(); i++) {
        ptmpvect.push_back( pv( ma_styles[i] ) );
    }*/
    json.member( "ma_styles", ma_styles );

    // disease
    json.member( "illness", illness );

    // "Looks like I picked the wrong week to quit smoking." - Steve McCroskey
    json.member( "addictions", addictions );

    // "Fracking Toasters" - Saul Tigh, toaster
    json.member( "my_bionics", my_bionics );

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

    json.member( "stomach_food", stomach_food );
    json.member( "stomach_water", stomach_water );

    // crafting etc
    json.member( "activity", activity );
    json.member( "backlog", backlog );

    // mutations; just like traits but can be removed.
    json.member( "mutations", my_mutations );
    json.member( "mutation_keys", trait_keys );

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
    json.member("active_mission", active_mission );

    json.member( "active_missions", active_missions );
    json.member( "completed_missions", completed_missions );
    json.member( "failed_missions", failed_missions );

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
    if ( data.read("profession", prof_ident) && profession::exists(prof_ident) ) {
        prof = profession::prof(prof_ident);
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

    data.read( "stomach_food", stomach_food);
    data.read( "stomach_water", stomach_water);

    data.read( "focus_pool", focus_pool);
    data.read( "style_selected", style_selected );
    data.read( "keep_hands_free", keep_hands_free );

    data.read( "mutations", my_mutations );
    data.read( "mutation_keys", trait_keys );

    set_highest_cat_level();
    drench_mut_calc();
    std::string scen_ident="(null)";
    if ( data.read("scenario",scen_ident) && scenario::exists(scen_ident) ) {
        g->scen = scenario::scen(scen_ident);
        start_location = g->scen->start_location();
    } else {
        scenario *generic_scenario = scenario::generic();
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

    data.read( "active_mission", active_mission );

    data.read( "active_missions", active_missions );
    data.read( "failed_missions", failed_missions );
    data.read( "completed_missions", completed_missions );

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

void npc_combat_rules::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("engagement", (int)engagement );
    json.member("use_guns", use_guns );
    json.member("use_grenades", use_grenades );
    json.member("use_silent", use_silent );
    //todo    json.member("guard_pos", guard_pos );
    json.end_object();
}

void npc_combat_rules::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int tmpeng;
    data.read("engagement", tmpeng);
    engagement = (combat_engagement)tmpeng;
    data.read( "use_guns", use_guns);
    data.read( "use_grenades", use_grenades);
    data.read( "use_silent", use_silent);
}

void npc_chatbin::serialize(JsonOut &json) const
{
    json.start_object();
    json.member( "first_topic", (int)first_topic );
    json.member( "mission_selected", mission_selected );
    json.member( "tempvalue",
                 tempvalue );     //No clue what this value does, but it is used all over the place. So it is NOT temp.
    if ( skill ) {
        json.member("skill", skill->ident() );
    }
    json.member( "missions", missions );
    json.member( "missions_assigned", missions_assigned );
    json.end_object();
}

void npc_chatbin::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int tmptopic;
    std::string skill_ident;

    data.read("first_topic", tmptopic);
    first_topic = talk_topic(tmptopic);

    if ( data.read("skill", skill_ident) ) {
        skill = Skill::skill(skill_ident);
    }

    data.read("tempvalue", tempvalue);
    data.read("mission_selected", mission_selected);
    data.read( "missions", missions );
    data.read( "missions_assigned", missions_assigned );
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
};

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
        skill = Skill::skill(jo.get_int("skill_id"));
    } else if (jo.has_string("skill_id")) {
        skill = Skill::skill(jo.get_string("skill_id"));
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

    int misstmp, classtmp, flagstmp, atttmp;
    std::string facID;

    data.read("name", name);
    data.read("marked_for_death", marked_for_death);
    data.read("dead", dead);
    if ( data.read( "myclass", classtmp) ) {
        myclass = npc_class( classtmp );
    }

    data.read("personality", personality);

    data.read("wandx", wandx);
    data.read("wandy", wandy);
    data.read("wandf", wandf);

    data.read("mapx", mapx);
    data.read("mapy", mapy);
    if(!data.read("mapz", mapz)) {
        data.read("omz", mapz); // was renamed to match mapx,mapy
    }
    int o;
    if(data.read("omx", o)) {
        mapx += o * OMAPX * 2;
    }
    if(data.read("omy", o)) {
        mapy += o * OMAPY * 2;
    }

    data.read("plx", plx);
    data.read("ply", ply);

    data.read("goalx", goal.x);
    data.read("goaly", goal.y);
    data.read("goalz", goal.z);

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

    data.read("op_of_u", op_of_u);
    data.read("chatbin", chatbin);
    data.read("combat_rules", combat_rules);
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
    json.member( "wandx", wandx );
    json.member( "wandy", wandy );
    json.member( "wandf", wandf );

    json.member( "mapx", mapx );
    json.member( "mapy", mapy );
    json.member( "mapz", mapz );
    json.member( "plx", plx );
    json.member( "ply", ply );
    json.member( "goalx", goal.x );
    json.member( "goaly", goal.y );
    json.member( "goalz", goal.z );

    json.member( "mission", mission ); // todo: stringid
    json.member( "flags", flags );
    if ( fac_id != "" ) { // set in constructor
        json.member( "my_fac", my_fac->id.c_str() );
    }
    json.member( "attitude", (int)attitude );
    json.member("op_of_u", op_of_u);
    json.member("chatbin", chatbin);
    json.member("combat_rules", combat_rules);
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
    } catch (std::string jsonerr) {
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
    } catch (std::string &jsonerr) {
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

    int iidtmp;
    std::string sidtmp;
    // load->str->int
    if ( ! data.read("typeid", sidtmp) ) {
        // or load->int->str->possibly_shifted_int
        data.read("typeid", iidtmp);
        sidtmp = legacy_mon_id[ iidtmp ];
    }
    type = GetMType(sidtmp);

    data.read("posx", _posx);
    data.read("posy", _posy);
    data.read("wandx", wandx);
    data.read("wandy", wandy);
    data.read("wandf", wandf);
    data.read("hp", hp);

    if (data.has_array("sp_timeout")) {
        JsonArray parray = data.get_array("sp_timeout");
        if ( !parray.empty() ) {
            int ptimeout = 0;
            while ( parray.has_more() ) {
                if ( parray.read_next(ptimeout) ) {
                    sp_timeout.push_back(ptimeout);
                }
            }
        }
    }
    for (size_t i = sp_timeout.size(); i < type->sp_freq.size(); ++i) {
        sp_timeout.push_back(rng(0, type->sp_freq[i]));
    }

    data.read("friendly", friendly);
    data.read("faction_id", faction_id);
    data.read("mission_id", mission_id);
    data.read("no_extra_death_drops", no_extra_death_drops);
    data.read("dead", dead);
    data.read("anger", anger);
    data.read("morale", morale);
    data.read("hallucination", hallucination);
    data.read("stairscount", staircount); // really?

    data.read("plans", plans);

    data.read("inv", inv);
    if( data.has_int("ammo") && !type->starting_ammo.empty() ) {
        // Legacy loading for ammo.
        normalize_ammo( data.get_int("ammo") );
    } else {
        data.read("ammo", ammo);
    }
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
    json.member("posx", _posx);
    json.member("posy", _posy);
    json.member("wandx", wandx);
    json.member("wandy", wandy);
    json.member("wandf", wandf);
    json.member("hp", hp);
    json.member("sp_timeout", sp_timeout);
    json.member("friendly", friendly);
    json.member("faction_id", faction_id);
    json.member("mission_id", mission_id);
    json.member("no_extra_death_drops", no_extra_death_drops );
    json.member("dead", dead);
    json.member("anger", anger);
    json.member("morale", morale);
    json.member("hallucination", hallucination);
    json.member("stairscount", staircount);
    json.member("plans", plans);
    json.member("ammo", ammo);

    json.member( "inv", inv );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// item.h

void item::deserialize(JsonObject &data)
{
    init();
    clear();

    std::string idtmp = "";
    std::string ammotmp = "null";
    int lettmp = 0;
    std::string corptmp = "null";
    int damtmp = 0;
    std::bitset<num_bp> tmp_covers;

    if ( ! data.read( "typeid", idtmp) ) {
        debugmsg("Invalid item type: %s ", data.str().c_str() );
        idtmp = "null";
    }

    data.read( "charges", charges );
    data.read( "burnt", burnt );

    data.read( "poison", poison );

    data.read( "bday", bday );

    std::string mode;
    if( data.read( "mode", mode ) ) {
        set_gun_mode( mode );
    }
    data.read( "mission_id", mission_id );
    data.read( "player_id", player_id );

    if (!data.read( "corpse", corptmp )) {
        int ctmp = -1;
        data.read( "corpse", ctmp );
        if (ctmp != -1) {
            corptmp = legacy_mon_id[ctmp];
        } else {
            corptmp = "null";
        }
    }
    if (corptmp != "null") {
        corpse = GetMType(corptmp);
    } else {
        corpse = NULL;
    }

    JsonObject pvars = data.get_object("item_vars");
    std::set<std::string> members = pvars.get_member_names();
    for( const auto &member : members ) {
        if( pvars.has_string( member ) ) {
            item_vars[member] = pvars.get_string( member );
        }
    }

    if( idtmp == "UPS_on" ) {
        idtmp = "UPS_off";
    } else if( idtmp == "adv_UPS_on" ) {
        idtmp = "adv_UPS_off" ;
    }
    make(idtmp);

    if ( ! data.read( "name", name ) ) {
        name = type_name(1);
    }
    // Compatiblity for item type changes: for example soap changed from being a generic item
    // (item::charges == -1) to comestible (and thereby counted by charges), old saves still have
    // charges == -1, this fixes the charges value to the default charges.
    if( count_by_charges() && charges < 0 ) {
        charges = item( type->id, 0 ).charges;
    }

    data.read( "invlet", lettmp );
    invlet = char(lettmp);

    data.read( "damage", damtmp );
    damage = damtmp; // todo: check why this is done after make(), using a tmp variable
    data.read( "active", active );
    data.read( "item_counter" , item_counter );
    data.read( "fridge", fridge );
    data.read( "rot", rot );
    data.read( "last_rot_check", last_rot_check );
    if( !active && !rotten() && goes_bad() ) {
        // Rotting found *must* be active to trigger the rotting process,
        // if it's already rotten, no need to do this.
        active = true;
    }
    if( active && dynamic_cast<it_comest*>(type) && (rotten() || !goes_bad()) ) {
        // There was a bug that set all comestibles active, this reverses that.
        active = false;
    }

    if( data.read( "curammo", ammotmp ) ) {
        set_curammo( ammotmp );
    }

    data.read( "covers", tmp_covers );
    const auto armor = type->armor.get();
    if( armor != nullptr && tmp_covers.none() ) {
        covered_bodyparts = armor->covers;
        if (armor->sided.any()) {
            make_handed( one_in( 2 ) ? LEFT : RIGHT );
        }
    } else {
        covered_bodyparts = tmp_covers;
    }

    data.read("item_tags", item_tags);


    int tmplum = 0;
    if ( data.read("light", tmplum) ) {

        light = nolight;
        int tmpwidth = 0;
        int tmpdir = 0;

        data.read("light_width", tmpwidth);
        data.read("light_dir", tmpdir);
        light.luminance = tmplum;
        light.width = (short)tmpwidth;
        light.direction = (short)tmpdir;
    }

    data.read("contents", contents);
    data.read("components", components);
}

void item::serialize(JsonOut &json, bool save_contents) const
{
    json.start_object();

    /////
    if (type == NULL) {
        debugmsg("Tried to save an item with NULL type!");
    }

    /////
    json.member( "invlet", int(invlet) );
    json.member( "typeid", typeId() );
    json.member( "bday", bday );

    if ( charges != -1 ) {
        json.member( "charges", long(charges) );
    }
    if ( damage != 0 ) {
        json.member( "damage", int(damage) );
    }
    if ( burnt != 0 ) {
        json.member( "burnt", burnt );
    }
    if ( covered_bodyparts.any() ) {
        json.member( "covers", covered_bodyparts );
    }
    if ( poison != 0 ) {
        json.member( "poison", poison );
    }
    if ( has_curammo() ) {
        json.member( "curammo", get_curammo_id() );
    }
    if ( active == true ) {
        json.member( "active", true );
    }
    if ( item_counter != 0) {
        json.member( "item_counter", item_counter );
    }
    // bug? // if ( fridge == true )    json.member( "fridge", true );
    if ( fridge != 0 ) {
        json.member( "fridge", fridge );
    }
    if ( rot != 0 ) {
        json.member( "rot", rot );
    }
    if ( last_rot_check != 0 ) {
        json.member( "last_rot_check", last_rot_check );
    }

    if ( corpse != NULL ) {
        json.member( "corpse", corpse->id );
    }

    if ( player_id != -1 ) {
        json.member( "player_id", player_id );
    }
    if ( mission_id != -1 ) {
        json.member( "mission_id", mission_id );
    }

    if ( ! item_tags.empty() ) {
        json.member( "item_tags", item_tags );
    }

    if ( ! item_vars.empty() ) {
        json.member( "item_vars", item_vars );
    }

    if ( name != type_name(1) ) {
        json.member( "name", name );
    }

    if ( light.luminance != 0 ) {
        json.member( "light", int(light.luminance) );
        if ( light.width != 0 ) {
            json.member( "light_width", int(light.width) );
            json.member( "light_dir", int(light.direction) );
        }
    }

    if ( save_contents && !contents.empty() ) {
        json.member("contents");
        json.start_array();
        for( auto &elem : contents ) {
            if( !( elem.contents.empty() ) && elem.contents[0].is_gunmod() ) {
                elem.serialize( json, true ); // save gun mods of holstered pistol
            } else {
                elem.serialize( json, false ); // no matryoshka dolls
            }
        }
        json.end_array();
    }

    json.member( "components", components );

    json.end_object();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// vehicle.h

/*
 * vehicle_part
 */
void vehicle_part::deserialize(JsonIn &jsin)
{
    JsonObject data = jsin.get_object();
    int intpid;
    std::string pid;
    if ( data.read("id_enum", intpid) && intpid < 74 ) {
        pid = legacy_vpart_id[intpid];
    } else {
        data.read("id", pid);
    }
    // if we don't know what type of part it is, it'll cause problems later.
    if (vehicle_part_types.find(pid) == vehicle_part_types.end()) {
        if (pid == "wheel_underbody") {
            pid = "wheel_wide";
        } else {
            throw (std::string)"bad vehicle part, id: %s" + pid;
        }
    }
    setid(pid);
    data.read("mount_dx", mount_dx);
    data.read("mount_dy", mount_dy);
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
    data.read("target_second_x", target.second.x);
    data.read("target_second_y", target.second.y);
}

void vehicle_part::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("id", id);
    json.member("mount_dx", mount_dx);
    json.member("mount_dy", mount_dy);
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
    json.member("target_second_x", target.second.x);
    json.member("target_second_y", target.second.y);
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
    data.read("cruise_velocity", cruise_velocity);
    data.read("music_id", music_id);
    data.read("cruise_on", cruise_on);
    data.read("engine_on", engine_on);
    data.read("tracking_on", tracking_on);
    data.read("lights_on", lights_on);
    data.read("stereo_on", stereo_on);
    data.read("overhead_lights_on", overhead_lights_on);
    data.read("fridge_on", fridge_on);
    data.read("recharger_on", recharger_on);
    data.read("skidding", skidding);
    data.read("turret_mode", turret_mode);
    data.read("of_turn_carry", of_turn_carry);
    data.read("is_locked", is_locked);
    data.read("is_alarm_on", is_alarm_on);

    face.init (fdir);
    move.init (mdir);
    data.read("name", name);

    data.read("parts", parts);

    // Need to manually backfill the active item cache since the part loader can't call its vehicle.
    for( auto cargo_index : all_parts_with_feature(VPFLAG_CARGO, true) ) {
        auto it = parts[cargo_index].items.begin();
        auto end = parts[cargo_index].items.end();
        for( ; it != end; ++it ) {
            if( it->needs_processing() ) {
                active_items.add( it, point( parts[cargo_index].mount_dx,
                                             parts[cargo_index].mount_dy ) );
            }
        }
    }
    /* After loading, check if the vehicle is from the old rules and is missing
     * frames. */
    if ( savegame_loading_version < 11 ) {
        add_missing_frames();
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
    json.member( "cruise_velocity", cruise_velocity );
    json.member( "music_id", music_id);
    json.member( "cruise_on", cruise_on );
    json.member( "engine_on", engine_on );
    json.member( "tracking_on", tracking_on );
    json.member( "lights_on", lights_on );
    json.member( "stereo_on", stereo_on);
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
    json.end_object();
}

////////////////// mission.h
////
void mission::deserialize(JsonIn &jsin)
{
    JsonObject jo = jsin.get_object();

    if (jo.has_member("type_id")) {
        type = &(g->mission_types[jo.get_int("type_id")]);
    }
    jo.read("description", description);
    jo.read("failed", failed);
    jo.read("value", value);
    jo.read("reward", reward);
    jo.read("uid", uid );
    JsonArray ja = jo.get_array("target");
    if (ja.size() == 2) {
        target.x = ja.get_int(0);
        target.y = ja.get_int(1);
    }
    follow_up = mission_id(jo.get_int("follow_up", follow_up));
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
}

void mission::serialize(JsonOut &json) const
{
    json.start_object();

    json.member("type_id", type == NULL ? -1 : (int)type->id);
    json.member("description", description);
    json.member("failed", failed);
    json.member("value", value);
    json.member("reward", reward);
    json.member("uid", uid);

    json.member("target");
    json.start_array();
    json.write(target.x);
    json.write(target.y);
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
    json.member("opinion_of", opinion_of);

    json.end_object();
}

void Creature::store( JsonOut &jsout ) const
{
    jsout.member( "str_cur", str_cur );
    jsout.member( "str_max", str_max );
    jsout.member( "dex_cur", dex_cur );
    jsout.member( "dex_max", dex_max );
    jsout.member( "int_cur", int_cur );
    jsout.member( "int_max", int_max );
    jsout.member( "per_cur", per_cur );
    jsout.member( "per_max", per_max );

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
            tmp_map[maps.first][convert.str()] = i.second;
        }
    }
    jsout.member( "effects", tmp_map );
    
    
    jsout.member( "values", values );

    jsout.member( "str_bonus", str_bonus );
    jsout.member( "dex_bonus", dex_bonus );
    jsout.member( "per_bonus", per_bonus );
    jsout.member( "int_bonus", int_bonus );

    jsout.member( "healthy", healthy );
    jsout.member( "healthy_mod", healthy_mod );

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
    jsin.read( "str_cur", str_cur );
    jsin.read( "str_max", str_max );
    jsin.read( "dex_cur", dex_cur );
    jsin.read( "dex_max", dex_max );
    jsin.read( "int_cur", int_cur );
    jsin.read( "int_max", int_max );
    jsin.read( "per_cur", per_cur );
    jsin.read( "per_max", per_max );

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
                for (auto i : maps.second) {
                    if ( !(std::istringstream(i.first) >> key_num) ) {
                        key_num = 0;
                    }
                    effects[maps.first][(body_part)key_num] = i.second;
                }
            }
        }
    }
    jsin.read( "values", values );

    jsin.read( "str_bonus", str_bonus );
    jsin.read( "dex_bonus", dex_bonus );
    jsin.read( "per_bonus", per_bonus );
    jsin.read( "int_bonus", int_bonus );

    jsin.read( "healthy", healthy );
    jsin.read( "healthy_mod", healthy_mod );

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
