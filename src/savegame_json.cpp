// Associated headers here are the ones for which their only non-inline
// functions are serialization functions.  This allows IWYU to check the
// includes in such headers.

#include "enums.h" // IWYU pragma: associated
#include "npc_favor.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <bitset>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <numeric>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "active_item_cache.h"
#include "activity_actor.h"
#include "activity_actor_definitions.h"
#include "activity_type.h"
#include "addiction.h"
#include "assign.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "basecamp.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_io.h"
#include "cata_utility.h"
#include "cata_variant.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "clone_ptr.h"
#include "clzones.h"
#include "colony.h"
#include "computer.h"
#include "construction.h"
#include "coordinates.h"
#include "craft_command.h"
#include "creature.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "dialogue_chatbin.h"
#include "effect.h"
#include "effect_source.h"
#include "event.h"
#include "faction.h"
#include "field.h"
#include "field_type.h"
#include "flag.h"
#include "flat_set.h"
#include "game.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_contents.h"
#include "item_factory.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "json.h"
#include "kill_tracker.h"
#include "lru_cache.h"
#include "magic.h"
#include "magic_teleporter_list.h"
#include "make_static.h"
#include "map.h"
#include "map_memory.h"
#include "mapdata.h"
#include "mattack_common.h"
#include "memory_fast.h"
#include "mission.h"
#include "monster.h"
#include "morale.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "npc_class.h"
#include "optional.h"
#include "options.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "profession.h"
#include "proficiency.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "relic.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "scenario.h"
#include "skill.h"
#include "stats_tracker.h"
#include "stomach.h"
#include "submap.h"
#include "text_snippets.h"
#include "tileray.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"

struct mutation_branch;
struct oter_type_t;

static const activity_id ACT_FETCH_REQUIRED( "ACT_FETCH_REQUIRED" );
static const activity_id ACT_MIGRATION_CANCEL( "ACT_MIGRATION_CANCEL" );

static const anatomy_id anatomy_human_anatomy( "human_anatomy" );

static const efftype_id effect_riding( "riding" );

static const itype_id itype_internal_battery_compartment( "internal_battery_compartment" );
static const itype_id itype_internal_ethanol_tank( "internal_ethanol_tank" );
static const itype_id itype_internal_gasoline_tank( "internal_gasoline_tank" );
static const itype_id itype_internal_oil_tank( "internal_oil_tank" );
static const itype_id itype_rad_badge( "rad_badge" );
static const itype_id itype_radio( "radio" );
static const itype_id itype_radio_on( "radio_on" );
static const itype_id itype_usb_drive( "usb_drive" );

static const matype_id style_none( "style_none" );

static const relic_procgen_id relic_procgen_data_cult( "cult" );

static const skill_id skill_chemistry( "chemistry" );

static const ter_str_id ter_t_ash( "t_ash" );
static const ter_str_id ter_t_pwr_sb_support_l( "t_pwr_sb_support_l" );
static const ter_str_id ter_t_pwr_sb_switchgear_l( "t_pwr_sb_switchgear_l" );
static const ter_str_id ter_t_pwr_sb_switchgear_s( "t_pwr_sb_switchgear_s" );
static const ter_str_id ter_t_rubble( "t_rubble" );
static const ter_str_id ter_t_wreckage( "t_wreckage" );

static const vpart_id vpart_turret_mount( "turret_mount" );
static const vproto_id vehicle_prototype_bicycle( "bicycle" );
static const vproto_id vehicle_prototype_bicycle_folding( "bicycle_folding" );

static const std::array<std::string, static_cast<size_t>( object_type::NUM_OBJECT_TYPES )>
obj_type_name = { { "OBJECT_NONE", "OBJECT_ITEM", "OBJECT_ACTOR", "OBJECT_PLAYER",
        "OBJECT_NPC", "OBJECT_MONSTER", "OBJECT_VEHICLE", "OBJECT_TRAP", "OBJECT_FIELD",
        "OBJECT_TERRAIN", "OBJECT_FURNITURE"
    }
};

// TODO: investigate serializing other members of the Creature class hierarchy
static void serialize( const weak_ptr_fast<monster> &obj, JsonOut &jsout )
{
    if( const auto monster_ptr = obj.lock() ) {
        jsout.start_object();

        jsout.member( "monster_at", monster_ptr->get_location() );
        // TODO: if monsters/Creatures ever get unique ids,
        // create a differently named member, e.g.
        //     jsout.member("unique_id", monster_ptr->getID());
        jsout.end_object();
    } else {
        // Monster went away. It's up the activity handler to detect this.
        jsout.write_null();
    }
}

static void deserialize( weak_ptr_fast<monster> &obj, const JsonObject &data )
{
    data.allow_omitted_members();
    tripoint_abs_ms temp_pos;

    obj.reset();
    if( data.read( "monster_at", temp_pos ) ) {
        const auto monp = g->critter_tracker->find( temp_pos );

        if( monp == nullptr ) {
            debugmsg( "no monster found at %s", temp_pos.to_string_writable() );
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

static tripoint read_legacy_creature_pos( const JsonObject &data )
{
    tripoint pos;
    if( !data.read( "posx", pos.x ) || !data.read( "posy", pos.y ) || !data.read( "posz", pos.z ) ) {
        debugmsg( R"(Bad Creature JSON: neither "location" nor "posx", "posy", "posz" found)" );
    }
    return pos;
}

void item_contents::serialize( JsonOut &json ) const
{
    if( !contents.empty() ) {
        json.start_object();

        json.member( "contents", contents );
        json.member( "additional_pockets", additional_pockets );

        json.end_object();
    }
}

void item_contents::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "contents", contents );
    data.read( "additional_pockets", additional_pockets );

}

void item_pocket::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "pocket_type", data->type );
    json.member( "contents", contents );
    json.member( "_sealed", _sealed );
    if( !this->settings.is_null() ) {
        json.member( "favorite_settings", this->settings );
    }
    json.end_object();
}

void item_pocket::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "contents", contents );
    int saved_type_int;
    data.read( "pocket_type", saved_type_int );
    _saved_type = static_cast<item_pocket::pocket_type>( saved_type_int );
    data.read( "_sealed", _sealed );
    _saved_sealed = _sealed;
    if( data.has_member( "favorite_settings" ) ) {
        data.read( "favorite_settings", this->settings );
    } else {
        this->settings.clear();
    }
}

void item_pocket::favorite_settings::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "priority", priority_rating );
    json.member( "item_whitelist", item_whitelist );
    json.member( "item_blacklist", item_blacklist );
    json.member( "category_whitelist", category_whitelist );
    json.member( "category_blacklist", category_blacklist );
    json.member( "collapsed", collapsed );
    json.member( "disabled", disabled );
    json.member( "unload", unload );
    json.end_object();
}

void item_pocket::favorite_settings::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "priority", priority_rating );
    data.read( "item_whitelist", item_whitelist );
    data.read( "item_blacklist", item_blacklist );
    data.read( "category_whitelist", category_whitelist );
    data.read( "category_blacklist", category_blacklist );
    if( data.has_member( "collapsed" ) ) {
        data.read( "collapsed", collapsed );
    }
    if( data.has_member( "disabled" ) ) {
        data.read( "disabled", disabled );
    }
    if( data.has_member( "unload" ) ) {
        data.read( "unload", unload );
    }
}

void pocket_data::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    load( data );
}

void sealable_data::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    load( data );
}

void pocket_noise::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    load( data );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// player_activity.h

void player_activity::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "type", type );

    if( !type.is_null() ) {
        json.member( "actor", actor );
        json.member( "moves_total", moves_total );
        json.member( "moves_left", moves_left );
        json.member( "interruptable", interruptable );
        json.member( "interruptable_with_kb", interruptable_with_kb );
        json.member( "index", index );
        json.member( "position", position );
        json.member( "coords", coords );
        json.member( "coord_set", coord_set );
        json.member( "name", name );
        json.member( "targets", targets );
        json.member( "placement", placement );
        json.member( "relative_placement", relative_placement );
        json.member( "values", values );
        json.member( "str_values", str_values );
        json.member( "auto_resume", auto_resume );
        json.member( "monsters", monsters );
    }
    json.end_object();
}

void player_activity::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    std::string tmptype;
    int tmppos = 0;

    bool is_obsolete = false;
    std::set<std::string> obs_activities {
        "ACT_MAKE_ZLAVE" // Remove after 0.F
    };
    if( !data.read( "type", tmptype ) ) {
        // Then it's a legacy save.
        int tmp_type_legacy = data.get_int( "type" );
        deserialize_legacy_type( tmp_type_legacy, type );
    } else if( !obs_activities.count( tmptype ) ) {
        type = activity_id( tmptype );
    } else {
        is_obsolete = true;
    }

    if( type.is_null() ) {
        return;
    }

    const bool has_actor = activity_actors::deserialize_functions.find( type ) !=
                           activity_actors::deserialize_functions.end();

    // Handle migration of pre-activity_actor activities
    // ACT_MIGRATION_CANCEL will clear the backlog and reset npc state
    // this may cause inconvenience but should avoid any lasting damage to npcs
    if( is_obsolete || ( has_actor && ( data.has_null( "actor" ) || !data.has_member( "actor" ) ) ) ) {
        type = ACT_MIGRATION_CANCEL;
        actor = std::make_unique<migration_cancel_activity_actor>();
    } else {
        data.read( "actor", actor );
    }

    if( !data.read( "position", tmppos ) ) {
        tmppos = INT_MIN;  // If loading a save before position existed, hope.
    }

    data.read( "moves_total", moves_total );
    data.read( "moves_left", moves_left );
    data.read( "interruptable", interruptable );
    data.read( "interruptable_with_kb", interruptable_with_kb );
    data.read( "index", index );
    position = tmppos;
    data.read( "coords", coords );
    data.read( "coord_set", coord_set );
    data.read( "name", name );
    data.read( "targets", targets );
    data.read( "placement", placement );
    data.read( "relative_placement", relative_placement );
    values = data.get_int_array( "values" );
    str_values = data.get_string_array( "str_values" );
    data.read( "auto_resume", auto_resume );
    data.read( "monsters", monsters );

}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// requirements.h
void requirement_data::serialize( JsonOut &json ) const
{
    json.start_object();

    if( !is_null() ) {
        json.member( "blacklisted", blacklisted );
        json.member( "req_comps_total", components );
        json.member( "tool_comps_total", tools );
        json.member( "quality_comps_total", qualities );
    }
    json.end_object();
}

void requirement_data::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();

    data.read( "blacklisted", blacklisted );

    data.read( "req_comps_total", components );
    data.read( "tool_comps_total", tools );
    data.read( "quality_comps_total", qualities );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// skill.h
void SkillLevel::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "level", _level );
    json.member( "exercise", _exercise );
    json.member( "istraining", _isTraining );
    json.member( "lastpracticed", _lastPracticed );
    json.member( "knowledgeLevel", _knowledgeLevel );
    json.member( "knowledgeExperience", _knowledgeExperience );
    json.member( "rustaccumulator", _rustAccumulator );
    json.end_object();
}

void SkillLevel::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "level", _level );
    data.read( "exercise", _exercise );
    if( _level < 0 ) {
        _level = 0;
        _exercise = 0;
    }
    if( _exercise < 0 ) {
        _exercise = 0;
    }
    data.read( "istraining", _isTraining );
    data.read( "rustaccumulator", _rustAccumulator );
    if( !data.read( "lastpracticed", _lastPracticed ) ) {
        _lastPracticed = calendar::start_of_cataclysm + time_duration::from_hours(
                             get_option<int>( "INITIAL_TIME" ) );
    }
    data.read( "knowledgeLevel", _knowledgeLevel );
    if( _knowledgeLevel < _level ) {
        _knowledgeLevel = _level;
    }
    data.read( "knowledgeExperience", _knowledgeExperience );
    if( _knowledgeLevel == _level && _knowledgeExperience < _exercise ) {
        _knowledgeExperience = _exercise;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// character_id.h

void character_id::serialize( JsonOut &jsout ) const
{
    jsout.write( value );
}

void character_id::deserialize( int i )
{
    value = i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// effect_source.h

void effect_source::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "character_id", this->character );
    json.member( "faction_id", this->fac );
    if( this->mfac ) {
        json.member( "mfaction_id", this->mfac->id().str() );
    }
    json.end_object();
}

void effect_source::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "character_id", this->character );
    data.read( "faction_id", this->fac );
    const std::string mfac_id = data.get_string( "mfaction_id", "" );
    this->mfac = !mfac_id.empty()
                 ? cata::optional<mfaction_id>( mfaction_str_id( mfac_id ).id() )
                 : cata::optional<mfaction_id>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// Character.h, avatar + npc

void Character::trait_data::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "key", key );
    json.member( "charge", charge );
    json.member( "powered", powered );
    json.member( "show_sprite", show_sprite );
    if( variant != nullptr ) {
        json.member( "variant-parent", variant->parent );
        json.member( "variant-id", variant->id );
    }
    json.end_object();
}

void Character::trait_data::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "key", key );

    //Remove after 0.G
    if( data.has_int( "charge" ) ) {
        charge = time_duration::from_turns( data.get_int( "charge" ) );
    } else {
        data.read( "charge", charge );
    }
    data.read( "powered", powered );
    data.read( "show_sprite", show_sprite );
    if( data.has_member( "variant-parent" ) ) {
        trait_id parent;
        std::string variant_id;

        data.read( "variant-parent", parent );
        data.read( "variant-id", variant_id );

        // No parent == nowhere to look up this variant. It's effectively lost
        if( !parent.is_valid() ) {
            return;
        }
        variant = parent->variant( variant_id );
    }
}

void consumption_event::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "time", time );
    json.member( "type_id", type_id );
    json.member( "component_hash", component_hash );
    json.end_object();
}

void consumption_event::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();
    jo.read( "time", time );
    jo.read( "type_id", type_id );
    jo.read( "component_hash", component_hash );
}

void activity_tracker::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "current_activity", current_activity );
    json.member( "accumulated_activity", accumulated_activity );
    json.member( "previous_activity", previous_activity );
    json.member( "previous_turn_activity", previous_turn_activity );
    json.member( "current_turn", current_turn );
    json.member( "activity_reset", activity_reset );
    json.member( "num_events", num_events );

    json.member( "tracker", tracker );
    json.member( "intake", intake );
    json.member( "low_activity_ticks", low_activity_ticks );
    json.end_object();
}

void activity_tracker::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();
    jo.read( "current_activity", current_activity );
    jo.read( "accumulated_activity", accumulated_activity );
    jo.read( "previous_activity", previous_activity );
    jo.read( "previous_turn_activity", previous_turn_activity );
    jo.read( "current_turn", current_turn );
    jo.read( "activity_reset", activity_reset );
    jo.read( "num_events", num_events );

    jo.read( "tracker", tracker );
    jo.read( "intake", intake );
    jo.read( "low_activity_ticks", low_activity_ticks );
    if( jo.has_member( "tick_counter" ) ) { // migration - remove after 0.G
        tracker *= 1000;
        intake *= 1000;
    }
}

// migration handling of items that used to have charges instead of real items.
// remove this migration function after 0.F
static void migrate_item_charges( item &it )
{
    if( it.charges != 0 && it.has_pocket_type( item_pocket::pocket_type::MAGAZINE ) ) {
        it.ammo_set( it.ammo_default(), it.charges );
        it.charges = 0;
    }
}

/**
 * Gather variables for saving. These variables are common to both the avatar and NPCs.
 */
void Character::load( const JsonObject &data )
{
    data.allow_omitted_members();
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
    data.read( "omt_path", omt_path );

    data.read( "name", name );
    data.read( "play_name", play_name );
    data.read( "base_age", init_age );
    data.read( "base_height", init_height );
    if( !data.read( "blood_type", my_blood_type ) ||
        !data.read( "blood_rh_factor", blood_rh_factor ) ) {
        randomize_blood();
    }
    data.read( "avg_nat_bpm", avg_nat_bpm );

    data.read( "custom_profession", custom_profession );

    // sleep
    data.read( "daily_sleep", daily_sleep );
    data.read( "continuous_sleep", continuous_sleep );

    // needs
    data.read( "thirst", thirst );
    data.read( "hunger", hunger );
    data.read( "fatigue", fatigue );
    data.read( "cardio_acc", cardio_acc );
    // Legacy read, remove after 0.F
    data.read( "weary", activity_history );
    data.read( "activity_history", activity_history );
    data.read( "sleep_deprivation", sleep_deprivation );
    data.read( "stored_calories", stored_calories );
    // stored_calories was changed from being in kcal to being in just cal
    if( savegame_loading_version <= 31 ) {
        stored_calories *= 1000;
    }
    data.read( "radiation", radiation );
    data.read( "oxygen", oxygen );
    data.read( "pkill", pkill );

    data.read( "type_of_scent", type_of_scent );
    data.read( "focus_pool", focus_pool );

    if( data.has_array( "ma_styles" ) ) {
        std::vector<matype_id> temp_styles;
        data.read( "ma_styles", temp_styles );
        bool temp_keep_hands_free = false;
        data.read( "keep_hands_free", temp_keep_hands_free );
        matype_id temp_selected_style;
        data.read( "style_selected", temp_selected_style );
        if( !temp_selected_style.is_valid() ) {
            temp_selected_style = style_none;
        }
        *martial_arts_data = character_martial_arts( temp_styles, temp_selected_style,
                             temp_keep_hands_free );
    } else {
        data.read( "martial_arts_data", martial_arts_data );
    }

    JsonObject vits = data.get_object( "vitamin_levels" );
    vits.allow_omitted_members();
    for( const std::pair<const vitamin_id, vitamin> &v : vitamin::all() ) {
        if( vits.has_member( v.first.str() ) ) {
            int lvl = vits.get_int( v.first.str() );
            vitamin_levels[v.first] = clamp( lvl, v.first->min(), v.first->max() );
        }
    }
    JsonObject vits_daily = data.get_object( "daily_vitamins" );
    vits_daily.allow_omitted_members();
    for( const std::pair<const vitamin_id, vitamin> &v : vitamin::all() ) {
        if( vits_daily.has_member( v.first.str() ) ) {
            JsonArray vals = vits_daily.get_array( v.first.str() );
            int speculative = vals.next_int();
            int lvl = vals.next_int();
            daily_vitamins[v.first] = { speculative, lvl };
        }
    }
    data.read( "consumption_history", consumption_history );
    data.read( "destination_activity", destination_activity );
    data.read( "stashed_outbounds_activity", stashed_outbounds_activity );
    data.read( "stashed_outbounds_backlog", stashed_outbounds_backlog );

    // npc activity on vehicles.
    data.read( "activity_vehicle_part_index", activity_vehicle_part_index );
    // health
    data.read( "healthy", lifestyle );
    data.read( "healthy_mod", daily_health );
    data.read( "health_tally", health_tally );

    // Remove check after 0.F
    if( savegame_loading_version >= 30 ) {
        if( data.has_array( "proficiencies" ) ) {
            _proficiencies->deserialize_legacy( data.get_array( "proficiencies" ) );
        } else {
            data.read( "proficiencies", _proficiencies );
        }
    }

    //energy
    data.read( "stim", stim );
    data.read( "stamina", stamina );

    // stats through kills
    data.read( "kill_xp", kill_xp );
    if( !data.read( "spent_upgrade_points", spent_upgrade_points ) ) {
        // TEMPORARY until 0.G, remove migration logic after
        int str_upgrade = 0;
        int dex_upgrade = 0;
        int int_upgrade = 0;
        int per_upgrade = 0;
        if( data.read( "str_upgrade", str_upgrade ) && data.read( "dex_upgrade", dex_upgrade ) &&
            data.read( "int_upgrade", int_upgrade ) && data.read( "per_upgrade", per_upgrade ) ) {
            str_max += str_upgrade;
            dex_max += dex_upgrade;
            int_max += int_upgrade;
            per_max += per_upgrade;
            spent_upgrade_points = str_upgrade + dex_upgrade + int_upgrade + per_upgrade;
        }
    }

    data.read( "moncams", moncams );

    data.read( "magic", magic );

    data.read( "underwater", underwater );

    data.read( "traits", my_traits );
    // If a trait has been migrated, we'll need to add it.
    // Queue them up to add at the end, because adding and removing at the same time is hard
    std::set<trait_id> traits_to_add;
    for( auto it = my_traits.begin(); it != my_traits.end(); ) {
        const auto &tid = *it;
        if( tid.is_valid() ) {
            ++it;
            continue;
        }

        const trait_replacement &rules = mutation_branch::trait_migration( tid );
        if( rules.prof ) {
            add_proficiency( *rules.prof );
        } else if( rules.trait ) {
            traits_to_add.emplace( rules.trait->trait );
        } else {
            if( rules.error ) {
                debugmsg( "character %s has invalid trait %s, it will be ignored", get_name(), tid.str() );
            }
        }
        it = my_traits.erase( it );
    }
    for( const trait_id &add : traits_to_add ) {
        my_traits.emplace( add );
    }

    data.read( "mutations", my_mutations );

    // Hold onto the mutations to add at the end...
    std::map<trait_id, trait_data> muts_to_add;
    for( auto it = my_mutations.begin(); it != my_mutations.end(); ) {
        const trait_id &mid = it->first;
        if( mid.is_valid() ) {
            ++it;
            continue;
        }

        const trait_replacement &rules = mutation_branch::trait_migration( mid );
        if( rules.prof ) {
            add_proficiency( *rules.prof );
        } else if( rules.trait ) {
            const trait_id &added = rules.trait->trait;
            const std::string &added_var = rules.trait->variant;
            auto add_it = muts_to_add.emplace( added, it->second ).first;
            add_it->second.variant = added->variant( added_var );
        } else {
            if( rules.error ) {
                debugmsg( "character %s has invalid mutation %s, it will be ignored", get_name(), mid.str() );
            }
        }
        it = my_mutations.erase( it );
    }
    for( const std::pair<const trait_id, trait_data> &add : muts_to_add ) {
        my_mutations.emplace( add.first, add.second );
    }
    // We need to ensure that my_mutations contains no invalid mutations before we do this
    // As every time we add a mutation, we rebuild the enchantment cache, causing errors if
    // we have invalid mutations.
    for( const std::pair<const trait_id, trait_data> &mut : my_mutations ) {
        on_mutation_gain( mut.first );
        cached_mutations.push_back( &mut.first.obj() );
    }
    recalculate_size();

    data.read( "my_bionics", *my_bionics );

    data.read( "known_monsters", known_monsters );

    invalidate_pseudo_items();
    update_bionic_power_capacity();
    data.read( "death_eocs", death_eocs );
    worn.on_takeoff( *this );
    worn.clear();
    // deprecate after 0.G
    if( data.has_array( "worn" ) ) {
        std::list<item> items;
        data.read( "worn", items );
        worn = outfit( items );
    } else {
        data.read( "worn", worn );
    }
    worn.on_item_wear( *this );

    // TEMPORARY until 0.F
    if( data.has_array( "hp_cur" ) ) {
        set_anatomy( anatomy_human_anatomy );
        set_body();
        std::array<int, 6> hp_cur;
        data.read( "hp_cur", hp_cur );
        std::array<int, 6> hp_max;
        data.read( "hp_max", hp_max );
        set_part_hp_cur( bodypart_id( "head" ), hp_cur[0] );
        set_part_hp_max( bodypart_id( "head" ), hp_max[0] );
        set_part_hp_cur( bodypart_id( "torso" ), hp_cur[1] );
        set_part_hp_max( bodypart_id( "torso" ), hp_max[1] );
        set_part_hp_cur( bodypart_id( "arm_l" ), hp_cur[2] );
        set_part_hp_max( bodypart_id( "arm_l" ), hp_max[2] );
        set_part_hp_cur( bodypart_id( "arm_r" ), hp_cur[3] );
        set_part_hp_max( bodypart_id( "arm_r" ), hp_max[3] );
        set_part_hp_cur( bodypart_id( "leg_l" ), hp_cur[4] );
        set_part_hp_max( bodypart_id( "leg_l" ), hp_max[4] );
        set_part_hp_cur( bodypart_id( "leg_r" ), hp_cur[5] );
        set_part_hp_max( bodypart_id( "leg_r" ), hp_max[5] );
    }
    if( data.has_array( "damage_bandaged" ) ) {
        set_anatomy( anatomy_human_anatomy );
        set_body();
        std::array<int, 6> damage_bandaged;
        data.read( "damage_bandaged", damage_bandaged );
        set_part_damage_bandaged( bodypart_id( "head" ), damage_bandaged[0] );
        set_part_damage_bandaged( bodypart_id( "torso" ), damage_bandaged[1] );
        set_part_damage_bandaged( bodypart_id( "arm_l" ), damage_bandaged[2] );
        set_part_damage_bandaged( bodypart_id( "arm_r" ), damage_bandaged[3] );
        set_part_damage_bandaged( bodypart_id( "leg_l" ), damage_bandaged[4] );
        set_part_damage_bandaged( bodypart_id( "leg_r" ), damage_bandaged[5] );
    }
    if( data.has_array( "damage_disinfected" ) ) {
        set_anatomy( anatomy_human_anatomy );
        set_body();
        std::array<int, 6> damage_disinfected;
        data.read( "damage_disinfected", damage_disinfected );
        set_part_damage_disinfected( bodypart_id( "head" ), damage_disinfected[0] );
        set_part_damage_disinfected( bodypart_id( "torso" ), damage_disinfected[1] );
        set_part_damage_disinfected( bodypart_id( "arm_l" ), damage_disinfected[2] );
        set_part_damage_disinfected( bodypart_id( "arm_r" ), damage_disinfected[3] );
        set_part_damage_disinfected( bodypart_id( "leg_l" ), damage_disinfected[4] );
        set_part_damage_disinfected( bodypart_id( "leg_r" ), damage_disinfected[5] );
    }
    if( data.has_array( "healed_24h" ) ) {
        set_anatomy( anatomy_human_anatomy );
        set_body();
        std::array<int, 6> healed_total;
        data.read( "healed_24h", healed_total );
        set_part_healed_total( bodypart_id( "head" ), healed_total[0] );
        set_part_healed_total( bodypart_id( "torso" ), healed_total[1] );
        set_part_healed_total( bodypart_id( "arm_l" ), healed_total[2] );
        set_part_healed_total( bodypart_id( "arm_r" ), healed_total[3] );
        set_part_healed_total( bodypart_id( "leg_l" ), healed_total[4] );
        set_part_healed_total( bodypart_id( "leg_r" ), healed_total[5] );
    }
    if( data.has_array( "body_wetness" ) ) {
        set_anatomy( anatomy_human_anatomy );
        set_body();
        std::array<int, 12> body_wetness;
        body_wetness.fill( 0 );
        data.read( "body_wetness", body_wetness );
        set_part_wetness( bodypart_id( "torso" ), body_wetness[0] );
        set_part_wetness( bodypart_id( "head" ), body_wetness[1] );
        set_part_wetness( bodypart_id( "eyes" ), body_wetness[2] );
        set_part_wetness( bodypart_id( "mouth" ), body_wetness[3] );
        set_part_wetness( bodypart_id( "arm_l" ), body_wetness[4] );
        set_part_wetness( bodypart_id( "arm_r" ), body_wetness[5] );
        set_part_wetness( bodypart_id( "hand_l" ), body_wetness[6] );
        set_part_wetness( bodypart_id( "hand_r" ), body_wetness[7] );
        set_part_wetness( bodypart_id( "leg_l" ), body_wetness[8] );
        set_part_wetness( bodypart_id( "leg_r" ), body_wetness[9] );
        set_part_wetness( bodypart_id( "foot_l" ), body_wetness[10] );
        set_part_wetness( bodypart_id( "foot_r" ), body_wetness[11] );
    }
    if( data.has_array( "temp_cur" ) ) {
        set_anatomy( anatomy_human_anatomy );
        set_body();
        std::array<int, 12> temp_cur;
        temp_cur.fill( BODYTEMP_NORM );
        data.read( "temp_cur", temp_cur );
        set_part_temp_cur( bodypart_id( "torso" ), temp_cur[0] );
        set_part_temp_cur( bodypart_id( "head" ), temp_cur[1] );
        set_part_temp_cur( bodypart_id( "eyes" ), temp_cur[2] );
        set_part_temp_cur( bodypart_id( "mouth" ), temp_cur[3] );
        set_part_temp_cur( bodypart_id( "arm_l" ), temp_cur[4] );
        set_part_temp_cur( bodypart_id( "arm_r" ), temp_cur[5] );
        set_part_temp_cur( bodypart_id( "hand_l" ), temp_cur[6] );
        set_part_temp_cur( bodypart_id( "hand_r" ), temp_cur[7] );
        set_part_temp_cur( bodypart_id( "leg_l" ), temp_cur[8] );
        set_part_temp_cur( bodypart_id( "leg_r" ), temp_cur[9] );
        set_part_temp_cur( bodypart_id( "foot_l" ), temp_cur[10] );
        set_part_temp_cur( bodypart_id( "foot_r" ), temp_cur[11] );
    }
    if( data.has_array( "temp_conv" ) ) {
        set_anatomy( anatomy_human_anatomy );
        set_body();
        std::array<int, 12> temp_conv;
        temp_conv.fill( BODYTEMP_NORM );
        data.read( "temp_conv", temp_conv );
        set_part_temp_conv( bodypart_id( "torso" ), temp_conv[0] );
        set_part_temp_conv( bodypart_id( "head" ), temp_conv[1] );
        set_part_temp_conv( bodypart_id( "eyes" ), temp_conv[2] );
        set_part_temp_conv( bodypart_id( "mouth" ), temp_conv[3] );
        set_part_temp_conv( bodypart_id( "arm_l" ), temp_conv[4] );
        set_part_temp_conv( bodypart_id( "arm_r" ), temp_conv[5] );
        set_part_temp_conv( bodypart_id( "hand_l" ), temp_conv[6] );
        set_part_temp_conv( bodypart_id( "hand_r" ), temp_conv[7] );
        set_part_temp_conv( bodypart_id( "leg_l" ), temp_conv[8] );
        set_part_temp_conv( bodypart_id( "leg_r" ), temp_conv[9] );
        set_part_temp_conv( bodypart_id( "foot_l" ), temp_conv[10] );
        set_part_temp_conv( bodypart_id( "foot_r" ), temp_conv[11] );
    }
    if( data.has_array( "frostbite_timer" ) ) {
        set_anatomy( anatomy_human_anatomy );
        set_body();
        std::array<int, 12> frostbite_timer;
        frostbite_timer.fill( 0 );
        data.read( "frostbite_timer", frostbite_timer );
        set_part_frostbite_timer( bodypart_id( "torso" ), frostbite_timer[0] );
        set_part_frostbite_timer( bodypart_id( "head" ), frostbite_timer[1] );
        set_part_frostbite_timer( bodypart_id( "eyes" ), frostbite_timer[2] );
        set_part_frostbite_timer( bodypart_id( "mouth" ), frostbite_timer[3] );
        set_part_frostbite_timer( bodypart_id( "arm_l" ), frostbite_timer[4] );
        set_part_frostbite_timer( bodypart_id( "arm_r" ), frostbite_timer[5] );
        set_part_frostbite_timer( bodypart_id( "hand_l" ), frostbite_timer[6] );
        set_part_frostbite_timer( bodypart_id( "hand_r" ), frostbite_timer[7] );
        set_part_frostbite_timer( bodypart_id( "leg_l" ), frostbite_timer[8] );
        set_part_frostbite_timer( bodypart_id( "leg_r" ), frostbite_timer[9] );
        set_part_frostbite_timer( bodypart_id( "foot_l" ), frostbite_timer[10] );
        set_part_frostbite_timer( bodypart_id( "foot_r" ), frostbite_timer[11] );
    }

    inv->clear();
    if( data.has_member( "inv" ) ) {
        inv->json_load_items( data.get_member( "inv" ) );
    }
    // this is after inventory is loaded to make it more obvious that
    // it needs to be changed again when Character::i_at is removed for nested containers
    if( savegame_loading_version < 28 ) {
        activity.migrate_item_position( *this );
        destination_activity.migrate_item_position( *this );
        stashed_outbounds_activity.migrate_item_position( *this );
        stashed_outbounds_backlog.migrate_item_position( *this );
    }

    set_wielded_item( item() );
    data.read( "weapon", weapon );

    data.read( "move_mode", move_mode );

    if( has_effect( effect_riding ) ) {
        int temp_id;
        if( data.read( "mounted_creature", temp_id ) ) {
            mounted_creature_id = temp_id;
            mounted_creature = g->critter_tracker->from_temporary_id( temp_id );
        } else {
            mounted_creature = nullptr;
        }
    }

    morale->load( data );

    _skills->clear();
    JsonObject skill_data = data.get_object( "skills" );
    skill_data.allow_omitted_members();
    for( const JsonMember member : skill_data ) {
        member.read( ( *_skills )[skill_id( member.name() )] );
    }
    if( savegame_loading_version <= 28 ) {
        if( !skill_data.has_member( "chemistry" ) && skill_data.has_member( "cooking" ) ) {
            skill_data.get_member( "cooking" ).read( ( *_skills )[skill_chemistry] );
        }
    }

    on_stat_change( "thirst", thirst );
    on_stat_change( "hunger", hunger );
    on_stat_change( "fatigue", fatigue );
    on_stat_change( "sleep_deprivation", sleep_deprivation );
    on_stat_change( "pkill", pkill );
    on_stat_change( "perceived_pain", get_perceived_pain() );
    recalc_sight_limits();
    calc_encumbrance();

    assign( data, "power_level", power_level, false, 0_kJ );
    assign( data, "max_power_level_modifier", max_power_level_modifier, false, units::energy_min );

    // Bionic power should not be negative!
    if( power_level < 0_mJ ) {
        power_level = 0_mJ;
    }

    JsonArray overmap_time_array = data.get_array( "overmap_time" );
    overmap_time.clear();
    while( overmap_time_array.has_more() ) {
        point_abs_omt pt;
        overmap_time_array.read_next( pt );
        time_duration tdr = 0_turns;
        overmap_time_array.read_next( tdr );
        overmap_time[pt] = tdr;
    }
    data.read( "stomach", stomach );
    data.read( "guts", guts );
    data.read( "automoveroute", auto_move_route );

    known_traps.clear();
    for( JsonObject pmap : data.get_array( "known_traps" ) ) {
        pmap.allow_omitted_members();
        const tripoint p( pmap.get_int( "x" ), pmap.get_int( "y" ), pmap.get_int( "z" ) );
        const std::string t = pmap.get_string( "trap" );
        known_traps.insert( trap_map::value_type( p, t ) );
    }

    // remove after 0.F
    if( savegame_loading_version < 33 ) {
        visit_items( []( item * it, item * ) {
            migrate_item_charges( *it );
            return VisitResponse::NEXT;
        } );
    }

    JsonArray parray;
    character_id tmpid;

    data.read( "slow_rad", slow_rad );
    data.read( "scent", scent );
    data.read( "male", male );
    data.read( "cash", cash );
    data.read( "recoil", recoil );
    data.read( "in_vehicle", in_vehicle );
    data.read( "last_sleep_check", last_sleep_check );
    if( data.read( "id", tmpid ) && tmpid.is_valid() ) {
        // Templates have invalid ids, so we only assign here when valid.
        // When the game starts, a new valid id will be assigned if not already
        // present.
        setID( tmpid );
    }

    data.read( "activity", activity );
    if( data.has_array( "backlog" ) ) {
        data.read( "backlog", backlog );
    }
    if( !backlog.empty() && !backlog.front().str_values.empty() && ( ( activity &&
            activity.id() == ACT_FETCH_REQUIRED ) || ( destination_activity &&
                    destination_activity.id() == ACT_FETCH_REQUIRED ) ) ) {
        requirement_data fetch_reqs;
        data.read( "fetch_data", fetch_reqs );
        const requirement_id req_id( backlog.front().str_values.back() );
        requirement_data::save_requirement( fetch_reqs, req_id );
    }

    data.read( "addictions", addictions );

    for( bionic &bio : *my_bionics ) {
        // Assign UID if missing before applying other migrations
        if( !bio.get_uid() ) {
            bio.set_uid( generate_bionic_uid() );
            // Migrated bionics might not have their initial weapon yet
            if( !bio.has_weapon() && bio.id->fake_weapon.is_valid() ) {
                const item new_weapon = item( bio.id->fake_weapon );
                bio.install_weapon( new_weapon, true );
            }
        }
    }

    item_location weapon = get_wielded_item();
    bool has_old_bionic_weapon = !is_using_bionic_weapon() && weapon &&
                                 ( weapon->has_flag( flag_BIONIC_WEAPON ) || weapon->has_flag( flag_BIONIC_GUN ) ) &&
                                 weapon->has_flag( flag_NO_UNWIELD ) && !weapon->ethereal;

    const auto find_parent = [this]( bionic_id & bio_id ) {
        for( const bionic &bio : *this->my_bionics ) {
            if( std::find( bio.id->included_bionics.begin(), bio.id->included_bionics.end(),
                           bio_id ) != bio.id->included_bionics.end() ) {
                return bio.get_uid();
            }
        }
        return bionic_uid( 0 );
    };

    // Migrations that depend on UIDs
    for( bionic &bio : *my_bionics ) {
        if( has_old_bionic_weapon && bio.powered && bio.has_weapon() &&
            bio.get_weapon().typeId() == get_wielded_item()->typeId() ) {
            weapon_bionic_uid = bio.get_uid();
            has_old_bionic_weapon = false;
        }
        // Assign parent if missing
        if( bio.id->included ) {
            if( !bio.get_parent_uid() ) {
                if( bionic_uid parent_uid = find_parent( bio.id ) ) {
                    bio.set_parent_uid( parent_uid );
                } else {
                    debugmsg( "Migration failed when trying to find a candidate parent bionic for \"%s\"",
                              bio.id.str() );
                }
            }
        } else {
            if( bio.get_parent_uid() ) {
                // Previously integrated CBM is now standalone
                bio.set_parent_uid( 0 );
            }
        }
    }

    if( has_old_bionic_weapon ) {
        debugmsg( "Couldn't find the bionic UID for the current bionic weapon.  You will need to reactivate it." );
        set_wielded_item( item() );
    }

    // Add included bionics that somehow hadn't been added on install
    // or are missing after a save migration
    for( const bionic &bio : *my_bionics ) {
        for( const bionic_id &bid : bio.id->included_bionics ) {
            if( !has_bionic( bid ) ) {
                add_bionic( bid, bio.get_uid() );
            }
        }
    }

    // Add missing pseudoitems
    std::vector<item_location> items = top_items_loc();
    for( const bionic &bio : *my_bionics ) {
        for( const itype_id &b_it : bio.id->passive_pseudo_items ) {
            bool pseudo_found = false;
            for( item_location it : items ) {
                if( it->typeId() == b_it ) {
                    pseudo_found = true;
                    break;
                }
            }
            if( !pseudo_found ) {
                // No pseudoitem was found so add it.
                item pseudo( b_it );

                if( pseudo.has_flag( flag_INTEGRATED ) ) {
                    // Migrate old fuels to new system.
                    // Needed to be compatible with 0.F
                    if( b_it == itype_internal_gasoline_tank && !get_value( "gasoline" ).empty() ) {
                        item gasoline( "gasoline" );
                        gasoline.charges = std::stoi( get_value( "gasoline" ) );
                        remove_value( "gasoline" );
                        pseudo.put_in( gasoline, item_pocket::pocket_type::CONTAINER );
                    } else if( b_it == itype_internal_ethanol_tank && !get_value( "alcohol" ).empty() ) {
                        item ethanol( "chem_ethanol" );
                        ethanol.charges = std::stoi( get_value( "alcohol" ) );
                        remove_value( "alcohol" );
                        pseudo.put_in( ethanol, item_pocket::pocket_type::CONTAINER );
                    } else if( b_it == itype_internal_oil_tank && !get_value( "motor_oil" ).empty() ) {
                        item oil( "motor_oil" );
                        oil.charges = std::stoi( get_value( "motor_oil" ) );
                        remove_value( "motor_oil" );
                        pseudo.put_in( oil, item_pocket::pocket_type::CONTAINER );
                    } else if( b_it == itype_internal_battery_compartment && !get_value( "battery" ).empty() ) {
                        item battery( "medium_battery_cell" );
                        item battery_charge( "battery" );
                        battery_charge.charges = std::min( 500, std::stoi( get_value( "battery" ) ) );
                        battery.put_in( battery_charge, item_pocket::pocket_type::MAGAZINE );
                        remove_value( "battery" );
                        pseudo.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );
                    }

                    wear_item( pseudo, false );
                }
            }
        }
    }

    // Fixes bugged characters for CBM's preventing mutations.
    for( const bionic_id &bid : get_bionics() ) {
        for( const trait_id &mid : bid->canceled_mutations ) {
            if( has_trait( mid ) ) {
                remove_mutation( mid );
            }
        }
    }

    if( data.has_array( "faction_warnings" ) ) {
        for( JsonObject warning_data : data.get_array( "faction_warnings" ) ) {
            warning_data.allow_omitted_members();
            std::string fac_id = warning_data.get_string( "fac_warning_id" );
            int warning_num = warning_data.get_int( "fac_warning_num" );
            time_point warning_time = calendar::before_time_starts;
            warning_data.read( "fac_warning_time", warning_time );
            warning_record[faction_id( fac_id )] = std::make_pair( warning_num, warning_time );
        }
    }

    int tmptar;
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
        last_target = overmap_buffer.find_npc( character_id( tmptar ) );
    } else if( tmptartyp == -1 ) {
        // Need to do this *after* the monsters have been loaded!
        last_target = g->critter_tracker->from_temporary_id( tmptar );
    }
    data.read( "destination_point", destination_point );
    camps.clear();
    for( JsonObject bcdata : data.get_array( "camps" ) ) {
        bcdata.allow_omitted_members();
        tripoint_abs_omt bcpt;
        bcdata.read( "pos", bcpt );
        camps.insert( bcpt );
    }
    //load queued_eocs
    for( JsonObject elem : data.get_array( "queued_effect_on_conditions" ) ) {
        queued_eoc temp;
        temp.time = time_point( elem.get_int( "time" ) );
        temp.eoc = effect_on_condition_id( elem.get_string( "eoc" ) );
        queued_effect_on_conditions.push( temp );
    }
    data.read( "inactive_eocs", inactive_effect_on_condition_vector );
    update_enchantment_mutations();
}

/**
 * Load variables from json into object. These variables are common to both the avatar and NPCs.
 */
void Character::store( JsonOut &json ) const
{
    Creature::store( json );

    if( !weapon.is_null() ) {
        json.member( "weapon", weapon ); // also saves contents
    }

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

    json.member( "name", name );
    json.member( "play_name", play_name );

    json.member( "base_age", init_age );
    json.member( "base_height", init_height );
    json.member_as_string( "blood_type", my_blood_type );
    json.member( "blood_rh_factor", blood_rh_factor );
    json.member( "avg_nat_bpm", avg_nat_bpm );

    json.member( "custom_profession", custom_profession );

    // health
    json.member( "healthy", lifestyle );
    json.member( "healthy_mod", daily_health );
    json.member( "health_tally", health_tally );

    //sleep
    json.member( "daily_sleep", daily_sleep );
    json.member( "continuous_sleep", continuous_sleep );

    // needs
    json.member( "thirst", thirst );
    json.member( "hunger", hunger );
    json.member( "fatigue", fatigue );
    json.member( "cardio_acc", cardio_acc );
    json.member( "activity_history", activity_history );
    json.member( "sleep_deprivation", sleep_deprivation );
    json.member( "stored_calories", stored_calories );
    json.member( "radiation", radiation );
    json.member( "stamina", stamina );
    json.member( "vitamin_levels", vitamin_levels );
    json.member( "daily_vitamins", daily_vitamins );
    json.member( "pkill", pkill );
    json.member( "omt_path", omt_path );
    json.member( "consumption_history", consumption_history );

    // crafting etc
    json.member( "destination_activity", destination_activity );
    json.member( "activity", activity );
    json.member( "stashed_outbounds_activity", stashed_outbounds_activity );
    json.member( "stashed_outbounds_backlog", stashed_outbounds_backlog );
    json.member( "backlog", backlog );
    json.member( "activity_vehicle_part_index", activity_vehicle_part_index ); // NPC activity

    // handling for storing activity requirements
    if( !backlog.empty() && !backlog.front().str_values.empty() && ( ( activity &&
            activity.id() == ACT_FETCH_REQUIRED ) || ( destination_activity &&
                    destination_activity.id() == ACT_FETCH_REQUIRED ) ) ) {
        requirement_data things_to_fetch = requirement_id( backlog.front().str_values.back() ).obj();
        json.member( "fetch_data", things_to_fetch );
    }

    json.member( "stim", stim );
    json.member( "type_of_scent", type_of_scent );
    json.member( "focus_pool", focus_pool );

    // stats through kills
    json.member( "kill_xp", kill_xp );
    json.member( "spent_upgrade_points", spent_upgrade_points );

    // breathing
    json.member( "underwater", underwater );
    json.member( "oxygen", oxygen );

    // traits: permanent 'mutations' more or less
    json.member( "traits", my_traits );
    json.member( "mutations", my_mutations );
    json.member( "moncams", moncams );
    json.member( "magic", magic );
    json.member( "martial_arts_data", martial_arts_data );
    // "Fracking Toasters" - Saul Tigh, toaster
    json.member( "my_bionics", *my_bionics );

    json.member_as_string( "move_mode",  move_mode );

    // monsters recorded by the character
    json.member( "known_monsters", known_monsters );

    // storing the mount
    if( is_mounted() ) {
        json.member( "mounted_creature", g->critter_tracker->temporary_id( *mounted_creature ) );
    }

    morale->store( json );

    // skills
    json.member( "skills" );
    json.start_object();
    for( const auto &pair : *_skills ) {
        json.member( pair.first.str(), pair.second );
    }
    json.end_object();

    json.member( "proficiencies", _proficiencies );

    // npc; unimplemented
    if( power_level < 1_J ) {
        json.member( "power_level", std::to_string( units::to_millijoule( power_level ) ) + " mJ" );
    } else if( power_level < 1_kJ ) {
        json.member( "power_level", std::to_string( units::to_joule( power_level ) ) + " J" );
    } else {
        json.member( "power_level", units::to_kilojoule( power_level ) );
    }
    json.member( "max_power_level_modifier", units::to_kilojoule( max_power_level_modifier ) );

    if( !overmap_time.empty() ) {
        json.member( "overmap_time" );
        json.start_array();
        for( const std::pair<const point_abs_omt, time_duration> &pr : overmap_time ) {
            json.write( pr.first );
            json.write( pr.second );
        }
        json.end_array();
    }
    json.member( "stomach", stomach );
    json.member( "guts", guts );
    json.member( "automoveroute", auto_move_route );
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

    // energy
    json.member( "last_sleep_check", last_sleep_check );
    // misc levels
    json.member( "slow_rad", slow_rad );
    json.member( "scent", static_cast<int>( scent ) );

    // gender
    json.member( "male", male );

    json.member( "cash", cash );
    json.member( "recoil", recoil );
    json.member( "in_vehicle", in_vehicle );
    json.member( "id", getID() );

    // "Looks like I picked the wrong week to quit smoking." - Steve McCroskey
    json.member( "addictions", addictions );
    json.member( "death_eocs", death_eocs );
    json.member( "worn", worn ); // also saves contents
    json.member( "inv" );
    inv->json_save_items( json );

    if( const auto lt_ptr = last_target.lock() ) {
        if( const npc *const guy = dynamic_cast<const npc *>( lt_ptr.get() ) ) {
            json.member( "last_target", guy->getID() );
            json.member( "last_target_type", +1 );
        } else if( const monster *const mon = dynamic_cast<const monster *>( lt_ptr.get() ) ) {
            // monsters don't have IDs, so get its index in the creature_tracker instead
            json.member( "last_target", g->critter_tracker->temporary_id( *mon ) );
            json.member( "last_target_type", -1 );
        }
    } else {
        json.member( "last_target_pos", last_target_pos );
    }

    json.member( "destination_point", destination_point );

    // faction warnings
    json.member( "faction_warnings" );
    json.start_array();
    for( const auto &elem : warning_record ) {
        json.start_object();
        json.member( "fac_warning_id", elem.first );
        json.member( "fac_warning_num", elem.second.first );
        json.member( "fac_warning_time", elem.second.second );
        json.end_object();
    }
    json.end_array();

    if( ammo_location ) {
        json.member( "ammo_location", ammo_location );
    }

    json.member( "camps" );
    json.start_array();
    for( const tripoint_abs_omt &bcpt : camps ) {
        json.start_object();
        json.member( "pos", bcpt );
        json.end_object();
    }
    json.end_array();

    //save queued effect_on_conditions
    std::priority_queue<queued_eoc, std::vector<queued_eoc>, eoc_compare> temp_queued(
        queued_effect_on_conditions );
    json.member( "queued_effect_on_conditions" );
    json.start_array();
    while( !temp_queued.empty() ) {
        json.start_object();
        json.member( "time", temp_queued.top().time );
        json.member( "eoc", temp_queued.top().eoc );
        json.end_object();
        temp_queued.pop();
    }

    json.end_array();
    json.member( "inactive_eocs", inactive_effect_on_condition_vector );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// avatar.h

void avatar::serialize( JsonOut &json ) const
{
    json.start_object();

    store( json );

    json.end_object();
}

void avatar::store( JsonOut &json ) const
{
    Character::store( json );

    // player-specific specifics
    if( prof != nullptr ) {
        json.member( "profession", prof->ident() );
    }
    if( get_scenario() != nullptr ) {
        json.member( "scenario", get_scenario()->ident() );
    }
    if( !hobbies.empty() ) {
        json.member( "hobbies" );
        json.start_array();
        for( const profession *hobby : hobbies ) {
            json.write( hobby->ident() );
        }
        json.end_array();
    }

    json.member( "followers", follower_ids );
    if( shadow_npc ) {
        json.member( "shadow_npc", *shadow_npc );
    }
    // someday, npcs may drive
    json.member( "controlling_vehicle", controlling_vehicle );

    // shopping carts, furniture etc
    json.member( "grab_point", grab_point );
    json.member( "grab_type", obj_type_name[static_cast<int>( grab_type ) ] );

    // npc: unimplemented, potentially useful
    json.member( "learned_recipes", *learned_recipes );

    // Player only, books they have read at least once.
    json.member( "items_identified", items_identified );

    // Player only, snippets they have read at least once.
    json.member( "snippets_read", snippets_read );

    json.member( "translocators", translocators );

    // mission stuff
    json.member( "active_mission", active_mission == nullptr ? -1 : active_mission->get_id() );

    json.member( "active_missions", mission::to_uid_vector( active_missions ) );
    json.member( "completed_missions", mission::to_uid_vector( completed_missions ) );
    json.member( "failed_missions", mission::to_uid_vector( failed_missions ) );

    json.member( "show_map_memory", show_map_memory );

    json.member( "assigned_invlet" );
    json.start_array();
    for( const auto &iter : inv->assigned_invlet ) {
        json.start_array();
        json.write( iter.first );
        json.write( iter.second );
        json.end_array();
    }
    json.end_array();

    json.member( "invcache" );
    inv->json_save_invcache( json );

    json.member( "calorie_diary", calorie_diary );

    json.member( "preferred_aiming_mode", preferred_aiming_mode );

    json.member( "power_prev_turn", power_prev_turn );
}

void avatar::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    load( data );
}

void avatar::load( const JsonObject &data )
{
    Character::load( data );

    // TEMPORARY until 0.G
    if( !data.has_member( "location" ) ) {
        set_location( get_map().getglobal( read_legacy_creature_pos( data ) ) );
    }

    // TEMPORARY until 0.G
    if( !data.has_member( "kill_xp" ) ) {
        kill_xp = g->get_kill_tracker().legacy_kill_xp();
    }

    // Remove after 0.F
    // Exists to prevent failed to visit member errors
    if( data.has_member( "reactor_plut" ) ) {
        data.get_int( "reactor_plut" );
    }
    if( data.has_member( "tank_plut" ) ) {
        data.get_int( "tank_plut" );
    }

    std::string prof_ident = "(null)";
    if( data.read( "profession", prof_ident ) && string_id<profession>( prof_ident ).is_valid() ) {
        prof = &string_id<profession>( prof_ident ).obj();
    } else {
        //We are likely an older profession which has since been removed so just set to default.  This is only cosmetic after game start.
        prof = profession::generic();
    }

    std::vector<string_id<profession>> hobby_ids;
    data.read( "hobbies", hobby_ids );
    for( const profession_id &hobby : hobby_ids ) {
        hobbies.insert( hobbies.end(), &hobby.obj() );
    }

    data.read( "followers", follower_ids );
    if( data.has_member( "shadow_npc" ) ) {
        shadow_npc = std::make_unique<npc>();
        data.read( "shadow_npc", *shadow_npc );
    }
    data.read( "controlling_vehicle", controlling_vehicle );

    data.read( "grab_point", grab_point );
    std::string grab_typestr = "OBJECT_NONE";
    if( grab_point.x != 0 || grab_point.y != 0 ) {
        grab_typestr = "OBJECT_VEHICLE";
        data.read( "grab_type", grab_typestr );
    } else {
        // we just want to read, but ignore grab_type
        std::string fake;
        data.read( "grab_type", fake );
    }

    // On Visual Studio array iterators are not pointers, so prevent clang-tidy
    // from converting this to a pointer type.
    // NOLINTNEXTLINE(readability-qualified-auto)
    const auto iter = std::find( obj_type_name.begin(), obj_type_name.end(), grab_typestr );
    grab( iter == obj_type_name.end() ?
          object_type::NONE : static_cast<object_type>(
              std::distance( obj_type_name.begin(), iter ) ),
          grab_point );

    data.read( "magic", magic );

    calc_mutation_levels();
    drench_mut_calc();
    std::string scen_ident = "(null)";
    if( data.read( "scenario", scen_ident ) && string_id<scenario>( scen_ident ).is_valid() ) {
        set_scenario( &string_id<scenario>( scen_ident ).obj() );

        if( !get_scenario()->allowed_start( start_location ) ) {
            start_location = get_scenario()->random_start_location();
        }
    } else {
        const scenario *generic_scenario = scenario::generic();

        debugmsg( "Tried to use non-existent scenario '%s'. Setting to generic '%s'.",
                  scen_ident.c_str(), generic_scenario->ident().c_str() );
        set_scenario( generic_scenario );
    }

    data.read( "learned_recipes", *learned_recipes );
    valid_autolearn_skills->clear(); // Invalidates the cache

    items_identified.clear();
    data.read( "items_identified", items_identified );

    data.read( "translocators", translocators );

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
        if( tmpactive_mission != -1 ) {
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

    data.read( "show_map_memory", show_map_memory );

    for( JsonArray pair : data.get_array( "assigned_invlet" ) ) {
        inv->assigned_invlet[static_cast<char>( pair.get_int( 0 ) )] =
            itype_id( pair.get_string( 1 ) );
    }

    if( data.has_member( "invcache" ) ) {
        inv->json_load_invcache( data.get_member( "invcache" ) );
    }

    data.read( "calorie_diary", calorie_diary );

    data.read( "preferred_aiming_mode", preferred_aiming_mode );

    if( data.has_member( "power_prev_turn" ) ) {
        data.read( "power_prev_turn", power_prev_turn );
    }

    data.read( "snippets_read", snippets_read );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// npc.h

void npc_follower_rules::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "engagement", static_cast<int>( engagement ) );
    json.member( "aim", static_cast<int>( aim ) );
    json.member( "cbm_reserve", static_cast<int>( cbm_reserve ) );
    json.member( "cbm_recharge", static_cast<int>( cbm_recharge ) );

    // serialize the flags so they can be changed between save games
    for( const auto &rule : ally_rule_strs ) {
        json.member( "rule_" + rule.first, has_flag( rule.second.rule, false ) );
    }
    for( const auto &rule : ally_rule_strs ) {
        json.member( "override_enable_" + rule.first, has_override_enable( rule.second.rule ) );
    }
    for( const auto &rule : ally_rule_strs ) {
        json.member( "override_" + rule.first, has_override( rule.second.rule ) );
    }

    json.member( "pickup_whitelist", *pickup_whitelist );

    json.end_object();
}

void npc_follower_rules::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    int tmpeng = 0;
    data.read( "engagement", tmpeng );
    engagement = static_cast<combat_engagement>( tmpeng );
    int tmpaim = 0;
    data.read( "aim", tmpaim );
    aim = static_cast<aim_rule>( tmpaim );
    int tmpreserve = 50;
    data.read( "cbm_reserve", tmpreserve );
    cbm_reserve = static_cast<cbm_reserve_rule>( tmpreserve );
    int tmprecharge = 50;
    data.read( "cbm_recharge", tmprecharge );
    cbm_recharge = static_cast<cbm_recharge_rule>( tmprecharge );

    // deserialize the flags so they can be changed between save games
    for( const auto &rule : ally_rule_strs ) {
        bool tmpflag = false;
        // legacy to handle rules that were saved before overrides
        data.read( rule.first, tmpflag );
        if( tmpflag ) {
            set_flag( rule.second.rule );
        } else {
            clear_flag( rule.second.rule );
        }
        data.read( "rule_" + rule.first, tmpflag );
        if( tmpflag ) {
            set_flag( rule.second.rule );
        } else {
            clear_flag( rule.second.rule );
        }
        data.read( "override_enable_" + rule.first, tmpflag );
        if( tmpflag ) {
            enable_override( rule.second.rule );
        } else {
            disable_override( rule.second.rule );
        }
        data.read( "override_" + rule.first, tmpflag );
        if( tmpflag ) {
            set_override( rule.second.rule );
        } else {
            clear_override( rule.second.rule );
        }

        // This and the following two entries are for legacy save game handling.
        // "avoid_combat" was renamed "follow_close" to better reflect behavior.
        if( data.has_member( "rule_avoid_combat" ) ) {
            data.read( "rule_avoid_combat", tmpflag );
            if( tmpflag ) {
                set_flag( ally_rule::follow_close );
            } else {
                clear_flag( ally_rule::follow_close );
            }
        }
        if( data.has_member( "override_enable_avoid_combat" ) ) {
            data.read( "override_enable_avoid_combat", tmpflag );
            if( tmpflag ) {
                enable_override( ally_rule::follow_close );
            } else {
                disable_override( ally_rule::follow_close );
            }
        }
        if( data.has_member( "override_avoid_combat" ) ) {
            data.read( "override_avoid_combat", tmpflag );
            if( tmpflag ) {
                set_override( ally_rule::follow_close );
            } else {
                clear_override( ally_rule::follow_close );
            }
        }
    }

    data.read( "pickup_whitelist", *pickup_whitelist );
}

void dialogue_chatbin::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "first_topic", first_topic );
    json.member( "talk_radio", talk_radio );
    json.member( "talk_leader", talk_leader );
    json.member( "talk_friend", talk_friend );
    json.member( "talk_stole_item", talk_stole_item );
    json.member( "talk_wake_up", talk_wake_up );
    json.member( "talk_mug", talk_mug );
    json.member( "talk_stranger_aggressive", talk_stranger_aggressive );
    json.member( "talk_stranger_scared", talk_stranger_scared );
    json.member( "talk_stranger_wary", talk_stranger_wary );
    json.member( "talk_stranger_friendly", talk_stranger_friendly );
    json.member( "talk_stranger_neutral", talk_stranger_neutral );
    json.member( "talk_friend_guard", talk_friend_guard );
    json.member( "snip_acknowledged", snip_acknowledged );
    json.member( "snip_camp_food_thanks", snip_camp_food_thanks );
    json.member( "snip_camp_larder_empty", snip_camp_larder_empty );
    json.member( "snip_camp_water_thanks", snip_camp_water_thanks );
    json.member( "snip_cant_flee", snip_cant_flee );
    json.member( "snip_close_distance", snip_close_distance );
    json.member( "snip_combat_noise_warning", snip_combat_noise_warning );
    json.member( "snip_danger_close_distance", snip_danger_close_distance );
    json.member( "snip_done_mugging", snip_done_mugging );
    json.member( "snip_far_distance", snip_far_distance );
    json.member( "snip_fire_bad", snip_fire_bad );
    json.member( "snip_fire_in_the_hole_h", snip_fire_in_the_hole_h );
    json.member( "snip_fire_in_the_hole", snip_fire_in_the_hole );
    json.member( "snip_general_danger_h", snip_general_danger_h );
    json.member( "snip_general_danger", snip_general_danger );
    json.member( "snip_heal_self", snip_heal_self );
    json.member( "snip_hungry", snip_hungry );
    json.member( "snip_im_leaving_you", snip_im_leaving_you );
    json.member( "snip_its_safe_h", snip_its_safe_h );
    json.member( "snip_its_safe", snip_its_safe );
    json.member( "snip_keep_up", snip_keep_up );
    json.member( "snip_kill_npc_h", snip_kill_npc_h );
    json.member( "snip_kill_npc", snip_kill_npc );
    json.member( "snip_kill_player_h", snip_kill_player_h );
    json.member( "snip_let_me_pass", snip_let_me_pass );
    json.member( "snip_lets_talk", snip_lets_talk );
    json.member( "snip_medium_distance", snip_medium_distance );
    json.member( "snip_monster_warning_h", snip_monster_warning_h );
    json.member( "snip_monster_warning", snip_monster_warning );
    json.member( "snip_movement_noise_warning", snip_movement_noise_warning );
    json.member( "snip_need_batteries", snip_need_batteries );
    json.member( "snip_need_booze", snip_need_booze );
    json.member( "snip_need_fuel", snip_need_fuel );
    json.member( "snip_no_to_thorazine", snip_no_to_thorazine );
    json.member( "snip_run_away", snip_run_away );
    json.member( "snip_speech_warning", snip_speech_warning );
    json.member( "snip_thirsty", snip_thirsty );
    json.member( "snip_wait", snip_wait );
    json.member( "snip_warn_sleep", snip_warn_sleep );
    json.member( "snip_yawn", snip_yawn );
    json.member( "snip_yes_to_lsd", snip_yes_to_lsd );
    json.member( "snip_pulp_zombie", snip_pulp_zombie );
    json.member( "snip_heal_player", snip_heal_player );
    json.member( "snip_mug_dontmove", snip_mug_dontmove );
    json.member( "snip_wound_infected", snip_wound_infected );
    json.member( "snip_wound_bite", snip_wound_bite );
    json.member( "snip_radiation_sickness", snip_radiation_sickness );
    json.member( "snip_bleeding", snip_bleeding );
    json.member( "snip_bleeding_badly", snip_bleeding_badly );
    json.member( "snip_lost_blood", snip_lost_blood );
    json.member( "snip_bye", snip_bye );
    json.member( "snip_consume_cant_accept", snip_consume_cant_accept );
    json.member( "snip_consume_cant_consume", snip_consume_cant_consume );
    json.member( "snip_consume_rotten", snip_consume_rotten );
    json.member( "snip_consume_eat", snip_consume_eat );
    json.member( "snip_consume_need_item", snip_consume_need_item );
    json.member( "snip_consume_med", snip_consume_med );
    json.member( "snip_consume_nocharge", snip_consume_nocharge );
    json.member( "snip_consume_use_med", snip_consume_use_med );
    json.member( "snip_give_nope", snip_give_nope );
    json.member( "snip_give_to_hallucination", snip_give_to_hallucination );
    json.member( "snip_give_cancel", snip_give_cancel );
    json.member( "snip_give_dangerous", snip_give_dangerous );
    json.member( "snip_give_wield", snip_give_wield );
    json.member( "snip_give_weapon_weak", snip_give_weapon_weak );
    json.member( "snip_give_carry", snip_give_carry );
    json.member( "snip_give_carry_cant", snip_give_carry_cant );
    json.member( "snip_give_carry_cant_few_space", snip_give_carry_cant_few_space );
    json.member( "snip_give_carry_cant_no_space", snip_give_carry_cant_no_space );
    json.member( "snip_give_carry_too_heavy", snip_give_carry_too_heavy );
    json.member( "snip_wear", snip_wear );

    if( mission_selected != nullptr ) {
        json.member( "mission_selected", mission_selected->get_id() );
    }
    json.member( "skill", skill );
    json.member( "style", style );
    json.member( "dialogue_spell", dialogue_spell );
    json.member( "proficiency", proficiency );
    json.member( "missions", mission::to_uid_vector( missions ) );
    json.member( "missions_assigned", mission::to_uid_vector( missions_assigned ) );
    json.end_object();
}

void dialogue_chatbin::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();

    if( data.has_int( "first_topic" ) ) {
        int tmptopic = 0;
        data.read( "first_topic", tmptopic );
        first_topic = convert_talk_topic( static_cast<talk_topic_enum>( tmptopic ) );
    } else {
        data.read( "first_topic", first_topic );
    }
    data.read( "talk_radio", talk_radio );
    data.read( "talk_leader", talk_leader );
    data.read( "talk_friend", talk_friend );
    data.read( "talk_stole_item", talk_stole_item );
    data.read( "talk_wake_up", talk_wake_up );
    data.read( "talk_mug", talk_mug );
    data.read( "talk_stranger_aggressive", talk_stranger_aggressive );
    data.read( "talk_stranger_scared", talk_stranger_scared );
    data.read( "talk_stranger_wary", talk_stranger_wary );
    data.read( "talk_stranger_friendly", talk_stranger_friendly );
    data.read( "talk_stranger_neutral", talk_stranger_neutral );
    data.read( "talk_friend_guard", talk_friend_guard );
    data.read( "skill", skill );
    data.read( "style", style );
    data.read( "dialogue_spell", dialogue_spell );
    data.read( "proficiency", proficiency );
    data.read( "snip_acknowledged", snip_acknowledged );
    data.read( "snip_camp_food_thanks", snip_camp_food_thanks );
    data.read( "snip_camp_larder_empty", snip_camp_larder_empty );
    data.read( "snip_camp_water_thanks", snip_camp_water_thanks );
    data.read( "snip_cant_flee", snip_cant_flee );
    data.read( "snip_close_distance", snip_close_distance );
    data.read( "snip_combat_noise_warning", snip_combat_noise_warning );
    data.read( "snip_danger_close_distance", snip_danger_close_distance );
    data.read( "snip_done_mugging", snip_done_mugging );
    data.read( "snip_far_distance", snip_far_distance );
    data.read( "snip_fire_bad", snip_fire_bad );
    data.read( "snip_fire_in_the_hole_h", snip_fire_in_the_hole_h );
    data.read( "snip_fire_in_the_hole", snip_fire_in_the_hole );
    data.read( "snip_general_danger_h", snip_general_danger_h );
    data.read( "snip_general_danger", snip_general_danger );
    data.read( "snip_heal_self", snip_heal_self );
    data.read( "snip_hungry", snip_hungry );
    data.read( "snip_im_leaving_you", snip_im_leaving_you );
    data.read( "snip_its_safe_h", snip_its_safe_h );
    data.read( "snip_its_safe", snip_its_safe );
    data.read( "snip_keep_up", snip_keep_up );
    data.read( "snip_kill_npc_h", snip_kill_npc_h );
    data.read( "snip_kill_npc", snip_kill_npc );
    data.read( "snip_kill_player_h", snip_kill_player_h );
    data.read( "snip_let_me_pass", snip_let_me_pass );
    data.read( "snip_lets_talk", snip_lets_talk );
    data.read( "snip_medium_distance", snip_medium_distance );
    data.read( "snip_monster_warning_h", snip_monster_warning_h );
    data.read( "snip_monster_warning", snip_monster_warning );
    data.read( "snip_movement_noise_warning", snip_movement_noise_warning );
    data.read( "snip_need_batteries", snip_need_batteries );
    data.read( "snip_need_booze", snip_need_booze );
    data.read( "snip_need_fuel", snip_need_fuel );
    data.read( "snip_no_to_thorazine", snip_no_to_thorazine );
    data.read( "snip_run_away", snip_run_away );
    data.read( "snip_speech_warning", snip_speech_warning );
    data.read( "snip_thirsty", snip_thirsty );
    data.read( "snip_wait", snip_wait );
    data.read( "snip_warn_sleep", snip_warn_sleep );
    data.read( "snip_yawn", snip_yawn );
    data.read( "snip_yes_to_lsd", snip_yes_to_lsd );
    data.read( "snip_pulp_zombie", snip_pulp_zombie );
    data.read( "snip_heal_player", snip_heal_player );
    data.read( "snip_mug_dontmove", snip_mug_dontmove );
    data.read( "snip_wound_infected", snip_wound_infected );
    data.read( "snip_wound_bite", snip_wound_bite );
    data.read( "snip_radiation_sickness", snip_radiation_sickness );
    data.read( "snip_bleeding", snip_bleeding );
    data.read( "snip_bleeding_badly", snip_bleeding_badly );
    data.read( "snip_lost_blood", snip_lost_blood );
    data.read( "snip_bye", snip_bye );
    data.read( "snip_consume_cant_accept", snip_consume_cant_accept );
    data.read( "snip_consume_cant_consume", snip_consume_cant_consume );
    data.read( "snip_consume_rotten", snip_consume_rotten );
    data.read( "snip_consume_eat", snip_consume_eat );
    data.read( "snip_consume_need_item", snip_consume_need_item );
    data.read( "snip_consume_med", snip_consume_med );
    data.read( "snip_consume_nocharge", snip_consume_nocharge );
    data.read( "snip_consume_use_med", snip_consume_use_med );
    data.read( "snip_give_nope", snip_give_nope );
    data.read( "snip_give_to_hallucination", snip_give_to_hallucination );
    data.read( "snip_give_cancel", snip_give_cancel );
    data.read( "snip_give_dangerous", snip_give_dangerous );
    data.read( "snip_give_wield", snip_give_wield );
    data.read( "snip_give_weapon_weak", snip_give_weapon_weak );
    data.read( "snip_give_carry", snip_give_carry );
    data.read( "snip_give_carry_cant", snip_give_carry_cant );
    data.read( "snip_give_carry_cant_few_space", snip_give_carry_cant_few_space );
    data.read( "snip_give_carry_cant_no_space", snip_give_carry_cant_no_space );
    data.read( "snip_give_carry_too_heavy", snip_give_carry_too_heavy );
    data.read( "snip_wear", snip_wear );

    std::vector<int> tmpmissions;
    data.read( "missions", tmpmissions );
    missions = mission::to_ptr_vector( tmpmissions );
    std::vector<int> tmpmissions_assigned;
    data.read( "missions_assigned", tmpmissions_assigned );
    missions_assigned = mission::to_ptr_vector( tmpmissions_assigned );

    int tmpmission_selected = 0;
    mission_selected = nullptr;
    if( data.read( "mission_selected", tmpmission_selected ) && tmpmission_selected != -1 ) {
        mission_selected = mission::find( tmpmission_selected );
    }
}

void npc_personality::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
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

void npc_opinion::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "trust", trust );
    data.read( "fear", fear );
    data.read( "value", value );
    data.read( "anger", anger );
    data.read( "owed", owed );
    data.read( "sold", sold );
}

void npc_opinion::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "trust", trust );
    json.member( "fear", fear );
    json.member( "value", value );
    json.member( "anger", anger );
    json.member( "owed", owed );
    json.member( "sold", sold );
    json.end_object();
}

void npc_favor::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();
    type = static_cast<npc_favor_type>( jo.get_int( "type" ) );
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

void job_data::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "task_priorities", task_priorities );
    json.end_object();
}

void job_data::deserialize( const JsonValue &jv )
{
    if( jv.test_object() ) {
        JsonObject jo = jv;
        jo.allow_omitted_members();
        jo.read( "task_priorities", task_priorities );
    }
}

/*
 * load npc
 */
void npc::deserialize( const JsonObject &data )
{
    load( data );
}

void npc::load( const JsonObject &data )
{
    Character::load( data );

    // TEMPORARY Remove if branch after 0.G (keep else branch)
    if( !data.has_member( "location" ) ) {
        point submap_coords;
        data.read( "submap_coords", submap_coords );
        const tripoint pos = read_legacy_creature_pos( data );
        set_location( tripoint_abs_ms( project_to<coords::ms>( point_abs_sm( submap_coords ) ),
                                       0 ) + tripoint( pos.x % SEEX, pos.y % SEEY, pos.z ) );
        cata::optional<tripoint> opt;
        if( data.read( "last_player_seen_pos", opt ) && opt ) {
            last_player_seen_pos = get_location() + *opt - pos;
        }
        if( data.read( "pulp_location", opt ) && opt ) {
            pulp_location = get_location() + *opt - pos;
        }
        tripoint tmp;
        if( data.read( "guardx", tmp.x ) && data.read( "guardy", tmp.y ) && data.read( "guardz", tmp.z ) &&
            tmp != tripoint_min ) {
            guard_pos = tripoint_abs_ms( tmp );
        }
        if( data.read( "chair_pos", tmp ) && tmp != tripoint_min ) {
            chair_pos = tripoint_abs_ms( tmp );
        }
        if( data.read( "wander_pos", tmp ) && tmp != tripoint_min ) {
            wander_pos = tripoint_abs_ms( tmp );
        }
    } else {
        data.read( "last_player_seen_pos", last_player_seen_pos );
        data.read( "guard_pos", guard_pos );
        data.read( "pulp_location", pulp_location );
        data.read( "chair_pos", chair_pos );
        data.read( "wander_pos", wander_pos );
    }

    int misstmp = 0;
    int classtmp = 0;
    int atttmp = 0;
    std::string facID;
    std::string comp_miss_id;
    std::string comp_miss_role;
    tripoint_abs_omt comp_miss_pt;
    std::string classid;
    std::string companion_mission_role;
    time_point companion_mission_t = calendar::turn_zero;
    time_point companion_mission_t_r = calendar::turn_zero;
    std::string act_id;

    // Remove after 0.F
    // Exists to prevent failed to visit member errors
    if( data.has_member( "reactor_plut" ) ) {
        data.get_int( "reactor_plut" );
    }
    if( data.has_member( "tank_plut" ) ) {
        data.get_int( "tank_plut" );
    }

    data.read( "marked_for_death", marked_for_death );
    data.read( "dead", dead );
    data.read( "patience", patience );
    if( data.has_number( "myclass" ) ) {
        data.read( "myclass", classtmp );
        myclass = npc_class::from_legacy_int( classtmp );
    } else if( data.has_string( "myclass" ) ) {
        data.read( "myclass", classid );
        myclass = npc_class_id( classid );
    }
    if( data.has_string( "idz" ) ) {
        data.read( "idz", idz );
    } else {
        idz = myclass;
    }
    data.read( "known_to_u", known_to_u );
    data.read( "personality", personality );

    data.read( "goalx", goal.x() );
    data.read( "goaly", goal.y() );
    data.read( "goalz", goal.z() );

    if( data.read( "current_activity_id", act_id ) ) {
        current_activity_id = activity_id( act_id );
    } else if( activity ) {
        current_activity_id = activity.id();
    }

    data.read( "assigned_camp", assigned_camp );
    data.read( "job", job );
    if( data.read( "mission", misstmp ) ) {
        mission = static_cast<npc_mission>( misstmp );
        static const std::set<npc_mission> legacy_missions = {{
                NPC_MISSION_LEGACY_1, NPC_MISSION_LEGACY_2,
                NPC_MISSION_LEGACY_3
            }
        };
        if( legacy_missions.count( mission ) > 0 ) {
            mission = NPC_MISSION_NULL;
        }
    }
    if( data.read( "previous_mission", misstmp ) ) {
        previous_mission = static_cast<npc_mission>( misstmp );
        static const std::set<npc_mission> legacy_missions = {{
                NPC_MISSION_LEGACY_1, NPC_MISSION_LEGACY_2,
                NPC_MISSION_LEGACY_3
            }
        };
        if( legacy_missions.count( mission ) > 0 ) {
            previous_mission = NPC_MISSION_NULL;
        }
    }

    if( data.read( "my_fac", facID ) ) {
        fac_id = faction_id( facID );
    }
    int temp_fac_api_ver = 0;
    if( data.read( "faction_api_ver", temp_fac_api_ver ) ) {
        faction_api_version = temp_fac_api_ver;
    } else {
        faction_api_version = 0;
    }

    if( data.read( "attitude", atttmp ) ) {
        attitude = static_cast<npc_attitude>( atttmp );
        static const std::set<npc_attitude> legacy_attitudes = {{
                NPCATT_LEGACY_1, NPCATT_LEGACY_2, NPCATT_LEGACY_3,
                NPCATT_LEGACY_4, NPCATT_LEGACY_5, NPCATT_LEGACY_6
            }
        };
        if( legacy_attitudes.count( attitude ) > 0 ) {
            attitude = NPCATT_NULL;
        }
    }
    if( data.read( "previous_attitude", atttmp ) ) {
        previous_attitude = static_cast<npc_attitude>( atttmp );
        static const std::set<npc_attitude> legacy_attitudes = {{
                NPCATT_LEGACY_1, NPCATT_LEGACY_2, NPCATT_LEGACY_3,
                NPCATT_LEGACY_4, NPCATT_LEGACY_5, NPCATT_LEGACY_6
            }
        };
        if( legacy_attitudes.count( attitude ) > 0 ) {
            previous_attitude = NPCATT_NULL;
        }
    }

    if( data.read( "comp_mission_id", comp_miss_id ) ) {
        comp_mission.miss_id = mission_id_of( comp_miss_id );
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

    std::vector<tripoint_abs_omt> companion_mission_pts;
    data.read( "companion_mission_points", companion_mission_pts );
    for( auto pt : companion_mission_pts ) {
        companion_mission_points.push_back( pt );
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
        companion_mission_inv.json_load_items( data.get_member( "companion_mission_inv" ) );
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
    real_weapon = item();
    data.read( "real_weapon", real_weapon );

    complaints.clear();
    for( const JsonMember member : data.get_object( "complaints" ) ) {
        // TODO: time_point does not have a default constructor, need to read in the map manually
        time_point p = calendar::turn_zero;
        member.read( p );
        complaints.emplace( member.name(), p );
    }
    data.read( "unique_id", unique_id );
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
    Character::store( json );

    json.member( "marked_for_death", marked_for_death );
    json.member( "dead", dead );
    json.member( "patience", patience );
    json.member( "myclass", myclass.str() );
    json.member( "idz", idz.str() );
    json.member( "known_to_u", known_to_u );
    json.member( "personality", personality );

    json.member( "last_player_seen_pos", last_player_seen_pos );

    json.member( "goalx", goal.x() );
    json.member( "goaly", goal.y() );
    json.member( "goalz", goal.z() );

    json.member( "guard_pos", guard_pos );
    json.member( "current_activity_id", current_activity_id.str() );
    json.member( "pulp_location", pulp_location );
    json.member( "assigned_camp", assigned_camp );
    json.member( "chair_pos", chair_pos );
    json.member( "wander_pos", wander_pos );
    json.member( "job", job );
    // TODO: stringid
    json.member( "mission", mission );
    json.member( "previous_mission", previous_mission );
    json.member( "faction_api_ver", faction_api_version );
    if( !fac_id.str().empty() ) { // set in constructor
        json.member( "my_fac", fac_id.c_str() );
    }
    json.member( "attitude", static_cast<int>( attitude ) );
    json.member( "previous_attitude", static_cast<int>( previous_attitude ) );
    json.member( "op_of_u", op_of_u );
    json.member( "chatbin", chatbin );
    json.member( "rules", rules );

    if( !real_weapon.is_null() ) {
        json.member( "real_weapon", real_weapon ); // also saves contents
    }

    json.member( "comp_mission_id", string_of( comp_mission.miss_id ) );
    json.member( "comp_mission_pt", comp_mission.position );
    json.member( "comp_mission_role", comp_mission.role_id );
    json.member( "companion_mission_role_id", companion_mission_role_id );
    json.member( "companion_mission_points", companion_mission_points );
    json.member( "companion_mission_time", companion_mission_time );
    json.member( "companion_mission_time_ret", companion_mission_time_ret );
    json.member( "companion_mission_inv" );
    companion_mission_inv.json_save_items( json );
    json.member( "restock", restock );

    json.member( "complaints", complaints );
    json.member( "unique_id", unique_id );
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
        json.member( elem.first.str() );
        json.start_array();
        for( const char &_sym : elem.second ) {
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
void inventory::json_load_invcache( const JsonValue &jsin )
{
    try {
        std::unordered_map<itype_id, std::string> map;
        for( JsonObject jo : jsin.get_array() ) {
            jo.allow_omitted_members();
            for( const JsonMember member : jo ) {
                std::string invlets;
                for( const int i : member.get_array() ) {
                    invlets.push_back( i );
                }
                map[itype_id( member.name() )] = invlets;
            }
        }
        invlet_cache = invlet_favorites{ map };
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
        for( const item &elem_stack_iter : elem ) {
            elem_stack_iter.serialize( json );
        }
    }
    json.end_array();
}

void inventory::json_load_items( const JsonArray &ja )
{
    for( JsonObject jo : ja ) {
        item tmp;
        tmp.deserialize( jo );
        add_item( tmp, true, false );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// monster.h

// TEMPORARY until 0.G
void monster::deserialize( const JsonObject &data, const tripoint_abs_sm &submap_loc )
{
    data.allow_omitted_members();
    load( data, submap_loc );
}

void monster::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    load( data );
}

// TEMPORARY until 0.G
void monster::load( const JsonObject &data, const tripoint_abs_sm &submap_loc )
{
    load( data );
    if( !data.has_member( "location" ) ) {
        // When loading an older save in which the monster's absolute location is not serialized
        // and the monster is not in the current map, the submap location inferred by load()
        // will be wrong. Use the supplied argument to fix it.
        const tripoint_abs_ms old_loc = get_location();
        point_abs_sm wrong_submap;
        tripoint_sm_ms local_pos;
        std::tie( wrong_submap, local_pos ) = project_remain<coords::sm>( get_location() );
        set_location( project_combine( submap_loc.xy(), local_pos ) );
        // adjust other relative coordinates that would be subject to the same error
        wander_pos = wander_pos - old_loc + get_location();
        if( goal ) {
            *goal = *goal - old_loc + get_location();
        }
    }
}

void monster::load( const JsonObject &data )
{
    Creature::load( data );

    // TEMPORARY until 0.G
    if( !data.has_member( "location" ) ) {
        set_location( get_map().getglobal( read_legacy_creature_pos( data ) ) );
        tripoint wand;
        data.read( "wandx", wand.x );
        data.read( "wandy", wand.y );
        data.read( "wandz", wand.z );
        wander_pos = get_map().getglobal( wand );
        tripoint destination;
        data.read( "destination", destination );
        if( destination != tripoint_zero ) {
            goal = get_location() + destination;
        }
    }

    std::string sidtmp;
    // load->str->int
    data.read( "typeid", sidtmp );
    type = &mtype_id( sidtmp ).obj();

    data.read( "unique_name", unique_name );
    data.read( "nickname", nickname );
    data.read( "goal", goal );
    data.read( "provocative_sound", provocative_sound );
    data.read( "wandf", wandf );
    data.read( "wander_pos", wander_pos );
    if( data.has_int( "next_patrol_point" ) ) {
        data.read( "next_patrol_point", next_patrol_point );
        data.read( "patrol_route", patrol_route );
    }
    if( data.has_object( "tied_item" ) ) {
        JsonValue tied_item_json = data.get_member( "tied_item" );
        item newitem;
        newitem.deserialize( tied_item_json );
        tied_item = cata::make_value<item>( newitem );
    }
    if( data.has_object( "tack_item" ) ) {
        JsonValue tack_item_json = data.get_member( "tack_item" );
        item newitem;
        newitem.deserialize( tack_item_json );
        tack_item = cata::make_value<item>( newitem );
    }
    if( data.has_object( "armor_item" ) ) {
        JsonValue armor_item_json = data.get_member( "armor_item" );
        item newitem;
        newitem.deserialize( armor_item_json );
        armor_item = cata::make_value<item>( newitem );
    }
    if( data.has_object( "storage_item" ) ) {
        JsonValue storage_item_json = data.get_member( "storage_item" );
        item newitem;
        newitem.deserialize( storage_item_json );
        storage_item = cata::make_value<item>( newitem );
    }
    if( data.has_object( "battery_item" ) ) {
        JsonValue battery_item_json = data.get_member( "battery_item" );
        item newitem;
        newitem.deserialize( battery_item_json );
        battery_item = cata::make_value<item>( newitem );
    }
    data.read( "hp", hp );

    // sp_timeout indicates an old save, prior to the special_attacks refactor
    if( data.has_array( "sp_timeout" ) ) {
        JsonArray parray = data.get_array( "sp_timeout" );
        size_t index = 0;
        int ptimeout = 0;
        while( parray.has_more() && index < type->special_attacks_names.size() ) {
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

    // special_attacks indicates a save after the special_attacks refactor
    if( data.has_object( "special_attacks" ) ) {
        for( const JsonMember member : data.get_object( "special_attacks" ) ) {
            JsonObject saobject = member.get_object();
            saobject.allow_omitted_members();
            auto &entry = special_attacks[member.name()];
            entry.cooldown = saobject.get_int( "cooldown" );
            entry.enabled = saobject.get_bool( "enabled" );
        }
    }

    // make sure the loaded monster has every special attack its type says it should have
    for( const auto &sa : type->special_attacks ) {
        const std::string &aname = sa.first;
        if( special_attacks.find( aname ) == special_attacks.end() ) {
            auto &entry = special_attacks[aname];
            entry.cooldown = rng( 0, sa.second->cooldown );
        }
    }

    data.read( "friendly", friendly );
    if( data.has_member( "mission_ids" ) ) {
        data.read( "mission_ids", mission_ids );
    } else if( data.has_member( "mission_id" ) ) {
        const int mission_id = data.get_int( "mission_id" );
        if( mission_id > 0 ) {
            mission_ids = { mission_id };
        } else {
            mission_ids.clear();
        }
    }
    data.read( "mission_fused", mission_fused );
    data.read( "no_extra_death_drops", no_extra_death_drops );
    data.read( "dead", dead );
    data.read( "anger", anger );
    data.read( "morale", morale );
    data.read( "hallucination", hallucination );
    data.read( "fish_population", fish_population );
    data.read( "lifespan_end", lifespan_end );
    //for older saves convert summon time limit to lifespan end
    cata::optional<time_duration> summon_time_limit;
    data.read( "summon_time_limit", summon_time_limit );
    if( summon_time_limit ) {
        set_summon_time( *summon_time_limit );
    }

    upgrades = data.get_bool( "upgrades", type->upgrades );
    upgrade_time = data.get_int( "upgrade_time", -1 );

    reproduces = data.get_bool( "reproduces", type->reproduces );
    baby_timer.reset();
    data.read( "baby_timer", baby_timer );
    if( baby_timer && *baby_timer == calendar::before_time_starts ) {
        baby_timer.reset();
    }

    biosignatures = data.get_bool( "biosignatures", type->biosignatures );
    biosig_timer = time_point( data.get_int( "biosig_timer", -1 ) );

    data.read( "udder_timer", udder_timer );

    horde_attraction = static_cast<monster_horde_attraction>( data.get_int( "horde_attraction", 0 ) );

    data.read( "inv", inv );
    data.read( "dissectable_inv", dissectable_inv );
    data.read( "dragged_foe_id", dragged_foe_id );

    data.read( "ammo", ammo );

    // TODO: Remove blob migration after 0.F
    const std::string faction_string = data.get_string( "faction", "" );
    faction = mfaction_str_id( faction_string == "blob" ? "slime" : faction_string );
    data.read( "mounted_player_id", mounted_player_id );
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
    json.member( "nickname", nickname );
    json.member( "goal", goal );
    json.member( "wander_pos", wander_pos );
    json.member( "wandf", wandf );
    json.member( "provocative_sound", provocative_sound );
    if( !patrol_route.empty() ) {
        json.member( "patrol_route", patrol_route );
        json.member( "next_patrol_point", next_patrol_point );
    }
    json.member( "hp", hp );
    json.member( "special_attacks", special_attacks );
    json.member( "friendly", friendly );
    json.member( "fish_population", fish_population );
    json.member( "faction", faction.id().str() );
    json.member( "mission_ids", mission_ids );
    json.member( "mission_fused", mission_fused );
    json.member( "no_extra_death_drops", no_extra_death_drops );
    json.member( "dead", dead );
    json.member( "anger", anger );
    json.member( "morale", morale );
    json.member( "hallucination", hallucination );
    if( tied_item ) {
        json.member( "tied_item", *tied_item );
    }
    if( tack_item ) {
        json.member( "tack_item", *tack_item );
    }
    if( armor_item ) {
        json.member( "armor_item", *armor_item );
    }
    if( storage_item ) {
        json.member( "storage_item", *storage_item );
    }
    if( battery_item ) {
        json.member( "battery_item", *battery_item );
    }
    json.member( "ammo", ammo );
    json.member( "underwater", underwater );
    json.member( "upgrades", upgrades );
    json.member( "upgrade_time", upgrade_time );
    json.member( "reproduces", reproduces );
    json.member( "baby_timer", baby_timer );
    json.member( "biosignatures", biosignatures );
    json.member( "biosig_timer", biosig_timer );
    json.member( "udder_timer", udder_timer );

    json.member( "lifespan_end", lifespan_end );

    if( horde_attraction > MHA_NULL && horde_attraction < NUM_MONSTER_HORDE_ATTRACTION ) {
        json.member( "horde_attraction", horde_attraction );
    }
    json.member( "inv", inv );
    json.member( "dissectable_inv", dissectable_inv );

    json.member( "dragged_foe_id", dragged_foe_id );
    // storing the rider
    json.member( "mounted_player_id", mounted_player_id );
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

void time_point::deserialize( int turn )
{
    turn_ = turn;
}

void time_duration::serialize( JsonOut &jsout ) const
{
    jsout.write( turns_ );
}

void time_duration::deserialize( const JsonValue &jsin )
{
    if( jsin.test_string() ) {
        *this = read_from_json_string<time_duration>( jsin, time_duration::units );
    } else {
        turns_ = jsin.get_int();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// item.h

void item::craft_data::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "making", making->ident().str() );
    jsout.member( "disassembly", disassembly );
    jsout.member( "comps_used", comps_used );
    jsout.member( "next_failure_point", next_failure_point );
    jsout.member( "tools_to_continue", tools_to_continue );
    jsout.member( "batch_size", batch_size );
    jsout.member( "cached_tool_selections", cached_tool_selections );
    jsout.end_object();
}

void item::craft_data::deserialize( const JsonObject &obj )
{
    obj.allow_omitted_members();
    std::string recipe_string = obj.get_string( "making" );
    disassembly = obj.get_bool( "disassembly", false );
    if( disassembly ) {
        making = &recipe_dictionary::get_uncraft( itype_id( recipe_string ) );
    } else {
        making = &recipe_id( recipe_string ).obj();
    }
    obj.read( "comps_used", comps_used );
    next_failure_point = obj.get_int( "next_failure_point", -1 );
    tools_to_continue = obj.get_bool( "tools_to_continue", false );
    batch_size = obj.get_int( "batch_size", -1 );
    obj.read( "cached_tool_selections", cached_tool_selections );
}

// Template parameter because item::craft_data is private and I don't want to make it public.
template<typename T>
static void load_legacy_craft_data( io::JsonObjectInputArchive &archive, T &value )
{
    archive.allow_omitted_members();
    if( archive.has_member( "making" ) ) {
        value = cata::make_value<typename T::element_type>();
        value->deserialize( archive );
    }
}

// Dummy function as we never load anything from an output archive.
template<typename T>
static void load_legacy_craft_data( io::JsonObjectOutputArchive &, T & )
{
}

static std::set<itype_id> charge_removal_blacklist;

void load_charge_removal_blacklist( const JsonObject &jo, const std::string &/*src*/ )
{
    jo.allow_omitted_members();
    std::set<itype_id> new_blacklist;
    jo.read( "list", new_blacklist );
    charge_removal_blacklist.insert( new_blacklist.begin(), new_blacklist.end() );
}

static std::set<itype_id> charge_migration_blacklist;

void load_charge_migration_blacklist( const JsonObject &jo, const std::string &/*src*/ )
{
    jo.allow_omitted_members();
    std::set<itype_id> new_blacklist;
    jo.read( "list", new_blacklist );
    charge_migration_blacklist.insert( new_blacklist.begin(), new_blacklist.end() );
}

template<typename Archive>
void item::io( Archive &archive )
{

    itype_id orig; // original ID as loaded from JSON
    const auto load_type = [&]( const std::string & id ) {
        orig = itype_id( id );
        convert( item_controller->migrate_id( orig ) );
    };

    const auto load_corpse = [this]( const std::string & id ) {
        if( itype_id( id ).is_null() ) {
            // backwards compatibility, nullptr should not be stored at all
            corpse = nullptr;
        } else {
            corpse = &mtype_id( id ).obj();
        }
    };
    archive.template io<const itype>( "typeid", type, load_type, []( const itype & i ) {
        return i.get_id().str();
    }, io::required_tag() );

    // normalize legacy saves to always have charges >= 0
    archive.io( "charges", charges, 0 );
    charges = std::max( charges, 0 );

    archive.io( "energy", energy, 0_mJ );

    int cur_phase = static_cast<int>( current_phase );
    archive.io( "burnt", burnt, 0 );
    archive.io( "poison", poison, 0 );
    archive.io( "frequency", frequency, 0 );
    archive.io( "snip_id", snip_id, snippet_id::NULL_ID() );
    // NB! field is named `irridation` in legacy files
    archive.io( "irridation", irradiation, 0 );
    archive.io( "bday", bday, calendar::start_of_cataclysm );
    archive.io( "mission_id", mission_id, -1 );
    archive.io( "player_id", player_id, -1 );
    archive.io( "item_vars", item_vars, io::empty_default_tag() );
    // TODO: change default to empty string
    archive.io( "name", corpse_name, std::string() );
    archive.io( "owner", owner, owner.NULL_ID() );
    archive.io( "old_owner", old_owner, old_owner.NULL_ID() );
    archive.io( "invlet", invlet, '\0' );
    archive.io( "damaged", damage_, 0 );
    archive.io( "degradation", degradation_, 0 );
    archive.io( "active", active, false );
    archive.io( "is_favorite", is_favorite, false );
    archive.io( "item_counter", item_counter, static_cast<decltype( item_counter )>( 0 ) );
    archive.io( "wetness", wetness, 0 );
    archive.io( "dropped_from", dropped_from, harvest_drop_type_id::NULL_ID() );
    archive.io( "rot", rot, 0_turns );
    archive.io( "last_temp_check", last_temp_check, calendar::start_of_cataclysm );
    archive.io( "current_phase", cur_phase, static_cast<int>( type->phase ) );
    archive.io( "techniques", techniques, io::empty_default_tag() );
    archive.io( "faults", faults, io::empty_default_tag() );
    archive.io( "item_tags", item_tags, io::empty_default_tag() );
    archive.io( "components", components, io::empty_default_tag() );
    archive.io( "specific_energy", specific_energy, units::from_joule_per_gram( -10.f ) );
    archive.io( "temperature", temperature, units::from_kelvin( 0.f ) );
    archive.io( "recipe_charges", recipe_charges, 1 );
    // Legacy: remove flag check/unset after 0.F
    archive.io( "ethereal", ethereal, has_flag( flag_ETHEREAL_ITEM ) );
    unset_flag( flag_ETHEREAL_ITEM );
    archive.template io<const mtype>( "corpse", corpse, load_corpse,
    []( const mtype & i ) {
        return i.id.str();
    } );
    archive.io( "craft_data", craft_data_, decltype( craft_data_ )() );
    const auto ivload = [this]( const std::string & variant ) {
        set_itype_variant( variant );
    };
    const auto ivsave = []( const itype_variant_data * iv ) {
        return iv->id;
    };
    archive.io( "variant", _itype_variant, ivload, ivsave, false );
    archive.io( "light", light.luminance, nolight.luminance );
    archive.io( "light_width", light.width, nolight.width );
    archive.io( "light_dir", light.direction, nolight.direction );

    static const cata::value_ptr<relic> null_relic_ptr = nullptr;
    archive.io( "relic_data", relic_data, null_relic_ptr );

    item_controller->migrate_item( orig, *this );

    if( !Archive::is_input::value ) {
        return;
    }
    /* Loading has finished, following code is to ensure consistency and fixes bugs in saves. */

    load_legacy_craft_data( archive, craft_data_ );

    double float_damage = 0;
    if( archive.read( "damage", float_damage ) ) {
        damage_ = std::min( std::max( min_damage(),
                                      static_cast<int>( float_damage * itype::damage_scale ) ),
                            max_damage() );
    }

    int note = 0;
    const bool note_read = archive.read( "note", note );

    // Old saves used to only contain one of those values (stored under "poison"), it would be
    // loaded into a union of those members. Now they are separate members and must be set separately.
    if( poison != 0 && note == 0 && !type->snippet_category.empty() ) {
        std::swap( note, poison );
    }
    if( poison != 0 && frequency == 0 && ( typeId() == itype_radio_on || typeId() == itype_radio ) ) {
        std::swap( frequency, poison );
    }
    if( poison != 0 && irradiation == 0 && typeId() == itype_rad_badge ) {
        std::swap( irradiation, poison );
    }

    // Compatibility with old 0.F saves
    // Tempreature used to be saved as 0.00001 K integer.
    // specific_energy used to be saved as 0.00001 J/g integer.
    if( temperature > 100000_K ) {
        temperature /= 100000;
        specific_energy /= 100000;
    }

    // erase all invalid flags (not defined in flags.json)
    // warning was generated earlier on load
    erase_if( item_tags, [&]( const flag_id & f ) {
        return !f.is_valid();
    } );

    if( note_read ) {
        snip_id = SNIPPET.migrate_hash_to_id( note );
    } else {
        cata::optional<std::string> snip;
        if( archive.read( "snippet_id", snip ) && snip ) {
            snip_id = snippet_id( snip.value() );
        }
    }

    // Compatibility for item type changes: for example soap changed from being a generic item
    // (item::charges -1 or 0 or anything else) to comestible (and thereby counted by charges),
    // old saves still have invalid charges, this fixes the charges value to the default charges.
    if( count_by_charges() && charges <= 0 ) {
        charges = item( type, calendar::turn_zero ).charges;
    }

    // Items from old saves before those items were temperature tracked need activating.
    if( has_temperature() ) {
        active = true;
    }
    if( !active && ( has_own_flag( flag_HOT ) || has_own_flag( flag_COLD ) ||
                     has_own_flag( flag_WET ) ) ) {
        // Some hot/cold items from legacy saves may be inactive
        active = true;
    }
    std::string mode;
    if( archive.read( "mode", mode ) ) {
        // only for backward compatibility (nowadays mode is stored in item_vars)
        gun_set_mode( gun_mode_id( mode ) );
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

    static const std::set<std::string> removed_item_vars = {
        // Searchlight monster setting vars
        "SL_PREFER_UP", "SL_PREFER_DOWN", "SL_PREFER_RIGHT", "SL_PREFER_LEFT", "SL_SPOT_X", "SL_SPOT_Y", "SL_POWER", "SL_DIR",
        // Remove stored translated gerund in favor of storing the inscription tool type
        "item_label_type", "item_note_type"
    };

    for( const std::string &var : removed_item_vars ) {
        item_vars.erase( var );
    }

    current_phase = static_cast<phase_id>( cur_phase );
    // override phase if frozen, needed for legacy save
    if( has_own_flag( flag_FROZEN ) && current_phase == phase_id::LIQUID ) {
        current_phase = phase_id::SOLID;
    }

    // Activate corpses from old saves
    if( is_corpse() && !active ) {
        active = true;
    }

    if( charges != 0 && !type->can_have_charges() ) {
        // Types that are known to have charges, but should not have them.
        // We fix it here, but it's expected from bugged saves and does not require a message.
        if( charge_migration_blacklist.count( type->get_id() ) != 0 ) {
            for( int i = 0; i < charges - 1; i++ ) {
                put_in( item( type ), item_pocket::pocket_type::MIGRATION );
            }
        } else if( charge_removal_blacklist.count( type->get_id() ) == 0 ) {
            debugmsg( "Item %s was loaded with charges, but can not have any!",
                      type->get_id().str() );
        }
        charges = 0;
    }
}

void item::migrate_content_item( const item &contained )
{
    if( contained.is_gunmod() || contained.is_toolmod() ) {
        put_in( contained, item_pocket::pocket_type::MOD );
    } else if( typeId() == itype_usb_drive ) {
        // as of this migration, only usb_drive has any software in it.
        put_in( contained, item_pocket::pocket_type::SOFTWARE );
    } else if( contents.insert_item( contained, item_pocket::pocket_type::MAGAZINE ).success() ||
               contents.insert_item( contained, item_pocket::pocket_type::MAGAZINE_WELL ).success() ) {
        // left intentionally blank
    } else if( is_corpse() ) {
        put_in( contained, item_pocket::pocket_type::CORPSE );
    } else if( can_contain( contained ).success() ) {
        put_in( contained, item_pocket::pocket_type::CONTAINER );
    } else {
        // we want this to silently fail - the contents will fall out later
        put_in( contained, item_pocket::pocket_type::MIGRATION );
    }
}

void item::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    // Since deserialization is handled by the archive, don't check it here.
    // CATA_DO_NOT_CHECK_SERIALIZE
    io::JsonObjectInputArchive archive( data );
    io( archive );
    archive.allow_omitted_members();
    data.copy_visited_members( archive );

    // first half of the if statement is for migration to nested containers. remove after 0.F
    if( data.has_array( "contents" ) ) {
        std::list<item> items;
        data.read( "contents", items );
        for( const item &it : items ) {
            migrate_content_item( it );
        }
    } else if( data.has_object( "contents" ) ) { // non-empty contents
        item_contents read_contents;
        data.read( "contents", read_contents );

        contents.read_mods( read_contents );
        update_modified_pockets();
        contents.combine( read_contents, false, true );

        if( data.has_object( "contents" ) ) {
            JsonObject tested = data.get_object( "contents" );
            tested.allow_omitted_members();
            if( tested.has_array( "items" ) ) {
                // migration for nested containers. leave until after 0.F
                std::list<item> items;
                tested.read( "items", items );
                for( const item &it : items ) {
                    migrate_content_item( it );
                }
            }
        }
        // contents may not be empty if other migration happened in item::io
    } else if( contents.empty() ) { // empty contents was not serialized, recreate pockets from the type
        contents = item_contents( type->pockets );
    }

    // Remove after 0.F: artifact migration code
    if( typeId().str().substr( 0, 9 ) == "artifact_" ) {
        relic_procgen_data::generation_rules rules;
        rules.max_attributes = 5;
        rules.max_negative_power = -1000;
        rules.power_level = 2000;

        item_contents temp_migrate( contents );

        *this = relic_procgen_data_cult->create_item( rules );

        if( !temp_migrate.empty() ) {
            for( const item *it : temp_migrate.all_items_top() ) {
                contents.insert_item( *it, item_pocket::pocket_type::MIGRATION );
            }
        }
    }

    // FIXME: batch_size migration from charges - remove after 0.G
    if( is_craft() && craft_data_->batch_size <= 0 ) {
        craft_data_->batch_size = clamp( charges, 1, charges );
        charges = 0;
    }

    if( !has_itype_variant( false ) && can_have_itype_variant() ) {
        if( possible_itype_variant( typeId().str() ) ) {
            set_itype_variant( typeId().str() );
        } else {
            select_itype_variant();
        }
    }

    update_inherited_flags();
    on_damage_changed();
}

void item::serialize( JsonOut &json ) const
{
    // Remove after 0.F: artifact migration code
    if( typeId().str().substr( 0, 9 ) == "artifact_" ) {
        relic_procgen_data::generation_rules rules;
        rules.max_attributes = 5;
        rules.max_negative_power = -1000;
        rules.power_level = 2000;

        relic_procgen_data_cult->create_item( rules ).serialize( json );
        return;
    }

    // Skip the serialization check because this is forwarding serialization to
    // another function
    // CATA_DO_NOT_CHECK_SERIALIZE

    io::JsonObjectOutputArchive archive( json );
    const_cast<item *>( this )->io( archive );
    if( !contents.empty_with_no_mods() || contents.has_additional_pockets() ) {
        json.member( "contents", contents );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///// vehicle.h

/*
 * vehicle_part
 */
void vehicle_part::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    vpart_id pid;
    data.read( "id", pid );

    pid = vpart_migration::migrate( pid );

    std::tie( pid, variant ) = get_vpart_id_variant( pid );

    // if we don't know what type of part it is, it'll cause problems later.
    if( !pid.is_valid() ) {
        data.throw_error_at( "id", "bad vehicle part" );
    }
    id = pid;
    if( variant.empty() ) {
        data.read( "variant", variant );
    }

    if( data.has_object( "base" ) ) {
        data.read( "base", base );
    } else {
        // handle legacy format which didn't include the base item
        base = item( id.obj().base_item );
    }

    data.read( "mount_dx", mount.x );
    data.read( "mount_dy", mount.y );
    data.read( "open", open );
    int direction_int;
    data.read( "direction", direction_int );
    direction = units::from_degrees( direction_int );
    data.read( "blood", blood );
    data.read( "enabled", enabled );
    data.read( "flags", flags );
    data.read( "passenger_id", passenger_id );
    if( data.has_int( "z_offset" ) ) {
        int z_offset = data.get_int( "z_offset" );
        if( std::abs( z_offset ) > 10 ) {
            data.throw_error_at( "z_offset", "z_offset out of range" );
        }
        precalc[0].z = z_offset;
        precalc[1].z = z_offset;
    }

    // load legacy bike rack data
    JsonArray ja = data.get_array( "carry" );
    // count down from size - 1, then stop after unsigned long 0 - 1 becomes MAX_INT
    static constexpr int name_offset = 7;
    for( size_t index = ja.size() - 1; index < ja.size(); index-- ) {
        const std::string raw = ja.get_string( index );
        const bool migrate_x_axis = raw[0] == 'X';
        const int mount_offset = std::stoi( raw.substr( 1, 3 ) );
        carried_stack.push( {
            tripoint( mount_offset, 0, 0 ),
            units::from_degrees( std::stoi( raw.substr( 4, 3 ) ) ),
            raw.substr( name_offset ),
            migrate_x_axis,
        } );
    }

    // load new bike rack data
    JsonArray ja_carried = data.get_array( "carried_stack" );
    // count down from size - 1, then stop after unsigned long 0 - 1 becomes MAX_INT
    for( size_t index = ja_carried.size() - 1; index < ja_carried.size(); index-- ) {
        vehicle_part::carried_part_data it;
        ja_carried.read( index, it );
        carried_stack.push( it );
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
        ammo_set( items.begin()->ammo_current(), qty );
        items.clear();
    }
}

void vehicle_part::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "id", id.str() );
    if( !variant.empty() ) {
        json.member( "variant", variant );
    }
    json.member( "base", base );
    json.member( "mount_dx", mount.x );
    json.member( "mount_dy", mount.y );
    json.member( "open", open );
    json.member( "direction", std::lround( to_degrees( direction ) ) );
    json.member( "blood", blood );
    json.member( "enabled", enabled );
    json.member( "flags", flags );
    if( !carried_stack.empty() ) {
        std::stack<vehicle_part::carried_part_data> carried_copy = carried_stack;
        json.member( "carried_stack" );
        json.start_array();
        while( !carried_copy.empty() ) {
            json.write( carried_copy.top() );
            carried_copy.pop();
        }
        json.end_array();
    }
    json.member( "passenger_id", passenger_id );
    json.member( "crew_id", crew_id );
    if( precalc[0].z ) {
        json.member( "z_offset", precalc[0].z );
    }
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

void vehicle_part::carried_part_data::deserialize( const JsonObject &data )
{
    data.read( "veh_name", veh_name );
    face_dir = units::from_degrees( data.get_int( "face_dir" ) );
    data.read( "mount_x", mount.x );
    data.read( "mount_y", mount.y );
    data.read( "mount_z", mount.z );
    data.read( "migrate_x_axis", migrate_x_axis );
}

void vehicle_part::carried_part_data::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "veh_name", veh_name );
    json.member( "face_dir", std::lround( to_degrees( face_dir ) ) );
    json.member( "mount_x", mount.x );
    json.member( "mount_y", mount.y );
    json.member( "mount_z", mount.z );
    json.member( "migrate_x_axis", migrate_x_axis, false );
    json.end_object();
}

/*
 * label
 */
void label::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "x", x );
    data.read( "y", y );
    data.read( "text", text );
}

void label::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "x", x );
    json.member( "y", y );
    json.member( "text", text );
    json.end_object();
}

void smart_controller_config::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "bat_lo", battery_lo );
    data.read( "bat_hi", battery_hi );
}

void smart_controller_config::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "bat_lo", battery_lo );
    json.member( "bat_hi", battery_hi );
    json.end_object();
}

/*
 * Load vehicle from a json blob that might just exceed player in size.
 */
void vehicle::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();

    int fdir = 0;
    int mdir = 0;

    data.read( "type", type );
    data.read( "posx", pos.x );
    data.read( "posy", pos.y );
    data.read( "om_id", om_id );
    data.read( "faceDir", fdir );
    data.read( "moveDir", mdir );
    int turn_dir_int;
    data.read( "turn_dir", turn_dir_int );
    turn_dir = units::from_degrees( turn_dir_int );
    data.read( "last_turn", turn_dir_int );
    last_turn = units::from_degrees( turn_dir_int );
    data.read( "velocity", velocity );
    data.read( "avg_velocity", avg_velocity );
    data.read( "falling", is_falling );
    data.read( "floating", is_floating );
    data.read( "in_water", in_water );
    data.read( "flying", is_flying );
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
    data.read( "autopilot_on", autopilot_on );
    if( !data.read( "last_update_turn", last_update ) ) {
        last_update = calendar::turn;
    }

    units::angle fdir_angle = units::from_degrees( fdir );
    face.init( fdir_angle );
    move.init( units::from_degrees( mdir ) );
    data.read( "name", name );
    std::string temp_id;
    std::string temp_old_id;
    data.read( "owner", temp_id );
    data.read( "old_owner", temp_old_id );
    // for savegames before the change to faction_id for ownership.
    if( temp_id.empty() ) {
        owner = faction_id::NULL_ID();
    } else {
        owner = faction_id( temp_id );
    }
    if( temp_old_id.empty() ) {
        old_owner = faction_id::NULL_ID();
    } else {
        old_owner = faction_id( temp_old_id );
    }
    data.read( "theft_time", theft_time );

    parts.clear();
    for( const JsonValue val : data.get_array( "parts" ) ) {
        vehicle_part part;
        try {
            val.read( part, true );
            parts.emplace_back( std::move( part ) );
        } catch( const JsonError &err ) {
            debugmsg( err.what() );
        }
    }

    // we persist the pivot anchor so that if the rules for finding
    // the pivot change, existing vehicles do not shift around.
    // Loading vehicles that predate the pivot logic is a special
    // case of this, they will load with an anchor of (0,0) which
    // is what they're expecting.
    data.read( "pivot", pivot_anchor[0] );
    pivot_anchor[1] = pivot_anchor[0];
    pivot_rotation[1] = pivot_rotation[0] = fdir_angle;
    data.read( "is_on_ramp", is_on_ramp );
    data.read( "is_autodriving", is_autodriving );
    data.read( "is_following", is_following );
    data.read( "is_patrolling", is_patrolling );
    data.read( "autodrive_local_target", autodrive_local_target );
    data.read( "airworthy", flyable );
    data.read( "requested_z_change", requested_z_change );
    data.read( "summon_time_limit", summon_time_limit );
    data.read( "magic", magic );

    smart_controller_cfg = cata::nullopt;
    data.read( "smart_controller", smart_controller_cfg );
    data.read( "vehicle_noise", vehicle_noise );

    // Need to manually backfill the active item cache since the part loader can't call its vehicle.
    for( const vpart_reference &vp : get_any_parts( VPFLAG_CARGO ) ) {
        auto it = vp.part().items.begin();
        auto end = vp.part().items.end();
        for( ; it != end; ++it ) {
            // remove after 0.F
            if( savegame_loading_version < 33 ) {
                migrate_item_charges( *it );
            }
        }
    }

    for( const vpart_reference &vp : get_any_parts( "TURRET" ) ) {
        install_part( vp.mount(), vpart_turret_mount );

        //Forcibly set turrets' targeting mode to manual if no turret control unit is
        //present on turret's tile on loading save
        if( !has_part( global_part_pos3( vp.part() ), "TURRET_CONTROLS" ) ) {
            vp.part().enabled = false;
        }
        //Set turret control unit's state equal to turret's targeting mode on loading save
        for( const vpart_reference &turret_part : get_any_parts( "TURRET_CONTROLS" ) ) {
            turret_part.part().enabled = vp.part().enabled;
        }
    }

    data.read( "tags", tags );
    data.read( "labels", labels );

    if( data.has_string( "fuel_remainder" ) ) {
        data.read( "fuel_remainder", fuel_remainder );
    } else {
        // Compatibility with 0.F
        // It is a small and not important number so just ignore it.
    }

    if( data.has_string( "fuel_used_last_turn" ) ) {
        data.read( "fuel_used_last_turn", fuel_used_last_turn );
    } else {
        // Compatibility with 0.F
        // It is a small and not important number so just ignore it.
    }

    // TODO: Remove after enough time passes for save games to migrate.
    // This migrates old "convertible" vehicles to new generic "folding" ones
    if( has_tag( "convertible" ) ) {
        // remove tags starting from "convertible"
        for( auto it = tags.begin(); it != tags.end(); ) {
            if( it->rfind( "convertible", 0 ) == 0 ) {
                it = tags.erase( it );
            } else {
                ++it;
            }
        }

        // Special case convertible folding bicycles as they had non-foldable parts
        // as part of their vehicle prototype. Other in-tree "convertibles"
        // (wheelchair, inflatable boat) can just have their tags removed as their
        // vehicles were made from foldable parts already
        if( type == vehicle_prototype_bicycle ) {
            type = vehicle_prototype_bicycle_folding;
            parts = type.obj().blueprint->parts;
        }
    }

    refresh();

    point p;
    zone_data zd;
    for( JsonObject sdata : data.get_array( "zones" ) ) {
        sdata.allow_omitted_members();
        sdata.read( "point", p );
        sdata.read( "zone", zd );
        loot_zones.emplace( p, zd );
    }
    data.read( "other_tow_point", tow_data.other_towing_point );
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
    json.member( "posx", pos.x );
    json.member( "posy", pos.y );
    json.member( "om_id", om_id );
    json.member( "faceDir", std::lround( to_degrees( face.dir() ) ) );
    json.member( "moveDir", std::lround( to_degrees( move.dir() ) ) );
    json.member( "turn_dir", std::lround( to_degrees( turn_dir ) ) );
    json.member( "last_turn", std::lround( to_degrees( last_turn ) ) );
    json.member( "velocity", velocity );
    json.member( "avg_velocity", avg_velocity );
    json.member( "falling", is_falling );
    json.member( "in_water", in_water );
    json.member( "floating", is_floating );
    json.member( "flying", is_flying );
    json.member( "cruise_velocity", cruise_velocity );
    json.member( "vertical_velocity", vertical_velocity );
    json.member( "cruise_on", cruise_on );
    json.member( "engine_on", engine_on );
    json.member( "tracking_on", tracking_on );
    json.member( "skidding", skidding );
    json.member( "of_turn_carry", of_turn_carry );
    json.member( "name", name );
    json.member( "owner", owner );
    json.member( "old_owner", old_owner );
    json.member( "theft_time", theft_time );
    json.member( "parts", real_parts() );
    json.member( "tags", tags );
    json.member( "fuel_remainder", fuel_remainder );
    json.member( "fuel_used_last_turn", fuel_used_last_turn );
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
    tripoint other_tow_temp_point;
    if( is_towed() ) {
        vehicle *tower = tow_data.get_towed_by();
        if( tower ) {
            other_tow_temp_point = tower->global_part_pos3( tower->get_tow_part() );
        }
    }
    json.member( "other_tow_point", other_tow_temp_point );

    json.member( "is_locked", is_locked );
    json.member( "is_alarm_on", is_alarm_on );
    json.member( "camera_on", camera_on );
    json.member( "autopilot_on", autopilot_on );
    json.member( "last_update_turn", last_update );
    json.member( "pivot", pivot_anchor[0] );
    json.member( "is_on_ramp", is_on_ramp );
    json.member( "is_autodriving", is_autodriving );
    json.member( "is_following", is_following );
    json.member( "is_patrolling", is_patrolling );
    json.member( "autodrive_local_target", autodrive_local_target );
    json.member( "airworthy", flyable );
    json.member( "requested_z_change", requested_z_change );
    json.member( "summon_time_limit", summon_time_limit );
    json.member( "magic", magic );
    json.member( "smart_controller", smart_controller_cfg );
    json.member( "vehicle_noise", vehicle_noise );

    json.end_object();
}

////////////////// mission.h
////
void mission::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();

    if( jo.has_int( "type_id" ) ) {
        type = &mission_type::from_legacy( jo.get_int( "type_id" ) ).obj();
    } else if( jo.has_string( "type_id" ) ) {
        type = &mission_type_id( jo.get_string( "type_id" ) ).obj();
    } else {
        debugmsg( "Saved mission has no type" );
        type = &mission_type::get_all().front();
    }

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
    jo.read( "kill_count_to_reach", kill_count_to_reach );
    jo.read( "reward", reward );
    jo.read( "uid", uid );
    JsonArray ja = jo.get_array( "target" );
    if( ja.size() == 3 ) {
        target.x() = ja.get_int( 0 );
        target.y() = ja.get_int( 1 );
        target.z() = ja.get_int( 2 );
    } else if( ja.size() == 2 ) {
        target.x() = ja.get_int( 0 );
        target.y() = ja.get_int( 1 );
    }

    if( jo.has_int( "follow_up" ) ) {
        follow_up = mission_type::from_legacy( jo.get_int( "follow_up" ) );
    } else if( jo.has_string( "follow_up" ) ) {
        follow_up = mission_type_id( jo.get_string( "follow_up" ) );
    }

    jo.read( "item_id", item_id );

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
    jo.read( "player_id", player_id );
}

void mission::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "type_id", type->id );
    json.member( "status", status_to_string( status ) );
    json.member( "value", value );
    json.member( "kill_count_to_reach", kill_count_to_reach );
    json.member( "reward", reward );
    json.member( "uid", uid );

    json.member( "target" );
    json.start_array();
    json.write( target.x() );
    json.write( target.y() );
    json.write( target.z() );
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

    json.end_object();
}

////////////////// faction.h
////
void faction::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();

    jo.read( "id", id );
    jo.read( "name", name );
    jo.read( "likes_u", likes_u );
    jo.read( "respects_u", respects_u );
    jo.read( "trusts_u", trusts_u );
    jo.read( "known_by_u", known_by_u );
    jo.read( "size", size );
    jo.read( "power", power );
    if( !jo.read( "food_supply", food_supply ) ) {
        food_supply = 100;
    }
    if( !jo.read( "wealth", wealth ) ) {
        wealth = 100;
    }
    if( jo.has_array( "opinion_of" ) ) {
        opinion_of = jo.get_int_array( "opinion_of" );
    }
    load_relations( jo );
}

void faction::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "id", id );
    json.member( "name", name );
    json.member( "likes_u", likes_u );
    json.member( "respects_u", respects_u );
    json.member( "trusts_u", trusts_u );
    json.member( "known_by_u", known_by_u );
    json.member( "size", size );
    json.member( "power", power );
    json.member( "food_supply", food_supply );
    json.member( "wealth", wealth );
    json.member( "opinion_of", opinion_of );
    json.member( "relations" );
    json.start_object();
    for( const auto &rel_data : relations ) {
        json.member( rel_data.first );
        json.start_object();
        for( const auto &rel_flag : npc_factions::relation_strs ) {
            json.member( rel_flag.first, rel_data.second.test( rel_flag.second ) );
        }
        json.end_object();
    }
    json.end_object();

    json.end_object();
}

void Creature::store( JsonOut &jsout ) const
{
    jsout.member( "location", location );

    jsout.member( "moves", moves );
    jsout.member( "pain", pain );

    // killer is not stored, it's temporary anyway, any creature that has a non-null
    // killer is dead (as per definition) and should not be stored.

    jsout.member( "effects", *effects );

    jsout.member( "damage_over_time_map", damage_over_time_map );
    jsout.member( "values", values );

    jsout.member( "blocks_left", num_blocks );
    jsout.member( "dodges_left", num_dodges );
    jsout.member( "num_blocks_bonus", num_blocks_bonus );
    jsout.member( "num_dodges_bonus", num_dodges_bonus );

    jsout.member( "armor_bash_bonus", armor_bash_bonus );
    jsout.member( "armor_cut_bonus", armor_cut_bonus );
    jsout.member( "armor_bullet_bonus", armor_bullet_bonus );

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

    jsout.member( "throw_resist", throw_resist );

    jsout.member( "archery_aim_counter", archery_aim_counter );

    jsout.member( "last_updated", last_updated );

    jsout.member( "body", body );

    // fake is not stored, it's temporary anyway, only used to fire with a gun.
}

void Creature::load( const JsonObject &jsin )
{
    jsin.allow_omitted_members();
    jsin.read( "location", location );
    jsin.read( "moves", moves );
    jsin.read( "pain", pain );

    killer = nullptr; // see Creature::load

    // TEMPORARY until 0.F
    if( savegame_loading_version < 31 ) {
        if( jsin.has_object( "effects" ) ) {
            // Because JSON requires string keys we need to convert back to our bp keys
            std::unordered_map<std::string, std::unordered_map<std::string, effect>> tmp_map;
            jsin.read( "effects", tmp_map );
            int key_num = 0;
            for( const auto &maps : tmp_map ) {
                const efftype_id id( maps.first );
                if( !id.is_valid() ) {
                    debugmsg( "Invalid effect: %s", id.c_str() );
                    continue;
                }
                for( const auto &i : maps.second ) {
                    if( !( std::istringstream( i.first ) >> key_num ) ) {
                        key_num = 0;
                    }
                    const bodypart_str_id &bp = convert_bp( static_cast<body_part>( key_num ) );
                    const effect &e = i.second;

                    ( *effects )[id][bp] = e;
                    on_effect_int_change( id, e.get_intensity(), bp );
                }
            }
        }
    } else {
        jsin.read( "effects", *effects );
    }

    jsin.read( "values", values );

    jsin.read( "damage_over_time_map", damage_over_time_map );

    jsin.read( "blocks_left", num_blocks );
    jsin.read( "dodges_left", num_dodges );
    jsin.read( "num_blocks_bonus", num_blocks_bonus );
    jsin.read( "num_dodges_bonus", num_dodges_bonus );

    jsin.read( "armor_bash_bonus", armor_bash_bonus );
    jsin.read( "armor_cut_bonus", armor_cut_bonus );
    jsin.read( "armor_bullet_bonus", armor_bullet_bonus );

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

    jsin.read( "throw_resist", throw_resist );

    jsin.read( "archery_aim_counter", archery_aim_counter );

    if( !jsin.read( "last_updated", last_updated ) ) {
        last_updated = calendar::turn;
    }

    jsin.read( "underwater", underwater );

    jsin.read( "body", body );

    fake = false; // see Creature::load

    on_stat_change( "pain", pain );
}

void player_morale::morale_point::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();
    if( !jo.read( "type", type ) ) {
        type = morale_type_data::convert_legacy( jo.get_int( "type_enum" ) );
    }
    itype_id tmpitype;
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

void player_morale::load( const JsonObject &jsin )
{
    jsin.allow_omitted_members();
    jsin.read( "morale", points );
}

struct mm_elem {
    memorized_terrain_tile tile;
    int symbol;

    bool operator==( const mm_elem &rhs ) const {
        return symbol == rhs.symbol && tile == rhs.tile;
    }
};

void mm_submap::serialize( JsonOut &jsout ) const
{
    jsout.start_array();

    // Uses RLE for compression.

    mm_elem last;
    int num_same = 1;

    const auto write_seq = [&]() {
        jsout.start_array();
        jsout.write( last.tile.tile );
        jsout.write( last.tile.subtile );
        jsout.write( last.tile.rotation );
        jsout.write( last.symbol );
        if( num_same != 1 ) {
            jsout.write( num_same );
        }
        jsout.end_array();
    };

    for( size_t y = 0; y < SEEY; y++ ) {
        for( size_t x = 0; x < SEEX; x++ ) {
            point p( x, y );
            const mm_elem elem = { tile( p ), symbol( p ) };
            if( x == 0 && y == 0 ) {
                last = elem;
                continue;
            }
            if( last == elem ) {
                num_same += 1;
                continue;
            }
            write_seq();
            num_same = 1;
            last = elem;
        }
    }
    write_seq();

    jsout.end_array();
}

void mm_submap::deserialize( const JsonValue &ja )
{
    // Uses RLE for compression.

    JsonArray sm_json = ja;

    mm_elem elem;
    size_t remaining = 0;

    for( size_t y = 0; y < SEEY; y++ ) {
        for( size_t x = 0; x < SEEX; x++ ) {
            if( remaining > 0 ) {
                remaining -= 1;
            } else {
                JsonArray elem_json = sm_json.next_array();
                elem.tile.tile = elem_json.next_string();
                elem.tile.subtile = elem_json.next_int();
                elem.tile.rotation = elem_json.next_int();
                elem.symbol = elem_json.next_int();
                if( elem_json.size() > 4 ) {
                    remaining = elem_json.next_int() - 1;
                }
                if( elem_json.has_more() ) {
                    elem_json.throw_error( "Too many values for RLE" );
                }
            }
            point p( x, y );
            // Try to avoid assigning to save up on memory
            if( elem.tile != mm_submap::default_tile ) {
                set_tile( p, elem.tile );
            }
            if( elem.symbol != mm_submap::default_symbol ) {
                set_symbol( p, elem.symbol );
            }
        }
    }
}

void mm_region::serialize( JsonOut &jsout ) const
{
    jsout.start_array();
    for( size_t y = 0; y < MM_REG_SIZE; y++ ) {
        // NOLINTNEXTLINE(modernize-loop-convert)
        for( size_t x = 0; x < MM_REG_SIZE; x++ ) {
            const shared_ptr_fast<mm_submap> &sm = submaps[x][y];
            if( sm->is_empty() ) {
                jsout.write_null();
            } else {
                sm->serialize( jsout );
            }
        }
    }
    jsout.end_array();
}

void mm_region::deserialize( const JsonValue &ja )
{
    JsonArray region_json = ja;
    for( size_t y = 0; y < MM_REG_SIZE; y++ ) {
        // NOLINTNEXTLINE(modernize-loop-convert)
        for( size_t x = 0; x < MM_REG_SIZE; x++ ) {
            shared_ptr_fast<mm_submap> &sm = submaps[x][y];
            sm = make_shared_fast<mm_submap>();
            JsonValue jsin = region_json.next_value();
            if( !jsin.test_null() ) {
                sm->deserialize( jsin );
            }
        }
    }
}

void map_memory::load_legacy( const JsonValue &jv )
{
    struct mig_elem {
        int symbol;
        memorized_terrain_tile tile;
    };
    std::map<tripoint, mig_elem> elems;

    JsonArray root_array = jv;
    for( JsonArray elem_json : root_array.next_array() ) {
        tripoint p;
        p.x = elem_json.next_int();
        p.y = elem_json.next_int();
        p.z = elem_json.next_int();
        mig_elem &elem = elems[p];
        elem.tile.tile = elem_json.next_string();
        elem.tile.subtile = elem_json.next_int();
        elem.tile.rotation = elem_json.next_int();
        if( elem_json.has_more() ) {
            elem_json.throw_error( "Too many values for map memory entry" );
        }
    }

    for( JsonArray symbols_json : root_array.next_array() ) {
        tripoint p;
        p.x = symbols_json.next_int();
        p.y = symbols_json.next_int();
        p.z = symbols_json.next_int();
        elems[p].symbol = symbols_json.next_int();
        if( symbols_json.has_more() ) {
            symbols_json.throw_error( "Too many values for map memory symbol" );
        }
    }

    for( const std::pair<const tripoint, mig_elem> &elem : elems ) {
        coord_pair cp( elem.first );
        shared_ptr_fast<mm_submap> sm = find_submap( cp.sm );
        if( !sm ) {
            sm = allocate_submap( cp.sm );
        }
        if( elem.second.tile != mm_submap::default_tile ) {
            sm->set_tile( cp.loc, elem.second.tile );
        }
        if( elem.second.symbol != mm_submap::default_symbol ) {
            sm->set_symbol( cp.loc, elem.second.symbol );
        }
    }
}

void point::deserialize( const JsonArray &jsin )
{
    x = jsin.get_int( 0 );
    y = jsin.get_int( 1 );
    if( jsin.size() > 2 ) {
        jsin.throw_error( "Too many values for tripoint" );
    }
}

void point::serialize( JsonOut &jsout ) const
{
    jsout.start_array();
    jsout.write( x );
    jsout.write( y );
    jsout.end_array();
}

void tripoint::deserialize( const JsonArray &jsin )
{
    x = jsin.get_int( 0 );
    y = jsin.get_int( 1 );
    z = jsin.get_int( 2 );
    if( jsin.size() > 3 ) {
        jsin.throw_error( "Too many values for tripoint" );
    }
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
    json.member( "type", type );
    json.member( "intensity", intensity );
    json.member( "sated", sated );
    json.end_object();
}

void addiction::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();
    // legacy
    if( jo.has_int( "type_enum" ) ) {
        enum class add_type_legacy : int {
            NONE,
            CAFFEINE,
            ALCOHOL,
            SLEEP,
            PKILLER,
            SPEED,
            CIG,
            COKE,
            CRACK,
            MUTAGEN,
            DIAZEPAM,
            MARLOSS_R,
            MARLOSS_B,
            MARLOSS_Y,
            NUM_ADD_TYPES
        };
        switch( static_cast<add_type_legacy>( jo.get_int( "type_enum" ) ) ) {
            case add_type_legacy::CAFFEINE:
                type = STATIC( addiction_id( "caffeine" ) );
                break;
            case add_type_legacy::ALCOHOL:
                type = STATIC( addiction_id( "alcohol" ) );
                break;
            case add_type_legacy::SLEEP:
                type = STATIC( addiction_id( "sleeping pill" ) );
                break;
            case add_type_legacy::PKILLER:
                type = STATIC( addiction_id( "opiate" ) );
                break;
            case add_type_legacy::SPEED:
                type = STATIC( addiction_id( "amphetamine" ) );
                break;
            case add_type_legacy::CIG:
                type = STATIC( addiction_id( "nicotine" ) );
                break;
            case add_type_legacy::COKE:
                type = STATIC( addiction_id( "cocaine" ) );
                break;
            case add_type_legacy::CRACK:
                type = STATIC( addiction_id( "crack" ) );
                break;
            case add_type_legacy::MUTAGEN:
                type = STATIC( addiction_id( "mutagen" ) );
                break;
            case add_type_legacy::DIAZEPAM:
                type = STATIC( addiction_id( "diazepam" ) );
                break;
            case add_type_legacy::MARLOSS_R:
                type = STATIC( addiction_id( "marloss_r" ) );
                break;
            case add_type_legacy::MARLOSS_B:
                type = STATIC( addiction_id( "marloss_b" ) );
                break;
            case add_type_legacy::MARLOSS_Y:
                type = STATIC( addiction_id( "marloss_y" ) );
                break;
            case add_type_legacy::NONE:
            case add_type_legacy::NUM_ADD_TYPES:
            default:
                type = addiction_id::NULL_ID();
                break;
        }
    } else {
        jo.read( "type", type );
    }
    intensity = jo.get_int( "intensity" );
    jo.read( "sated", sated );
}

void serialize( const recipe_subset &value, JsonOut &jsout )
{
    jsout.start_array();
    for( const recipe * const &entry : value ) {
        jsout.write( entry->ident() );
    }
    jsout.end_array();
}

void deserialize( recipe_subset &value, const JsonArray &ja )
{
    value.clear();
    for( std::string && recipe_id_string : ja ) {
        value.include( &recipe_id( std::move( recipe_id_string ) ).obj() );
    }
}

static void serialize( const item_comp &value, JsonOut &jsout )
{
    jsout.start_object();

    jsout.member( "type", value.type );
    jsout.member( "count", value.count );
    jsout.member( "recoverable", value.recoverable );

    jsout.end_object();
}

static void serialize( const tool_comp &value, JsonOut &jsout )
{
    jsout.start_object();

    jsout.member( "type", value.type );
    jsout.member( "count", value.count );
    jsout.member( "recoverable", value.recoverable );

    jsout.end_object();
}

static void deserialize( item_comp &value, const JsonObject &jo )
{
    jo.allow_omitted_members();
    jo.read( "type", value.type );
    jo.read( "count", value.count );
    jo.read( "recoverable", value.recoverable );
}

static void deserialize( tool_comp &value, const JsonObject &jo )
{
    jo.allow_omitted_members();
    jo.read( "type", value.type );
    jo.read( "count", value.count );
    jo.read( "recoverable", value.recoverable );
}

static void serialize( const quality_requirement &value, JsonOut &jsout )
{
    jsout.start_object();

    jsout.member( "type", value.type );
    jsout.member( "count", value.count );
    jsout.member( "level", value.level );

    jsout.end_object();
}

static void deserialize( quality_requirement &value, const JsonObject &jo )
{
    jo.allow_omitted_members();
    jo.read( "type", value.type );
    jo.read( "count", value.count );
    jo.read( "level", value.level );
}

// basecamp
void basecamp::serialize( JsonOut &json ) const
{
    if( omt_pos != tripoint_abs_omt() ) {
        json.start_object();
        json.member( "name", name );
        json.member( "pos", omt_pos );
        json.member( "bb_pos", bb_pos );
        json.member( "dumping_spot", dumping_spot );
        json.member( "hidden_missions" );
        json.start_array();
        for( const auto &list : hidden_missions ) {
            json.start_object();
            json.member( "dir" );
            json.start_array();
            for( const ui_mission_id &miss_id : list ) {
                json.start_object();
                json.member( "mission_id", string_of( miss_id.id ) );
                json.end_object();
            }
            json.end_array();
            json.end_object();
        }
        json.end_array();
        json.member( "expansions" );
        json.start_array();
        for( const auto &expansion : expansions ) {
            json.start_object();
            json.member( "dir", expansion.first );
            json.member( "type", expansion.second.type );
            json.member( "provides" );
            json.start_array();
            for( const auto &provide : expansion.second.provides ) {
                json.start_object();
                json.member( "id", provide.first );
                json.member( "amount", provide.second );
                json.end_object();
            }
            json.end_array();
            json.member( "in_progress" );
            json.start_array();
            for( const auto &working : expansion.second.in_progress ) {
                json.start_object();
                json.member( "id", working.first );
                json.member( "amount", working.second );
                json.end_object();
            }
            json.end_array();
            json.member( "pos", expansion.second.pos );
            json.end_object();
        }
        json.end_array();
        json.member( "fortifications" );
        json.start_array();
        for( const auto &fortification : fortifications ) {
            json.start_object();
            json.member( "pos", fortification );
            json.end_object();
        }
        json.end_array();
        json.member( "salt_water_pipes" );
        json.start_array();
        for( expansion_salt_water_pipe * const &pipe : salt_water_pipes ) {
            json.start_object();
            json.member( "expansion", pipe->expansion );
            json.member( "connection_direction", pipe->connection_direction );
            json.member( "segments" );
            json.start_array();
            for( const expansion_salt_water_pipe_segment &segment : pipe->segments ) {
                json.start_object();
                json.member( "point", segment.point );
                json.member( "started", segment.started );
                json.member( "finished", segment.finished );
                json.end_object();
            }
            json.end_array();
            json.end_object();
        }
        json.end_array();
        json.end_object();

    } else {
        return;
    }
}

void basecamp::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "name", name );
    data.read( "pos", omt_pos );
    data.read( "bb_pos", bb_pos );
    data.read( "dumping_spot", dumping_spot );
    for( int tab_num = base_camps::TAB_MAIN; tab_num <= base_camps::TAB_NW; tab_num++ ) {
        std::vector<ui_mission_id> temp;
        hidden_missions.push_back( temp );
    }
    int tab_num = base_camps::TAB_MAIN;
    for( JsonObject list : data.get_array( "hidden_missions" ) ) {
        list.allow_omitted_members();
        for( JsonObject miss_id_json : list.get_array( "dir" ) ) {
            miss_id_json.allow_omitted_members();
            std::string miss_id_string;
            miss_id_json.read( "mission_id", miss_id_string );
            ui_mission_id miss_id;
            miss_id.id = mission_id_of( miss_id_string );
            miss_id.ret = false;
            hidden_missions[tab_num].push_back( miss_id );
        }
        tab_num++;
    }

    for( JsonObject edata : data.get_array( "expansions" ) ) {
        edata.allow_omitted_members();
        expansion_data e;
        point dir;
        if( edata.has_string( "dir" ) ) {
            // old save compatibility
            const std::string dir_id = edata.get_string( "dir" );
            dir = base_camps::direction_from_id( dir_id );
        } else {
            edata.read( "dir", dir );
        }
        edata.read( "type", e.type );
        if( edata.has_int( "cur_level" ) ) {
            edata.read( "cur_level", e.cur_level );
        }
        if( edata.has_array( "provides" ) ) {
            e.cur_level = -1;
            for( JsonObject provide_data : edata.get_array( "provides" ) ) {
                provide_data.allow_omitted_members();
                std::string id = provide_data.get_string( "id" );
                int amount = provide_data.get_int( "amount" );
                e.provides[ id ] = amount;
            }
        }
        // in case of save corruption, sanity check provides from expansions
        const std::string &initial_provide = base_camps::faction_encode_abs( e, 0 );
        if( e.provides.find( initial_provide ) == e.provides.end() ) {
            e.provides[ initial_provide ] = 1;
        }
        for( JsonObject in_progress_data : edata.get_array( "in_progress" ) ) {
            in_progress_data.allow_omitted_members();
            std::string id = in_progress_data.get_string( "id" );
            int amount = in_progress_data.get_int( "amount" );
            e.in_progress[ id ] = amount;
        }
        edata.read( "pos", e.pos );
        expansions[ dir ] = e;
        if( dir != base_camps::base_dir ) {
            directions.push_back( dir );
        }
    }
    for( JsonObject edata : data.get_array( "fortifications" ) ) {
        edata.allow_omitted_members();
        tripoint_abs_omt restore_pos;
        edata.read( "pos", restore_pos );
        fortifications.push_back( restore_pos );
    }

    for( JsonObject edata : data.get_array( "salt_water_pipes" ) ) {
        edata.allow_omitted_members();
        expansion_salt_water_pipe *pipe = new expansion_salt_water_pipe;
        edata.read( "expansion", pipe->expansion );
        edata.read( "connection_direction", pipe->connection_direction );
        for( JsonObject seg : edata.get_array( "segments" ) ) {
            seg.allow_omitted_members();
            expansion_salt_water_pipe_segment segment;
            seg.read( "point", segment.point );
            seg.read( "started", segment.started );
            seg.read( "finished", segment.finished );
            pipe->segments.push_back( segment );
        }
        salt_water_pipes.push_back( pipe );
    }
}

void kill_tracker::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "kills" );
    jsout.start_object();
    for( const auto &elem : kills ) {
        jsout.member( elem.first.str(), elem.second );
    }
    jsout.end_object();

    jsout.member( "npc_kills" );
    jsout.start_array();
    for( const auto &elem : npc_kills ) {
        jsout.write( elem );
    }
    jsout.end_array();
    jsout.end_object();
}

void kill_tracker::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    for( const JsonMember member : data.get_object( "kills" ) ) {
        kills[mtype_id( member.name() )] = member.get_int();
    }

    for( const std::string npc_name : data.get_array( "npc_kills" ) ) {
        npc_kills.push_back( npc_name );
    }
}

void cata_variant::serialize( JsonOut &jsout ) const
{
    jsout.start_array();
    jsout.write_as_string( type_ );
    jsout.write( value_ );
    jsout.end_array();
}

void cata_variant::deserialize( const JsonValue &jsin )
{
    if( jsin.test_int() ) {
        *this = cata_variant::make<cata_variant_type::int_>( jsin.get_int() );
    } else if( jsin.test_bool() ) {
        *this = cata_variant::make<cata_variant_type::bool_>( jsin.get_bool() );
    } else {
        JsonArray ja = jsin.get_array();
        // FIXME: add_type migration - remove after 0.G
        if( ja.get_string( 0 ) == "add_type" ) {
            type_ = cata_variant_type::addiction_id;
            value_ = add_type_legacy_conv( ja.get_string( 1 ) );
        } else if( !( ja.read_next( type_ ) && ja.read_next( value_ ) ) ) {
            ja.throw_error( "Failed to read cata_variant" );
        }
        if( ja.size() > 2 ) {
            ja.throw_error( "Too many values for cata_variant" );
        }
    }
}

void event_multiset::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    std::vector<summaries_type::value_type> copy( summaries_.begin(), summaries_.end() );
    jsout.member( "event_counts", copy );
    jsout.end_object();
}

void event_multiset::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();
    JsonArray events = jo.get_array( "event_counts" );
    if( !events.empty() && events.get_array( 0 ).has_int( 1 ) ) {
        // TEMPORARY until 0.F
        // Read legacy format with just ints
        std::vector<std::pair<cata::event::data_type, int>> copy;
        jo.read( "event_counts", copy );
        summaries_.clear();
        for( const std::pair<cata::event::data_type, int> &p : copy ) {
            event_summary summary{ p.second, calendar::start_of_game, calendar::start_of_game };
            summaries_.emplace( p.first, summary );
        }
    } else {
        // Read actual summaries
        std::vector<std::pair<cata::event::data_type, event_summary>> copy;
        jo.read( "event_counts", copy );
        summaries_ = { copy.begin(), copy.end() };
    }
}

void stats_tracker::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "data", data );
    jsout.member( "initial_scores", initial_scores );
    jsout.end_object();
}

void stats_tracker::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();
    jo.read( "data", data );
    for( std::pair<const event_type, event_multiset> &d : data ) {
        d.second.set_type( d.first );
    }
    jo.read( "initial_scores", initial_scores );
}

void submap::store( JsonOut &jsout ) const
{
    jsout.member( "turn_last_touched", last_touched );
    jsout.member( "temperature", temperature_mod );

    // Terrain is saved using a simple RLE scheme.  Legacy saves don't have
    // this feature but the algorithm is backward compatible.
    jsout.member( "terrain" );
    jsout.start_array();
    std::string last_id;
    int num_same = 1;
    for( int j = 0; j < SEEY; j++ ) {
        // NOLINTNEXTLINE(modernize-loop-convert)
        for( int i = 0; i < SEEX; i++ ) {
            const std::string this_id = ter[i][j].obj().id.str();
            if( !last_id.empty() ) {
                if( this_id == last_id ) {
                    num_same++;
                } else {
                    if( num_same == 1 ) {
                        // if there's only one element don't write as an array
                        jsout.write( last_id );
                    } else {
                        jsout.start_array();
                        jsout.write( last_id );
                        jsout.write( num_same );
                        jsout.end_array();
                        num_same = 1;
                    }
                    last_id = this_id;
                }
            } else {
                last_id = this_id;
            }
        }
    }
    // Because of the RLE scheme we have to do one last pass
    if( num_same == 1 ) {
        jsout.write( last_id );
    } else {
        jsout.start_array();
        jsout.write( last_id );
        jsout.write( num_same );
        jsout.end_array();
    }
    jsout.end_array();

    // Write out the radiation array in a simple RLE scheme.
    // written in intensity, count pairs
    jsout.member( "radiation" );
    jsout.start_array();
    int lastrad = -1;
    int count = 0;
    for( int j = 0; j < SEEY; j++ ) {
        for( int i = 0; i < SEEX; i++ ) {
            const point p( i, j );
            // Save radiation, re-examine this because it doesn't look like it works right
            int r = get_radiation( p );
            if( r == lastrad ) {
                count++;
            } else {
                if( count ) {
                    jsout.write( count );
                }
                jsout.write( r );
                lastrad = r;
                count = 1;
            }
        }
    }
    jsout.write( count );
    jsout.end_array();

    jsout.member( "furniture" );
    jsout.start_array();
    for( int j = 0; j < SEEY; j++ ) {
        for( int i = 0; i < SEEX; i++ ) {
            const point p( i, j );
            // Save furniture
            if( get_furn( p ) ) {
                jsout.start_array();
                jsout.write( p.x );
                jsout.write( p.y );
                jsout.write( get_furn( p ).obj().id );
                jsout.end_array();
            }
        }
    }
    jsout.end_array();

    jsout.member( "items" );
    jsout.start_array();
    for( int j = 0; j < SEEY; j++ ) {
        for( int i = 0; i < SEEX; i++ ) {
            if( itm[i][j].empty() ) {
                continue;
            }
            jsout.write( i );
            jsout.write( j );
            jsout.write( itm[i][j] );
        }
    }
    jsout.end_array();

    jsout.member( "traps" );
    jsout.start_array();
    for( int j = 0; j < SEEY; j++ ) {
        for( int i = 0; i < SEEX; i++ ) {
            const point p( i, j );
            // Save traps
            if( get_trap( p ) ) {
                jsout.start_array();
                jsout.write( p.x );
                jsout.write( p.y );
                // TODO: jsout should support writing an id like jsout.write( trap_id )
                jsout.write( get_trap( p ).id().str() );
                jsout.end_array();
            }
        }
    }
    jsout.end_array();

    jsout.member( "fields" );
    jsout.start_array();
    for( int j = 0; j < SEEY; j++ ) {
        for( int i = 0; i < SEEX; i++ ) {
            // Save fields
            if( fld[i][j].field_count() > 0 ) {
                jsout.write( i );
                jsout.write( j );
                jsout.start_array();
                for( const auto &elem : fld[i][j] ) {
                    const field_entry &cur = elem.second;
                    jsout.write( cur.get_field_type().id() );
                    jsout.write( cur.get_field_intensity() );
                    jsout.write( cur.get_field_age() );
                }
                jsout.end_array();
            }
        }
    }
    jsout.end_array();

    // Write out as array of arrays of single entries
    jsout.member( "cosmetics" );
    jsout.start_array();
    for( const submap::cosmetic_t &cosm : cosmetics ) {
        jsout.start_array();
        jsout.write( cosm.pos.x );
        jsout.write( cosm.pos.y );
        jsout.write( cosm.type );
        jsout.write( cosm.str );
        jsout.end_array();
    }
    jsout.end_array();

    // Output the spawn points
    jsout.member( "spawns" );
    jsout.start_array();
    for( const spawn_point &elem : spawns ) {
        jsout.start_array();
        // TODO: json should know how to write string_ids
        jsout.write( elem.type.str() );
        jsout.write( elem.count );
        jsout.write( elem.pos.x );
        jsout.write( elem.pos.y );
        jsout.write( elem.faction_id );
        jsout.write( elem.mission_id );
        jsout.write( elem.friendly );
        jsout.write( elem.name );
        jsout.end_array();
    }
    jsout.end_array();

    jsout.member( "vehicles" );
    jsout.start_array();
    for( const auto &elem : vehicles ) {
        // json lib doesn't know how to turn a vehicle * into a vehicle,
        // so we have to iterate manually.
        jsout.write( *elem );
    }
    jsout.end_array();

    jsout.member( "partial_constructions" );
    jsout.start_array();
    for( const auto &elem : partial_constructions ) {
        jsout.write( elem.first.x() );
        jsout.write( elem.first.y() );
        jsout.write( elem.first.z() );
        jsout.write( elem.second.counter );
        jsout.write( elem.second.id.id() );
        jsout.start_array();
        for( const item &it : elem.second.components ) {
            jsout.write( it );
        }
        jsout.end_array();
    }
    jsout.end_array();

    if( legacy_computer ) {
        // it's possible that no access to computers has been made and legacy_computer
        // is not cleared
        jsout.member( "computers", *legacy_computer );
    } else if( !computers.empty() ) {
        jsout.member( "computers" );
        jsout.start_array();
        for( const auto &elem : computers ) {
            jsout.write( elem.first );
            jsout.write( elem.second );
        }
        jsout.end_array();
    }

    // Output base camp if any
    if( camp ) {
        jsout.member( "camp", *camp );
    }
}

void submap::load( const JsonValue &jv, const std::string &member_name, int version )
{
    bool rubpow_update = version < 22;
    if( member_name == "turn_last_touched" ) {
        last_touched = time_point( jv.get_int() );
    } else if( member_name == "temperature" ) {
        temperature_mod = jv.get_int();
    } else if( member_name == "terrain" ) {
        // TODO: try block around this to error out if we come up short?
        JsonArray terrain_json = jv;
        // Small duplication here so that the update check is only performed once
        if( rubpow_update ) {
            item rock = item( "rock", calendar::turn_zero );
            item chunk = item( "steel_chunk", calendar::turn_zero );
            for( int j = 0; j < SEEY; j++ ) {
                for( int i = 0; i < SEEX; i++ ) {
                    const ter_str_id tid( terrain_json.next_string() );

                    if( tid == ter_t_rubble ) {
                        ter[i][j] = ter_id( "t_dirt" );
                        frn[i][j] = furn_id( "f_rubble" );
                        itm[i][j].insert( rock );
                        itm[i][j].insert( rock );
                    } else if( tid == ter_t_wreckage ) {
                        ter[i][j] = ter_id( "t_dirt" );
                        frn[i][j] = furn_id( "f_wreckage" );
                        itm[i][j].insert( chunk );
                        itm[i][j].insert( chunk );
                    } else if( tid == ter_t_ash ) {
                        ter[i][j] = ter_id( "t_dirt" );
                        frn[i][j] = furn_id( "f_ash" );
                    } else if( tid == ter_t_pwr_sb_support_l ) {
                        ter[i][j] = ter_id( "t_support_l" );
                    } else if( tid == ter_t_pwr_sb_switchgear_l ) {
                        ter[i][j] = ter_id( "t_switchgear_l" );
                    } else if( tid == ter_t_pwr_sb_switchgear_s ) {
                        ter[i][j] = ter_id( "t_switchgear_s" );
                    } else {
                        ter[i][j] = tid.id();
                    }
                }
            }
        } else {
            // terrain is encoded using simple RLE
            int remaining = 0;
            int_id<ter_t> iid;
            for( int j = 0; j < SEEY; j++ ) {
                // NOLINTNEXTLINE(modernize-loop-convert)
                for( int i = 0; i < SEEX; i++ ) {
                    if( !remaining ) {
                        JsonValue terrain_entry = terrain_json.next_value();
                        if( terrain_entry.test_string() ) {
                            iid = ter_str_id( terrain_entry.get_string() ).id();
                        } else if( terrain_entry.test_array() ) {
                            JsonArray terrain_rle = terrain_entry;
                            iid = ter_str_id( terrain_rle.next_string() ).id();
                            remaining = terrain_rle.next_int() - 1;
                            if( terrain_rle.size() > 2 ) {
                                terrain_rle.throw_error( "Too many values for terrain RLE" );
                            }
                        } else {
                            debugmsg( "Mapbuffer terrain data is corrupt, expected string or array." );
                        }
                    } else {
                        --remaining;
                    }
                    ter[i][j] = iid;
                }
            }
            if( remaining ) {
                debugmsg( "Mapbuffer terrain data is corrupt, tile data remaining." );
            }
        }
    } else if( member_name == "radiation" ) {
        int rad_cell = 0;
        JsonArray radiation_rle = jv;
        while( radiation_rle.has_more() ) {
            int rad_strength = radiation_rle.next_int();
            int rad_num = radiation_rle.next_int();
            for( int i = 0; i < rad_num; ++i ) {
                if( rad_cell < SEEX * SEEY ) {
                    set_radiation( { rad_cell % SEEX, rad_cell / SEEX }, rad_strength );
                    rad_cell++;
                }
            }
        }
    } else if( member_name == "furniture" ) {
        JsonArray furniture_json = jv;
        for( JsonArray furniture_entry : furniture_json ) {
            int i = furniture_entry.next_int();
            int j = furniture_entry.next_int();
            frn[i][j] = furn_id( furniture_entry.next_string() );
            if( furniture_entry.size() > 3 ) {
                furniture_entry.throw_error( "Too many values for furniture entry." );
            }
        }
    } else if( member_name == "items" ) {
        JsonArray items_json = jv;
        while( items_json.has_more() ) {
            int i = items_json.next_int();
            int j = items_json.next_int();
            const point p( i, j );

            if( !items_json.next_value().read( itm[p.x][p.y], false ) ) {
                debugmsg( "Items array is corrupt in submap at: %s, skipping", p.to_string() );
            }
            // some portion could've been read even if error occurred
            for( item &it : itm[p.x][p.y] ) {
                if( it.is_emissive() ) {
                    update_lum_add( p, it );
                }
                if( it.needs_processing() ) {
                    active_items.add( it, p );
                }
                if( savegame_loading_version < 33 ) {
                    // remove after 0.F
                    migrate_item_charges( it );
                }
            }
        }
    } else if( member_name == "traps" ) {
        JsonArray traps_json = jv;
        for( JsonArray trap_entry : traps_json ) {
            int i = trap_entry.next_int();
            int j = trap_entry.next_int();
            const point p( i, j );
            // TODO: jsin should support returning an id like jsin.get_id<trap>()
            const trap_str_id trid( trap_entry.next_string() );
            trp[p.x][p.y] = trid.id();
            if( trap_entry.size() > 3 ) {
                trap_entry.throw_error( "Too many values for trap entry" );
            }
        }
    } else if( member_name == "fields" ) {
        JsonArray fields_json = jv;
        while( fields_json.has_more() ) {
            // Coordinates loop
            int i = fields_json.next_int();
            int j = fields_json.next_int();
            JsonArray field_json = fields_json.next_array();
            while( field_json.has_more() ) {
                // TODO: Check enum->string migration below
                int type_int = 0;
                std::string type_str;
                JsonValue type_value = field_json.next_value();
                if( type_value.test_int() ) {
                    type_int = type_value.get_int();
                } else {
                    type_str = type_value.get_string();
                }
                int intensity = field_json.next_int();
                int age = field_json.next_int();
                field_type_id ft;
                if( !type_str.empty() ) {
                    ft = field_type_id( type_str );
                } else {
                    ft = field_types::get_field_type_by_legacy_enum( type_int ).id;
                }
                if( fld[i][j].add_field( ft, intensity, time_duration::from_turns( age ) ) ) {
                    field_count++;
                }
            }
        }
    } else if( member_name == "graffiti" ) {
        JsonArray graffiti_json = jv;
        for( JsonArray graffiti_entry : graffiti_json ) {
            int i = graffiti_entry.next_int();
            int j = graffiti_entry.next_int();
            const point p( i, j );
            set_graffiti( p, graffiti_entry.next_string() );
            if( graffiti_entry.size() > 3 ) {
                graffiti_entry.throw_error( "Too many values for graffiti" );
            }
        }
    } else if( member_name == "cosmetics" ) {
        JsonArray cosmetics_json = jv;
        std::map<std::string, std::string> tcosmetics;

        for( JsonArray cosmetic_entry : cosmetics_json ) {
            int i = cosmetic_entry.next_int();
            int j = cosmetic_entry.next_int();
            const point p( i, j );
            std::string type;
            std::string str;
            JsonValue cosmetic_value = cosmetic_entry.next_value();
            // Try to read as current format
            if( cosmetic_value.test_string() ) {
                type = cosmetic_value.get_string();
                str = cosmetic_entry.next_string();
                insert_cosmetic( p, type, str );
            } else {
                // Otherwise read as most recent old format
                cosmetic_value.read( tcosmetics );
                for( auto &cosm : tcosmetics ) {
                    insert_cosmetic( p, cosm.first, cosm.second );
                }
                tcosmetics.clear();
            }
            if( cosmetic_entry.has_more() ) {
                cosmetic_entry.throw_error( "Too many values for cosmetics" );
            }
        }
    } else if( member_name == "spawns" ) {
        JsonArray spawns_json = jv;
        for( JsonArray spawn_entry : spawns_json ) {
            // TODO: json should know how to read an string_id
            const mtype_id type = mtype_id( spawn_entry.next_string() );
            int count = spawn_entry.next_int();
            int i = spawn_entry.next_int();
            int j = spawn_entry.next_int();
            const point p( i, j );
            int faction_id = spawn_entry.next_int();
            int mission_id = spawn_entry.next_int();
            bool friendly = spawn_entry.next_bool();
            std::string name = spawn_entry.next_string();
            if( spawn_entry.has_more() ) {
                spawn_entry.throw_error( "Too many values for spawn" );
            }
            spawn_point tmp( type, count, p, faction_id, mission_id, friendly, name );
            spawns.push_back( tmp );
        }
    } else if( member_name == "vehicles" ) {
        JsonArray vehicles_json = jv;
        for( JsonValue vehicle_json : vehicles_json ) {
            std::unique_ptr<vehicle> tmp = std::make_unique<vehicle>();
            try {
                tmp->deserialize( vehicle_json );
                vehicles.push_back( std::move( tmp ) );
            } catch( const JsonError &err ) {
                debugmsg( err.what() );
            }
        }
    } else if( member_name == "partial_constructions" ) {
        JsonArray partial_constructions_json = jv;
        while( partial_constructions_json.has_more() ) {
            partial_con pc;
            int i = partial_constructions_json.next_int();
            int j = partial_constructions_json.next_int();
            int k = partial_constructions_json.next_int();
            tripoint_sm_ms pt( i, j, k );
            pc.counter = partial_constructions_json.next_int();
            JsonValue construction_id_value = partial_constructions_json.next_value();
            if( construction_id_value.test_int() ) {
                // Oops, int id incorrectly saved by legacy code, just load it and hope for the best
                pc.id = construction_id( construction_id_value.get_int() );
            } else {
                pc.id = construction_str_id( construction_id_value.get_string() ).id();
            }
            JsonArray components_json = partial_constructions_json.next_array();
            for( JsonObject component_json : components_json ) {
                item tmp;
                tmp.deserialize( component_json );
                pc.components.push_back( tmp );
            }
            partial_constructions[pt] = pc;
        }
    } else if( member_name == "computers" ) {
        if( jv.test_array() ) {
            JsonArray computers_json = jv;
            while( computers_json.has_more() ) {
                point loc;
                computers_json.next_value().read( loc );
                auto new_comp_it = computers.emplace( loc, computer( "BUGGED_COMPUTER", -100,
                                                      tripoint_zero ) ).first;
                computers_json.next_value().read( new_comp_it->second );
            }
        } else {
            // only load legacy data here, but do not update to std::map, since
            // the terrain may not have been loaded yet.
            legacy_computer = std::make_unique<computer>( "BUGGED_COMPUTER", -100, tripoint_zero );
            legacy_computer->deserialize( jv );
        }
    } else if( member_name == "camp" ) {
        camp = std::make_unique<basecamp>();
        camp->deserialize( jv );
    }
}
