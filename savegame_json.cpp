#include "player.h"
#include "npc.h"
#include "profession.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "disease.h"
#include "addiction.h"
#include "keypress.h"
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
#include "catajson.h"
#include "disease.h"
#include "crafting.h"
#include "get_version.h"

#include <ctime>

#include "picofunc.h"

#include "debug.h"
#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h
/*
 * output picojson::value wrapped map of an activity
 */
picojson::value player_activity::json_save() {
    std::map<std::string, picojson::value> data;
    std::vector<picojson::value> pvector;

    data["moves_left"] = pv( moves_left );
    data["type"] = pv ( int(type) );
    data["index"] = pv ( int(index) );
    data["invlet"] = pv ( int(invlet) );
    data["name"] = pv ( name );

    pvector.push_back( pv( placement.x ) );
    pvector.push_back( pv( placement.y ) );
    data["placement"] = pv ( pvector );
    pvector.clear();

    for (int i = 0; i < values.size(); i++) {
        pvector.push_back( pv( values[i] ) );
    }
    data["values"] = pv ( pvector );

    return pv ( data );
}

/*
 * Convert activity struct to picojson::value map
 */
bool player_activity::json_load(picojson::value & parsed) {
    picojson::object &data = parsed.get<picojson::object>();
    int tmptype;
    int tmpinv;
    if ( picoint(data,"type",tmptype) && type < NUM_ACTIVITIES ) {
        type = activity_type(tmptype);
        picoint(data,"moves_left",moves_left);
        picoint(data,"index",index);
        picoint(data,"invlet",tmpinv);
        invlet = (char)tmpinv;
        picostring(data,"name",name);
        picopoint(data,"placement",placement);
        picojson::array * parray = pgetarray(data, "values");
        if ( parray != NULL ) {
            for( picojson::array::const_iterator pt = parray->begin(); pt != parray->end(); ++pt) {
                if ( (*pt).is<double>() ) {
                    values.push_back( (int)(*pt).get<double>() );
                }
            }
        }
        return true;
    } else {
        debugmsg("Bad activity data:\n%s", parsed.serialize().c_str() );
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// skill.h
picojson::value SkillLevel::json_save() {
    std::map<std::string, picojson::value> data;
    data["level"] = pv( level() );
    data["exercise"] = pv( exercise(true) );
    data["istraining"] = pv( isTraining() );
    data["lastpracticed"] = pv( int ( lastPracticed() ) );
    return pv ( data );
}

bool SkillLevel::json_load(std::map<std::string, picojson::value> & data ){
   int lastpractice=0;
   picoint(data,"level",_level);
   picoint(data,"exercise",_exercise);
   picobool(data,"istraining",_isTraining);
   picoint(data,"lastpracticed",lastpractice);
   if(lastpractice == 0) {
        _lastPracticed = HOURS(OPTIONS["INITIAL_TIME"]);
   } else {
        _lastPracticed = lastpractice;
   }
   return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h, player + npc
/*
 * Gather variables for saving. These variables are common to both the player and NPCs (which is a kind of player).
 * Do not overload; NPC or player specific stuff should go to player::json_save or npc::json_save.
 */

void player::json_load_common_variables( std::map<std::string, picojson::value> & data ) {

// todo/maybe:
// std::map<std::string, int*> strmap_common_variables;
// void player::init_strmap_common_variables() {
//     strmap_common_variables["posx"]=&posx; // + all this below and in save_common_variables
// }
// load:
// for(std::map<std::string, int*>::iterator it...
//     picoint(data,it->first,it->second);
// save:
// for(...
//     data[it->first] = pv ( it->second );
    if(!picoint(data,"posx",posx) ) { // uh-oh.
         debugmsg("BAD PLAYER/NPC JSON: no 'posx'?");
    }
    picoint(data,"posy",posy);
    picoint(data,"str_cur",str_cur);                picoint(data,"str_max",str_max);
    picoint(data,"dex_cur",dex_cur);                picoint(data,"dex_max",dex_max);
    picoint(data,"int_cur",int_cur);                picoint(data,"int_max",int_max);
    picoint(data,"per_cur",per_cur);                picoint(data,"per_max",per_max);
    picoint(data,"hunger",hunger);                picoint(data,"thirst",thirst);
    picoint(data,"fatigue",fatigue);                picoint(data,"stim",stim);
    picoint(data,"pain",pain);                picoint(data,"pkill",pkill);
    picoint(data,"radiation",radiation);
    picouint(data,"scent",scent);
    picoint(data,"moves",moves);
    picoint(data,"dodges_left",dodges_left);
    picobool(data,"underwater",underwater);
    picoint(data,"oxygen",oxygen);
    picobool(data,"male",male);
    picoint(data,"cash",cash);
    picouint(data,"recoil",recoil);

    picojson::array * parray=pgetarray(data,"hp_cur");
    if ( parray != NULL && parray->size() == num_hp_parts ) {
        for(int i=0; i < num_hp_parts; i++) {
            hp_cur[i]=(int)parray->at(i).get<double>();
        }
    } else {
        debugmsg("Error, incompatible hp_cur in save file '%s'",parray!=NULL ? pv( *parray ).serialize().c_str() : "missing" );
    }

    parray=pgetarray(data,"hp_max");
    if ( parray != NULL && parray->size() == num_hp_parts ) {
        for(int i=0; i < num_hp_parts; i++) {
            hp_max[i]=(int)parray->at(i).get<double>();
        }
    } else {
        debugmsg("Error, incompatible hp_max in save file '%s'",parray!=NULL ? pv( *parray ).serialize().c_str() : "(missing)" );
    }


    picoint(data,"power_level",power_level);
    picoint(data,"max_power_level",max_power_level);

    parray=pgetarray(data,"traits");
    if ( parray != NULL ) {
        for( picojson::array::const_iterator pt = parray->begin(); pt != parray->end(); ++pt) {
             if ( (*pt).is<std::string>() ) {
                  my_traits.insert ( (*pt).get<std::string>() );
             } else {
                  // FIXME: error on type failures?
             }
        }       
    } else {
        debugmsg("player/npc:: no 'traits' array!");
    }

    picojson::object * pmap=pgetmap(data,"skills");
    if ( pmap != NULL ) {
        for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
            picojson::object::const_iterator pfind = pmap->find( (*aSkill)->ident() );
            if ( pfind != pmap->end() && pfind->second.is<picojson::object>() ) {
                  picojson::object skdata = pfind->second.get<picojson::object>();
                  skillLevel(*aSkill).json_load ( skdata );
            } else {
                 debugmsg("Load (%s) Missing skill %s","",(*aSkill)->ident().c_str() );
            }
        }
    } else {
        debugmsg("Skills[] no bueno");
    }

    styles.clear();
    parray=pgetarray(data,"styles");
    if ( parray != NULL ) {
        for( picojson::array::const_iterator pt = parray->begin(); pt != parray->end(); ++pt) {
            if ( (*pt).is<std::string>() ) {
                 styles.push_back( (*pt).get<std::string>() );
            }
        }
    }

    illness.clear();
    parray=pgetarray(data,"illness");
    if ( parray != NULL ) {
        for( picojson::array::iterator pt = parray->begin(); pt != parray->end(); ++pt) {
            if ( (*pt).is<picojson::object>() ) {
                std::map<std::string, picojson::value> & pdata = (*pt).get<picojson::object>();
                disease tmpill;
                if ( picostring(pdata,"type",tmpill.type) && picoint(pdata,"duration",tmpill.duration) ) {
                    picoint(pdata,"intensity",tmpill.intensity);
                    int tmpbp=0;
                    picoint(pdata,"bp", tmpbp);
                    tmpill.bp = (body_part)tmpbp;
                    picoint(pdata,"side", tmpill.side);
                    illness.push_back(tmpill);
                }
            }
        }
    }

    addictions.clear();
    parray=pgetarray(data,"addictions");
    if ( parray != NULL ) {
        for( picojson::array::iterator pt = parray->begin(); pt != parray->end(); ++pt) {
            if ( (*pt).is<picojson::object>() ) {
                std::map<std::string, picojson::value> & pdata = (*pt).get<picojson::object>();
                addiction tmpaddict;
                int tmpaddtype;
                if ( picoint(pdata,"type_enum",tmpaddtype) && picoint(pdata,"intensity",tmpaddict.intensity) ) {
                    tmpaddict.type = (add_type)tmpaddtype;
                    picoint(pdata,"sated",tmpaddict.sated);
                    addictions.push_back(tmpaddict);
                }

            }
        }
    }

    my_bionics.clear();
    parray=pgetarray(data,"my_bionics");
    if ( parray != NULL ) {
        for( picojson::array::iterator pt = parray->begin(); pt != parray->end(); ++pt) {
            if ( (*pt).is<picojson::object>() ) {
                std::map<std::string, picojson::value> & pdata = (*pt).get<picojson::object>();
                bionic tmpbio;
                int tmpbioinv;
                if ( picostring(pdata,"id",tmpbio.id) ) {
                  if ( bionics.find(tmpbio.id) == bionics.end() ) {
                    debugmsg("json_load: bad bionics:\n%s", (*pt).serialize().c_str()  );
                  } else {
                    picoint(pdata,"invlet",tmpbioinv);
                    tmpbio.invlet = (char)tmpbioinv;
                    picobool(pdata,"powered",tmpbio.powered);
                    picoint(pdata,"charge",tmpbio.charge);
                    my_bionics.push_back(tmpbio);
                  }
                }
            }
        }
    }

}

/*
 * Variables common to player and npc
 */
void player::json_save_common_variables( std::map<std::string, picojson::value> & data ) {

    std::vector<picojson::value> ptmpvect;
    std::map <std::string, picojson::value> ptmpmap;

    // positional data
    data["posx"] = pv( posx );                 data["posy"] = pv( posy );

    // attributes, current / max levels
    data["str_cur"] = pv( str_cur );           data["str_max"] = pv( str_max );
    data["dex_cur"] = pv( dex_cur );           data["dex_max"] = pv( dex_max );
    data["int_cur"] = pv( int_cur );           data["int_max"] = pv( int_max );
    data["per_cur"] = pv( per_cur );           data["per_max"] = pv( per_max );

    // om-noms or lack thereof
    data["hunger"] = pv( hunger );             data["thirst"] = pv( thirst );
    // energy
    data["fatigue"] = pv( fatigue );           data["stim"] = pv( stim );
    // pain
    data["pain"] = pv( pain );                 data["pkill"] = pv( pkill );
    // misc levels
    data["radiation"] = pv( radiation );
    data["scent"] = pv( int(scent) );

    // initiative type stuff
    data["moves"] = pv( moves );               data["dodges_left"] = pv( dodges_left );

    // breathing
    data["underwater"] = pv( underwater );     data["oxygen"] = pv( oxygen );

    // gender
    data["male"] = pv( male );

    data["cash"] = pv( cash );
    data["recoil"] = pv( int(recoil) );

    // potential incompatibility with future expansion
    // todo: consider ["parts"]["head"]["hp_cur"] instead of ["hp_cur"][head_enum_value]
    data["hp_cur"] = json_wrapped_vector ( std::vector<int>( hp_cur, hp_cur + num_hp_parts ) );
    data["hp_max"] = json_wrapped_vector ( std::vector<int>( hp_max, hp_max + num_hp_parts ) );

    // npc; unimplemented
    data["power_level"] = pv( power_level );
    data["max_power_level"] = pv( max_power_level );


    // traits: permanent 'mutations' more or less
    for (std::set<std::string>::iterator iter = my_traits.begin(); iter != my_traits.end(); ++iter) {
        ptmpvect.push_back( pv ( *iter ) );
    }
    data["traits"] = pv( ptmpvect );
    ptmpvect.clear();

    // skills
    for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
        ptmpmap[ (*aSkill)->ident() ] = skillLevel(*aSkill).json_save();
    }
    data["skills"] = pv ( ptmpmap );
    ptmpmap.clear();

    // martial arts
    for (int i = 0; i < styles.size(); i++) {
        ptmpvect.push_back( pv( styles[i] ) );
    }
    data["styles"] = pv ( ptmpvect );
    ptmpvect.clear();

    // disease
    for (int i = 0; i < illness.size();  i++) {
        ptmpmap[ "type" ] = pv ( illness[i].type );
        ptmpmap[ "duration" ] = pv ( illness[i].duration );
        ptmpmap[ "intensity" ] = pv ( illness[i].intensity );
        ptmpmap[ "bp" ] = pv ( (int)illness[i].bp );
        ptmpmap[ "side" ] = pv ( illness[i].side );
        ptmpvect.push_back ( pv ( ptmpmap ) );
        ptmpmap.clear();
    }
    data["illness"] = pv ( ptmpvect );
    ptmpvect.clear();

    // "Looks like I picked the wrong week to quit smoking." - Steve McCroskey
    for (int i = 0; i < addictions.size();  i++) {
        ptmpmap[ "type_enum" ] = pv ( addictions[i].type );
        ptmpmap[ "intensity" ] = pv ( addictions[i].intensity );
        ptmpmap[ "sated" ] = pv ( addictions[i].sated );
        ptmpvect.push_back ( pv ( ptmpmap ) );
        ptmpmap.clear();
    }
    data["addictions"] = pv ( ptmpvect );
    ptmpvect.clear();

    // "Fracking Toasters" - Saul Tigh, toaster
    for (int i = 0; i < my_bionics.size();  i++) {
        ptmpmap[ "id" ] = pv ( std::string(my_bionics[i].id) );
        ptmpmap[ "invlet" ] = pv ( (int)my_bionics[i].invlet );
        ptmpmap[ "powered" ] = pv ( my_bionics[i].powered );
        ptmpmap[ "charge" ] = pv ( my_bionics[i].charge );
        ptmpvect.push_back ( pv ( ptmpmap ) );
        ptmpmap.clear();
    }
    data["my_bionics"] = pv ( ptmpvect );
    ptmpvect.clear();
}

/*
 * save worn item pseudo-inventory
 */
picojson::value json_save_worn(player * p) {
    std::vector<picojson::value> data;
   
    for (int i = 0; i < p->worn.size(); i++) {
        data.push_back( p->worn[i].json_save(true) );
    }
    return pv ( data );
}
/*
 * load worn items
 */
bool json_load_worn(picojson::value & parsed, player * p, game * g) {
    picojson::array &data = parsed.get<picojson::array>();
    p->worn.clear();
    for( picojson::array::iterator pit = data.begin(); pit != data.end(); ++pit) {
        if ( (*pit).is<picojson::object>() ) {
            p->worn.push_back( item( *pit, g ) );
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player.h, player
/*
 * Prepare a json object for player, including player specific data, and data common to
   players and npcs ( which json_save_actor_data() handles ).
 */
picojson::value player::json_save(bool save_contents)
{
    std::map<std::string, picojson::value> data;

    std::vector<picojson::value> ptmpvect;
    std::map <std::string, picojson::value> ptmpmap;

    json_save_common_variables( data );

    // player-specific specifics
    data["profession"] = pv( prof->ident() );

    // someday, npcs may drive
    data["driving_recoil"] = pv( int(driving_recoil) );
    data["in_vehicle"] = pv( in_vehicle );
    data["controlling_vehicle"] = pv( controlling_vehicle );

    // shopping carts etc
    std::vector<picojson::value> pgrab;
    pgrab.push_back( pv( grab_point.x ) );
    pgrab.push_back( pv( grab_point.y ) );
    data["grab_point"] = pv( pgrab );

    // misc player specific stuff
    data["blocks_left"] = pv( blocks_left );
    data["focus_pool"] = pv( focus_pool );
    data["style_selected"] = pv( style_selected );

    // possibly related to illness[] ?
    data["health"] = pv( health );

    // crafting etc
    data["activity"] = activity.json_save();
    data["backlog"] = activity.json_save();

    // mutations; just like traits but can be removed.
    for (std::set<std::string>::iterator iter = my_mutations.begin(); iter != my_mutations.end(); ++iter) {
       ptmpvect.push_back( pv ( *iter ) );
    }
    data["mutations"] = pv( ptmpvect );
    ptmpvect.clear();

    // "The cold wakes you up."
    data["temp_cur"] = json_wrapped_vector ( std::vector<int>( temp_cur, temp_cur + num_bp ) );
    data["temp_conv"] = json_wrapped_vector ( std::vector<int>( temp_conv, temp_conv + num_bp ) );
    data["frostbite_timer"] = json_wrapped_vector ( std::vector<int>( frostbite_timer, frostbite_timer + num_bp ) );

    // npc: unimplemented, potentially useful
    for (std::map<std::string, recipe*>::iterator iter = learned_recipes.begin(); iter != learned_recipes.end(); ++iter) {
        ptmpvect.push_back( pv( iter->first ) );
    }
    data["learned_recipes"] = pv ( ptmpvect );
    ptmpvect.clear();

    // :(
    for (int i = 0; i < morale.size();  i++) {
        ptmpmap[ "type_enum" ] = pv ( morale[i].type );
        if (morale[i].item_type != NULL) {
            ptmpmap[ "item_type" ] = pv ( morale[i].item_type->id );
        }
        ptmpmap[ "bonus" ] = pv ( morale[i].bonus );
        ptmpmap[ "duration" ] = pv ( morale[i].duration );
        ptmpmap[ "decay_start" ] = pv ( morale[i].decay_start );
        ptmpmap[ "age" ] = pv ( morale[i].age );
        ptmpvect.push_back ( pv ( ptmpmap ) );
        ptmpmap.clear();
    }
    data["morale"] = pv ( ptmpvect );
    ptmpvect.clear();

    // mission stuff
    data["active_mission"] = pv( active_mission );

    data["active_missions"] = json_wrapped_vector ( active_missions );
    data["completed_missions"] = json_wrapped_vector ( completed_missions );
    data["failed_missions"] = json_wrapped_vector ( failed_missions );

    ptmpmap.clear();
    ptmpmap["squares_walked"] = pv( (*lifetime_stats()).squares_walked );
    data["player_stats"] = pv (ptmpmap);

    if ( save_contents ) {
        data["worn"] = json_save_worn(this);
        data["inv"] = inv.json_save_items();
        data["invcache"] = inv.json_save_invcache();
        if (!weapon.is_null()) {
            data["weapon"] = weapon.json_save(true);
        }       
//FIXME: seperate function, better still another file       
  /*      for(int i = 0; i < memorial_log.size(); i++) {
            ptmpvect.push_back(pv(memorial_log[i]));
        }
        data["memorial"]=pv(ptmpvect);
*/
        ptmpvect.clear();

    }
    return picojson::value(data);
}

/*
 * load player from ginormous json blob
 */
void player::json_load(picojson::value & parsed, game *g) {
    if ( ! parsed.is<picojson::object>() ) {
         debugmsg("player::json_load: bad json:\n%s",parsed.serialize().c_str() );
    }
    picojson::object &data = parsed.get<picojson::object>();

    json_load_common_variables( data );

    std::string prof_ident="(null)";
    if ( picostring(data,"profession",prof_ident) && profession::exists(prof_ident) ) {
        prof = profession::prof(prof_ident);
    } else {
        debugmsg("Tried to use non-existent profession '%s'", prof_ident.c_str());
    }

    picojson::object::iterator actfind = data.find("activity");
    if ( actfind != data.end() && actfind->second.is<picojson::object>() ) {
        activity.json_load( actfind->second );
    }
    picojson::object::iterator backfind = data.find("backlog");
    if ( backfind != data.end() && backfind->second.is<picojson::object>() ) {

        backlog.json_load( actfind->second );
    }
   
    picouint(data,"driving_recoil",driving_recoil);
    picobool(data,"in_vehicle",in_vehicle);
    picobool(data,"controlling_vehicle",controlling_vehicle);

    picopoint(data,"grab_point", grab_point);

    picoint(data, "blocks_left", blocks_left);
    picoint(data, "focus_pool", focus_pool);
    picostring(data, "style_selected", style_selected );

    picoint(data, "health", health );

    picojson::array * parray=pgetarray(data,"mutations");
    if ( parray != NULL ) {
        for( picojson::array::const_iterator pt = parray->begin(); pt != parray->end(); ++pt) {
             if ( (*pt).is<std::string>() ) {
                  my_mutations.insert ( (*pt).get<std::string>() );
             } else {}
        }
    }
    set_highest_cat_level();
// testme  drench_mut_calc();

    parray=pgetarray(data,"temp_cur");
    if ( parray != NULL && parray->size() == num_bp ) {
        for(int i=0; i < num_bp; i++) {
            temp_cur[i]=(int)parray->at(i).get<double>();
        }
    } else {
        debugmsg("Error, incompatible temp_cur in save file %s",parray!=NULL ? pv( *parray ).serialize().c_str() : "(missing)" );
    }

    parray=pgetarray(data,"temp_conv");
    if ( parray != NULL && parray->size() == num_bp ) {
        for(int i=0; i < num_bp; i++) {
            temp_conv[i]=(int)parray->at(i).get<double>();
        }
    } else {
        debugmsg("Error, incompatible temp_conv in save file %s",parray!=NULL ? pv( *parray ).serialize().c_str() : "(missing)" );
    }

    parray=pgetarray(data,"frostbite_timer");
    if ( parray != NULL && parray->size() == num_bp ) {
        for(int i=0; i < num_bp; i++) {
            frostbite_timer[i]=(int)parray->at(i).get<double>();
        }
    } else {
        debugmsg("Error, incompatible frostbite_timer in save file %s",parray!=NULL ? pv( *parray ).serialize().c_str() : "(missing)" );
    }

    parray=pgetarray(data,"learned_recipes");
    if ( parray != NULL ) {
        learned_recipes.clear();
        std::string pstr="";
        for( picojson::array::iterator pit = parray->begin(); pit != parray->end(); ++pit) {
           if ( (*pit).is<std::string>() ) {
               pstr = (*pit).get<std::string>();
               learned_recipes[ pstr ] = recipe_by_name( pstr );
           }
        }
    }   

    // todo: morale_point::json_save/load
    parray = pgetarray(data,"morale");
    if ( parray != NULL ) {
        morale.clear();
        for( picojson::array::iterator pit = parray->begin(); pit != parray->end(); ++pit) {
            if ( (*pit).is<picojson::object>() ) {
                 picojson::object & pmorale = (*pit).get<picojson::object>();
                 int tmptype;
                 if ( picoint(pmorale,"type_enum", tmptype) ) {
                     morale_point tmpmorale;
                     std::string tmpitype;
                     tmpmorale.type = (morale_type)tmptype;
                     if (picostring(pmorale,"item_type",tmpitype) ) {
                         if ( g->itypes.find(tmpitype) != g->itypes.end()) {
                             tmpmorale.item_type = g->itypes[tmpitype];
                         }
                     }
                     picoint(pmorale,"bonus",tmpmorale.bonus);
                     picoint(pmorale,"duration",tmpmorale.duration);
                     picoint(pmorale,"decay_start",tmpmorale.decay_start);
                     picoint(pmorale,"age",tmpmorale.age);
                     morale.push_back(tmpmorale);
                 } else debugmsg("Bad morale: %s", pv(pmorale).serialize().c_str() );
            }
        }
    }

    picoint(data, "active_mission", active_mission );

    picovector(data, "active_missions", active_missions );
    picovector(data, "failed_missions", failed_missions );
    picovector(data, "completed_missions", completed_missions );

//FIXME  dump << player_stats.squares_walked << " ";
    picojson::object * pmap=pgetmap(data,"player_stats");
    if ( pmap != NULL ) {
        stats & pstats = *lifetime_stats();
//        int & wk = pstats.squares_walked;
        picoint(*pmap,"squares_walked", pstats.squares_walked );
    }   

    inv.clear();
    if ( data.find("inv") != data.end() ) {
        inv.json_load_items( data.find("inv")->second, g );
    }

    worn.clear();
    picojson::object::iterator wfind = data.find("worn");
    if ( wfind != data.end() && wfind->second.is<picojson::array>() ) {
        json_load_worn(wfind->second, this, g);
    }

    weapon.contents.clear();
    if ( data.find("weapon") != data.end() ) {
        weapon.json_load( data.find("weapon")->second, g );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// npc.h

picojson::value npc_combat_rules::json_save() {
    std::map<std::string, picojson::value> data;
    data["engagement"] = pv( (int)engagement );
    data["use_guns"] = pv( use_guns );
    data["use_grenades"] = pv( use_grenades );
    data["use_silent"] = pv( use_silent );
    //todo    data["guard_pos"] = pv( guard_pos );
    return pv ( data );
}

bool npc_combat_rules::json_load(std::map<std::string, picojson::value> & data) {
    int tmpeng;
    picoint(data,"engagement", tmpeng);
    engagement = (combat_engagement)tmpeng;
    picobool(data, "use_guns", use_guns);
    picobool(data, "use_grenades", use_grenades);
    picobool(data, "use_silent", use_silent);
    return true;
}

picojson::value npc_chatbin::json_save() {
    std::map<std::string, picojson::value> data;
    data["first_topic"] = pv ( (int)first_topic );   
    data["mission_selected"] = pv ( mission_selected );   
    data["tempvalue"] = pv ( tempvalue );     //No clue what this value does, but it is used all over the place. So it is NOT temp.
    if ( skill ) {
        data["skill"] = pv ( skill->ident() );
    }
    data["missions"] = json_wrapped_vector( missions );
    data["missions_assigned"] = json_wrapped_vector( missions_assigned );
    return pv ( data );
}

bool npc_chatbin::json_load(std::map<std::string, picojson::value> & data) {
    int tmptopic;
    std::string skill_ident;

    picoint(data, "first_topic", tmptopic);
    first_topic = talk_topic(tmptopic);

    if ( picostring(data, "skill", skill_ident) ) {
        skill = Skill::skill(skill_ident);
    }

    picoint(data, "tempvalue", tempvalue);
    picoint(data, "mission_selected", mission_selected);
    picovector(data, "missions", missions );
    picovector(data, "missions_assigned", missions_assigned );
    return true;
}

void npc_personality::json_load(std::map<std::string, picojson::value> & data) {
    int tmpagg, tmpbrav, tmpcol, tmpalt;
    if ( picoint(data,"aggression", tmpagg) &&
        picoint(data,"bravery", tmpbrav) &&
        picoint(data,"collector", tmpcol) &&
        picoint(data,"altruism", tmpalt) ) {
        aggression=(signed char)tmpagg;
        bravery=(signed char)tmpbrav;
        collector=(signed char)tmpcol;
        altruism=(signed char)tmpalt;
    } else {
        debugmsg("npc_personality: bad data");
    }
}

picojson::value npc_personality::json_save() {
    std::map<std::string, picojson::value> data;
    data["aggression"] = pv( (int)aggression );
    data["bravery"] = pv( (int)bravery );
    data["collector"] = pv( (int)collector );
    data["altruism"] = pv( (int)altruism );
    return pv( data );
};

bool npc_opinion::json_load(std::map<std::string, picojson::value> & data) {
    picoint(data,"trust",trust);
    picoint(data,"fear",fear);
    picoint(data,"value",value);
    picoint(data,"anger",anger);
    picoint(data,"owed",owed);
    picojson::array * parray=pgetarray(data,"favors");
    if ( parray != NULL ) {
        favors.clear();
        for( picojson::array::iterator pfav = parray->begin(); pfav != parray->end(); ++pfav) {
            if ( (*pfav).is<picojson::object>() ) {
                npc_favor tmpfavor;
                tmpfavor.json_load( (*pfav).get<picojson::object>() );
                favors.push_back( tmpfavor );
            }
        }
    }
    return true;
}

picojson::value npc_opinion::json_save() {
    std::map<std::string, picojson::value> data;
    data["trust"] = pv( trust );
    data["fear"] = pv( fear );
    data["value"] = pv( value );
    data["anger"] = pv( anger );
    data["owed"] = pv( owed );
    if ( favors.size() > 0 ) {
        std::vector<picojson::value> fdata;
        for (int i = 0; i < favors.size(); i++) {
            fdata.push_back( favors[i].json_save() );
        }
        data["favors"] = pv( fdata );
    }
    return pv( data );
}

bool npc_favor::json_load(std::map<std::string, picojson::value> & data) {
   int tmptype, tmpskill;
   std::string tmpitem;
   if ( picoint(data,"type",tmptype) &&
      picoint(data,"value",value) &&
      picostring(data,"itype_id",tmpitem) &&
      picoint(data,"skill_id",tmpskill) ) {
      type = npc_favor_type(tmptype);
      item_id = tmpitem;
      skill =  Skill::skill(tmpskill);
      return true;
   } else {
      debugmsg("npc_favor::load: bad favor");
      return false;
   }
}

picojson::value npc_favor::json_save()  {
    std::map<std::string, picojson::value> data;
    data["type"] = pv( (int)type );
    data["value"] = pv( value );
    data["itype_id"] = pv( (std::string)item_id );
    data["skill_id"] = pv( (int)skill->id() ); // FIXME: use ident()
    return pv( data );
}

/*
 * load npc
 */
void npc::json_load(picojson::value & parsed, game * g) {
    if ( ! parsed.is<picojson::object>() ) {
         debugmsg("npc::json_load: bad json:\n%s",parsed.serialize().c_str() );
    }
    picojson::object &data = parsed.get<picojson::object>();

    json_load_common_variables(data);

    int misstmp, classtmp, flagstmp, atttmp, tmpid;

    picoint(data,"id",tmpid);  setID(tmpid);
    picostring(data,"name",name);
    picobool(data, "marked_for_death", marked_for_death);
    picobool(data, "dead", dead);
    if ( picoint(data, "myclass", classtmp) ) {
        myclass = npc_class( classtmp );
    }

    picojson::object * pmap=pgetmap(data,"personality");
    if ( pmap != NULL ) {
        personality.json_load( *pmap );
    }   

    picoint(data,"wandx",wandx);
    picoint(data,"wandy",wandy);
    picoint(data,"wandf",wandf);
    picoint(data,"omx",omx);
    picoint(data,"omy",omy);
    picoint(data,"omz",omz);

    picoint(data,"mapx",mapx);
    picoint(data,"mapy",mapy);

    picoint(data,"plx",plx);
    picoint(data,"ply",ply);

    picoint(data,"goalx",goalx);
    picoint(data,"goaly",goaly);
    picoint(data,"goalz",goalz);

    if ( picoint(data,"mission",misstmp) ) {
        mission = npc_mission( misstmp );
    }


    if ( picoint(data, "flags", flagstmp) ) {
        flags = flagstmp;
    }
    if ( picoint(data, "attitude", atttmp) ) {
        attitude = npc_attitude(atttmp);
    }
   
    inv.clear();
    if ( data.find("inv") != data.end() ) {
        inv.json_load_items( data.find("inv")->second, g );
    }

    worn.clear();
    picojson::object::iterator wfind = data.find("worn");
    if ( wfind != data.end() && wfind->second.is<picojson::array>() ) {
        json_load_worn(wfind->second, this, g);
    }

    weapon.contents.clear();
    if ( data.find("weapon") != data.end() ) {
        weapon.json_load( data.find("weapon")->second, g );
    }

    pmap=pgetmap(data,"op_of_u");
    if ( pmap != NULL ) {
        op_of_u.json_load( *pmap );
    }

    pmap=pgetmap(data,"chatbin");
    if ( pmap != NULL ) {
        chatbin.json_load( *pmap );
    }

    pmap=pgetmap(data,"combat_rules");
    if ( pmap != NULL ) {
        combat_rules.json_load( *pmap );
    }
}

/*
 * save npc
 */
picojson::value npc::json_save(bool save_contents) {
    std::map<std::string, picojson::value> data;
    std::vector<picojson::value> ptmpvect;
    std::map <std::string, picojson::value> ptmpmap;

    json_save_common_variables( data );

    data["name"] = pv( name );
    data["id"] = pv( getID() );
    data["marked_for_death"] = pv( marked_for_death );
    data["dead"] = pv( dead );
    data["patience"] = pv( patience );
    data["myclass"] = pv( (int)myclass );

    data["personality"] = personality.json_save();
    data["wandx"] = pv( wandx );
    data["wandy"] = pv( wandy );
    data["wandf"] = pv( wandf );
    data["omx"] = pv( omx );
    data["omy"] = pv( omy );
    data["omz"] = pv( omz );

    data["mapx"] = pv( mapx );
    data["mapy"] = pv( mapy );
    data["plx"] = pv( plx );
    data["ply"] = pv( ply );
    data["goalx"] = pv( goalx );
    data["goaly"] = pv( goaly );
    data["goalz"] = pv( goalz );

    data["mission"] = pv( mission ); // todo: stringid
    data["flags"] = pv( flags );
    if ( my_fac != NULL ) { // set in constructor
        data["my_fac"] = pv ( my_fac->id );
    }
    data["attitude"] = pv ( (int)attitude );
    data["op_of_u"] = op_of_u.json_save();
    data["chatbin"] = chatbin.json_save();
    data["combat_rules"] = combat_rules.json_save();

    if ( save_contents ) {
        data["worn"] = json_save_worn(this);
        data["inv"] = inv.json_save_items();
        if (!weapon.is_null()) {
            data["weapon"] = weapon.json_save(true);
        }
    }

    return pv( data );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// inventory.h

////////////////////////////////////////////////////////////////////////////////////////////////////
///// vehicle.h
/*
 * Load vehicle from a json blob that might just exceed player in size.
 */
void vehicle::json_load(picojson::value & parsed, game * g ) {
    if ( ! parsed.is<picojson::object>() ) {
         debugmsg("vehicle::json_load: bad json:\n%s",parsed.serialize().c_str() );
    }
    picojson::object &data = parsed.get<picojson::object>();

    int fdir, mdir;
   
    picostring(data,"type",type);
    picoint(data, "posx", posx);   
    picoint(data, "posy", posy);
    picoint(data, "faceDir", fdir);
    picoint(data, "moveDir", mdir);
    picoint(data, "turn_dir", turn_dir);
    picoint(data, "velocity", velocity);
    picoint(data, "cruise_velocity", cruise_velocity);
    picobool(data, "cruise_on", cruise_on);
    picobool(data, "lights_on", lights_on);
    picobool(data, "skidding", skidding);
    picoint(data, "turret_mode", turret_mode);
    picojson::object::const_iterator oftcit = data.find("of_turn_carry");
    if(oftcit != data.end() && oftcit->second.is<double>() ) {
        of_turn_carry = (float)oftcit->second.get<double>();
    }
    face.init (fdir);
    move.init (mdir);
    picostring(data,"name",name);

    parts.clear();

    picojson::array * parray=pgetarray(data,"parts");
    if ( parray != NULL ) {
        for( picojson::array::iterator pt = parray->begin(); pt != parray->end(); ++pt) {
            if ( (*pt).is<picojson::object>() ) {
                std::map<std::string, picojson::value> & pdata = (*pt).get<picojson::object>();
                int pid, pflag;
 // FIXME: this is temporary until migration to string id (?)
                std::string tmpid;
                if ( picostring(pdata,"id",tmpid) && vpart_enums.find(tmpid) != vpart_enums.end() ) {
                    pid = vpart_enums[tmpid];
                } else {
                    // FIXME: stash int to stringid array in _legacy, for 0.8
                    picoint(pdata, "id_enum", pid);
                }
                vehicle_part new_part;
                new_part.id = (vpart_id) pid;
               
                picoint(pdata, "mount_dx", new_part.mount_dx);
                picoint(pdata, "mount_dy", new_part.mount_dy);
                picoint(pdata, "hp", new_part.hp );
                picoint(pdata, "amount", new_part.amount );
                picoint(pdata, "blood", new_part.blood );
                picoint(pdata, "bigness", new_part.bigness );
                if ( picoint(pdata, "flags", pflag ) ) {
                    new_part.flags = pflag;
                }               
                picoint(pdata, "passenger_id", new_part.passenger_id );
               
                picojson::array * piarray=pgetarray(pdata,"items");
                new_part.items.clear();
                if ( piarray != NULL ) {
                    for( picojson::array::iterator pit = piarray->begin(); pit != piarray->end(); ++pit) {
                        if ( (*pit).is<picojson::object>() ) {
                            new_part.items.push_back( item( *pit, g ) );
                        }
                    }
                }           
                parts.push_back (new_part);
            }
        }
        parray = NULL;
    }
    find_external_parts ();
    find_exhaust ();
    insides_dirty = true;
    precalc_mounts (0, face.dir());
    parray = pgetarray(data,"tags");
    if ( parray != NULL ) {
        for( picojson::array::iterator pt = parray->begin(); pt != parray->end(); ++pt) {
            if ( (*pt).is<std::string>() ) {
                tags.insert( (*pt).get<std::string>() );
            }
        }
    }
    return;
}


picojson::value vehicle::json_save( bool save_contents ) {
    std::map<std::string, picojson::value> data;
    data["type"] = pv ( type );
    data["posx"] = pv ( posx );
    data["posy"] = pv ( posy );
    data["faceDir"] = pv ( face.dir() );
    data["moveDir"] = pv ( move.dir() );
    data["turn_dir"] = pv ( turn_dir );
    data["velocity"] = pv ( velocity );
    data["cruise_velocity"] = pv ( cruise_velocity );

    data["cruise_on"] = pv ( cruise_on );
    data["lights_on"] = pv ( lights_on );
    data["turret_mode"] = pv ( turret_mode );
    data["skidding"] = pv ( skidding );

    data["of_turn_carry"] = pv ( of_turn_carry );
    data["name"] = pv ( name );

    std::vector<picojson::value> ptmpvec;
    std::map<std::string, picojson::value> pdata;
    std::vector<picojson::value> pitms;
    for (int p = 0; p < parts.size(); p++) {
        pdata["id"] = pv ( vpart_list[ parts[p].id ].id ); // FIXME; after vpart enum is dropped
        pdata["mount_dx"] = pv ( parts[p].mount_dx );
        pdata["mount_dy"] = pv ( parts[p].mount_dy );
        pdata["hp"] = pv ( parts[p].hp );
        pdata["amount"] = pv ( parts[p].amount );
        pdata["blood"] = pv ( parts[p].blood );
        pdata["bigness"] = pv ( parts[p].bigness );
        pdata["flags"] = pv ( parts[p].flags );
        pdata["passenger_id"] = pv ( parts[p].passenger_id );
        if ( parts[p].items.size() > 0 ) {
            for (int i = 0; i < parts[p].items.size(); i++) {
                pitms.push_back( parts[p].items[i].json_save(true) );
            }
            pdata["items"] = pv ( pitms );
            pitms.clear();
        }
        ptmpvec.push_back( pv ( pdata ) );
        pdata.clear();
    }
    data["parts"] = pv ( ptmpvec );
    ptmpvec.clear();

    for( std::set<std::string>::const_iterator it = tags.begin(); it != tags.end(); ++it ) {
        ptmpvec.push_back( pv( *it ) );
    }
    data["tags"] = pv ( ptmpvec );
    ptmpvec.clear();
    return pv( data );
}

