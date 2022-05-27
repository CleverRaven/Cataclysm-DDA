#ifndef CATA_SRC_SHOP_CONS_RATE_H
#define CATA_SRC_SHOP_CONS_RATE_H

#include "type_id.h"
#include "units.h"

class JsonObject;

constexpr char const *SHOPKEEPER_CONSUMPTION_RATES = "shopkeeper_consumption_rates";

struct shopkeeper_cons_rate_entry {
    itype_id itype;
    item_category_id category;
    item_group_id item_group;
    int rate = 0;

    bool operator==( shopkeeper_cons_rate_entry const &rhs ) const;
};

struct shopkeeper_cons_rates {
    shopkeeper_cons_rates_id id;
    bool was_loaded = false;

    int default_rate = -1;
    units::money junk_threshold = 1_cent;
    std::vector<shopkeeper_cons_rate_entry> rates;

    static void reset();
    static const std::vector<shopkeeper_cons_rates> &get_all();
    static void load_rate( const JsonObject &jo, std::string const &src );
    static void check_all();
    void load( const JsonObject &jo, std::string const &src );
    void check() const;

    int get_rate( item const &it ) const;
};

#endif // CATA_SRC_SHOP_CONS_RATE_H
