#pragma once
#ifndef CATA_SRC_IEXAMINE_ACTORS_H
#define CATA_SRC_IEXAMINE_ACTORS_H

#include "iexamine.h"

#include <map>

class Character;
class item_location;

class appliance_convert_examine_actor : public iexamine_actor
{
    private:
        cata::optional<furn_str_id> furn_set = cata::nullopt;
        cata::optional<ter_str_id> ter_set = cata::nullopt;
        itype_id appliance_item;

    public:
        explicit appliance_convert_examine_actor( const std::string &type = "appliance_convert" )
            : iexamine_actor( type ) {}

        void load( const JsonObject &jo ) override;
        void call( Character &you, const tripoint &examp ) const override;
        void finalize() const override;

        std::unique_ptr<iexamine_actor> clone() const override;
};

class cardreader_examine_actor : public iexamine_actor
{
    private:
        std::vector<flag_id> allowed_flags;
        bool consume = true;
        bool allow_hacking = true;
        bool despawn_monsters = true;

        // Option 1: walk the map, do some stuff
        int radius = 3;
        std::map<ter_str_id, ter_str_id> terrain_changes;
        std::map<furn_str_id, furn_str_id> furn_changes;
        // Option 2: Regenerate entire current overmap tile
        update_mapgen_id mapgen_id;

        cata::optional<int> omt_allowed_radius;

        bool map_regen = false;

        bool query = true;
        std::string query_msg;

        std::string success_msg;
        std::string redundant_msg;

        void consume_card( const std::vector<item_location> &cards ) const;
        std::vector<item_location> get_cards( Character &you, const tripoint &examp ) const;
        bool apply( const tripoint &examp ) const;

    public:
        explicit cardreader_examine_actor( const std::string &type = "cardreader" )
            : iexamine_actor( type ) {}

        void load( const JsonObject &jo ) override;
        void call( Character &you, const tripoint &examp ) const override;
        void finalize() const override;

        std::unique_ptr<iexamine_actor> clone() const override;
};

class eoc_examine_actor : public iexamine_actor
{
    private:
        std::vector<effect_on_condition_id> eocs;
    public:
        explicit eoc_examine_actor( const std::string &type = "effect_on_condition" )
            : iexamine_actor( type ) {}

        void load( const JsonObject &jo ) override;
        void call( Character &you, const tripoint &examp ) const override;
        void finalize() const override;

        std::unique_ptr<iexamine_actor> clone() const override;
};

#endif // CATA_SRC_IEXAMINE_ACTORS_H
