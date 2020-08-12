#pragma once
#ifndef CATA_SRC_ACTIVE_TILE_DATA_H
#define CATA_SRC_ACTIVE_TILE_DATA_H

#include <string>
#include "calendar.h"

class JsonObject;
class JsonOut;
class map;
struct tripoint;

class active_tile_data
{
    public:
        static active_tile_data *create( const std::string &id );

        virtual ~active_tile_data();

        virtual void update( time_point from, time_point to, map &m, const tripoint &p ) = 0;
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

        void update( time_point from, time_point to, map &m, const tripoint &p ) override;
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

        void update( time_point from, time_point to, map &m, const tripoint &p ) override;
        active_tile_data *clone() const override;
        const std::string &get_type() const override;
        void store( JsonOut &jsout ) const override;
        void load( JsonObject &jo ) override;

        int mod_energy( int amt );
};

class charger_tile : public active_tile_data
{
    public:
        /* In W */
        int power;

        void update( time_point from, time_point to, map &m, const tripoint &p ) override;
        active_tile_data *clone() const override;
        const std::string &get_type() const override;
        void store( JsonOut &jsout ) const override;
        void load( JsonObject &jo ) override;
};

#endif // CATA_SRC_ACTIVE_TILE_DATA_H
