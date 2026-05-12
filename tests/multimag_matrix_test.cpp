#include <algorithm>
#include <climits>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "ammo.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "debug.h"
#include "flag.h"
#include "item.h"
#include "item_factory.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "output.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

// Cartesian product of 1-3 pockets over the ammotype palette, installed
// as runtime itypes. Same-ammo tuples are intentional: they exercise
// sibling-pocket-borrow regressions.

static const ammotype ammo_9mm( "9mm" );
static const ammotype ammo_battery( "battery" );
static const ammotype ammo_graphite( "graphite" );
static const ammotype ammo_lamp_oil( "lamp_oil" );
static const ammotype ammo_oxygen( "oxygen" );
static const ammotype ammo_water( "water" );

static const gun_mode_id gun_mode_DEFAULT( "DEFAULT" );

static const material_id material_steel( "steel" );

static const skill_id skill_pistol( "pistol" );

namespace
{

// Liquid/gas projectile pockets exist in shipping data (e.g. watercannon).
// ensure_synth marks those pockets watertight/airtight to match.
const std::vector<ammotype> &palette()
{
    static const std::vector<ammotype> r = {
        ammo_9mm, ammo_battery, ammo_water, ammo_oxygen, ammo_lamp_oil, ammo_graphite
    };
    return r;
}

// Bullets fit one per chamber; everything else gets tank-like capacity.
int pocket_cap_for( const ammotype &at )
{
    return at == ammo_9mm ? 1 : 50;
}

// Battery draws are larger per use to mimic the railgun pattern.
int pocket_qty_for( const ammotype &at )
{
    return at == ammo_battery ? 5 : 1;
}

struct synth_spec {
    itype_id id;
    bool is_gun;
    std::vector<ammotype> pockets;
};

itype_id synth_id_for( bool is_gun, const std::vector<ammotype> &pockets )
{
    std::string s = is_gun ? "mm_mx_gun" : "mm_mx_tool";
    for( const ammotype &a : pockets ) {
        s += "__" + a.str();
    }
    return itype_id( s );
}

// item ctor's qty maps to charges only for count_by_charges itypes;
// set charges explicitly so both representations carry the same amount.
item make_synth_ammo( const itype_id &id, int qty )
{
    item r( id, calendar::turn, qty );
    if( r.count_by_charges() ) {
        r.charges = qty;
    }
    return r;
}

// Sum of live charges across a pocket's top items, excluding spent
// casings (those remain in the pocket after fire but should not count
// toward loaded ammo for matrix invariants).
int pocket_live_charges( const item_pocket *p )
{
    int n = 0;
    for( const item *e : p->all_items_top() ) {
        if( e->has_flag( flag_CASING ) ) {
            continue;
        }
        n += e->charges > 0 ? e->charges : 1;
    }
    return n;
}

const itype *ensure_synth( const synth_spec &spec )
{
    if( item::type_is_defined( spec.id ) ) {
        return &*spec.id;
    }
    const itype *ct = item_controller->add_runtime(
                          spec.id,
                          no_translation( "synth multimag matrix item" ),
                          no_translation( "Runtime-built multimag item for matrix tests." ) );
    // const_cast: add_runtime returns const* and the factory exposes no
    // mutable accessor. Safe here - nothing observes the entry until
    // find_template hands it out.
    itype *t = const_cast<itype *>( ct );

    t->weight = 1000_gram;
    t->volume = 1500_ml;
    // finalize_pre is skipped for runtime types; populate the fields it
    // would have set so generic all-items iteration tests don't trip.
    t->longest_side = 200_mm;
    t->materials[material_steel] = 1;
    t->mat_portion_total = 1;
    t->using_legacy_to_hit = false;
    t->sym = "(";
    t->color = c_light_gray;

    if( spec.is_gun ) {
        t->gun = cata::make_value<islot_gun>();
        t->gun->skill_used = skill_pistol;
        for( const ammotype &a : spec.pockets ) {
            t->gun->ammo.insert( a );
        }
        t->gun->modes[gun_mode_DEFAULT] =
            gun_modifier_data( to_translation( "default" ), 1, std::set<std::string> {} );
    } else {
        t->tool = cata::make_value<islot_tool>();
    }

    int idx = 0;
    for( const ammotype &a : spec.pockets ) {
        pocket_data pd( pocket_type::MAGAZINE );
        pd.rigid = true;
        pd.pocket_id = "p" + std::to_string( idx );
        pd.ammo_restriction[a] = pocket_cap_for( a );
        // Liquid into a non-watertight pocket spills, leaving stragglers
        // that surface later as remove_item debug noise. Gas needs
        // airtight for the same reason.
        if( a == ammo_water || a == ammo_lamp_oil ) {
            pd.watertight = true;
        }
        if( a == ammo_oxygen ) {
            pd.airtight = true;
        }
        t->pockets.push_back( pd );
        ++idx;
    }

    for( size_t i = 0; i < spec.pockets.size(); ++i ) {
        pocket_consumption_entry e;
        e.pocket = "p" + std::to_string( i );
        e.qty = pocket_qty_for( spec.pockets[i] );
        t->firing_requirements.per_mode[gun_mode_DEFAULT].push_back( e );
    }

    return ct;
}

const std::vector<synth_spec> &synth_registry()
{
    static const std::vector<synth_spec> r = []() {
        std::vector<synth_spec> out;
        const std::vector<ammotype> &pal = palette();
        for( bool is_gun : {
                 true, false
             } ) {
            for( const ammotype &a : pal ) {
                out.push_back( { synth_id_for( is_gun, { a } ), is_gun, { a } } );
            }
            for( const ammotype &a : pal ) {
                for( const ammotype &b : pal ) {
                    std::vector<ammotype> pockets = { a, b };
                    out.push_back( { synth_id_for( is_gun, pockets ), is_gun, pockets } );
                }
            }
            for( const ammotype &a : pal ) {
                for( const ammotype &b : pal ) {
                    for( const ammotype &c : pal ) {
                        std::vector<ammotype> pockets = { a, b, c };
                        out.push_back( { synth_id_for( is_gun, pockets ), is_gun, pockets } );
                    }
                }
            }
        }
        return out;
    }
    ();
    return r;
}

void install_all_synth_types()
{
    static bool installed = false;
    if( installed ) {
        return;
    }
    for( const synth_spec &spec : synth_registry() ) {
        ensure_synth( spec );
    }
    installed = true;
}

// Bypasses item::reload and item::ammo_set on purpose: those are
// host-level APIs that can route to a different pocket or reset others
// on multimag hosts. The matrix wants each pocket exercised in
// isolation against its own ammo_restriction.
bool load_pocket( item_location &host, int pocket_idx,
                  const ammotype &at, int qty, std::string &api_used )
{
    const itype_id ammo_id = at->default_ammotype();
    if( ammo_id.is_null() || !item::type_is_defined( ammo_id ) ) {
        api_used = "no_default_ammo";
        return false;
    }
    std::vector<item_pocket *> pockets = host->get_pockets(
    []( const item_pocket & ) {
        return true;
    } );
    if( pocket_idx >= static_cast<int>( pockets.size() ) ) {
        api_used = "pocket_index_out_of_range";
        return false;
    }
    item synth = make_synth_ammo( ammo_id, qty );
    ret_val<item *> ins = pockets[pocket_idx]->insert_item( synth );
    if( ins.success() ) {
        api_used = "pocket_insert";
        return true;
    }
    api_used = "pocket_insert_rejected:" + std::string( ins.str() );
    return false;
}

int pocket_count_for( const item &it )
{
    int n = 0;
    for( const item_pocket *p : it.get_pockets( []( const item_pocket & ) {
    return true;
} ) ) {
        if( p->is_type( pocket_type::MAGAZINE ) || p->is_type( pocket_type::MAGAZINE_WELL ) ) {
            ++n;
        }
    }
    return n;
}

int segments_in_display( std::string_view display )
{
    const std::size_t lparen = display.rfind( '(' );
    const std::size_t rparen = display.rfind( ')' );
    if( lparen == std::string_view::npos || rparen == std::string_view::npos ||
        rparen <= lparen ) {
        return 0;
    }
    int commas = 0;
    for( std::size_t i = lparen + 1; i < rparen; ++i ) {
        if( display[i] == ',' ) {
            ++commas;
        }
    }
    return commas + 1;
}

} // namespace

TEST_CASE( "multimag_matrix_display_name_structural", "[multimag][matrix][display]" )
{
    install_all_synth_types();
    for( const synth_spec &spec : synth_registry() ) {
        CAPTURE( spec.id.str() );
        item it( spec.id );
        const int pc = pocket_count_for( it );
        REQUIRE( pc == static_cast<int>( spec.pockets.size() ) );

        const std::string name_empty = remove_color_tags( it.display_name() );
        INFO( "empty: " << name_empty );
        // 1-pocket multimag tnames may render a single bare "(X/Y)" or
        // skip the parens entirely depending on display_name code paths,
        // so segments_in_display >= 1 is the conservative assertion.
        if( pc == 1 ) {
            CHECK( segments_in_display( name_empty ) >= 1 );
        } else {
            CHECK( segments_in_display( name_empty ) == pc );
        }
        CHECK( name_empty.find( "invalid" ) == std::string::npos );
    }
}

TEST_CASE( "multimag_matrix_load_then_display", "[multimag][matrix][load][display]" )
{
    install_all_synth_types();
    clear_avatar();
    Character &chr = get_player_character();

    for( const synth_spec &spec : synth_registry() ) {
        CAPTURE( spec.id.str() );
        item_location host = chr.i_add( item( spec.id ) );
        REQUIRE( host );

        for( size_t i = 0; i < spec.pockets.size(); ++i ) {
            CAPTURE( i );
            CAPTURE( spec.pockets[i].str() );
            std::string api_used;
            const bool loaded = load_pocket( host, static_cast<int>( i ),
                                             spec.pockets[i], 1, api_used );
            CAPTURE( api_used );
            CHECK( loaded );
        }

        const std::string name_loaded = remove_color_tags( host->display_name() );
        INFO( "loaded: " << name_loaded );
        const int pc = pocket_count_for( *host );
        if( pc == 1 ) {
            CHECK( segments_in_display( name_loaded ) >= 1 );
        } else {
            CHECK( segments_in_display( name_loaded ) == pc );
        }
        CHECK( name_loaded.find( "invalid" ) == std::string::npos );
        // "0/0" segment indicates the per-pocket renderer fell back to a
        // zero-capacity branch; never legitimate for a loaded host.
        CHECK( name_loaded.find( "0/0" ) == std::string::npos );
    }
}

TEST_CASE( "multimag_matrix_tool_consume_drains_each_pocket",
           "[multimag][matrix][consume]" )
{
    install_all_synth_types();
    map &here = get_map();

    for( const synth_spec &spec : synth_registry() ) {
        if( spec.is_gun ) {
            continue;
        }
        CAPTURE( spec.id.str() );
        // Fresh avatar per shape: accumulating synth tools loaded with
        // comestible ammo seeds active_item_cache stragglers that
        // surface later as remove_item debug noise.
        // TODO: active_item_cache holds raw pointers; pocket/item
        // destruction doesn't deregister, so stale refs linger.
        clear_avatar();
        Character &chr = get_player_character();
        const tripoint_bub_ms pos = chr.pos_bub( here );
        item_location host = chr.i_add( item( spec.id ) );
        REQUIRE( host );

        for( size_t i = 0; i < spec.pockets.size(); ++i ) {
            const int cap = pocket_cap_for( spec.pockets[i] );
            std::string api_used;
            const bool loaded = load_pocket( host, static_cast<int>( i ),
                                             spec.pockets[i], cap, api_used );
            CAPTURE( i );
            CAPTURE( spec.pockets[i].str() );
            CAPTURE( api_used );
            REQUIRE( loaded );
        }

        std::vector<int> before;
        for( const item_pocket *p : host->get_pockets( []( const item_pocket & ) {
        return true;
    } ) ) {
            if( p->is_type( pocket_type::MAGAZINE ) ) {
                before.push_back( pocket_live_charges( p ) );
            }
        }
        REQUIRE( before.size() == spec.pockets.size() );

        const int got = host->consume_tool_uses( 1, here, pos, &chr );
        CHECK( got == 1 );

        std::vector<int> after;
        for( const item_pocket *p : host->get_pockets( []( const item_pocket & ) {
        return true;
    } ) ) {
            if( p->is_type( pocket_type::MAGAZINE ) ) {
                after.push_back( pocket_live_charges( p ) );
            }
        }
        for( size_t i = 0; i < before.size(); ++i ) {
            CAPTURE( i );
            CAPTURE( spec.pockets[i].str() );
            CAPTURE( before[i] );
            CAPTURE( after[i] );
            CHECK( after[i] < before[i] );
        }
    }
}

TEST_CASE( "multimag_matrix_reload_pocket_index_loads_target",
           "[multimag][matrix][reload]" )
{
    install_all_synth_types();

    for( const synth_spec &spec : synth_registry() ) {
        if( !spec.is_gun ) {
            continue;
        }
        CAPTURE( spec.id.str() );
        for( size_t target_idx = 0; target_idx < spec.pockets.size(); ++target_idx ) {
            CAPTURE( target_idx );
            CAPTURE( spec.pockets[target_idx].str() );

            // item::reload(pocket_index) is the bullet/charge-style path.
            // Liquid (water, lamp_oil) and gas (oxygen) pockets load via
            // item::fill_with; their default ammotype itype is a
            // comestible whose is_ammo() is false, so dispatching them
            // through reload refuses and leaks the source comestible.
            const itype_id ammo_id = spec.pockets[target_idx]->default_ammotype();
            if( ammo_id.is_null() || !item::type_is_defined( ammo_id ) ) {
                continue;
            }
            if( !item( ammo_id ).is_ammo() ) {
                continue;
            }

            // Fresh avatar per iteration so prior stacked ammo doesn't
            // invalidate the per-call item_location handle.
            clear_avatar();
            Character &chr = get_player_character();
            item_location host = chr.i_add( item( spec.id ) );
            REQUIRE( host );

            const int qty = pocket_cap_for( spec.pockets[target_idx] );
            item_location ammo_loc = chr.i_add( make_synth_ammo( ammo_id, qty ) );
            REQUIRE( ammo_loc );

            const bool reloaded = host->reload( chr, ammo_loc, qty, target_idx );
            CAPTURE( reloaded );

            std::vector<item_pocket *> pockets = host->get_pockets(
            []( const item_pocket & ) {
                return true;
            } );
            REQUIRE( target_idx < pockets.size() );
            const int loaded_target = pocket_live_charges( pockets[target_idx] );

            for( size_t i = 0; i < pockets.size(); ++i ) {
                if( i == target_idx ) {
                    continue;
                }
                const int sibling = pocket_live_charges( pockets[i] );
                CAPTURE( i );
                CAPTURE( sibling );
                CHECK( sibling == 0 );
            }

            if( reloaded ) {
                CHECK( loaded_target >= 1 );
            } else {
                // Reload refused must leave the pocket untouched - no
                // partial state.
                CHECK( loaded_target == 0 );
            }
        }
    }
}

TEST_CASE( "multimag_matrix_unload_action_rated_good_when_loaded",
           "[multimag][matrix][unload]" )
{
    install_all_synth_types();
    clear_avatar();
    Character &chr = get_player_character();

    for( const synth_spec &spec : synth_registry() ) {
        CAPTURE( spec.id.str() );
        item host( spec.id );
        std::vector<item_pocket *> pockets = host.get_pockets(
        []( const item_pocket & ) {
            return true;
        } );

        const hint_rating empty_rating = chr.rate_action_unload( host );
        CAPTURE( static_cast<int>( empty_rating ) );
        CHECK( empty_rating != hint_rating::good );

        for( size_t i = 0; i < spec.pockets.size(); ++i ) {
            ret_val<item *> ins = pockets[i]->insert_item(
                                      make_synth_ammo( spec.pockets[i]->default_ammotype(), 1 ) );
            REQUIRE( ins.success() );
        }
        const hint_rating loaded_rating = chr.rate_action_unload( host );
        CAPTURE( static_cast<int>( loaded_rating ) );
        CHECK( loaded_rating == hint_rating::good );
    }
}

TEST_CASE( "multimag_matrix_fire_gun_no_invalid_or_shortage",
           "[multimag][matrix][fire]" )
{
    install_all_synth_types();

    // Fire is the most expensive setup (clear_map + clear_avatar +
    // wield + capture_debugmsg per case), so sample a representative
    // subset instead of the full Cartesian: every 1-pocket gun shape,
    // plus the two-pocket bullet+power "railgun" pattern, plus one
    // 3-pocket exotic. The shape-coverage axes (display, load, reload,
    // unload) stay exhaustive in the other TEST_CASEs.
    std::set<itype_id> fire_shapes;
    for( const ammotype &a : palette() ) {
        fire_shapes.insert( synth_id_for( /*is_gun=*/true, { a } ) );
    }
    for( const ammotype &a : palette() ) {
        fire_shapes.insert( synth_id_for( true, { a, ammo_battery } ) );
    }
    fire_shapes.insert( synth_id_for( true, { ammo_9mm, ammo_battery, ammo_water } ) );

    for( const synth_spec &spec : synth_registry() ) {
        if( !spec.is_gun || fire_shapes.find( spec.id ) == fire_shapes.end() ) {
            continue;
        }
        CAPTURE( spec.id.str() );

        // Fresh avatar + map per shape so prior fire effects (catching
        // fire, recoil-induced unwield) don't carry over and drop the
        // synth gun mid-shot in the next iteration.
        clear_map();
        clear_avatar();
        avatar &dummy = get_avatar();
        dummy.set_body();
        dummy.clear_worn();
        dummy.remove_weapon();
        dummy.setpos( get_map(), tripoint_bub_ms( point_bub_ms::zero, 0 ) );

        item gun( spec.id );
        std::vector<item_pocket *> pockets = gun.get_pockets(
        []( const item_pocket & ) {
            return true;
        } );
        REQUIRE( pockets.size() == spec.pockets.size() );
        for( size_t i = 0; i < spec.pockets.size(); ++i ) {
            const int cap = pocket_cap_for( spec.pockets[i] );
            ret_val<item *> ins = pockets[i]->insert_item(
                                      make_synth_ammo( spec.pockets[i]->default_ammotype(), cap ) );
            REQUIRE( ins.success() );
        }
        dummy.set_wielded_item( gun );
        REQUIRE( dummy.get_wielded_item() );

        map &here = get_map();
        const tripoint_bub_ms target = dummy.pos_bub( here ) + point( 5, 0 );
        const std::string dmsg = capture_debugmsg_during( [&]() {
            dummy.fire_gun( here, target, 1, *dummy.get_wielded_item() );
        } );
        INFO( "debugmsg: " << dmsg );
        CHECK( dmsg.find( "invalid recipe id" ) == std::string::npos );
        CHECK( dmsg.find( "Unexpected shortage of ammo" ) == std::string::npos );
        CHECK( dmsg.find( "Unexpected shortage of energy" ) == std::string::npos );
    }
}

TEST_CASE( "multimag_matrix_tool_uses_remaining_local_after_load",
           "[multimag][matrix][charges]" )
{
    install_all_synth_types();
    for( const synth_spec &spec : synth_registry() ) {
        if( spec.is_gun ) {
            continue;
        }
        CAPTURE( spec.id.str() );
        item tool( spec.id );

        std::vector<item_pocket *> pockets = tool.get_pockets(
        []( const item_pocket & ) {
            return true;
        } );
        REQUIRE( pockets.size() == spec.pockets.size() );

        // Canonical multimag math: floor(local / qty) per pocket, then
        // min across pockets. tool_uses_remaining_local() recomputes the
        // same invariant from the host's own state.
        int expected = INT_MAX;
        for( size_t i = 0; i < spec.pockets.size(); ++i ) {
            const int cap = pocket_cap_for( spec.pockets[i] );
            const int qty = pocket_qty_for( spec.pockets[i] );
            ret_val<item *> ins = pockets[i]->insert_item(
                                      make_synth_ammo( spec.pockets[i]->default_ammotype(), cap ) );
            REQUIRE( ins.success() );
            expected = std::min( expected, cap / qty );
        }
        if( expected == INT_MAX ) {
            expected = 0;
        }

        const int local = tool.tool_uses_remaining_local();
        CAPTURE( expected );
        CAPTURE( local );
        CHECK( local == expected );
    }
}
