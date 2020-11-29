#pragma once
#ifndef CATA_SRC_ACTIVE_TILE_DATA_H
#define CATA_SRC_ACTIVE_TILE_DATA_H

#include <string>
#include "calendar.h"

class JsonObject;
class JsonOut;
class map;
struct tripoint;
class distribution_grid;

class active_tile_data
{
    public:
        static active_tile_data *create( const std::string &id );

    private:
        time_point last_updated;

    protected:
        virtual void update_internal( time_point to, const tripoint &p, distribution_grid &grid ) = 0;

    public:
        void update( time_point to, const tripoint &p, distribution_grid &grid ) {
            update_internal( to, p, grid );
            last_updated = to;
        }

        time_point get_last_updated() {
            return last_updated;
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        virtual int get_resource() const {
            return 0;
        }

        virtual int mod_resource( int amt ) {
            return amt;
        }

        virtual ~active_tile_data();
        virtual active_tile_data *clone() const = 0;
        virtual const std::string &get_type() const = 0;

        virtual void store( JsonOut &jsout ) const = 0;
        virtual void load( JsonObject &jo ) = 0;
};

class solar_tile : public active_tile_data
{
    public:
        /* In W */
        int power;

        void update_internal( time_point to, const tripoint &p, distribution_grid &grid ) override;
        active_tile_data *clone() const override;
        const std::string &get_type() const override;
        void store( JsonOut &jsout ) const override;
        void load( JsonObject &jo ) override;
};

class battery_tile : public active_tile_data
{
    public:
        /* In J */
        int stored;
        int max_stored;

        void update_internal( time_point to, const tripoint &p, distribution_grid &grid ) override;
        active_tile_data *clone() const override;
        const std::string &get_type() const override;
        void store( JsonOut &jsout ) const override;
        void load( JsonObject &jo ) override;

        int get_resource() const override;
        int mod_resource( int amt ) override;
};

class charger_tile : public active_tile_data
{
    public:
        /* In W */
        int power;

        void update_internal( time_point to, const tripoint &p, distribution_grid &grid ) override;
        active_tile_data *clone() const override;
        const std::string &get_type() const override;
        void store( JsonOut &jsout ) const override;
        void load( JsonObject &jo ) override;
};

#endif // CATA_SRC_ACTIVE_TILE_DATA_H
