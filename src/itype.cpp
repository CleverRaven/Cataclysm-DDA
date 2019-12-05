#include "itype.h"

#include <utility>

#include "debug.h"
#include "player.h"
#include "translations.h"
#include "item.h"
#include "ret_val.h"

struct tripoint;

std::string gunmod_location::name() const
{
    // Yes, currently the name is just the translated id.
    return _( _id );
}

namespace io
{
template<>
std::string enum_to_string<condition_type>( condition_type data )
{
    switch( data ) {
        case condition_type::FLAG:
            return "FLAG";
        case condition_type::COMPONENT_ID:
            return "COMPONENT_ID";
        case condition_type::num_condition_types:
            break;
    }
    debugmsg( "Invalid condition_type" );
    abort();
}
} // namespace io

void itype::copy( const itype &other )
{
    if( other.container ) {
        container = std::make_unique<islot_container>( *other.container );
    }
    if( other.tool ) {
        tool = std::make_unique<islot_tool>( *other.tool );
    }
    if( other.comestible ) {
        comestible = std::make_unique<islot_comestible>( *other.comestible );
    }
    if( other.brewable ) {
        brewable = std::make_unique<islot_brewable>( *other.brewable );
    }
    if( other.armor ) {
        armor = std::make_unique<islot_armor>( *other.armor );
    }
    if( other.pet_armor ) {
        pet_armor = std::make_unique<islot_pet_armor>( *other.pet_armor );
    }
    if( other.book ) {
        book = std::make_unique<islot_book>( *other.book );
    }
    if( other.mod ) {
        mod = std::make_unique<islot_mod>( *other.mod );
    }
    if( other.engine ) {
        engine = std::make_unique<islot_engine>( *other.engine );
    }
    if( other.wheel ) {
        wheel = std::make_unique<islot_wheel>( *other.wheel );
    }
    if( other.fuel ) {
        fuel = std::make_unique<islot_fuel>( *other.fuel );
    }
    if( other.gun ) {
        gun = std::make_unique<islot_gun>( *other.gun );
    }
    if( other.gunmod ) {
        gunmod = std::make_unique<islot_gunmod>( *other.gunmod );
    }
    if( other.magazine ) {
        magazine = std::make_unique<islot_magazine>( *other.magazine );
    }
    if( other.battery ) {
        battery = std::make_unique<islot_battery>( *other.battery );
    }
    if( other.bionic ) {
        bionic = std::make_unique<islot_bionic>( *other.bionic );
    }
    if( other.ammo ) {
        ammo = std::make_unique<islot_ammo>( *other.ammo );
    }
    if( other.seed ) {
        seed = std::make_unique<islot_seed>( *other.seed );
    }
    if( other.artifact ) {
        artifact = std::make_unique<islot_artifact>( *other.artifact );
    }
    if( other.relic_data ) {
        relic_data = std::make_unique<relic>( *other.relic_data );
    }

    stackable_ = other.stackable_;

    damage_min_ = other.damage_min_;
    damage_max_ = other.damage_max_;

    id = other.id;
    name = other.name;

    looks_like = other.looks_like;

    snippet_category = other.snippet_category;
    description = other.description;

    default_container = other.default_container;

    qualities = other.qualities;
    properties = other.properties;
    materials = other.materials;

    use_methods = other.use_methods;
    drop_action = other.drop_action;

    emits = other.emits;

    item_tags = other.item_tags;
    techniques = other.techniques;

    min_skills = other.min_skills;
    min_str = other.min_str;
    min_dex = other.min_dex;
    min_int = other.min_int;
    min_per = other.min_per;

    phase = other.phase;

    explosion = other.explosion;
    explode_in_fire = other.explode_in_fire;

    countdown_destroy = other.countdown_destroy;
    countdown_interval = other.countdown_interval;
    countdown_action = other.countdown_action;

    weight = other.weight;
    integral_weight = other.integral_weight;

    volume = other.volume;
    integral_volume = other.integral_volume;

    stack_size = other.stack_size;

    price = other.price;
    price_post = other.price_post;

    rigid = other.rigid;

    melee = other.melee;
    thrown_damage = other.thrown_damage;
    m_to_hit = other.m_to_hit;

    light_emission = other.light_emission;

    category_force = other.category_force;

    sym = other.sym;
    color = other.color;

    repair = other.repair;
    faults = other.faults;

    magazines = other.magazines;
    magazine_default = other.magazine_default;
    magazine_well = other.magazine_well;

    layer = other.layer;
    insulation_factor = other.insulation_factor;
}

itype::itype( const itype &other )
{
    copy( other );
}

itype &itype::operator=( itype other )
{
    copy( other );
    return *this;
}

std::string itype::nname( unsigned int quantity ) const
{
    // Always use singular form for liquids.
    // (Maybe gases too?  There are no gases at the moment)
    if( phase == LIQUID ) {
        quantity = 1;
    }
    return name.translated( quantity );
}

int itype::charges_per_volume( const units::volume &vol ) const
{
    if( volume == 0_ml ) {
        return item::INFINITE_CHARGES; // TODO: items should not have 0 volume at all!
    }
    return ( count_by_charges() ? stack_size : 1 ) * vol / volume;
}

// Members of iuse struct, which is slowly morphing into a class.
bool itype::has_use() const
{
    return !use_methods.empty();
}

bool itype::can_use( const std::string &iuse_name ) const
{
    return get_use( iuse_name ) != nullptr;
}

const use_function *itype::get_use( const std::string &iuse_name ) const
{
    const auto iter = use_methods.find( iuse_name );
    return iter != use_methods.end() ? &iter->second : nullptr;
}

int itype::tick( player &p, item &it, const tripoint &pos ) const
{
    // Note: can go higher than current charge count
    // Maybe should move charge decrementing here?
    int charges_to_use = 0;
    for( auto &method : use_methods ) {
        const int val = method.second.call( p, it, true, pos );
        if( charges_to_use < 0 || val < 0 ) {
            charges_to_use = -1;
        } else {
            charges_to_use += val;
        }
    }

    return charges_to_use;
}

int itype::invoke( player &p, item &it, const tripoint &pos ) const
{
    if( !has_use() ) {
        return 0;
    }
    return invoke( p, it, pos, use_methods.begin()->first );
}

int itype::invoke( player &p, item &it, const tripoint &pos, const std::string &iuse_name ) const
{
    const use_function *use = get_use( iuse_name );
    if( use == nullptr ) {
        debugmsg( "Tried to invoke %s on a %s, which doesn't have this use_function",
                  iuse_name, nname( 1 ) );
        return 0;
    }

    const auto ret = use->can_call( p, it, false, pos );

    if( !ret.success() ) {
        p.add_msg_if_player( m_info, ret.str() );
        return 0;
    }

    return use->call( p, it, false, pos );
}

std::string gun_type_type::name() const
{
    return pgettext( "gun_type_type", name_.c_str() );
}

bool itype::can_have_charges() const
{
    if( count_by_charges() ) {
        return true;
    }
    if( tool && tool->max_charges > 0 ) {
        return true;
    }
    if( gun && gun->clip > 0 ) {
        return true;
    }
    if( item_tags.count( "CAN_HAVE_CHARGES" ) ) {
        return true;
    }
    return false;
}
