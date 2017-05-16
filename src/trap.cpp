#include "string_id.h"
#include "int_id.h"
#include "generic_factory.h"
#include "trap.h"
#include "debug.h"
#include "line.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "translations.h"
#include "player.h"

#include <vector>
#include <memory>

namespace
{

generic_factory<trap> trap_factory( "trap" );

}

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

const skill_id skill_traps( "traps" );

const efftype_id effect_lack_sleep( "lack_sleep" );

const std::vector<const trap *> &trap::get_funnels()
{
    return funnel_traps;
}

size_t trap::count()
{
    return trap_factory.size();
}

void trap::load_trap( JsonObject &jo, const std::string &src )
{
    trap_factory.load( jo, src );
}

void trap::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", name, translated_string_reader );
    mandatory( jo, was_loaded, "color", color, color_reader{} );
    mandatory( jo, was_loaded, "symbol", sym, one_char_symbol_reader );
    mandatory( jo, was_loaded, "visibility", visibility );
    mandatory( jo, was_loaded, "avoidance", avoidance );
    mandatory( jo, was_loaded, "difficulty", difficulty );
    // @todo Is there a generic_factory version of this?
    act = trap_function_from_string( jo.get_string( "action" ) );

    optional( jo, was_loaded, "benign", benign, false );
    optional( jo, was_loaded, "funnel_radius", funnel_radius_mm, 0 );
    optional( jo, was_loaded, "trigger_weight", trigger_weight, -1 );
    optional( jo, was_loaded, "drops", components );
}

void trap::reset()
{
    funnel_traps.clear();
    trap_factory.reset();
}

bool trap::detect_trap( const tripoint &pos, const player &p ) const
{
    // Some decisions are based around:
    // * Starting, and thus average perception, is 8.
    // * Buried landmines, the silent killer, has a visibility of 10.
    // * There will always be a distance malus of 1 unless you're on top of the trap.
    // * ...and an average character should at least have a minor chance of
    //   noticing a buried landmine if standing right next to it.
    // Effective Perception...
    ///\EFFECT_PER increases chance of detecting a trap
    return ( p.per_cur - ( p.encumb( bp_eyes ) / 10 ) ) +
           // ...small bonus from stimulants...
           ( p.stim > 10 ? rng( 1, 2 ) : 0 ) +
           // ...bonus from trap skill...
           ///\EFFECT_TRAPS increases chance of detecting a trap
           ( p.get_skill_level( skill_traps ) * 2 ) +
           // ...luck, might be good, might be bad...
           rng( -4, 4 ) -
           // ...malus if we are tired...
           ( p.has_effect( effect_lack_sleep ) ? rng( 1, 5 ) : 0 ) -
           // ...malus farther we are from trap...
           rl_dist( p.pos(), pos ) +
           // Police are trained to notice Something Wrong.
           ( p.has_trait( trait_id( "PROF_POLICE" ) ) ? 1 : 0 ) +
           ( p.has_trait( trait_id( "PROF_PD_DET" ) ) ? 2 : 0 ) >
           // ...must all be greater than the trap visibility.
           visibility;
}

// Whether or not, in the current state, the player can see the trap.
bool trap::can_see( const tripoint &pos, const player &p ) const
{
    if( is_null() ) {
        // There is no trap at all, so logically one can not see it.
        return false;
    }
    return visibility < 0 || p.knows_trap( pos );
}

void trap::trigger( const tripoint &pos, Creature *creature ) const
{
    act( creature, pos );
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

bool trap::is_3x3_trap() const
{
    // TODO make this a json flag, implement more 3x3 traps.
    return id == trap_str_id( "tr_engine" );
}

void trap::on_disarmed( const tripoint &p ) const
{
    map &m = g->m;
    for( auto &i : components ) {
        m.spawn_item( p.x, p.y, i, 1, 1 );
    }
    // TODO: make this a flag, or include it into the components.
    if( id == trap_str_id( "tr_shotgun_1" ) || id == trap_str_id( "tr_shotgun_2" ) ) {
        m.spawn_item( p.x, p.y, "shot_00", 1, 2 );
    }
    if( is_3x3_trap() ) {
        for( int i = -1; i <= 1; i++ ) {
            for( int j = -1; j <= 1; j++ ) {
                m.remove_trap( tripoint( p.x + i, p.y + j, p.z ) );
            }
        }
    } else {
        m.remove_trap( p );
    }
}

//////////////////////////
// convenient int-lookup names for hard-coded functions
trap_id
tr_null,
tr_bubblewrap,
tr_cot,
tr_brazier,
tr_funnel,
tr_metal_funnel,
tr_makeshift_funnel,
tr_leather_funnel,
tr_rollmat,
tr_fur_rollmat,
tr_beartrap,
tr_beartrap_buried,
tr_nailboard,
tr_caltrops,
tr_tripwire,
tr_crossbow,
tr_shotgun_2,
tr_shotgun_1,
tr_engine,
tr_blade,
tr_light_snare,
tr_heavy_snare,
tr_landmine,
tr_landmine_buried,
tr_telepad,
tr_goo,
tr_dissector,
tr_sinkhole,
tr_pit,
tr_spike_pit,
tr_lava,
tr_portal,
tr_ledge,
tr_boobytrap,
tr_temple_flood,
tr_temple_toggle,
tr_glow,
tr_hum,
tr_shadow,
tr_drain,
tr_snake,
tr_glass_pit;

void trap::check_consistency()
{
    for( const auto &t : trap_factory.get_all() ) {
        for( auto &i : t.components ) {
            if( !item::type_is_defined( i ) ) {
                debugmsg( "trap %s has unknown item as component %s", t.id.c_str(), i.c_str() );
            }
        }
    }
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
    }
    const auto trapfind = []( const char *id ) {
        return trap_str_id( id ).id();
    };
    tr_null = trap_str_id::NULL_ID.id();
    tr_bubblewrap = trapfind( "tr_bubblewrap" );
    tr_cot = trapfind( "tr_cot" );
    tr_brazier = trapfind( "tr_brazier" );
    tr_funnel = trapfind( "tr_funnel" );
    tr_metal_funnel = trapfind( "tr_metal_funnel" );
    tr_makeshift_funnel = trapfind( "tr_makeshift_funnel" );
    tr_leather_funnel = trapfind( "tr_leather_funnel" );
    tr_rollmat = trapfind( "tr_rollmat" );
    tr_fur_rollmat = trapfind( "tr_fur_rollmat" );
    tr_beartrap = trapfind( "tr_beartrap" );
    tr_beartrap_buried = trapfind( "tr_beartrap_buried" );
    tr_nailboard = trapfind( "tr_nailboard" );
    tr_caltrops = trapfind( "tr_caltrops" );
    tr_tripwire = trapfind( "tr_tripwire" );
    tr_crossbow = trapfind( "tr_crossbow" );
    tr_shotgun_2 = trapfind( "tr_shotgun_2" );
    tr_shotgun_1 = trapfind( "tr_shotgun_1" );
    tr_engine = trapfind( "tr_engine" );
    tr_blade = trapfind( "tr_blade" );
    tr_light_snare = trapfind( "tr_light_snare" );
    tr_heavy_snare = trapfind( "tr_heavy_snare" );
    tr_landmine = trapfind( "tr_landmine" );
    tr_landmine_buried = trapfind( "tr_landmine_buried" );
    tr_telepad = trapfind( "tr_telepad" );
    tr_goo = trapfind( "tr_goo" );
    tr_dissector = trapfind( "tr_dissector" );
    tr_sinkhole = trapfind( "tr_sinkhole" );
    tr_pit = trapfind( "tr_pit" );
    tr_spike_pit = trapfind( "tr_spike_pit" );
    tr_lava = trapfind( "tr_lava" );
    tr_portal = trapfind( "tr_portal" );
    tr_ledge = trapfind( "tr_ledge" );
    tr_boobytrap = trapfind( "tr_boobytrap" );
    tr_temple_flood = trapfind( "tr_temple_flood" );
    tr_temple_toggle = trapfind( "tr_temple_toggle" );
    tr_glow = trapfind( "tr_glow" );
    tr_hum = trapfind( "tr_hum" );
    tr_shadow = trapfind( "tr_shadow" );
    tr_drain = trapfind( "tr_drain" );
    tr_snake = trapfind( "tr_snake" );
    tr_glass_pit = trapfind( "tr_glass_pit" );
}
