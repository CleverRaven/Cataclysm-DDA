#include "shop_cons_rate.h"

#include "avatar.h"
#include "condition.h"
#include "generic_factory.h"
#include "item_category.h"
#include "item_group.h"
#include "itype.h"
#include "json.h"

namespace
{

generic_factory<shopkeeper_cons_rates> shop_cons_rate_factory( SHOPKEEPER_CONSUMPTION_RATES );
generic_factory<shopkeeper_blacklist> shop_blacklist_factory( SHOPKEEPER_BLACKLIST );

} // namespace

bool icg_entry::operator==( icg_entry const &rhs ) const
{
    return itype == rhs.itype && category == rhs.category && item_group == rhs.item_group;
}

bool icg_entry::matches( item const &it, npc const &beta ) const
{
    dialogue const temp( get_talker_for( get_avatar() ), get_talker_for( beta ) );
    return ( !condition || condition( temp ) ) &&
           ( itype.is_empty() || it.typeId() == itype ) &&
           ( category.is_empty() || it.get_category_shallow().id == category ) &&
           ( item_group.is_empty() ||
             item_group::group_contains_item( item_group, it.typeId() ) );
}

/** @relates string_id */
template<>
shopkeeper_blacklist const &string_id<shopkeeper_blacklist>::obj() const
{
    return shop_blacklist_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<shopkeeper_blacklist>::is_valid() const
{
    return shop_blacklist_factory.is_valid( *this );
}

void shopkeeper_blacklist::reset()
{
    shop_blacklist_factory.reset();
}

std::vector<shopkeeper_blacklist> const &shopkeeper_blacklist::get_all()
{
    return shop_blacklist_factory.get_all();
}

/** @relates string_id */
template<>
shopkeeper_cons_rates const &string_id<shopkeeper_cons_rates>::obj() const
{
    return shop_cons_rate_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<shopkeeper_cons_rates>::is_valid() const
{
    return shop_cons_rate_factory.is_valid( *this );
}

void shopkeeper_cons_rates::reset()
{
    shop_cons_rate_factory.reset();
}

std::vector<shopkeeper_cons_rates> const &shopkeeper_cons_rates::get_all()
{
    return shop_cons_rate_factory.get_all();
}

void shopkeeper_cons_rates::load_rate( const JsonObject &jo, std::string const &src )
{
    shop_cons_rate_factory.load( jo, src );
}

void shopkeeper_blacklist::load_blacklist( const JsonObject &jo, std::string const &src )
{
    shop_blacklist_factory.load( jo, src );
}

void shopkeeper_cons_rates::check_all()
{
    shop_cons_rate_factory.check();
}

icg_entry icg_entry_reader::_part_get_next( JsonObject const &jo )
{
    icg_entry ret;
    optional( jo, false, "item", ret.itype );
    optional( jo, false, "category", ret.category );
    optional( jo, false, "group", ret.item_group );
    optional( jo, false, "message", ret.message );
    if( jo.has_member( "condition" ) ) {
        read_condition( jo, "condition", ret.condition, false );
    }
    return ret;
}
icg_entry icg_entry_reader::get_next( JsonValue &jv )
{
    JsonObject jo = jv.get_object();
    icg_entry ret( _part_get_next( jo ) );
    return ret;
}

class shopkeeper_cons_rates_reader : public generic_typed_reader<shopkeeper_cons_rates_reader>
{
    public:
        static shopkeeper_cons_rate_entry get_next( JsonValue &jv ) {
            JsonObject jo = jv.get_object();
            shopkeeper_cons_rate_entry ret( icg_entry_reader::_part_get_next( jo ) );
            mandatory( jo, false, "rate", ret.rate );
            return ret;
        }
};

bool shopkeeper_cons_rate_entry::operator==( shopkeeper_cons_rate_entry const &rhs ) const
{
    return icg_entry::operator==( rhs ) && rate == rhs.rate;
}

void shopkeeper_blacklist::load( JsonObject const &jo, std::string const &/*src*/ )
{
    optional( jo, was_loaded, "entries", entries, icg_entry_reader {} );
}

void shopkeeper_cons_rates::load( JsonObject const &jo, std::string const &/*src*/ )
{
    optional( jo, was_loaded, "default_rate", default_rate );
    optional( jo, was_loaded, "junk_threshold", junk_threshold, money_reader {}, 1_cent );
    optional( jo, was_loaded, "rates", rates, shopkeeper_cons_rates_reader {} );
}

void shopkeeper_cons_rates::check() const
{
    for( const shopkeeper_cons_rate_entry &rate : rates ) {
        if( !rate.itype.is_empty() &&
            ( !rate.category.is_empty() || !rate.item_group.is_empty() ) ) {
            debugmsg( "category/item_group filters are redundant when itype is specified in %s.",
                      id.c_str() );
        }
        if( rate.itype.is_empty() && rate.category.is_empty() && rate.item_group.is_empty() ) {
            debugmsg( "empty shop rate filter in %s.", id.c_str() );
        }
    }
}

int shopkeeper_cons_rates::get_rate( item const &it, npc const &beta ) const
{
    if( it.type->price_post < junk_threshold ) {
        return -1;
    }
    for( auto rit = rates.crbegin(); rit != rates.crend(); ++rit ) {
        if( rit->matches( it, beta ) ) {
            return rit->rate;
        }
    }
    return default_rate;
}

icg_entry const *shopkeeper_blacklist::matches( item const &it, npc const &beta ) const
{
    auto const el = std::find_if( entries.begin(), entries.end(),
    [&it, &beta]( icg_entry const & rit ) {
        return rit.matches( it, beta );
    } );
    if( el != entries.end() ) {
        return &*el;
    }

    return nullptr;
}

bool shopkeeper_cons_rates::matches( item const &it, npc const &beta ) const
{
    return it.type->price_post < junk_threshold ||
           std::any_of( rates.begin(), rates.end(),
    [&it, &beta]( shopkeeper_cons_rate_entry const & rit ) {
        return rit.matches( it, beta );
    } );
}
