#include <vector>
#include <memory>
#include "game.h"
#include "map.h"
#include "debug.h"
#include "translations.h"

void trap::load( JsonObject &jo )
{
    std::unique_ptr<trap> trap_ptr( new trap() );
    trap &t = *trap_ptr;

    if( jo.has_member( "drops" ) ) {
        JsonArray drops_list = jo.get_array( "drops" );
        while( drops_list.has_more() ) {
            t.components.push_back( drops_list.next_string() );
        }
    }
    t.name = jo.get_string( "name" );
    if( !t.name.empty() ) {
        t.name = _( t.name.c_str() );
    }
    t.id = jo.get_string( "id" );
    t.loadid = traplist.size();
    t.color = color_from_string( jo.get_string( "color" ) );
    t.sym = jo.get_string( "symbol" ).at( 0 );
    t.visibility = jo.get_int( "visibility" );
    t.avoidance = jo.get_int( "avoidance" );
    t.difficulty = jo.get_int( "difficulty" );
    t.act = trap_function_from_string( jo.get_string( "action" ) );
    t.benign = jo.get_bool( "benign", false );
    t.funnel_radius_mm = jo.get_int( "funnel_radius", 0 );
    t.trigger_weight = jo.get_int( "trigger_weight", -1 );

    trapmap[t.id] = t.loadid;
    traplist.push_back( &t );
    trap_ptr.release();
}

void trap::reset()
{
    for( auto & tptr : traplist ) {
        delete tptr;
    }
    traplist.clear();
    trapmap.clear();
}

std::vector <trap *> traplist;
std::map<std::string, int> trapmap;

trap_id trapfind(const std::string id)
{
    if( trapmap.find(id) == trapmap.end() ) {
        popup("Can't find trap %s", id.c_str());
        return 0;
    }
    return traplist[trapmap[id]]->loadid;
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
    return (p.per_cur - (p.encumb(bp_eyes) / 10)) +
           // ...small bonus from stimulants...
           (p.stim > 10 ? rng(1, 2) : 0) +
           // ...bonus from trap skill...
           (p.get_skill_level("traps") * 2) +
           // ...luck, might be good, might be bad...
           rng(-4, 4) -
           // ...malus if we are tired...
           (p.has_effect("lack_sleep") ? rng(1, 5) : 0) -
           // ...malus farther we are from trap...
           rl_dist( p.pos(), pos ) +
           // Police are trained to notice Something Wrong.
           (p.has_trait("PROF_POLICE") ? 1 : 0) +
           (p.has_trait("PROF_PD_DET") ? 2 : 0) >
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
    if( act != nullptr ) {
        trapfunc f;
        (f.*act)( creature, pos.x, pos.y );
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

bool trap::is_3x3_trap() const
{
    // TODO make this a json flag, implement more 3x3 traps.
    return id == "tr_engine";
}

void trap::on_disarmed( const tripoint &p ) const
{
    map &m = g->m;
    for( auto & i : components ) {
        m.spawn_item( p.x, p.y, i, 1, 1 );
    }
    // TODO: make this a flag, or include it into the components.
    if( id == "tr_shotgun_1" || id == "tr_shotgun_2" ) {
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
    for( auto & tptr : traplist ) {
        for( auto & i : tptr->components ) {
            if( !item::type_is_defined( i ) ) {
                debugmsg( "trap %s has unknown item as component %s", tptr->id.c_str(), i.c_str() );
            }
        }
    }
}

void trap::finalize()
{
    tr_null = trapfind("tr_null");
    tr_bubblewrap = trapfind("tr_bubblewrap");
    tr_cot = trapfind("tr_cot");
    tr_brazier = trapfind("tr_brazier");
    tr_funnel = trapfind("tr_funnel");
    tr_makeshift_funnel = trapfind("tr_makeshift_funnel");
    tr_leather_funnel = trapfind("tr_leather_funnel");
    tr_rollmat = trapfind("tr_rollmat");
    tr_fur_rollmat = trapfind("tr_fur_rollmat");
    tr_beartrap = trapfind("tr_beartrap");
    tr_beartrap_buried = trapfind("tr_beartrap_buried");
    tr_nailboard = trapfind("tr_nailboard");
    tr_caltrops = trapfind("tr_caltrops");
    tr_tripwire = trapfind("tr_tripwire");
    tr_crossbow = trapfind("tr_crossbow");
    tr_shotgun_2 = trapfind("tr_shotgun_2");
    tr_shotgun_1 = trapfind("tr_shotgun_1");
    tr_engine = trapfind("tr_engine");
    tr_blade = trapfind("tr_blade");
    tr_light_snare = trapfind("tr_light_snare");
    tr_heavy_snare = trapfind("tr_heavy_snare");
    tr_landmine = trapfind("tr_landmine");
    tr_landmine_buried = trapfind("tr_landmine_buried");
    tr_telepad = trapfind("tr_telepad");
    tr_goo = trapfind("tr_goo");
    tr_dissector = trapfind("tr_dissector");
    tr_sinkhole = trapfind("tr_sinkhole");
    tr_pit = trapfind("tr_pit");
    tr_spike_pit = trapfind("tr_spike_pit");
    tr_lava = trapfind("tr_lava");
    tr_portal = trapfind("tr_portal");
    tr_ledge = trapfind("tr_ledge");
    tr_boobytrap = trapfind("tr_boobytrap");
    tr_temple_flood = trapfind("tr_temple_flood");
    tr_temple_toggle = trapfind("tr_temple_toggle");
    tr_glow = trapfind("tr_glow");
    tr_hum = trapfind("tr_hum");
    tr_shadow = trapfind("tr_shadow");
    tr_drain = trapfind("tr_drain");
    tr_snake = trapfind("tr_snake");
    tr_glass_pit = trapfind("tr_glass_pit");

    // Set ter_t.trap using ter_t.trap_id_str.
    for( auto &elem : terlist ) {
        if( elem.trap_id_str.empty() ) {
            elem.trap = tr_null;
        } else {
            elem.trap = trapfind( elem.trap_id_str );
        }
    }
}
