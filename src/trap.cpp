#include "trap.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

#include "assign.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "event.h"
#include "event_bus.h"
#include "generic_factory.h"
#include "item.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"

static const flag_id json_flag_SONAR_DETECTABLE( "SONAR_DETECTABLE" );

static const proficiency_id proficiency_prof_spotting( "prof_spotting" );
static const proficiency_id proficiency_prof_traps( "prof_traps" );
static const proficiency_id proficiency_prof_trapsetting( "prof_trapsetting" );

static const skill_id skill_traps( "traps" );

const trap_str_id tr_beartrap_buried( "tr_beartrap_buried" );
const trap_str_id tr_blade( "tr_blade" );
const trap_str_id tr_dissector( "tr_dissector" );
const trap_str_id tr_drain( "tr_drain" );
const trap_str_id tr_glow( "tr_glow" );
const trap_str_id tr_goo( "tr_goo" );
const trap_str_id tr_hum( "tr_hum" );
const trap_str_id tr_landmine( "tr_landmine" );
const trap_str_id tr_landmine_buried( "tr_landmine_buried" );
const trap_str_id tr_lava( "tr_lava" );
const trap_str_id tr_ledge( "tr_ledge" );
const trap_str_id tr_pit( "tr_pit" );
const trap_str_id tr_portal( "tr_portal" );
const trap_str_id tr_shadow( "tr_shadow" );
const trap_str_id tr_shotgun_1( "tr_shotgun_1" );
const trap_str_id tr_shotgun_2( "tr_shotgun_2" );
const trap_str_id tr_sinkhole( "tr_sinkhole" );
const trap_str_id tr_snake( "tr_snake" );
const trap_str_id tr_telepad( "tr_telepad" );
const trap_str_id tr_temple_flood( "tr_temple_flood" );
const trap_str_id tr_temple_toggle( "tr_temple_toggle" );

static const update_mapgen_id update_mapgen_none( "none" );

namespace
{

generic_factory<trap> trap_factory( "trap" );

} // namespace

/** @relates string_id */
template<>
inline bool int_id<trap>::is_valid() const
{
    return trap_factory.is_valid( *this );
}

/** @relates int_id */
template<>
const trap &int_id<trap>::obj() const
{
    return trap_factory.obj( *this );
}

/** @relates int_id */
template<>
const string_id<trap> &int_id<trap>::id() const
{
    return trap_factory.convert( *this );
}

/** @relates string_id */
template<>
int_id<trap> string_id<trap>::id() const
{
    return trap_factory.convert( *this, tr_null );
}

/** @relates string_id */
template<>
int_id<trap>::int_id( const string_id<trap> &id ) : _id( id.id() )
{
}

/** @relates string_id */
template<>
const trap &string_id<trap>::obj() const
{
    return trap_factory.obj( *this );
}

/** @relates int_id */
template<>
bool string_id<trap>::is_valid() const
{
    return trap_factory.is_valid( *this );
}

static std::vector<const trap *> funnel_traps;

const std::vector<const trap *> &trap::get_funnels()
{
    return funnel_traps;
}

static std::vector<const trap *> sound_triggered_traps;

const std::vector<const trap *> &trap::get_sound_triggered_traps()
{
    return sound_triggered_traps;
}

size_t trap::count()
{
    return trap_factory.size();
}

void trap::load_trap( const JsonObject &jo, const std::string &src )
{
    trap_factory.load( jo, src );
}

void trap::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", name_ );
    if( !assign( jo, "color", color ) ) {
        jo.throw_error( "missing mandatory member \"color\"" );
    }
    mandatory( jo, was_loaded, "symbol", sym, one_char_symbol_reader );
    mandatory( jo, was_loaded, "visibility", visibility );
    mandatory( jo, was_loaded, "avoidance", avoidance );
    mandatory( jo, was_loaded, "difficulty", difficulty );

    optional( jo, was_loaded, "memorial_male", memorial_male );
    optional( jo, was_loaded, "memorial_female", memorial_female );

    optional( jo, was_loaded, "trigger_message_u", trigger_message_u );
    optional( jo, was_loaded, "trigger_message_npc", trigger_message_npc );

    // Require either none, or both
    if( !!memorial_male != !!memorial_female ) {
        jo.throw_error_at(
            id.str(),
            "Only one gender of memorial message specified for trap " + id.str() + ", but none "
            "or both required." );
    }

    optional( jo, was_loaded, "flags", _flags );
    optional( jo, was_loaded, "trap_radius", trap_radius, 0 );
    // TODO: Is there a generic_factory version of this?
    act = trap_function_from_string( jo.get_string( "action" ) );

    optional( jo, was_loaded, "map_regen", map_regen, update_mapgen_none );
    optional( jo, was_loaded, "benign", benign, false );
    optional( jo, was_loaded, "always_invisible", always_invisible, false );
    optional( jo, was_loaded, "funnel_radius", funnel_radius_mm, 0 );
    optional( jo, was_loaded, "comfort", comfort, 0 );
    optional( jo, was_loaded, "floor_bedding_warmth", floor_bedding_warmth, 0 );
    optional( jo, was_loaded, "spell_data", spell_data );
    assign( jo, "trigger_weight", trigger_weight );
    optional( jo, was_loaded, "sound_threshold", sound_threshold );
    for( const JsonValue entry : jo.get_array( "drops" ) ) {
        itype_id item_type;
        int quantity = 0;
        int charges = 0;
        if( entry.test_object() ) {
            JsonObject jc = entry.get_object();
            jc.read( "item", item_type, true );
            quantity = jc.get_int( "quantity", 1 );
            charges = jc.get_int( "charges", 1 );
        } else {
            entry.read( item_type, true );
            quantity = 1;
            charges = 1;
        }
        if( !item_type.is_empty() && quantity > 0 && charges > 0 ) {
            components.emplace_back( item_type, quantity, charges );
        }
    }
    if( jo.has_object( "vehicle_data" ) ) {
        JsonObject jv = jo.get_object( "vehicle_data" );
        vehicle_data.remove_trap = jv.get_bool( "remove_trap", false );
        vehicle_data.do_explosion = jv.get_bool( "do_explosion", false );
        vehicle_data.is_falling = jv.get_bool( "is_falling", false );
        vehicle_data.chance = jv.get_int( "chance", 100 );
        vehicle_data.damage = jv.get_int( "damage", 0 );
        vehicle_data.shrapnel = jv.get_int( "shrapnel", 0 );
        vehicle_data.sound_volume = jv.get_int( "sound_volume", 0 );
        jv.read( "sound", vehicle_data.sound );
        vehicle_data.sound_type = jv.get_string( "sound_type", "" );
        vehicle_data.sound_variant = jv.get_string( "sound_variant", "" );
        vehicle_data.spawn_items.clear();
        if( jv.has_array( "spawn_items" ) ) {
            for( const JsonValue entry : jv.get_array( "spawn_items" ) ) {
                if( entry.test_object() ) {
                    JsonObject joitm = entry.get_object();
                    vehicle_data.spawn_items.emplace_back(
                        itype_id( joitm.get_string( "id" ) ), joitm.get_float( "chance" ) );
                } else {
                    vehicle_data.spawn_items.emplace_back( itype_id( entry.get_string() ), 1.0 );
                }
            }
        }
        vehicle_data.set_trap = trap_str_id::NULL_ID();
        if( jv.read( "set_trap", vehicle_data.set_trap ) ) {
            vehicle_data.remove_trap = false;
        }
    }
}

std::string trap::name() const
{
    return name_.translated();
}

update_mapgen_id trap::map_regen_target() const
{
    return map_regen;
}

void trap::reset()
{
    funnel_traps.clear();
    sound_triggered_traps.clear();
    trap_factory.reset();
}

bool trap::is_trivial_to_spot() const
{
    // @TODO technically the trap may not be detected even with visibility == 0, see trap::detect_trap
    return visibility <= 0 && !is_always_invisible();
}

bool trap::detected_by_ground_sonar() const
{
    return has_flag( json_flag_SONAR_DETECTABLE );
}

bool trap::detect_trap( const tripoint &pos, const Character &p ) const
{
    // * Buried landmines, the silent killer, have a visibility of 10.
    // Assuming no knowledge of traps or proficiencies, average per/int, and a focus of 50,
    // most characters will get a mean_roll of 6.
    // With a std deviation of 3, that leaves a 10% chance of spotting a landmine when you are next to it.
    // This gets worse if you are fatigued, or can't see as well.
    // Obviously it rapidly gets better as your skills improve.

    // Devices skill is helpful for spotting traps
    const float traps_skill_level = p.get_skill_level( skill_traps );

    // Perception is the main stat for spotting traps, int helps a bit.
    // In this case, stats are more important than skills.
    const float weighted_stat_average = ( 4.0f * p.per_cur + p.int_cur ) / 5.0f;

    // Eye encumbrance will penalize spotting
    const float encumbrance_penalty = p.encumb( bodypart_id( "eyes" ) ) / 10.0f;

    // Your current focus strongly affects your ability to spot things.
    const float focus_effect = ( p.get_focus() / 25.0f ) - 2.0f;

    // The further away the trap is, the harder it is to spot.
    // Subtract 1 so that we don't get an unfair penalty when not quite on top of the trap.
    const int distance_penalty = rl_dist( p.pos(), pos ) - 1;

    int proficiency_effect = -2;
    // Without at least a basic traps proficiency, your skill level is effectively 2 levels lower.
    if( p.has_proficiency( proficiency_prof_traps ) ) {
        proficiency_effect += 2;
        // If you have the basic traps prof, negate the above penalty
    }
    if( p.has_proficiency( proficiency_prof_spotting ) ) {
        proficiency_effect += 4;
        // If you have the spotting proficiency, add 4 levels.
    }
    if( p.has_proficiency( proficiency_prof_trapsetting ) ) {
        proficiency_effect += 1;
        // Knowing how to set traps gives you a small bonus to spotting them as well.
    }

    // For every 100 points of sleep deprivation after 200, reduce your roll by 1.
    // That represents a -2 at dead tired, -4 at exhausted, and so on.
    const float fatigue_penalty = std::min( 0, p.get_fatigue() - 200 ) / 100.0f;

    const float mean_roll = weighted_stat_average + ( traps_skill_level / 3.0f ) +
                            proficiency_effect +
                            focus_effect - distance_penalty - fatigue_penalty - encumbrance_penalty;

    const int roll = std::round( normal_roll( mean_roll, 3 ) );

    return roll > visibility;
}

// Whether or not, in the current state, the player can see the trap.
bool trap::can_see( const tripoint &pos, const Character &p ) const
{
    if( is_null() ) {
        // There is no trap at all, so logically one can not see it.
        return false;
    }
    if( is_always_invisible() ) {
        return false;
    }
    return visibility < 0 || p.knows_trap( pos );
}

void trap::trigger( const tripoint &pos ) const
{
    if( is_null() ) {
        return;
    }
    act( pos, nullptr, nullptr );
}

void trap::trigger( const tripoint &pos, Creature &creature ) const
{
    return trigger( pos, &creature, nullptr );
}

void trap::trigger( const tripoint &pos, item &item ) const
{
    return trigger( pos, nullptr, &item );
}

void trap::trigger( const tripoint &pos, Creature *creature, item *item ) const
{
    if( is_null() ) {
        return;
    }
    const bool is_real_creature = creature != nullptr && !creature->is_hallucination();
    if( is_real_creature || item != nullptr ) {
        bool triggered = act( pos, creature, item );
        if( triggered && is_real_creature ) {
            if( Character *ch = creature->as_character() ) {
                get_event_bus().send<event_type::character_triggers_trap>( ch->getID(), id );
            }
        }
    }
}

bool trap::is_null() const
{
    return loadid == tr_null;
}

bool trap::triggered_by_item( const item &itm ) const
{
    return !is_null() && itm.weight() >= trigger_weight;
}

bool trap::is_funnel() const
{
    return !is_null() && funnel_radius_mm > 0;
}

bool trap::has_sound_trigger() const
{
    return !is_null() && sound_threshold > 0;
}

bool trap::triggered_by_sound( int vol, int dist ) const
{
    const int volume = vol - dist;
    return !is_null() && volume >= sound_threshold;
}

void trap::on_disarmed( map &m, const tripoint &p ) const
{
    for( const auto &i : components ) {
        const itype_id &item_type = std::get<0>( i );
        const int quantity = std::get<1>( i );
        const int charges = std::get<2>( i );
        m.spawn_item( p.xy(), item_type, quantity, charges );
    }
    for( const tripoint &dest : m.points_in_radius( p, trap_radius ) ) {
        m.remove_trap( dest );
    }
}

//////////////////////////
// convenient int-lookup names for hard-coded functions
trap_id tr_null;

void trap::check_consistency()
{
    for( const trap &t : trap_factory.get_all() ) {
        for( const auto &i : t.components ) {
            const itype_id &item_type = std::get<0>( i );
            if( !item::type_is_defined( item_type ) ) {
                debugmsg( "trap %s has unknown item as component %s", t.id.str(), item_type.str() );
            }
        }
    }
}

bool trap::easy_take_down() const
{
    return avoidance == 0 && difficulty == 0;
}

bool trap::can_not_be_disarmed() const
{
    return difficulty >= 99;
}

void trap::finalize()
{
    for( const trap &t_const : trap_factory.get_all() ) {
        trap &t = const_cast<trap &>( t_const );
        // We need to set int ids manually now
        t.loadid = t.id.id();
        if( t.is_funnel() ) {
            funnel_traps.push_back( &t );
        }
        if( t.has_sound_trigger() ) {
            sound_triggered_traps.push_back( &t );
        }
    }

    tr_null = trap_str_id::NULL_ID().id();
}

std::string trap::debug_describe() const
{
    return string_format( _( "Visible: %d\nAvoidance: %d\nDifficulty: %d\nBenign: %s" ), visibility,
                          avoidance, difficulty, is_benign() ? _( "Yes" ) : _( "No" ) );
}
