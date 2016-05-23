#ifndef ITEM_UID_H
#define ITEM_UID_H

#include <cstdint>

#include "json.h"

class item_uid : public JsonSerializer, public JsonDeserializer
{
        friend class item_location; /** needs clone() */

    public:
        item_uid( unsigned long long val = 0 ) : val( val ) {}

        item_uid( item_uid && ) = default;
        item_uid &operator=( item_uid && ) = default;

        item_uid( const item_uid & );
        item_uid &operator=( const item_uid & );

        bool operator==( const item_uid &rhs ) const {
            return val == rhs.val;
        }
        bool operator!=( const item_uid &rhs ) const {
            return val != rhs.val;
        }
        bool operator< ( const item_uid &rhs ) const {
            return val < rhs.val;
        }

        void serialize( JsonOut &js ) const;
        void deserialize( JsonIn &js );

    private:
        unsigned long long val;
        item_uid clone() const;
};

#endif
