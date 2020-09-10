#pragma once
#ifndef CATA_SRC_LOCATION_H
#define CATA_SRC_LOCATION_H

struct tripoint;

// This is strictly an interface that provides access to a game entity's location.
class location
{
    public:
        virtual int posx() const = 0;
        virtual int posy() const = 0;
        virtual int posz() const = 0;
        virtual const tripoint &pos() const = 0;

        virtual void setpos( const tripoint &pos ) = 0;

        virtual ~location() = default;
};

location &get_player_location();

#endif // CATA_SRC_LOCATION_H
