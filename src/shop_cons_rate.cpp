#include "shop_cons_rate.h"

#include "generic_factory.h"
#include "item_category.h"
#include "item_group.h"
#include "itype.h"
#include "json.h"

namespace
{

generic_factory<shopkeeper_cons_rates> shop_cons_rate_factory( SHOPKEEPER_CONSUMPTION_RATES );

} // namespace

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

void shopkeeper_cons_rates::check_all()
{
    shop_cons_rate_factory.check();
}

class shopkeeper_cons_rates_reader : public generic_typed_reader<shopkeeper_cons_rates_reader>
{
    public:
        static shopkeeper_cons_rate_entry get_next( JsonValue &jv ) {
            JsonObject jo = jv.get_object();
            shopkeeper_cons_rate_entry ret;
            optional( jo, false, "item", ret.itype );
            optional( jo, false, "category", ret.category );
            optional( jo, false, "group", ret.item_group );
            mandatory( jo, false, "rate", ret.rate );
            return ret;
        }
};

bool shopkeeper_cons_rate_entry::operator==( shopkeeper_cons_rate_entry const &rhs ) const
{
    return itype == rhs.itype and category == rhs.category and item_group == rhs.item_group and
           rate == rhs.rate;
}

void shopkeeper_cons_rates::load( JsonObject const &jo, std::string const &/*src*/ )
{
    optional( jo, was_loaded, "default_rate", default_rate );
    optional( jo, was_loaded, "junk_threshold", junk_threshold, money_reader {}, 1_cent );
    optional( jo, was_loaded, "rates", rates, shopkeeper_cons_rates_reader {} );
}

void shopkeeper_cons_rates::check() const
{
    for( auto const &rate : rates ) {
        if( !rate.itype.is_empty() and
            ( !rate.category.is_empty() or !rate.item_group.is_empty() ) ) {
            debugmsg( "category/item_group filters are redundant when itype is specified in %s.",
                      id.c_str() );
        }
        if( rate.itype.is_empty() and rate.category.is_empty() and rate.item_group.is_empty() ) {
            debugmsg( "empty shop rate filter in %s.", id.c_str() );
        }
    }
}

int shopkeeper_cons_rates::get_rate( item const &it ) const
{
    if( it.type->price_post < junk_threshold ) {
        return -1;
    }
    for( auto rit = rates.crbegin(); rit != rates.crend(); ++rit ) {
        bool const has =
            ( rit->itype.is_empty() or it.typeId() == rit->itype ) and
            ( rit->category.is_empty() or it.get_category_shallow().id == rit->category ) and
            ( rit->item_group.is_empty() or
              item_group::group_contains_item( rit->item_group, it.typeId() ) );
        if( has ) {
            return rit->rate;
        }
    }
    return default_rate;
}
