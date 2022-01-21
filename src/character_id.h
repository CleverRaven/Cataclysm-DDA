#pragma once
#ifndef CATA_SRC_CHARACTER_ID_H
#define CATA_SRC_CHARACTER_ID_H

#include <iosfwd>

class JsonOut;

class character_id
{
    public:
        character_id() : value( -1 ) {}

        explicit character_id( int i ) : value( i ) {
        }

        bool is_valid() const {
            return value > 0;
        }

        int get_value() const {
            return value;
        }

        character_id &operator++() {
            ++value;
            return *this;
        }

        void serialize( JsonOut & ) const;
        void deserialize( int );

    private:
        int value;
};

inline bool operator==( character_id l, character_id r )
{
    return l.get_value() == r.get_value();
}

inline bool operator!=( character_id l, character_id r )
{
    return l.get_value() != r.get_value();
}

inline bool operator<( character_id l, character_id r )
{
    return l.get_value() < r.get_value();
}

std::ostream &operator<<( std::ostream &o, character_id id );

#endif // CATA_SRC_CHARACTER_ID_H
