#pragma once
#ifndef CATA_CHARACTER_ID_H
#define CATA_CHARACTER_ID_H

#include <cassert>
#include <ostream>

class JsonIn;
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
        void deserialize( JsonIn & );
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

inline std::ostream &operator<<( std::ostream &o, character_id id )
{
    return o << id.get_value();
}

#endif // CATA_CHARACTER_ID_H
