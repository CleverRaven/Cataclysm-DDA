#include "ammo.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "debug.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "mortar.h"
#include "type_id.h"
#include "value_ptr.h"
#include "worldfactory.h"

static const ammotype ammo_120mm( "120mm" );
static const ammotype ammo_afs_titanium_small( "afs_titanium_small" );
static const ammotype ammo_gunpowder( "gunpowder" );
static const ammotype ammo_gunpowder_artillery( "gunpowder_artillery" );
static const ammotype ammo_gunpowder_large_rifle( "gunpowder_large_rifle" );
static const ammotype ammo_gunpowder_magnum_pistol( "gunpowder_magnum_pistol" );
static const ammotype ammo_gunpowder_pistol( "gunpowder_pistol" );
static const ammotype ammo_gunpowder_rifle( "gunpowder_rifle" );
static const ammotype ammo_gunpowder_shotgun( "gunpowder_shotgun" );
static const ammotype ammo_mana( "mana" );
static const ammotype ammo_plutonium( "plutonium" );
static const ammotype ammo_rolling_paper( "rolling_paper" );
static const ammotype ammo_test_1g_weight( "test_1g_weight" );
static const ammotype ammo_test_graphite( "test_graphite" );
static const ammotype ammo_test_solid_1ml( "test_solid_1ml" );
static const ammotype ammo_throwing_stick_with_charges( "throwing_stick_with_charges" );
static const ammotype ammo_unfinished_char( "unfinished_char" );

static const mod_id MOD_INFORMATION_generic_guns( "generic_guns" );

namespace
{
using ammo_map_t = std::unordered_map<ammotype, ammunition_type>;

ammo_map_t &all_ammunition_types()
{
    static ammo_map_t the_map;
    return the_map;
}
} //namespace

ammunition_type::ammunition_type() : name_( no_translation( "null" ) )
{
}

void ammunition_type::load_ammunition_type( const JsonObject &jsobj )
{
    ammunition_type &res = all_ammunition_types()[ ammotype( jsobj.get_string( "id" ) ) ];
    jsobj.read( "name", res.name_ );
    jsobj.read( "default", res.default_ammotype_, true );
}

/** @relates string_id */
template<>
bool string_id<ammunition_type>::is_valid() const
{
    return all_ammunition_types().count( *this ) > 0;
}

/** @relates string_id */
template<>
const ammunition_type &string_id<ammunition_type>::obj() const
{
    const ammo_map_t &the_map = all_ammunition_types();

    const auto it = the_map.find( *this );
    if( it != the_map.end() ) {
        return it->second;
    }

    debugmsg( "Tried to get invalid ammunition: %s", c_str() );
    static const ammunition_type null_ammunition;
    return null_ammunition;
}

void ammunition_type::reset()
{
    all_ammunition_types().clear();
}

void ammunition_type::check_consistency()
{

    // FIXME: We would like to stop skipping these eventually, but for various reasons these need to skip the unused check.
    std::set<ammotype> whitelist = {
        ammo_unfinished_char, // iexamine::kiln_fire sets charges directly and the resulting charcoal is still ammo, so it's unsafe for this not to be ammo.
        ammo_plutonium, // Some C++ code assumes that itype plut_cell has charges. TODO: Investigate and remove if safe.
        ammo_gunpowder, // Has issues with uncrafting before it can be de-charged
        ammo_mana, // Causes an error in xedrawood due to obsoleting some random item or another, needs to be tracked down
        ammo_gunpowder_artillery, // Ditto
        ammo_gunpowder_large_rifle, // Ditto
        ammo_gunpowder_magnum_pistol, // Ditto
        ammo_gunpowder_pistol, // Ditto
        ammo_gunpowder_rifle, // Ditto
        ammo_gunpowder_shotgun, // Ditto
        ammo_afs_titanium_small, // Just waiting for someone to do the work on de-charging this
        ammo_120mm, // Has some hardcoded references despite the fact nothing can actually shoot this. Need to clean those up before it can be safely removed
        ammo_rolling_paper, // Removing it causes an inexplicable autopickup test failure.
        ammo_throwing_stick_with_charges, // Used in a test
        ammo_test_1g_weight, // Used in a test
        ammo_test_solid_1ml, // Used in a test
        ammo_test_graphite // Used in a test. I think this IS used by a multimag test gun so possibly the unused check is faulty in some way??
    };

    std::vector<mod_id> &loaded_mods = world_generator->active_world->active_mod_order;
    const bool skip_all_unused_checks = std::find( loaded_mods.begin(), loaded_mods.end(),
                                        MOD_INFORMATION_generic_guns ) != loaded_mods.end();

    for( const auto &ammo : all_ammunition_types() ) {
        const auto &id = ammo.first;
        const itype_id &at = ammo.second.default_ammotype_;

        if( !item::type_is_defined( at ) ) {
            debugmsg( "ammo type %s has invalid default ammo %s", id.c_str(), at.c_str() );
        }


        // Because generic guns overrides TONS of guns into null objects which don't shoot anything, we risk
        // throwing tons and tons of errors and maintenance costs for generic guns if we try to check when that mod is loaded.
        // Therefore we ONLY check to make sure that the default ammo itype is at least legitimate. It's better than nothing.
        if( skip_all_unused_checks ) {
            continue;
        }


        std::vector<const itype *> shoots_this_ammo = Item_factory::find( [&]( const itype & t ) {
            item maybe_ammo_user( t.get_id() );
            // Either we use this ammo directly (i.e. we're a gun), or we're a weapon mod (a bore, or another type of gunmod) that lets a gun use this ammo.
            return maybe_ammo_user.ammo_types().count( id ) ||
                   ( t.mod && t.mod->ammo_modifier.count( id ) );
        } );

        // Mortars are furniture, so their ammo comes from mortar_type.
        for( const mortar_type &mortar : mortar_type::get_all() ) {
            if( mortar.ammo() == id ) {
                // These IDs are never dereferenced, so it's fine.
                shoots_this_ammo.push_back( &*itype_id::NULL_ID() );
                break;
            }
        }

        std::vector<const itype *> solid_types_as_this_ammo = Item_factory::find( [&]( const itype & t ) {
            return t.ammo && t.ammo->type == id && t.phase == phase_id::SOLID;
        } );


        const bool is_whitelisted = whitelist.count( id );

        // Liquids still need charge handling, so a ton of liquids need their own fake ammo types.
        const bool is_liquid_sole_type = solid_types_as_this_ammo.empty();

        if( shoots_this_ammo.empty() && !is_whitelisted && !is_liquid_sole_type ) {
            debugmsg( "ammotype %s exists but is unused", id.c_str() );
        }

        // The whitelist is specifically locked down to prevent shoving anything in there that doesn't strictly need to be in there.
        if( is_whitelisted && solid_types_as_this_ammo.size() > 1 ) {
            debugmsg( "whitelisted ammo type %s has more than one entry.", id.c_str() );
        }

    }
}

std::string ammunition_type::name() const
{
    return name_.translated();
}
