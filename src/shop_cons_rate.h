#pragma once
#ifndef CATA_SRC_SHOP_CONS_RATE_H
#define CATA_SRC_SHOP_CONS_RATE_H

#include "generic_factory.h"
#include "type_id.h"
#include "units.h"

class JsonObject;
class npc;
struct dialogue;

constexpr char const *SHOPKEEPER_CONSUMPTION_RATES = "shopkeeper_consumption_rates";
constexpr char const *SHOPKEEPER_BLACKLIST = "shopkeeper_blacklist";

struct icg_entry {
    itype_id itype;
    item_category_id category;
    item_group_id item_group;
    translation message;

    std::function<bool( dialogue & )> condition;

    bool operator==( icg_entry const &rhs ) const;
    bool matches( item const &it, npc const &beta ) const;
};

class icg_entry_reader : public generic_typed_reader<icg_entry_reader>
{
    public:
        static icg_entry _part_get_next( JsonObject const &jo );
        static icg_entry get_next( JsonValue &jv );
};

struct shopkeeper_cons_rate_entry: public icg_entry {
    int rate = 0;

    shopkeeper_cons_rate_entry() = default;
    explicit shopkeeper_cons_rate_entry( icg_entry const &rhs ) : icg_entry( rhs ), rate( 0 ) {}

    bool operator==( shopkeeper_cons_rate_entry const &rhs ) const;
};

struct shopkeeper_cons_rates {
    shopkeeper_cons_rates_id id;
    bool was_loaded = false;

    int default_rate = -1;
    units::money junk_threshold = 1_cent;
    std::vector<shopkeeper_cons_rate_entry> rates;

    shopkeeper_cons_rates() = default;

    static void reset();
    static const std::vector<shopkeeper_cons_rates> &get_all();
    static void load_rate( const JsonObject &jo, std::string const &src );
    static void check_all();
    void load( const JsonObject &jo, std::string_view src );
    void check() const;

    int get_rate( item const &it, npc const &beta ) const;
    bool matches( item const &it, npc const &beta ) const;
};

struct shopkeeper_blacklist {
    shopkeeper_blacklist_id id;
    bool was_loaded = false;

    std::vector<icg_entry> entries;

    static void reset();
    static const std::vector<shopkeeper_blacklist> &get_all();
    static void load_blacklist( const JsonObject &jo, std::string const &src );
    void load( const JsonObject &jo, std::string_view src );
    icg_entry const *matches( item const &it, npc const &beta ) const;
};

#endif // CATA_SRC_SHOP_CONS_RATE_H
